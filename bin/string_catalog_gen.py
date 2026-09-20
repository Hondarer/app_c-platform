#!/usr/bin/env python3
"""カタログ定義 (JSONC) から cplat 文字列カタログの生成物 2 ファイルを書き出す。"""

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

# 引数種別から、型付きラッパーの仮引数の型への対応。
# 正本は prod/include/cplat/string_catalog/argument.h の対応表。
ARGUMENT_TYPES = {
    "STRING": "const char *",
    "CHAR": "char",
    "INT8": "int8_t",
    "UINT8": "uint8_t",
    "INT16": "int16_t",
    "UINT16": "uint16_t",
    "INT32": "int32_t",
    "UINT32": "uint32_t",
    "INT64": "int64_t",
    "UINT64": "uint64_t",
    "HEX8": "uint8_t",
    "HEX16": "uint16_t",
    "HEX32": "uint32_t",
    "HEX64": "uint64_t",
    "SIZE": "size_t",
    "SSIZE": "int64_t",
    "POINTER": "const void *",
    "DOUBLE": "double",
    "ERROR_CODE": "int",
}

# ライブラリ側の接頭辞。カタログ定義には書かず、生成器が補う。
# 文字列カタログの型名と API 名を決めるのはライブラリであり、利用者の選択肢ではないため。
LIBRARY_PREFIX = "cplat_string_catalog"

# 生成物が include するライブラリの公開ヘッダー。接頭辞から機械的に導けないため、別に定数化する。
LIBRARY_HEADER = "cplat/string_catalog/string_catalog.h"

# texts と notes のキーに書ける言語。cplat_string_catalog_language の並びと揃える。
# 言語はライブラリが定める仕様であり、カタログ定義が増減できる項目ではない。
LANGUAGES = ("neutral", "japanese", "english")

# 位置指定の添字に書ける最大の桁数。
# prod/libsrc/cplat/string_catalog/string_catalog_render.c の INDEX_DIGITS_MAX と揃える。
INDEX_DIGITS_MAX = 2

# 1 つの文字列が取れる引数の最大個数。CPLAT_STRING_CATALOG_ARGUMENT_MAX と揃える。
ARGUMENT_MAX = 50


class DefinitionError(Exception):
    """カタログ定義の内容が不正であることを表す。"""


def strip_jsonc(text: str) -> str:
    """JSONC から行コメント、ブロック コメント、末尾コンマを取り除く。

    文字列リテラルの中は書き換えない。エスケープも解釈する。
    """
    out = []
    index = 0
    length = len(text)
    in_string = False

    while index < length:
        char = text[index]

        if in_string:
            out.append(char)
            if char == "\\" and (index + 1) < length:
                out.append(text[index + 1])
                index += 2
                continue
            if char == '"':
                in_string = False
            index += 1
            continue

        if char == '"':
            in_string = True
            out.append(char)
            index += 1
            continue

        if text.startswith("//", index):
            while index < length and text[index] != "\n":
                index += 1
            continue

        if text.startswith("/*", index):
            end = text.find("*/", index + 2)
            index = length if end < 0 else end + 2
            continue

        if char == ",":
            # 末尾コンマなら落とす。次の非空白が閉じ括弧のとき。
            probe = index + 1
            while probe < length and text[probe] in " \t\r\n":
                probe += 1
            if probe < length and text[probe] in "}]":
                index += 1
                continue

        out.append(char)
        index += 1

    return "".join(out)


def load_definition(path: Path) -> dict:
    """定義ファイルを読み込む。"""
    text = path.read_text(encoding="utf-8")
    try:
        return json.loads(strip_jsonc(text))
    except json.JSONDecodeError as error:
        raise DefinitionError(f"{path}: JSON として解釈できません: {error}") from error


def join_text(value) -> str:
    """文字列、または文字列の配列を 1 つの文字列にする。"""
    if isinstance(value, str):
        return value
    if isinstance(value, list):
        if not all(isinstance(item, str) for item in value):
            raise DefinitionError("文字列の配列に文字列以外が含まれています。")
        return " ".join(value)
    raise DefinitionError(f"文字列または文字列の配列である必要があります: {value!r}")


def placeholder_indices(text: str) -> list[int]:
    """書式に現れる位置指定の添字を返す。構文が不正なら例外にする。

    受け付ける構文は string_catalog_render.c の render_scan_text と同じ。
    """
    found = []
    index = 0
    length = len(text)

    while index < length:
        char = text[index]

        if char == "}":
            if not text.startswith("}}", index):
                raise DefinitionError(f"対を成さない }} があります: {text!r}")
            index += 2
            continue

        if char != "{":
            index += 1
            continue

        if text.startswith("{{", index):
            index += 2
            continue

        digits = 0
        while (
            digits < INDEX_DIGITS_MAX
            and (index + 1 + digits) < length
            and text[index + 1 + digits].isdigit()
            and text[index + 1 + digits].isascii()
        ):
            digits += 1

        if digits == 0 or (index + 1 + digits) >= length or text[index + 1 + digits] != "}":
            raise DefinitionError(f"位置指定の構文が不正です: {text!r}")
        if digits > 1 and text[index + 1] == "0":
            raise DefinitionError(f"位置指定の添字に先頭のゼロは書けません: {text!r}")

        found.append(int(text[index + 1 : index + 1 + digits]))
        index += 2 + digits

    return found


