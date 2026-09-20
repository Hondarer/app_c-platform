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

# カタログ定義の value に書ける文字列キーの上限。
# 添字テーブルは最大の値までを網羅するため、この上限が表の大きさ (4096 要素、16 キロバイト) を決める。
KEY_VALUE_MAX = 4095

# 公開ヘッダーとして出力した場合に、利用側の include パスを置く場所。
PUBLIC_INCLUDE_KEY = "public_include"

# カタログ定義が app 単位の設定ファイルを指す項目と、読み込んだ内容を置く場所。
SETTINGS_REFERENCE = "settings"
SETTINGS_KEY = "settings_document"

# カタログ定義の export に書ける公開範囲。省略した場合は公開しない。
# api は戻り値が cplat の構造体を指さない関数だけを公開し、full はすべてを公開する。
EXPORT_SCOPE_API = "api"
EXPORT_SCOPE_FULL = "full"
EXPORT_SCOPES = (EXPORT_SCOPE_API, EXPORT_SCOPE_FULL)

# カタログ定義の kind に書ける値。省略した場合は message として扱う。
CATALOG_KIND_MESSAGE = "message"
CATALOG_KIND_TRACE = "trace"
CATALOG_KINDS = (CATALOG_KIND_MESSAGE, CATALOG_KIND_TRACE)

# トレース種別の level に書ける値。cplat_trace_level の並びと揃える。
# 生成器は名前を分類値の整数へ変換し、生成物は cplat_trace_level へ戻して使用する。
TRACE_LEVELS = ("CRITICAL", "ERROR", "WARNING", "INFO", "VERBOSE", "DEBUG", "NONE")

# 文脈引数を置き始める位置指定。利用者が記載できる引数は、この番号の手前までとなる。
CONTEXT_ARGUMENT_BASE = 40

# トレース種別で、生成器が引数配列へ付け加える文脈引数。
# macro_value を持つものは呼び出し位置で確定するため、マクロが型付きラッパーへ渡す。
# inline_value を持つものは実行時の値のため、型付きラッパーの内部で取得する。
CONTEXT_ARGUMENTS = (
    {
        "name": "source_file_path",
        "kind": "STRING",
        "description": "呼び出し位置のソース ファイル。コンパイラへ渡した表記のままです。",
        "macro_value": "__FILE__",
    },
    {
        "name": "source_file_name",
        "kind": "STRING",
        "description": "呼び出し位置のソース ファイル名。ディレクトリを除いた表記です。",
        "macro_value": "cplat_path_basename(__FILE__)",
    },
    {
        "name": "source_line",
        "kind": "INT32",
        "description": "呼び出し位置の行番号。",
        "macro_value": "__LINE__",
    },
    {
        "name": "function_name",
        "kind": "STRING",
        "description": "呼び出し位置の関数名。",
        "macro_value": "__func__",
    },
    {
        "name": "process_id",
        "kind": "UINT32",
        "description": "出力を要求したプロセスの ID。",
        "inline_value": "cplat_process_get_pid()",
    },
    {
        "name": "thread_id",
        "kind": "UINT32",
        "description": "出力を要求したスレッドの ID。",
        "inline_value": "cplat_process_get_tid()",
    },
)

# トレース種別の生成物が追加で参照する公開ヘッダー。
TRACE_HEADERS = ("cplat/trace/tracer.h", "cplat/crt/path.h", "cplat/runtime/process.h")


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


def settings_path(definition: Path, document: dict) -> Path | None:
    """カタログ定義が指す設定ファイルのパスを返す。参照がない場合は None を返す。"""
    reference = document.get(SETTINGS_REFERENCE)
    if reference is None:
        return None
    if not isinstance(reference, str):
        raise DefinitionError(f"{SETTINGS_REFERENCE} は定義ファイルからの相対パスを文字列で指定してください。")
    return definition.parent / reference


def load_settings(definition: Path, document: dict) -> None:
    """app 単位の設定ファイルを読み込み、カタログ定義へ取り込む。

    設定は app 内の複数のカタログが共有するため、カタログ定義とは別のファイルに置く。
    参照はカタログ定義からの相対パスで書く。
    """
    path = settings_path(definition, document)
    if path is None:
        return
    if not path.is_file():
        raise DefinitionError(f"{SETTINGS_REFERENCE} が指す設定ファイルがありません: {path}")
    document[SETTINGS_KEY] = load_definition(path)


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


def validate_export(document: dict) -> None:
    """カタログを外部へ公開する設定を検査する。"""
    scope = document.get("export")
    if scope is None:
        return

    if scope not in EXPORT_SCOPES:
        raise DefinitionError(
            f"未知の公開範囲です: {scope}。{' または '.join(EXPORT_SCOPES)} を指定してください。"
        )

    settings = document.get(SETTINGS_KEY, {}).get("export")
    if settings is None:
        raise DefinitionError(
            f"export を指定する場合は、{SETTINGS_REFERENCE} が指す設定ファイルへ export を記載してください。"
        )

    for key in ("prefix", "header"):
        if key not in settings:
            raise DefinitionError(f"設定ファイルの export に {key} がありません。")
        if not isinstance(settings[key], str) or not settings[key]:
            raise DefinitionError(f"設定ファイルの export の {key} は空でない文字列で指定してください。")

    # 接頭辞からマクロ名を導く。cplat/base/dll_exports.h が定める規約と同じ形にする。
    prefix = settings["prefix"]
    if not prefix.isupper() or not prefix.replace("_", "").isalnum():
        raise DefinitionError(f"設定ファイルの export の prefix は英大文字と数字で指定してください: {prefix}")


def validate(document: dict) -> list[dict]:
    """定義の内容を検査し、文字列の一覧を返す。"""
    for key in ("strings",):
        if key not in document:
            raise DefinitionError(f"必須の項目がありません: {key}")

    if catalog_kind(document) not in CATALOG_KINDS:
        raise DefinitionError(
            f"未知のカタログ種別です: {document['kind']}。{' または '.join(CATALOG_KINDS)} を指定してください。"
        )

    validate_export(document)

    strings = document["strings"]
    if not strings:
        raise DefinitionError("strings が空です。")

    trace = is_trace(document)
    context_names = {argument["name"] for argument in CONTEXT_ARGUMENTS}
    seen_keys: set[str] = set()
    seen_values: set[int] = set()
    suffixes = MODULE_FUNCTION_SUFFIXES + (("write", "set_tracer", "get_tracer") if trace else ())
    reserved_names = {f"{document['module_prefix']}_{suffix}" for suffix in suffixes}

    for entry in strings:
        # 分類値の書き方は種別で異なる。トレース種別はトレース レベルの名前で記載する。
        classifier = "level" if trace else "category"

        # id は処理では意味を持たない補足の文字列のため、message 種別では必須にしない。
        # トレース種別では、言語の設定によらず項目を識別する文字列として必須とする。
        required = ("key", classifier, "brief", "arguments", "texts", "notes")
        for key in (required + ("id",)) if trace else required:
            if key not in entry:
                raise DefinitionError(f"{entry.get('key', '?')}: 必須の項目がありません: {key}")

        unexpected = "category" if trace else "level"
        if unexpected in entry:
            raise DefinitionError(
                f"{entry['key']}: {catalog_kind(document)} 種別では {unexpected} を指定できません。"
                f"{classifier} を使用してください。"
            )

        if trace:
            if entry["level"] not in TRACE_LEVELS:
                raise DefinitionError(
                    f"{entry['key']}: 未知のトレース レベルです: {entry['level']}。"
                    f"{'、'.join(TRACE_LEVELS)} のいずれかを指定してください。"
                )
        # 分類値は生値とする。生成物を特定の app の列挙から独立させるため。
        elif not isinstance(entry["category"], int) or isinstance(entry["category"], bool):
            raise DefinitionError(f"{entry['key']}: category は整数で指定してください。")

        for key in ("key", "brief"):
            if not isinstance(entry[key], str):
                raise DefinitionError(f"{entry['key']}: {key} は文字列で指定してください。")

        if "id" in entry and not isinstance(entry["id"], str):
            raise DefinitionError(f"{entry['key']}: id は文字列で指定してください。")

        # value は列挙値を定義で固定するための項目。添字テーブルの大きさを決めるため上限も検査する。
        if "value" in entry:
            value = entry["value"]
            if not isinstance(value, int) or isinstance(value, bool) or value < 1:
                raise DefinitionError(f"{entry['key']}: value は 1 以上の整数で指定してください。")
            if value > KEY_VALUE_MAX:
                raise DefinitionError(f"{entry['key']}: value が上限 {KEY_VALUE_MAX} を超えています。")
            if value in seen_values:
                raise DefinitionError(f"{entry['key']}: value が重複しています: {value}")
            seen_values.add(value)

        # 長文は 1 行が長くなるため、文字列の配列でも書けるようにする。連結は join_text が行う。
        for key in ("details", "remarks"):
            if key in entry and (
                not isinstance(entry[key], (str, list))
                or (isinstance(entry[key], list) and not all(isinstance(item, str) for item in entry[key]))
            ):
                raise DefinitionError(f"{entry['key']}: {key} は文字列または文字列の配列で指定してください。")

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
        argument_max = user_argument_max(document)
        if len(arguments) > argument_max:
            raise DefinitionError(f"{entry['key']}: 引数が上限 {argument_max} 個を超えています。")

        for argument in arguments:
            for key in ("kind", "name", "description"):
                if key not in argument:
                    raise DefinitionError(f"{entry['key']}: 引数に {key} がありません。")
            if argument["kind"] not in ARGUMENT_TYPES:
                raise DefinitionError(f"{entry['key']}: 未知の引数種別です: {argument['kind']}")
            if not isinstance(argument["name"], str) or not isinstance(argument["description"], str):
                raise DefinitionError(f"{entry['key']}: 引数の name と description は文字列で指定してください。")
            # 文脈引数は生成器が付け加えるため、同じ名前の引数を利用者が定義すると仮引数が重複する。
            if trace and argument["name"] in context_names:
                raise DefinitionError(
                    f"{entry['key']}: 引数名 {argument['name']} は生成器が付け加える文脈引数と重複しています。"
                )

        for section in ("texts", "notes"):
            if "neutral" not in entry[section]:
                raise DefinitionError(f"{entry['key']}: {section} に neutral が必要です。")
            for language in entry[section]:
                if language not in LANGUAGES:
                    raise DefinitionError(f"{entry['key']}: ライブラリが扱わない言語です: {language}")

        allowed = allowed_placeholder_indices(document, entry)
        for language, text in entry["texts"].items():
            indices = placeholder_indices(join_text(text))
            for found in indices:
                if found not in allowed:
                    if not trace:
                        raise DefinitionError(
                            f"{entry['key']}: {language} の位置指定 {{{found}}} が"
                            f"引数個数 {len(arguments)} を超えています。"
                        )
                    raise DefinitionError(
                        f"{entry['key']}: {language} の位置指定 {{{found}}} に引数がありません。"
                        f"利用者の引数は 0 から {len(arguments) - 1 if arguments else -1}、"
                        f"文脈引数は {CONTEXT_ARGUMENT_BASE} から "
                        f"{CONTEXT_ARGUMENT_BASE + len(CONTEXT_ARGUMENTS) - 1} です。"
                    )

    # 値を固定した項目と並び順から決める項目が混在すると、暗黙の値が固定した値と衝突しうる。
    # 混在を許すと衝突の有無が並び順に依存するため、カタログ単位でどちらかに揃える。
    if seen_values and (len(seen_values) != len(strings)):
        raise DefinitionError("value は、カタログのすべての文字列へ記載するか、すべてで省略してください。")

    return strings