def validate(document: dict) -> list[dict]:
    """定義の内容を検査し、文字列の一覧を返す。"""
    for key in ("strings",):
        if key not in document:
            raise DefinitionError(f"必須の項目がありません: {key}")

    strings = document["strings"]
    if not strings:
        raise DefinitionError("strings が空です。")

    seen_keys: set[str] = set()
    reserved_names = {f"{document['module_prefix']}_{suffix}" for suffix in MODULE_FUNCTION_SUFFIXES}

    for entry in strings:
        # id は処理では意味を持たない補足の文字列のため、必須にしない。
        for key in ("key", "category", "brief", "arguments", "texts", "notes"):
            if key not in entry:
                raise DefinitionError(f"{entry.get('key', '?')}: 必須の項目がありません: {key}")

        # 分類値は生値とする。生成物を特定の app の列挙から独立させるため。
        if not isinstance(entry["category"], int) or isinstance(entry["category"], bool):
            raise DefinitionError(f"{entry['key']}: category は整数で指定してください。")

        for key in ("key", "brief"):
            if not isinstance(entry[key], str):
                raise DefinitionError(f"{entry['key']}: {key} は文字列で指定してください。")

        if "id" in entry and not isinstance(entry["id"], str):
            raise DefinitionError(f"{entry['key']}: id は文字列で指定してください。")

        if "details" in entry and not isinstance(entry["details"], str):
            raise DefinitionError(f"{entry['key']}: details は文字列で指定してください。")

        if "remarks" in entry and (
            not isinstance(entry["remarks"], (str, list))
            or (isinstance(entry["remarks"], list) and not all(isinstance(item, str) for item in entry["remarks"]))
        ):
            raise DefinitionError(f"{entry['key']}: remarks は文字列または文字列の配列で指定してください。")

        # id の重複は検査しない。id は処理で項目を識別しないため。
        if entry["key"] in seen_keys:
            raise DefinitionError(f"文字列キーが重複しています: {entry['key']}")
        seen_keys.add(entry["key"])

        # 型付きラッパーは接頭辞を持たずモジュール接頭辞の名前空間に収まるため、
        # 同じ生成物が出す簡易関数 (@MODULE@_category など) と名前が衝突しうる。
        wrapper = wrapper_name(entry["key"])
        if wrapper in reserved_names:
            raise DefinitionError(
                f"{entry['key']}: 型付きラッパー名 {wrapper} が同じ生成物の簡易関数名と衝突しています。"
            )

        arguments = entry["arguments"]
        if len(arguments) > ARGUMENT_MAX:
            raise DefinitionError(f"{entry['key']}: 引数が上限 {ARGUMENT_MAX} 個を超えています。")

        for argument in arguments:
            for key in ("kind", "name", "description"):
                if key not in argument:
                    raise DefinitionError(f"{entry['key']}: 引数に {key} がありません。")
            if argument["kind"] not in ARGUMENT_TYPES:
                raise DefinitionError(f"{entry['key']}: 未知の引数種別です: {argument['kind']}")
            if not isinstance(argument["name"], str) or not isinstance(argument["description"], str):
                raise DefinitionError(f"{entry['key']}: 引数の name と description は文字列で指定してください。")

        for section in ("texts", "notes"):
            if "neutral" not in entry[section]:
                raise DefinitionError(f"{entry['key']}: {section} に neutral が必要です。")
            for language in entry[section]:
                if language not in LANGUAGES:
                    raise DefinitionError(f"{entry['key']}: ライブラリが扱わない言語です: {language}")

        for language, text in entry["texts"].items():
            indices = placeholder_indices(join_text(text))
            for found in indices:
                if found >= len(arguments):
                    raise DefinitionError(
                        f"{entry['key']}: {language} の位置指定 {{{found}}} が引数個数 {len(arguments)} を超えています。"
                    )

    return strings


GENERATED_NOTE = """ *  本ヘッダーと `{source}` は、カタログ定義 `{definition}` から自動生成されたファイルです。\\n
 *  列挙型とテーブルは 1 組の生成単位のため、常に同時に生成してください。\\n
 *  手作業で直接編集せず、生成元の定義を変更してから `app/c-platform/bin/string_catalog_gen.py` を実行してください。"""


def c_string(text: str) -> str:
    """C の文字列リテラルへ変換する。"""
    escaped = text.replace("\\", "\\\\").replace('"', '\\"')
    return f'"{escaped}"'


def language_constant(document: dict, language: str) -> str:
    """言語のキーを、ライブラリの列挙定数へ変換する。"""
    return f"{LIBRARY_PREFIX.upper()}_LANGUAGE_{language.upper()}"


def kind_constant(document: dict, kind: str) -> str:
    """引数種別のキーを、ライブラリの列挙定数へ変換する。"""
    return f"{LIBRARY_PREFIX.upper()}_ARGUMENT_KIND_{kind}"


# ACCESSOR_DECLARATIONS (および SOURCE_TAIL) が @MODULE@_ に続けて出力する簡易関数の名前一覧。
# 型付きラッパーの関数名が、これらの簡易関数と衝突していないかの検査に使う。
# 一覧はテンプレートの実際の出力に合わせるため、テンプレートを変更した場合はここも見直す。
MODULE_FUNCTION_SUFFIXES = (
    "entries",
    "entry_count",
    "entry",
    "key_index",
    "key_index_count",
    "catalog",
    "format",
    "vformat",
    "verify",
    "category",
    "id",
    "note",
)