GENERATED_NOTE =""" *  本ヘッダーと `{source}` は、カタログ定義 `{definition}` から自動生成されたファイルです。\\n
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


def catalog_kind(document: dict) -> str:
    """カタログ定義の種別を返す。記載がない場合は message として扱う。"""
    return document.get("kind", CATALOG_KIND_MESSAGE)


def is_trace(document: dict) -> bool:
    """カタログ定義がトレース種別かどうかを返す。"""
    return catalog_kind(document) == CATALOG_KIND_TRACE


def key_value(entry: dict, position: int) -> int:
    """文字列キーの列挙値を返す。

    カタログ定義に value がある場合はその値とする。外部へ公開するカタログでは、
    定義の並べ替えや途中への挿入で値が変わらないようにするために記載する。
    記載がない場合は、従来どおり並び順から 1 始まりで決める。
    """
    return entry["value"] if "value" in entry else (position + 1)


def key_values(strings: list[dict]) -> list[int]:
    """文字列の一覧から、列挙値を並び順で返す。"""
    return [key_value(entry, position) for position, entry in enumerate(strings)]


def trace_level_value(level: str) -> int:
    """トレース レベルの名前を、分類値として保持する整数へ変換する。"""
    return TRACE_LEVELS.index(level)


def context_argument_count(document: dict) -> int:
    """生成器が付け加える文脈引数の個数を返す。"""
    return len(CONTEXT_ARGUMENTS) if is_trace(document) else 0


def user_argument_max(document: dict) -> int:
    """利用者がカタログ定義へ記載できる引数の上限を返す。"""
    return CONTEXT_ARGUMENT_BASE if is_trace(document) else ARGUMENT_MAX


def argument_array_length(document: dict, entry: dict) -> int:
    """生成物の引数配列の要素数を返す。

    トレース種別では、利用者の引数と文脈引数の間に、値を受け取らないインデックスが並ぶ。
    要素数は最後に使用するインデックスに 1 を加えた値となる。
    """
    if not is_trace(document):
        return len(entry["arguments"])
    return CONTEXT_ARGUMENT_BASE + len(CONTEXT_ARGUMENTS)


def allowed_placeholder_indices(document: dict, entry: dict) -> set[int]:
    """書式の位置指定が指してよいインデックスを返す。"""
    indices = set(range(len(entry["arguments"])))
    if is_trace(document):
        indices |= set(range(CONTEXT_ARGUMENT_BASE, CONTEXT_ARGUMENT_BASE + len(CONTEXT_ARGUMENTS)))
    return indices


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


def parameter_declaration(argument: dict) -> str:
    """引数定義から、型付きラッパーの仮引数の宣言を組み立てる。"""
    c_type = ARGUMENT_TYPES[argument["kind"]]
    if c_type.endswith("*"):
        return f"{c_type}{argument['name']}"
    return f"const {c_type} {argument['name']}"


def format_par_lines(entry: dict) -> list[str]:
    """言語別の書式を @par として並べる。"""
    lines = ["     *  @par            書式"]
    texts = entry["texts"]
    languages = [language for language in LANGUAGES if language in texts]
    for position, language in enumerate(languages):
        suffix = "\\n" if position < (len(languages) - 1) else ""
        lines.append(f"     *  `{join_text(texts[language])}`{suffix}")
    return lines


def remark_doc_lines(entry: dict) -> list[str]:
    """定義の remarks を @remark の行にする。"""
    remark_text = join_text(entry["remarks"]) if entry.get("remarks") else ""
    if not remark_text:
        return []

    remark_indent = "     *" + " " * 18
    remark_lines = doc_lines(remark_text, remark_indent)
    return [f"     *  @remark         {remark_lines[0][len(remark_indent):]}"] + remark_lines[1:]


def trace_context_doc_lines() -> list[str]:
    """文脈引数の位置指定と内容の対応表を、ヘッダーのファイル コメント用に組み立てる。

    定義作成者が書式から文脈値を参照するには、どの番号が何かを知る必要がある。
    対応表は CONTEXT_ARGUMENTS から組み立て、生成器の定義と食い違わないようにする。
    """
    lines = [
        " *  本カタログはトレース種別です。呼び出し位置と実行文脈を、生成器が引数として付け加えます。\\n",
        f" *  利用者が記載した引数は `{{0}}` から順に並び、"
        f"`{{{CONTEXT_ARGUMENT_BASE}}}` から次の文脈引数が並びます。",
        " *",
        " *  | 位置指定 | 引数名 | 引数種別 | 値 |",
        " *  | --- | --- | --- | --- |",
    ]

    for offset, argument in enumerate(CONTEXT_ARGUMENTS):
        index = CONTEXT_ARGUMENT_BASE + offset
        kind = kind_constant({}, argument["kind"])
        lines.append(f" *  | `{{{index}}}` | {argument['name']} | {kind} | {argument['description']} |")

    last_index = CONTEXT_ARGUMENT_BASE + len(CONTEXT_ARGUMENTS) - 1
    lines.extend(
        [
            " *",
            " *  言語別の書式へこれらの位置指定を書くと、組み立てた文字列へ文脈値が現れます。\\n",
            " *  書かない場合は現れません。実装を変えずに、定義の変更だけで切り替えられます。",
            " *",
            f" *  記載した引数の個数から `{{{CONTEXT_ARGUMENT_BASE - 1}}}` までは、値を受け取らないインデックスです。\\n",
            f" *  書式から参照すると定義の誤りになります。使用できるのは利用者の引数と "
            f"`{{{CONTEXT_ARGUMENT_BASE}}}` から `{{{last_index}}}` までです。",
        ]
    )

    return lines


def emit_trace_wrapper(document: dict, entry: dict) -> str:
    """1 件分のトレース出力ラッパーを、マクロとともに書き出す。"""
    name = wrapper_name(entry["key"])
    module = document["module_prefix"]
    arguments = entry["arguments"]
    macro_context = [argument for argument in CONTEXT_ARGUMENTS if "macro_value" in argument]
    inline_context = [argument for argument in CONTEXT_ARGUMENTS if "inline_value" in argument]
    user_names = [argument["name"] for argument in arguments]

    names = user_names + [argument["name"] for argument in macro_context]
    name_width = max(len(name_item) for name_item in names) + 1
    # `     *  ` の 8 文字と、`@param[in]      ` の 16 文字のあとに名前欄が並ぶ
    continuation = "     *" + " " * (8 + 16 + name_width - 6)

    lines = ["    /**"]
    lines.append(f"     *  @brief          {entry['brief']}")
    lines.append("     *")
    # 1 文ずつ改行する。長い 1 行にすると、整形時に @c と対象の間で折り返される。
    lines.append(f"     *  関数形式マクロ @c {name} の実体です。\\n")
    lines.append("     *  呼び出し位置は展開の位置で確定する必要があるため、マクロから受け取ります。\\n")
    lines.append("     *  呼び出し側はマクロを使用してください。")
    lines.append("     *")

    for argument in arguments + macro_context:
        padded = argument["name"].ljust(name_width)
        lines.append(f"     *  @param[in]      {padded}{argument['description']}")
        lines.append(f"{continuation}引数種別は @c {kind_constant(document, argument['kind'])} です。")

    lines.append(f"     *  @return         戻り値は @c {module}_write と同じです。")
    lines.append("     */")

    parameters = [parameter_declaration(argument) for argument in arguments + macro_context]

    call = [entry["key"]]
    call.extend(user_names)
    call.extend(argument["name"] for argument in macro_context)
    call.extend(argument["inline_value"] for argument in inline_context)

    lines.append(f"    static inline int {name}_with_source({', '.join(parameters)})")
    lines.append("    {")
    lines.append(f"        return {module}_write({', '.join(call)});")
    lines.append("    }")
    lines.append("")

    macro_names = list(user_names)
    macro_width = (max(len(name_item) for name_item in macro_names) + 1) if macro_names else 1
    macro_continuation = "     *" + " " * (8 + 16 + macro_width - 6)

    lines.append("    /**")
    lines.append(f"     *  @brief          {entry['brief']}")
    lines.append("     *")
    details_text = join_text(entry["details"]) if entry.get("details") else ""
    if details_text:
        lines.extend(doc_lines(details_text, "     *  "))
        lines.append("     *")
    lines.append("     *  ソース ファイル、行番号、関数名、プロセス ID、スレッド ID を呼び出しごとに付けて出力します。")
    lines.append(f"     *  出力先は @c {module}_set_tracer で設定したトレーサーです。")
    lines.append("     *")
    for argument in arguments:
        padded = argument["name"].ljust(macro_width)
        lines.append(f"     *  @param[in]      {padded}{argument['description']}")
        lines.append(f"{macro_continuation}引数種別は @c {kind_constant(document, argument['kind'])} です。")
    lines.append(f"     *  @return         戻り値は @c {module}_write と同じです。")
    lines.extend(remark_doc_lines(entry))
    lines.extend(format_par_lines(entry))
    lines.append("     */")

    macro_tail = ", ".join(argument["macro_value"] for argument in macro_context)
    macro_arguments = "".join(f"({name_item}), " for name_item in macro_names) + macro_tail
    lines.append(f"#define {name}({', '.join(macro_names)}) {name}_with_source({macro_arguments})")

    return "\n".join(lines)


def emit_wrapper(document: dict, entry: dict) -> str:
    """1 件分の型付きラッパーを、Doxygen コメントとともに書き出す。"""
    if is_trace(document):
        return emit_trace_wrapper(document, entry)

    name = wrapper_name(entry["key"])
    arguments = entry["arguments"]

    names = ["dest", "dest_size"] + [argument["name"] for argument in arguments]
    name_width = max(len(name) for name in names) + 1
    # `     *  ` の 8 文字と、`@param[out]     ` の 16 文字のあとに名前欄が並ぶ
    continuation = "     *" + " " * (8 + 16 + name_width - 6)

    lines = ["    /**"]
    lines.append(f"     *  @brief          {entry['brief']}")
    lines.append("     *")
    details_text = join_text(entry["details"]) if entry.get("details") else ""
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

    module = document["module_prefix"]
    lines.append(f"     *  @return         戻り値は @c {module}_format と同じです。")

    lines.extend(remark_doc_lines(entry))
    lines.extend(format_par_lines(entry))
    lines.append("     */")

    parameters = ["char *dest", "const size_t dest_size"]
    parameters.extend(parameter_declaration(argument) for argument in arguments)

    # カタログ オブジェクトを補う簡易関数を経由する。ラッパーを展開する呼び出し側が、
    # cplat の関数と cplat_string_catalog のレイアウトへ依存しないようにするため。
    call = ["dest", "dest_size", entry["key"]]
    call.extend(argument["name"] for argument in arguments)

    lines.append(f"    static inline int {name}({', '.join(parameters)})")
    lines.append("    {")
    lines.append(f"        return {module}_format({', '.join(call)});")
    lines.append("    }")

    return "\n".join(lines)


def export_macros(document: dict) -> dict[str, str]:
    """エクスポート装飾の目印に対する置換文字列を返す。

    目印は空でない場合に末尾の空白を含む。公開しない場合は空文字列となり、
    装飾のない宣言がそのまま残る。
    """
    empty = {"@EXPORT@": "", "@API@": "", "@EXPORT_FULL@": "", "@API_FULL@": ""}
    scope = document.get("export")
    if scope is None:
        return empty

    prefix = document[SETTINGS_KEY]["export"]["prefix"]
    macros = dict(empty)
    macros["@EXPORT@"] = f"{prefix}_EXPORT "
    macros["@API@"] = f"{prefix}_API "
    if scope == EXPORT_SCOPE_FULL:
        macros["@EXPORT_FULL@"] = macros["@EXPORT@"]
        macros["@API_FULL@"] = macros["@API@"]
    return macros


def expand(template: str, module: str, library: str, macros: dict[str, str] | None = None) -> str:
    """テンプレート中の接頭辞の目印を置き換える。

    テンプレートは C のコードを含み波括弧が現れるため、str.format は使わない。
    """
    text = (
        template.replace("@MODULE_UPPER@", module.upper())
        .replace("@MODULE@", module)
        .replace("@LIBRARY@", library)
    )
    for marker, replacement in (macros or export_macros({})).items():
        text = text.replace(marker, replacement)
    return text


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


def header_include_path(header_dir: Path, module: str) -> str | None:
    """公開ヘッダーとして取り込む場合の include パスを返す。

    出力先が公開ヘッダーの置き場所 (prod/include/ 配下) である場合、そこからの相対パスが
    利用側の `#include <...>` に現れる。置き場所から導けない場合は None を返す。
    """
    parts = header_dir.resolve().parts
    for marker in ("include", "include_internal"):
        if marker in parts:
            index = len(parts) - 1 - parts[::-1].index(marker)
            return "/".join(parts[index + 1 :] + (f"{module}.h",))
    return None


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
    macros = export_macros(document)
    guard = f"{module.upper()}_H"
    header_name = f"{module}.h"
    source_name = f"{module}.c"
    module_dir = document.get("module_dir", ".")
    output_dir = output_dir_display(document, out_relative)
    include_path = header_name if output_dir == module_dir else f"{out_relative}/{header_name}"
    public_include = document.get(PUBLIC_INCLUDE_KEY)

    out = [
        "/**",
        " " + "*" * 79,
        f" *  @file           {header_name}",
        f" *  @brief          利用者が定義する文字列キーの列挙型と、カタログ取得関数を宣言します。",
        f" *  @author         {document.get('author', '')}",
        f" *  @date           {document.get('date', '')}",
        f" *  @version        {document.get('version', '')}",
        " *",
        *(
            [
                f" *  本ヘッダーは `{output_dir}/` の公開ヘッダーです。\\n",
                f" *  利用側は `#include <{public_include}>` でインクルードします。",
            ]
            if public_include is not None
            else [
                f" *  本ヘッダーは `{output_dir}/` のモジュール私有ヘッダーです。\\n",
                f' *  `{module_dir}/` の実装ファイルからのみ `#include "{include_path}"` でインクルードします。',
            ]
        ),
        " *",
        GENERATED_NOTE.format(source=source_name, definition=definition_name),
        " *",
        " *  この 2 ファイルは、文字列カタログを利用するアプリケーション側で用意するファイルです。\\n",
        " *  言語、引数種別、書式構文はライブラリ側で規定されます。\\n",
        " *  分類値の意味付けは利用側の取り決めであり、別ヘッダーで個別に定義します。",
        " *",
    ]

    if is_trace(document):
        out.extend(trace_context_doc_lines())
        out.append(" *")

    out += [
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

    includes = [f"#include <{LIBRARY_HEADER}>"]
    # トレース種別は、出力先のハンドルと、文脈引数の値を取得する API を参照する。
    includes.extend(f"#include <{header}>" for header in (TRACE_HEADERS if is_trace(document) else ()))
    # 公開するカタログは、app のエクスポート マクロを定義するヘッダーを参照する。
    if document.get("export") is not None:
        includes.append(f"#include <{document[SETTINGS_KEY]['export']['header']}>")

    out.extend(
        includes
        + [
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
            "     *  列挙値はカタログ定義の value です。value の記載がない場合は定義の並び順に基づいて割り当てられます。\\n",
            "     *  定義の id は処理では意味を持たないため、列挙には現れません。",
            "     */",
            f"    typedef enum {key_enum_name(document)}",
            "    {",
        ]
    )

    for position, entry in enumerate(strings):
        comma = "," if position < (len(strings) - 1) else ""
        out.append(f"        {entry['key']} = {key_value(entry, position)}{comma} /**< {entry['brief']} */")

    out.append(f"    }} {key_enum_name(document)};")
    out.append("")
    out.append(expand(ACCESSOR_DECLARATIONS, module, library, macros))
    if is_trace(document):
        out.append(expand(TRACE_WRITE_DECLARATION, module, library, macros))
    out.extend(
        [
            "#ifdef __cplusplus",
            "}",
            "#endif /* __cplusplus */",
            "",
        ]
    )

    wrapper_group_id = f"{group_id}_TYPED_FORMATTERS"
    wrapper_group_title = (
        "文字列キーごとの型付きトレース出力関数" if is_trace(document) else "文字列キーごとの型付き組み立て関数"
    )
    wrapper_group_brief = (
        "文字列キーごとに引数の型を固定したトレース出力関数です。"
        if is_trace(document)
        else "文字列キーごとに引数の型を固定した組み立て関数です。"
    )
    out.extend(
        [
            "/**",
            f" *  @defgroup       {wrapper_group_id} {wrapper_group_title}",
            f" *  @brief          {wrapper_group_brief}",
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
    @EXPORT_FULL@const @LIBRARY@_entry *@API_FULL@@MODULE@_entries(void);

    /**
     *  @brief          カタログの登録件数を取得します。
     *  @return         文字列の登録件数です。1 以上を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。読み取り専用の静的データだけを参照します。
     */
    @EXPORT@int @API@@MODULE@_entry_count(void);

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
    @EXPORT_FULL@const int *@API_FULL@@MODULE@_key_index(void);

    /**
     *  @brief          添字テーブルの要素数を取得します。
     *  @return         添字テーブルの要素数です。最大の文字列キーに 1 を加えた値となります。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。読み取り専用の静的データだけを参照します。
     */
    @EXPORT_FULL@int @API_FULL@@MODULE@_key_index_count(void);

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
    @EXPORT_FULL@const @LIBRARY@ *@API_FULL@@MODULE@_catalog(void);

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
    @EXPORT_FULL@const @LIBRARY@_entry *@API_FULL@@MODULE@_entry(int string_key);

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
    @EXPORT@int @API@@MODULE@_format(char *dest, size_t dest_size, int string_key, ...);

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
    @EXPORT@int @API@@MODULE@_vformat(char *dest, size_t dest_size, int string_key, va_list args);

    /**
     *  @brief          本カタログ定義の内容を確認します。
     *  @param[out]     string_key_out 不正を検出した文字列キーの格納先。不要な場合は NULL を指定できます。
     *  @param[out]     language_out   不正を検出した言語の格納先。不要な場合は NULL を指定できます。
     *  @return         戻り値は @c @LIBRARY@_verify と同じです。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。
     */
    @EXPORT_FULL@int @API_FULL@@MODULE@_verify(int *string_key_out, @LIBRARY@_language *language_out);

    /**
     *  @brief          本カタログ定義から、文字列の分類値を取得します。
     *  @param[in]      string_key 参照する文字列のキー。
     *  @return         戻り値は @c @LIBRARY@_get_category と同じです。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。
     */
    @EXPORT@int @API@@MODULE@_category(int string_key);

    /**
     *  @brief          本カタログ定義から、文字列キーに対応する ID を取得します。
     *  @param[in]      string_key 参照する文字列のキー。
     *  @return         戻り値は @c @LIBRARY@_get_id と同じです。ID が未設定の項目では NULL を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。
     */
    @EXPORT@const char *@API@@MODULE@_id(int string_key);

    /**
     *  @brief          本カタログ定義から、現在の言語設定における文字列の備考を取得します。
     *  @param[in]      string_key 参照する文字列のキー。
     *  @return         戻り値は @c @LIBRARY@_get_note と同じです。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。
     */
    @EXPORT@const char *@API@@MODULE@_note(int string_key);

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


TRACE_WRITE_DECLARATION = """\
    /**
     *  @brief          本カタログの出力先となるトレーサーを設定します。
     *  @param[in]      tracer 出力先のトレーサー ハンドル。NULL を指定すると出力しない状態へ戻します。
     *
     *  設定したトレーサーは、本カタログのすべての出力で使用します。\\n
     *  呼び出しごとにトレーサーを指定する必要をなくすため、カタログ単位で保持します。
     *
     *  トレーサーの所有権は移りません。\\n
     *  設定を外す呼び出しは、原則として不要です。トレーサーはプロセスの終了時に cplat が破棄し、
     *  その後に出力を要求する経路がないためです。
     *
     *  トレーサーを明示的に破棄し、そのあとに出力を要求しうる場合に限り、破棄の前に NULL を設定してください。\\n
     *  解放済みのハンドルを参照しないようにするためです。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\\n
     *  出力を行うスレッドと並行して呼び出さないでください。\\n
     *  出力を開始する前に設定し、以降は変更しない使い方を想定しています。
     */
    @EXPORT@void @API@@MODULE@_set_tracer(cplat_tracer *tracer);

    /**
     *  @brief          本カタログの出力先に設定されているトレーサーを取得します。
     *  @return         設定されているトレーサー ハンドルです。未設定の場合は NULL を返します。
     *
     *  設定を一時的に差し替える場合に、元のハンドルを保存するために使用します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。@c @MODULE@_set_tracer と並行して呼び出さないでください。
     */
    @EXPORT@cplat_tracer *@API@@MODULE@_get_tracer(void);

    /**
     *  @brief          本カタログ定義を使用して、組み立てた文字列をトレースへ出力します。
     *  @param[in]      string_key 出力する文字列のキー。
     *  @param[in]      ...        引数スキーマが定める順序と型の引数リスト。
     *  @return         トレーサーが未設定の場合は @ref CPLAT_ERR_INVALID_ARGUMENT を返します。
     *  @return         組み立てに失敗した場合は @c @LIBRARY@_format と同じ値を返します。
     *  @return         組み立てに成功した場合は @c cplat_tracer_write_at と同じ値を返します。
     *
     *  文字列キーごとの型付きラッパーが呼び出す関数です。\\n
     *  引数の個数と型の検査を働かせるため、呼び出し側は型付きラッパーのマクロを使用してください。
     *
     *  出力先は @c @MODULE@_set_tracer で設定したトレーサーです。\\n
     *  未設定の場合は、文字列の組み立ても出力も行わずに失敗を返します。
     *  設定の漏れが成功として隠れないようにするためです。
     *
     *  トレース レベルは、カタログ定義の level から変換した分類値を使用します。\\n
     *  呼び出し位置は引数として受け取るため、トレース側で重ねて付与しません。
     *
     *  @par            スレッド セーフ
     *  スレッド セーフ性は @c cplat_tracer_write_at と同じです。\\n
     *  ただし、出力先の設定を変更している間は並行して呼び出せません。
     */
    @EXPORT@int @API@@MODULE@_write(int string_key, ...);