def wrapper_name(string_key: str) -> str:
    """文字列キーから型付きラッパーの関数名を導出する。

    ライブラリ側の接頭辞は前置せず、文字列キーの定数名をそのまま小文字化した名前にする。
    文字列キーにはカタログ定義ごとのモジュール接頭辞が含まれるため、
    追加の導出規則なしに利用側の名前空間へ収まる。
    ライブラリ側の接頭辞を前置しないのは、利用者が定義した関数がライブラリのリンカー名前空間を
    名乗ってしまうのを避けるため。
    """
    return string_key.lower()


def doc_lines(text: str, indent: str, width: int = 112) -> list[str]:
    """Doxygen 本文の 1 段落を、指定幅で折り返した行にする。"""
    words = text.split()
    lines: list[str] = []
    current = indent

    for word in words:
        candidate = f"{current}{word}" if current == indent else f"{current} {word}"
        if current != indent and len(candidate) > width:
            lines.append(current)
            current = f"{indent}{word}"
        else:
            current = candidate

    if current != indent:
        lines.append(current)
    return lines


def emit_wrapper(document: dict, entry: dict) -> str:
    """1 件分の型付きラッパーを、Doxygen コメントとともに書き出す。"""
    name = wrapper_name(entry["key"])
    arguments = entry["arguments"]

    names = ["dest", "dest_size"] + [argument["name"] for argument in arguments]
    name_width = max(len(name) for name in names) + 1
    # `     *  ` の 8 文字と、`@param[out]     ` の 16 文字のあとに名前欄が並ぶ
    continuation = "     *" + " " * (8 + 16 + name_width - 6)

    lines = ["    /**"]
    lines.append(f"     *  @brief          {entry['brief']}")
    lines.append("     *")
    details_text = entry.get("details", "")
    if details_text:
        lines.extend(doc_lines(details_text, "     *  "))
    if not arguments:
        lines.extend(doc_lines("この文字列は引数を必要としません。", "     *  "))
    if details_text or not arguments:
        lines.append("     *")
    lines.append(
        f"     *  @param[out]     {'dest'.ljust(name_width)}文字列の格納先バッファー。NULL を渡してはなりません。"
    )
    lines.append(
        f"     *  @param[in]      {'dest_size'.ljust(name_width)}@p dest のバイト数。1 以上を指定してください。"
    )

    for argument in arguments:
        padded = argument["name"].ljust(name_width)
        lines.append(f"     *  @param[in]      {padded}{argument['description']}")
        lines.append(f"{continuation}引数種別は @c {kind_constant(document, argument['kind'])} です。")

    lines.append(
        f"     *  @return         戻り値は @c {LIBRARY_PREFIX}_format と同じです。"
    )

    remark_text = join_text(entry["remarks"]) if entry.get("remarks") else ""
    if remark_text:
        remark_indent = "     *" + " " * 18
        remark_lines = doc_lines(remark_text, remark_indent)
        lines.append(f"     *  @remark         {remark_lines[0][len(remark_indent):]}")
        lines.extend(remark_lines[1:])

    lines.append("     *  @par            書式")
    texts = entry["texts"]
    languages = [language for language in LANGUAGES if language in texts]
    for position, language in enumerate(languages):
        suffix = "\\n" if position < (len(languages) - 1) else ""
        lines.append(f"     *  `{join_text(texts[language])}`{suffix}")
    lines.append("     */")

    parameters = ["char *dest", "const size_t dest_size"]
    for argument in arguments:
        c_type = ARGUMENT_TYPES[argument["kind"]]
        if c_type.endswith("*"):
            parameters.append(f"{c_type}{argument['name']}")
        else:
            parameters.append(f"const {c_type} {argument['name']}")

    call = [f"{document['module_prefix']}_catalog()", "dest", "dest_size", entry["key"]]
    call.extend(argument["name"] for argument in arguments)

    lines.append(f"    static inline int {name}({', '.join(parameters)})")
    lines.append("    {")
    lines.append(f"        return {LIBRARY_PREFIX}_format({', '.join(call)});")
    lines.append("    }")

    return "\n".join(lines)


def expand(template: str, module: str, library: str) -> str:
    """テンプレート中の接頭辞の目印を置き換える。

    テンプレートは C のコードを含み波括弧が現れるため、str.format は使わない。
    """
    return (
        template.replace("@MODULE_UPPER@", module.upper())
        .replace("@MODULE@", module)
        .replace("@LIBRARY@", library)
    )


def derive_module_prefix(definition: Path) -> str:
    """定義ファイルの名前から、モジュール接頭辞を導出する。

    生成物のファイル名と、カタログを省略する口の名前がここから決まる。
    C の識別子の一部になるため、英小文字で始まる snake_case だけを認める。
    """
    prefix = definition.stem
    if re.fullmatch(r"[a-z][a-z0-9_]*", prefix) is None:
        raise DefinitionError(
            "定義ファイルの名前をモジュール接頭辞に使います。"
            f"英小文字で始まり、英小文字、数字、下線だけで綴ってください: {definition.name}"
        )
    return prefix


def key_enum_name(document: dict) -> str:
    """文字列キーの列挙名を導出する。

    モジュール接頭辞に `_key` を続ける。定義ファイルには書かない。
    """
    return f"{document['module_prefix']}_key"


def derive_module_dir(definition: Path) -> str:
    """定義ファイルの位置から、app 直下を起点としたディレクトリを導出する。

    app の配下は `prod/` と `test/` に分かれる。定義ファイルの絶対パスから、
    最も近い `prod` または `test` を探し、そこからの部分をディレクトリとする。
    どちらも無い場所に置いた場合は `.` とする。
    """
    parts = definition.resolve().parent.parts
    for anchor in ("prod", "test"):
        if anchor in parts:
            index = len(parts) - 1 - parts[::-1].index(anchor)
            return "/".join(parts[index:])
    return "."


def output_dir_display(document: dict, out_relative: str) -> str:
    """生成物の置き場所を、リポジトリ相対のディレクトリとして表す。

    out_relative は、定義ファイルの置き場所から見た出力先の相対パス。
    Doxygen の @file は実際のパスと一致している必要がある。
    """
    module_dir = document.get("module_dir", ".")
    if out_relative in ("", "."):
        return module_dir
    return f"{module_dir}/{out_relative}"


def emit_header(document: dict, strings: list[dict], definition_name: str, out_relative: str = ".") -> str:
    """ヘッダー側の生成物を組み立てる。"""
    module = document["module_prefix"]
    library = LIBRARY_PREFIX
    guard = f"{module.upper()}_H"
    header_name = f"{module}.h"
    source_name = f"{module}.c"
    module_dir = document.get("module_dir", ".")
    output_dir = output_dir_display(document, out_relative)
    include_path = header_name if output_dir == module_dir else f"{out_relative}/{header_name}"

    out = [
        "/**",
        " " + "*" * 79,
        f" *  @file           {header_name}",
        f" *  @brief          利用者が定義する文字列キーの列挙型と、カタログ取得関数を宣言します。",
        f" *  @author         {document.get('author', '')}",
        f" *  @date           {document.get('date', '')}",
        f" *  @version        {document.get('version', '')}",
        " *",
        f" *  本ヘッダーは `{output_dir}/` のモジュール私有ヘッダーです。\\n",
        f' *  `{module_dir}/` の実装ファイルからのみ `#include "{include_path}"` でインクルードします。',
        " *",
        GENERATED_NOTE.format(source=source_name, definition=definition_name),
        " *",
        " *  この 2 ファイルは、文字列カタログを利用するアプリケーション側で用意するファイルです。\\n",
        " *  言語、引数種別、書式構文はライブラリ側で規定されます。\\n",
        " *  分類値の意味付けは利用側の取り決めであり、別ヘッダーで個別に定義します。",
        " *",
        " *  列挙名や関数名はカタログ定義に基づいて決まり、ライブラリの接頭辞とは異なる名前空間に属します。\\n",
        " *  ライブラリ側ではこれらの名前を定義せず、文字列キーを `int` 型として受け取ります。",
        " *",
        f" *  @copyright      Copyright (C) {document.get('author', '')}. 2026. All rights reserved.",
        " *",
        " " + "*" * 79,
        " */",
        "",
        f"#ifndef {guard}",
        f"#define {guard}",
        "",
    ]

    group_id = module.upper()
    group_title = f"文字列カタログ ({module})"

    out.extend(
        [
            f"#include <{LIBRARY_HEADER}>",
            "#include <stdarg.h>",
            "#include <stddef.h>",
            "#include <stdint.h>",
            "",
            "/**",
            f" *  @defgroup       {group_id} {group_title}",
            f" *  @brief          カタログ定義 `{definition_name}` から自動生成された文字列カタログです。",
            " *  @{",
            " */",
            "",
            "#ifdef __cplusplus",
            'extern "C"',
            "{",
            "#endif /* __cplusplus */",
            "",
            "    /**",
            "     *  @brief          カタログに登録された文字列を識別する列挙型です。",
            "     *",
            "     *  各 ID の引数定義、分類値、説明文、言語別の書式および備考は、同一の生成単位のテーブルで保持します。\\n",
            "     *  列挙定数の名前はカタログ定義の key で、処理から文字列を参照する識別子です。\\n",
            "     *  列挙値は定義の並び順に基づいて割り当てられます。定義の id は処理では意味を持たないため、列挙には現れません。",
            "     */",
            f"    typedef enum {key_enum_name(document)}",
            "    {",
        ]
    )

    for position, entry in enumerate(strings):
        comma = "," if position < (len(strings) - 1) else ""
        out.append(f"        {entry['key']} = {position + 1}{comma} /**< {entry['brief']} */")

    out.append(f"    }} {key_enum_name(document)};")
    out.append("")
    out.append(expand(ACCESSOR_DECLARATIONS, module, library))
    out.extend(
        [
            "#ifdef __cplusplus",
            "}",
            "#endif /* __cplusplus */",
            "",
        ]
    )

    wrapper_group_id = f"{group_id}_TYPED_FORMATTERS"
    out.extend(
        [
            "/**",
            f" *  @defgroup       {wrapper_group_id} 文字列キーごとの型付き組み立て関数",
            " *  @brief          文字列キーごとに引数の型を固定した組み立て関数です。",
            f" *  @ingroup        {group_id}",
            " *  @{",
            " */",
            "",
            "#ifdef __cplusplus",
            'extern "C"',
            "{",
            "#endif /* __cplusplus */",
            "",
        ]
    )

    for entry in strings:
        out.append(emit_wrapper(document, entry))
        out.append("")

    out.extend(
        [
            "#ifdef __cplusplus",
            "}",
            "#endif /* __cplusplus */",
            "",
            "/** @} */",
            "",
            "/** @} */",
            "",
            f"#endif /* {guard} */",
            "",
        ]
    )

    return "\n".join(out)