"""


TRACE_SOURCE_TAIL = """\
/** 本カタログの出力先です。@ref @MODULE@_set_tracer で設定します。 */
static cplat_tracer *s_tracer = NULL;

/* Doxygen コメントは、ヘッダーに記載 */

void @MODULE@_set_tracer(cplat_tracer *tracer)
{
    s_tracer = tracer;
}

/* Doxygen コメントは、ヘッダーに記載 */

cplat_tracer *@MODULE@_get_tracer(void)
{
    return s_tracer;
}

/* Doxygen コメントは、ヘッダーに記載 */

int @MODULE@_write(const int string_key, ...)
{
    char text[CPLAT_STRING_CATALOG_TEXT_MAX];
    va_list args;
    int ret;

    /* 出力先が未設定なら、組み立てを行わずに失敗を返す。設定の漏れを成功として隠さないため */
    if (s_tracer == NULL)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }

    va_start(args, string_key);
    ret = @LIBRARY@_vformat(&s_catalog, text, sizeof(text), string_key, args);
    va_end(args);

    if (ret != CPLAT_OK)
    {
        return ret;
    }

    /* 呼び出し位置は引数として渡しているため、呼び出し位置を付与しない API を使う */
    return cplat_tracer_write_at(s_tracer, (cplat_trace_level)@MODULE@_category(string_key), NULL, text);
}
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
    # 添字テーブルの網羅を検証する基準は、値が最大の文字列キーとする。
    # 値を定義で固定した場合、並び順の最後が最大とは限らない。
    largest_key = max(zip(key_values(strings), (entry["key"] for entry in strings)))[1]

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
        (
            f"#include <{document[PUBLIC_INCLUDE_KEY]}>"
            if document.get(PUBLIC_INCLUDE_KEY) is not None
            else f'#include "{header_name}"'
        ),
        "",
        "#include <assert.h>",
        "#include <stdarg.h>",
        "#include <stddef.h>",
        "",
    ]

    trace = is_trace(document)

    for position, entry in enumerate(strings):
        arguments = entry["arguments"]
        if not arguments and not trace:
            continue

        brief = f"/** {entry['key']} の引数定義です。 */"
        if trace:
            brief = (
                f"/** {entry['key']} の引数定義です。"
                f"{CONTEXT_ARGUMENT_BASE} 番から先は生成器が付け加える文脈引数です。 */"
            )

        out.extend(
            [
                "",
                brief,
                f"static const {library}_argument s_arguments_{position}[] = {{",
            ]
        )
        for argument in arguments:
            out.append(
                f"    {{{kind_constant(document, argument['kind'])}, 0, {c_string(argument['name'])}, "
                f"{c_string(argument['description'])}}},"
            )
        if trace:
            if len(arguments) < CONTEXT_ARGUMENT_BASE:
                out.append(
                    f"    /* {len(arguments)} 番から {CONTEXT_ARGUMENT_BASE - 1} 番は、"
                    "要素を明示しないことで値を受け取らないインデックスになります。 */"
                )
            for offset, argument in enumerate(CONTEXT_ARGUMENTS):
                out.append(
                    f"    [{CONTEXT_ARGUMENT_BASE + offset}] = "
                    f"{{{kind_constant(document, argument['kind'])}, 0, {c_string(argument['name'])}, "
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
        arguments_line = f"s_arguments_{position}" if (arguments or trace) else "NULL"
        remarks_line = c_string(join_text(entry["remarks"])) if "remarks" in entry else "NULL"
        category = trace_level_value(entry["level"]) if trace else entry["category"]

        row = [
            f"    {{{entry['key']},",
            f"     {category},",
            f"     {argument_array_length(document, entry)},",
            "     0, /* 明示的アラインメント */",
            f"     {arguments_line},",
            f"     {c_string(entry['id']) if 'id' in entry else 'NULL'},",
            f"     {c_string(entry['brief'])},",
            f"     {c_string(join_text(entry['details'])) if entry.get('details') is not None else 'NULL'},",
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

    # 値を定義で固定したカタログでは欠番が生じる。表は最大の値までを網羅する。
    position_of_value = {value: position for position, value in enumerate(key_values(strings))}
    key_of_value = {key_value(entry, position): entry["key"] for position, entry in enumerate(strings)}
    for value in range(1, max(position_of_value) + 1):
        comma = "," if value < max(position_of_value) else ""
        if value in position_of_value:
            out.append(f"    {position_of_value[value]}{comma} /* {key_of_value[value]} */")
        else:
            out.append(f"    {module_upper}_KEY_INDEX_ABSENT{comma} /* {value}: 欠番 */")

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
            " *  対象は、値が最大の文字列キーの定数です。",
            " */",
            f'static_assert({module_upper}_KEY_INDEX_COUNT > {largest_key}, "key_index must cover every string key");',
            "",
            expand(SOURCE_TAIL, module, library).rstrip("\n"),
            "",
        ]
    )

    if trace:
        out.extend(["", expand(TRACE_SOURCE_TAIL, module, library).rstrip("\n"), ""])

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
    parser.add_argument(
        "--header-dir",
        type=Path,
        default=None,
        help="ヘッダーの出力先。既定は --out-dir と同じ場所。公開ヘッダーを分けて置く場合に指定する",
    )
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
        load_settings(args.definition, document)
        strings = validate(document)
    except DefinitionError as error:
        print(f"エラー: {error}", file=sys.stderr)
        return 1

    out_dir = args.out_dir if args.out_dir is not None else args.definition.parent
    out_dir.mkdir(parents=True, exist_ok=True)
    header_dir = args.header_dir if args.header_dir is not None else out_dir
    header_dir.mkdir(parents=True, exist_ok=True)
    # ヘッダーを別の場所へ出す場合、利用側は公開ヘッダーの置き場所からの相対パスで取り込む。
    if header_dir.resolve() != out_dir.resolve():
        document[PUBLIC_INCLUDE_KEY] = header_include_path(header_dir, document["module_prefix"])
    definition_name = args.definition.name
    module = document["module_prefix"]

    if args.if_newer and not args.check:
        targets = [header_dir / f"{module}.h", out_dir / f"{module}.c"]
        # 設定ファイルを変えた場合も再生成する。app 内の複数のカタログが同じ設定を共有するため。
        settings = settings_path(args.definition, document)
        sources = [args.definition, Path(__file__)] + ([settings] if settings is not None else [])
        if all(target.exists() for target in targets):
            newest_source = max(source.stat().st_mtime for source in sources)
            oldest_target = min(target.stat().st_mtime for target in targets)
            if oldest_target >= newest_source:
                return 0

    # 出力先が定義ファイルと別のディレクトリなら、Doxygen の @file もその位置を指す
    out_relative = os.path.relpath(out_dir, args.definition.parent).replace("\\", "/")
    header_relative = os.path.relpath(header_dir, args.definition.parent).replace("\\", "/")

    style = find_clang_format_style(out_dir)
    outputs = {
        header_dir / f"{module}.h": format_source(
            emit_header(document, strings, definition_name, header_relative), f"{module}.h", style
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