ACCESSOR_DECLARATIONS = """\
    /**
     *  @brief          カタログ配列の先頭を取得します。
     *  @return         カタログ配列の先頭ポインターです。NULL は返しません。
     *
     *  返されるポインターは静的領域を指しているため、呼び出し側で解放してはなりません。\\n
     *  @c @MODULE@_entry_count とともに @c @LIBRARY@ を構築するための構成要素です。\\n
     *  構築済みのカタログ オブジェクトを取得する場合は @c @MODULE@_catalog を使用してください。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。読み取り専用の静的データだけを参照します。
     */
    const @LIBRARY@_entry *@MODULE@_entries(void);

    /**
     *  @brief          カタログの登録件数を取得します。
     *  @return         文字列の登録件数です。1 以上を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。読み取り専用の静的データだけを参照します。
     */
    int @MODULE@_entry_count(void);

    /**
     *  @brief          文字列キーからカタログ配列の添字を引くテーブルを取得します。
     *  @return         添字テーブルへのポインターです。NULL は返しません。
     *
     *  文字列キーを添字として、カタログ配列の添字を格納しています。\\n
     *  未登録の文字列キーに対応する要素には負の値を格納します。\\n
     *  このテーブルを使用することで、文字列キーからカタログを引く探索を線形探索からインデックス参照へ置き換えます。
     *
     *  返されるポインターは静的領域を指しているため、呼び出し側で解放してはなりません。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。読み取り専用の静的データだけを参照します。
     */
    const int *@MODULE@_key_index(void);

    /**
     *  @brief          添字テーブルの要素数を取得します。
     *  @return         添字テーブルの要素数です。最大の文字列キーに 1 を加えた値となります。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。読み取り専用の静的データだけを参照します。
     */
    int @MODULE@_key_index_count(void);

    /**
     *  @brief          本カタログ定義のカタログ識別オブジェクトを取得します。
     *  @return         カタログ識別オブジェクトへのポインターです。NULL は返しません。
     *
     *  配列と添字テーブルを 1 つのカタログ構造体にまとめたオブジェクトです。\\n
     *  ライブラリ側ではカタログを保持しないため、文字列組み立て API へはこのオブジェクトへのポインターを渡します。
     *
     *  返されるポインターは静的領域を指しているため、呼び出し側で解放してはなりません。\\n
     *  他のカタログ定義と組み合わせる場合は、対象に応じたカタログ オブジェクトを使い分けます。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。読み取り専用の静的データだけを参照します。
     */
    const @LIBRARY@ *@MODULE@_catalog(void);

    /**
     *  @brief          文字列キーに対応するカタログ項目を取得します。
     *  @param[in]      string_key 参照する文字列のキー。
     *  @return         カタログ項目へのポインターです。見つからない場合は NULL を返します。
     *
     *  本カタログ定義から文字列キーに対応する項目を取得するための簡易関数です。\n
     *  内部で @c @MODULE@_catalog と @c @LIBRARY@_get_entry を使用します。
     *  項目の識別には @c key を使用します。@c id は処理では意味を持たない補足の文字列です。
     *  @c id、@c details、@c remarks は、定義で省略されている場合に NULL です。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。読み取り専用の静的データだけを参照します。
     */
    const @LIBRARY@_entry *@MODULE@_entry(int string_key);

    /**
     *  @brief          本カタログ定義を使用して、文字列を組み立てます。
     *  @param[out]     dest       文字列の格納先バッファー。NULL を渡してはなりません。
     *  @param[in]      dest_size  @p dest のバイト数。1 以上を指定してください。
     *  @param[in]      string_key 組み立てる文字列のキー。
     *  @param[in]      ...        引数スキーマが定める順序と型の引数リスト。
     *  @return         戻り値は @c @LIBRARY@_format と同じです。
     *
     *  カタログの指定を省略して呼び出すための簡易関数です。\\n
     *  内部で @c @MODULE@_catalog を補って @c @LIBRARY@_format を呼び出します。
     *
     *  @par            スレッド セーフ
     *  スレッド セーフ性は @c @LIBRARY@_format と同じです。
     */
    int @MODULE@_format(char *dest, size_t dest_size, int string_key, ...);

    /**
     *  @brief          本カタログ定義を使用して、@c va_list から文字列を組み立てます。
     *  @param[out]     dest       文字列の格納先バッファー。NULL を渡してはなりません。
     *  @param[in]      dest_size  @p dest のバイト数。1 以上を指定してください。
     *  @param[in]      string_key 組み立てる文字列のキー。
     *  @param[in]      args       引数スキーマが定める順序と型の値を保持する引数リスト。
     *  @return         戻り値は @c @LIBRARY@_vformat と同じです。
     *
     *  カタログの指定を省略して呼び出すための簡易関数です。\\n
     *  内部で @c @MODULE@_catalog を補って @c @LIBRARY@_vformat を呼び出します。
     *
     *  @par            スレッド セーフ
     *  スレッド セーフ性は @c @LIBRARY@_vformat と同じです。
     */
    int @MODULE@_vformat(char *dest, size_t dest_size, int string_key, va_list args);

    /**
     *  @brief          本カタログ定義の内容を確認します。
     *  @param[out]     string_key_out 不正を検出した文字列キーの格納先。不要な場合は NULL を指定できます。
     *  @param[out]     language_out   不正を検出した言語の格納先。不要な場合は NULL を指定できます。
     *  @return         戻り値は @c @LIBRARY@_verify と同じです。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。
     */
    int @MODULE@_verify(int *string_key_out, @LIBRARY@_language *language_out);

    /**
     *  @brief          本カタログ定義から、文字列の分類値を取得します。
     *  @param[in]      string_key 参照する文字列のキー。
     *  @return         戻り値は @c @LIBRARY@_get_category と同じです。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。
     */
    int @MODULE@_category(int string_key);

    /**
     *  @brief          本カタログ定義から、文字列キーに対応する ID を取得します。
     *  @param[in]      string_key 参照する文字列のキー。
     *  @return         戻り値は @c @LIBRARY@_get_id と同じです。ID が未設定の項目では NULL を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。
     */
    const char *@MODULE@_id(int string_key);

    /**
     *  @brief          本カタログ定義から、現在の言語設定における文字列の備考を取得します。
     *  @param[in]      string_key 参照する文字列のキー。
     *  @return         戻り値は @c @LIBRARY@_get_note と同じです。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。
     */
    const char *@MODULE@_note(int string_key);

    /*
     *  ここから下は、文字列キーごとに引数の型を固定したラッパーです。
     *
     *  可変長引数を取る関数では、コンパイラが引数の個数および型を検査できません。
     *  書式および引数スキーマがカタログ内に保持されており、書式文字列が関数呼び出しの実引数ではないためです。
     *  型付きラッパーを経由することで、通常のプロトタイプ検査が働き、引数の個数や型の不整合をビルド時に検出できます。
     *
     *  実体を持つ翻訳単位を増やさないよう、`static inline` 関数として提供します。
     *  これにより、文字列キーの増加に伴って公開シンボルが増加するのを防ぎます。
     *
     *  関数名は文字列キーから機械的に導出します。
     *  ライブラリ側の接頭辞は前置せず、文字列キーの定数名を小文字化した名前をそのまま使用します。
     *  文字列キーにはカタログ定義ごとのモジュール接頭辞が含まれるため、
     *  この関数がどのカタログ定義に属するかは呼び出し側の名前から判別できます。
     *  語句の削除や順序の入れ替えは行わないため、導出規則に例外はありません。
     *  導出規則の全体は docs/architecture.md を参照してください。
     */
"""


SOURCE_TAIL = """\
/* Doxygen コメントは、ヘッダーに記載 */

const @LIBRARY@_entry *@MODULE@_entries(void)
{
    return s_entries;
}

/* Doxygen コメントは、ヘッダーに記載 */

int @MODULE@_entry_count(void)
{
    return @MODULE_UPPER@_ENTRY_COUNT;
}

/* Doxygen コメントは、ヘッダーに記載 */

const int *@MODULE@_key_index(void)
{
    return s_key_index;
}

/* Doxygen コメントは、ヘッダーに記載 */

int @MODULE@_key_index_count(void)
{
    return @MODULE_UPPER@_KEY_INDEX_COUNT;
}

/**
 *  @brief          本カタログ定義のカタログ識別オブジェクトです。
 *
 *  配列と添字テーブルを 1 つのカタログ構造体にまとめます。\\n
 *  すべてのメンバーを初期化子で設定可能なため `const` とし、初期化関数は提供しません。
 */
static const @LIBRARY@ s_catalog = {
    s_entries, s_key_index, @MODULE_UPPER@_ENTRY_COUNT, @MODULE_UPPER@_KEY_INDEX_COUNT};

/* Doxygen コメントは、ヘッダーに記載 */

const @LIBRARY@ *@MODULE@_catalog(void)
{
    return &s_catalog;
}

/* Doxygen コメントは、ヘッダーに記載 */

const @LIBRARY@_entry *@MODULE@_entry(const int string_key)
{
    return @LIBRARY@_get_entry(&s_catalog, string_key);
}

/* Doxygen コメントは、ヘッダーに記載 */

int @MODULE@_vformat(char *dest, const size_t dest_size, const int string_key, va_list args)
{
    return @LIBRARY@_vformat(&s_catalog, dest, dest_size, string_key, args);
}

/* Doxygen コメントは、ヘッダーに記載 */

int @MODULE@_format(char *dest, const size_t dest_size, const int string_key, ...)
{
    va_list args;
    int ret;

    va_start(args, string_key);
    ret = @LIBRARY@_vformat(&s_catalog, dest, dest_size, string_key, args);
    va_end(args);

    return ret;
}

/* Doxygen コメントは、ヘッダーに記載 */

int @MODULE@_verify(int *string_key_out, @LIBRARY@_language *language_out)
{
    return @LIBRARY@_verify(&s_catalog, string_key_out, language_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int @MODULE@_category(const int string_key)
{
    return @LIBRARY@_get_category(&s_catalog, string_key);
}

/* Doxygen コメントは、ヘッダーに記載 */

const char *@MODULE@_id(const int string_key)
{
    return @LIBRARY@_get_id(&s_catalog, string_key);
}

/* Doxygen コメントは、ヘッダーに記載 */

const char *@MODULE@_note(const int string_key)
{
    return @LIBRARY@_get_note(&s_catalog, string_key);
}
"""


def emit_source(document: dict, strings: list[dict], definition_name: str, out_relative: str = ".") -> str:
    """実装側の生成物を組み立てる。"""
    module = document["module_prefix"]
    module_upper = module.upper()
    library = LIBRARY_PREFIX
    header_name = f"{module}.h"
    source_name = f"{module}.c"
    # @file はリポジトリの慣習に合わせ、prod/ を除いた相対パスで示す
    output_dir = output_dir_display(document, out_relative)
    source_display = output_dir[len("prod/") :] if output_dir.startswith("prod/") else output_dir
    last_key = strings[-1]["key"]

    out = [
        "/**",
        " " + "*" * 79,
        f" *  @file           {source_display}/{source_name}",
        " *  @brief          文字列キーごとの引数スキーマ、分類値、メタデータ、および言語別リソースを保持します。",
        f" *  @author         {document.get('author', '')}",
        f" *  @date           {document.get('date', '')}",
        f" *  @version        {document.get('version', '')}",
        " *",
        f" *  本ファイルは、カタログ定義 `{definition_name}` から自動生成されたファイルです。\\n",
        f" *  同じ生成元から作成される `{header_name}` と合わせて 1 組の生成単位です。\\n",
        " *  手作業で直接編集せず、生成元の定義を変更してから `app/c-platform/bin/string_catalog_gen.py` を実行してください。",
        " *",
        " *  このテーブルは利用側で用意する定義情報であり、ライブラリ側では保持しません。\\n",
        " *  配列と添字テーブルを @ref s_catalog へまとめ、組み立て API の呼び出しごとに渡します。\\n",
        " *  カタログの指定を省略して呼び出すための簡易関数も、本生成物で提供します。",
        " *",
        " *  分類値はライブラリ側では解釈しない補足情報です。\\n",
        " *  意味や有効範囲は利用側で定義します。本生成物では生値のまま保持し、特定の列挙型には依存しません。",
        " *",
        " *  カタログ配列に加えて、文字列キーを添字とする添字テーブルを保持します。\\n",
        " *  ライブラリはこのテーブルを参照して文字列キーからカタログ エントリを直接引き、線形探索を回避します。",
        " *",
        " *  各要素は、定義間で一意な文字列キー、分類値、引数の個数、明示的なアラインメント、引数定義、",
        " *  補足の ID、説明文、補足説明、言語別の書式、言語別の備考の順に配置します。\\n",
        f" *  `texts` と `notes` は、@c {library}_language をキーとした指示付き初期化子で記述します。\\n",
        " *  記述を省略した言語の要素は暗黙的にヌル ポインターとなり、ニュートラル言語の要素へフォールバック（読み替え）されます。",
        " *",
        " *  引数の型と文字列表現はこのテーブルで定義し、言語別リソースでは語順のみを管理します。\\n",
        " *  書式中の `{0}` から `{49}` は引数の位置を表します。\\n",
        " *  `{` や `}` そのものを出力する場合は `{{` および `}}` と記述します。",
        " *",
        " *  ニュートラル言語の書式は、英語と同一の表現とします。\\n",
        " *  そのため英語の要素は個別に記載せず、ニュートラル言語の書式へフォールバックされます。\\n",
        " *  英語とニュートラル言語で表現を分ける必要が生じた時点で、英語の要素を追加してください。",
        " *",
        " *  ソース ファイルの文字コードおよび出力する文字列は UTF-8 です。",
        " *",
        f" *  @copyright      Copyright (C) {document.get('author', '')}. 2026. All rights reserved.",
        " *",
        " " + "*" * 79,
        " */",
        "",
        f'#include "{header_name}"',
        "",
        "#include <assert.h>",
        "#include <stdarg.h>",
        "#include <stddef.h>",
        "",
    ]

    for position, entry in enumerate(strings):
        arguments = entry["arguments"]
        if not arguments:
            continue

        out.extend(
            [
                "",
                f"/** {entry['key']} の引数定義です。 */",
                f"static const {library}_argument s_arguments_{position}[] = {{",
            ]
        )
        for argument in arguments:
            out.append(
                f"    {{{kind_constant(document, argument['kind'])}, 0, {c_string(argument['name'])}, "
                f"{c_string(argument['description'])}}},"
            )
        out.append("};")

    out.extend(
        [
            "",
            "/** 文字列キーごとのカタログ テーブルです。文字列キーの昇順に定義します。 */",
            f"static const {library}_entry s_entries[] = {{",
        ]
    )

    rows = []
    for position, entry in enumerate(strings):
        arguments = entry["arguments"]
        arguments_line = f"s_arguments_{position}" if arguments else "NULL"
        remarks_line = c_string(join_text(entry["remarks"])) if "remarks" in entry else "NULL"

        row = [
            f"    {{{entry['key']},",
            f"     {entry['category']},",
            f"     {len(arguments)},",
            "     0, /* 明示的アラインメント */",
            f"     {arguments_line},",
            f"     {c_string(entry['id']) if 'id' in entry else 'NULL'},",
            f"     {c_string(entry['brief'])},",
            f"     {c_string(entry['details']) if entry.get('details') is not None else 'NULL'},",
            f"     {remarks_line},",
        ]

        for section in ("texts", "notes"):
            items = []
            for language in LANGUAGES:
                if language in entry[section]:
                    constant = language_constant(document, language)
                    items.append(f"[{constant}] = {c_string(join_text(entry[section][language]))}")
            separator = ",\n      "
            terminator = "}," if section == "texts" else "}}"
            row.append(f"     {{{separator.join(items)}{terminator}")

        rows.append("\n".join(row))

    out.append(",\n".join(rows) + "};")
    out.extend(
        [
            "",
            "/** @ref s_entries の要素数です。 */",
            f"#define {module_upper}_ENTRY_COUNT ((int)(sizeof(s_entries) / sizeof(s_entries[0])))",
            "",
            "/** 添字テーブルにおいて、文字列キーが未登録であることを表す値です。 */",
            f"#define {module_upper}_KEY_INDEX_ABSENT (-1)",
            "",
            "/**",
            " *  @brief          文字列キーを添字として、@ref s_entries の添字を引くためのテーブルです。",
            " *",
            " *  文字列キーは 1 から始まるため、添字 0 は使用しません。\\n",
            f" *  文字列キーが連続せず欠番となる場合は、該当する添字へ @ref {module_upper}_KEY_INDEX_ABSENT を格納します。",
            " */",
            "static const int s_key_index[] = {",
            f"    {module_upper}_KEY_INDEX_ABSENT, /* 0: 未使用 */",
        ]
    )

    for position, entry in enumerate(strings):
        comma = "," if position < (len(strings) - 1) else ""
        out.append(f"    {position}{comma} /* {entry['key']} */")

    out.extend(
        [
            "};",
            "",
            "/** @ref s_key_index の要素数です。 */",
            f"#define {module_upper}_KEY_INDEX_COUNT ((int)(sizeof(s_key_index) / sizeof(s_key_index[0])))",
            "",
            "/*",
            " *  添字テーブルが最大の文字列キーまでを網羅していることを、ビルド時に検証します。",
            " *  網羅されていない文字列キーは線形探索にフォールバックするため動作自体は可能ですが、添字テーブルの拡張漏れとなります。",
            " *  対象は文字列キーの昇順で最後の定数です。",
            " */",
            f'static_assert({module_upper}_KEY_INDEX_COUNT > {last_key}, "key_index must cover every string key");',
            "",
            expand(SOURCE_TAIL, module, library).rstrip("\n"),
            "",
        ]
    )

    return "\n".join(out)


def find_clang_format_style(start: Path) -> Path | None:
    """出力先から親をたどって .clang-format を探す。"""
    for directory in [start.resolve()] + list(start.resolve().parents):
        candidate = directory / ".clang-format"
        if candidate.is_file():
            return candidate
    return None


def format_source(text: str, filename: str, style: Path | None) -> str:
    """生成した内容を clang-format へ通す。

    生成物はリポジトリの整形規則に従う必要があり、整形まで生成器の責務とする。
    そうしないと --check が常に差分を報告することになる。
    """
    if style is None or shutil.which("clang-format") is None:
        print("警告: clang-format が見つからないため、整形せずに出力します。", file=sys.stderr)
        return text

    completed = subprocess.run(
        ["clang-format", f"--style=file:{style}", f"--assume-filename={filename}"],
        input=text,
        capture_output=True,
        text=True,
        encoding="utf-8",
        check=True,
    )
    return completed.stdout


def write_text_lf(path: Path, content: str) -> None:
    """UTF-8 のテキストを LF 固定で書き込む。"""
    with path.open("w", encoding="utf-8", newline="\n") as output:
        output.write(content)


def main(argv: list[str] | None = None) -> int:
    """コマンドの入口。"""
    parser = argparse.ArgumentParser(description="カタログ定義から cplat 文字列カタログの生成物を書き出します。")
    parser.add_argument("definition", type=Path, help="カタログ定義 (JSONC) のパス")
    parser.add_argument("--out-dir", type=Path, default=None, help="出力先。既定は定義ファイルと同じ場所")
    parser.add_argument("--check", action="store_true", help="書き出さず、既存の生成物と一致するかだけ確かめる")
    parser.add_argument(
        "--if-newer",
        action="store_true",
        help="生成物が定義ファイルと生成器より新しければ何もしない。make の parse 時に呼ぶ用",
    )
    args = parser.parse_args(argv)

    try:
        document = load_definition(args.definition)
        # 名前と置き場所は定義ファイル自身から決まる。定義の中には書かない。
        document["module_prefix"] = derive_module_prefix(args.definition)
        document["module_dir"] = derive_module_dir(args.definition)
        strings = validate(document)
    except DefinitionError as error:
        print(f"エラー: {error}", file=sys.stderr)
        return 1

    out_dir = args.out_dir if args.out_dir is not None else args.definition.parent
    out_dir.mkdir(parents=True, exist_ok=True)
    definition_name = args.definition.name
    module = document["module_prefix"]

    if args.if_newer and not args.check:
        targets = [out_dir / f"{module}.h", out_dir / f"{module}.c"]
        sources = [args.definition, Path(__file__)]
        if all(target.exists() for target in targets):
            newest_source = max(source.stat().st_mtime for source in sources)
            oldest_target = min(target.stat().st_mtime for target in targets)
            if oldest_target >= newest_source:
                return 0

    # 出力先が定義ファイルと別のディレクトリなら、Doxygen の @file もその位置を指す
    out_relative = os.path.relpath(out_dir, args.definition.parent).replace("\\", "/")

    style = find_clang_format_style(out_dir)
    outputs = {
        out_dir / f"{module}.h": format_source(
            emit_header(document, strings, definition_name, out_relative), f"{module}.h", style
        ),
        out_dir / f"{module}.c": format_source(
            emit_source(document, strings, definition_name, out_relative), f"{module}.c", style
        ),
    }

    differs = False
    for path, content in outputs.items():
        if args.check:
            current = path.read_text(encoding="utf-8") if path.exists() else ""
            if current != content:
                print(f"差分あり: {path}", file=sys.stderr)
                differs = True
        else:
            write_text_lf(path, content)
            print(f"生成: {path}")

    return 1 if differs else 0


if __name__ == "__main__":
    sys.exit(main())
