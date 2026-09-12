#!/usr/bin/env python3
"""string_catalog_gen.py の単体テスト。"""

from __future__ import annotations

import unittest
from pathlib import Path
from unittest import mock

import string_catalog_gen as gen


class StripJsoncTest(unittest.TestCase):
    """JSONC の前処理を確認する。"""

    def test_line_comment(self):
        self.assertEqual(gen.strip_jsonc('{"a": 1} // 末尾'), '{"a": 1} ')

    def test_block_comment(self):
        self.assertEqual(gen.strip_jsonc('{/* 中 */"a": 1}'), '{"a": 1}')

    def test_keeps_comment_like_text_in_string(self):
        source = '{"url": "https://example.com/a//b", "path": "/*not a comment*/"}'
        self.assertEqual(gen.strip_jsonc(source), source)

    def test_keeps_escaped_quote(self):
        source = '{"text": "彼は \\"はい\\" と言った // ではない"}'
        self.assertEqual(gen.strip_jsonc(source), source)

    def test_removes_trailing_comma(self):
        self.assertEqual(gen.strip_jsonc('{"a": 1,}'), '{"a": 1}')
        self.assertEqual(gen.strip_jsonc('[1, 2,\n]'), '[1, 2\n]')

    def test_keeps_comma_in_string(self):
        source = '{"text": "a,}"}'
        self.assertEqual(gen.strip_jsonc(source), source)


class JoinTextTest(unittest.TestCase):
    """文字列と文字列配列の受け取りを確認する。"""

    def test_string(self):
        self.assertEqual(gen.join_text("あ"), "あ")

    def test_list(self):
        self.assertEqual(gen.join_text(["あ", "い"]), "あ い")

    def test_rejects_other(self):
        with self.assertRaises(gen.DefinitionError):
            gen.join_text(1)


class PlaceholderTest(unittest.TestCase):
    """位置指定の解析が、実装と同じ構文を受け付けることを確認する。"""

    def test_single_digit(self):
        self.assertEqual(gen.placeholder_indices("{0}/{9}"), [0, 9])

    def test_two_digits(self):
        self.assertEqual(gen.placeholder_indices("{10}/{31}"), [10, 31])

    def test_escape(self):
        self.assertEqual(gen.placeholder_indices("{{ default }}"), [])

    def test_rejects_three_digits(self):
        with self.assertRaises(gen.DefinitionError):
            gen.placeholder_indices("{100}")

    def test_rejects_leading_zero(self):
        with self.assertRaises(gen.DefinitionError):
            gen.placeholder_indices("{01}")

    def test_rejects_unpaired_brace(self):
        with self.assertRaises(gen.DefinitionError):
            gen.placeholder_indices("a}b")

    def test_rejects_non_digit(self):
        with self.assertRaises(gen.DefinitionError):
            gen.placeholder_indices("{abc}")


def minimal_document(**overrides):
    """検査の対象となる最小の定義を組み立てる。"""
    document = {
        "module_prefix": "sample_messages",
        "strings": [
            {
                "id": "SAMPLE_MESSAGES_ID_A",
                "id_text": "ID_0001",
                "category": 1,
                "brief": "あ。",
                "summary": "あを組み立てます。",
                "arguments": [{"kind": "STRING", "name": "path", "description": "パス。"}],
                "texts": {"neutral": "{0}"},
                "notes": {"neutral": ""},
            }
        ],
    }
    document.update(overrides)
    return document


class ValidateTest(unittest.TestCase):
    """定義の検査を確認する。"""

    def test_accepts_minimal(self):
        self.assertEqual(len(gen.validate(minimal_document())), 1)

    def test_does_not_require_derived_keys(self):
        # 生成器が持つ名前と仕様は、カタログ定義へ書かない
        document = minimal_document()
        for key in ("library_prefix", "id_enum", "module_dir", "languages"):
            self.assertNotIn(key, document)
        self.assertEqual(len(gen.validate(document)), 1)

    def test_rejects_duplicate_id(self):
        document = minimal_document()
        document["strings"].append(dict(document["strings"][0]))
        document["strings"][1]["id_text"] = "ID_0002"
        with self.assertRaises(gen.DefinitionError):
            gen.validate(document)

    def test_rejects_duplicate_id_text(self):
        document = minimal_document()
        duplicated = dict(document["strings"][0])
        duplicated["id"] = "SAMPLE_MESSAGES_ID_B"
        document["strings"].append(duplicated)
        with self.assertRaises(gen.DefinitionError):
            gen.validate(document)

    def test_rejects_unknown_kind(self):
        document = minimal_document()
        document["strings"][0]["arguments"][0]["kind"] = "FLOAT"
        with self.assertRaises(gen.DefinitionError):
            gen.validate(document)

    def test_rejects_placeholder_over_argument_count(self):
        document = minimal_document()
        document["strings"][0]["texts"]["neutral"] = "{1}"
        with self.assertRaises(gen.DefinitionError):
            gen.validate(document)

    def test_rejects_missing_neutral_text(self):
        document = minimal_document()
        document["strings"][0]["texts"] = {"japanese": "{0}"}
        with self.assertRaises(gen.DefinitionError):
            gen.validate(document)

    def test_rejects_unlisted_language(self):
        document = minimal_document()
        document["strings"][0]["texts"]["german"] = "{0}"
        with self.assertRaises(gen.DefinitionError):
            gen.validate(document)

    def test_rejects_symbolic_category(self):
        # 分類値を生値に限るのは、生成物を特定の app の列挙から独立させるため
        document = minimal_document()
        document["strings"][0]["category"] = "SAMPLE_TRACE_LEVEL_ERROR"
        with self.assertRaises(gen.DefinitionError):
            gen.validate(document)

    def test_rejects_boolean_category(self):
        document = minimal_document()
        document["strings"][0]["category"] = True
        with self.assertRaises(gen.DefinitionError):
            gen.validate(document)

    def test_rejects_too_many_arguments(self):
        document = minimal_document()
        argument = document["strings"][0]["arguments"][0]
        document["strings"][0]["arguments"] = [dict(argument) for _ in range(gen.ARGUMENT_MAX + 1)]
        with self.assertRaises(gen.DefinitionError):
            gen.validate(document)

    def test_rejects_wrapper_name_colliding_with_a_simple_function(self):
        # 型付きラッパーはライブラリ接頭辞を前置しないため、
        # 文字列 ID の小文字形が同じ生成物の簡易関数名と衝突しうる。
        # 衝突対象は ACCESSOR_DECLARATIONS (および SOURCE_TAIL) が実際に @MODULE@_ に続けて
        # 出力する名前一覧 (gen.MODULE_FUNCTION_SUFFIXES) のすべてで確認する。
        for suffix in gen.MODULE_FUNCTION_SUFFIXES:
            with self.subTest(suffix=suffix):
                document = minimal_document()
                document["strings"][0]["id"] = f"SAMPLE_MESSAGES_{suffix.upper()}"
                with self.assertRaises(gen.DefinitionError):
                    gen.validate(document)

    def test_accepts_id_that_does_not_collide(self):
        # 通常の文字列 ID では衝突検査に引っかからないことを確認する (回帰防止)。
        document = minimal_document()
        self.assertEqual(len(gen.validate(document)), 1)


class WrapperNameTest(unittest.TestCase):
    """関数名の導出規則を確認する。"""

    def test_lowercases_whole_id(self):
        self.assertEqual(
            gen.wrapper_name("SAMPLE_MESSAGES_ID_FILE_OPEN_FAILED"),
            "sample_messages_id_file_open_failed",
        )

    def test_has_no_double_underscore(self):
        self.assertNotIn("__", gen.wrapper_name("SAMPLE_MESSAGES_ID_A"))

    def test_fits_into_the_caller_namespace(self):
        # ライブラリ側の接頭辞は前置しない。文字列 ID に含まれるモジュール接頭辞だけで
        # 利用側の名前空間に収まる。
        name = gen.wrapper_name("SAMPLE_MESSAGES_ID_A")
        self.assertFalse(name.startswith(f"{gen.LIBRARY_PREFIX}_"))
        self.assertTrue(name.startswith("sample_messages_"))

    def test_follows_the_module_prefix_embedded_in_the_id(self):
        # 名前空間は、文字列 ID に含まれるモジュール接頭辞がそのまま担う
        self.assertEqual(gen.wrapper_name("APP_ID_A"), "app_id_a")


class DerivedNameTest(unittest.TestCase):
    """定義に書かない名前の導出を確認する。"""

    def test_module_prefix_comes_from_the_file_name(self):
        self.assertEqual(gen.derive_module_prefix(Path("/tmp/app/sample_messages.jsonc")), "sample_messages")

    def test_module_prefix_rejects_a_name_that_is_not_an_identifier(self):
        for name in ("Sample_Messages.jsonc", "sample-messages.jsonc", "1st.jsonc", "サンプル.jsonc"):
            with self.subTest(name=name):
                with self.assertRaises(gen.DefinitionError):
                    gen.derive_module_prefix(Path(f"/tmp/app/{name}"))

    def test_id_enum_follows_the_module_prefix(self):
        self.assertEqual(gen.id_enum_name(minimal_document()), "sample_messages_id")
        self.assertEqual(gen.id_enum_name({"module_prefix": "app_messages"}), "app_messages_id")

    def test_module_dir_starts_at_prod(self):
        path = Path("/tmp/workspace/app/example/prod/src/cmd/example/messages.jsonc")
        self.assertEqual(gen.derive_module_dir(path), "prod/src/cmd/example")

    def test_module_dir_starts_at_test(self):
        path = Path("/tmp/workspace/app/example/test/src/exampleTest/messages.jsonc")
        self.assertEqual(gen.derive_module_dir(path), "test/src/exampleTest")

    def test_module_dir_falls_back_to_current_directory(self):
        path = Path("/tmp/workspace/messages.jsonc")
        self.assertEqual(gen.derive_module_dir(path), ".")


class LibrarySpecTest(unittest.TestCase):
    """ライブラリ側の仕様が生成器の定数であることを確認する。"""

    def test_prefix_is_cplat_string_catalog(self):
        self.assertEqual(gen.LIBRARY_PREFIX, "cplat_string_catalog")

    def test_languages_match_the_library_enumeration(self):
        # cplat_string_catalog_language の並びと揃える
        self.assertEqual(gen.LANGUAGES, ("neutral", "japanese", "english"))

    def test_is_used_for_the_library_constants(self):
        document = minimal_document()
        self.assertEqual(gen.language_constant(document, "neutral"), "CPLAT_STRING_CATALOG_LANGUAGE_NEUTRAL")
        self.assertEqual(gen.kind_constant(document, "STRING"), "CPLAT_STRING_CATALOG_ARGUMENT_KIND_STRING")


class ArgumentTypeTest(unittest.TestCase):
    """引数種別と C の型の対応を確認する。"""

    def test_covers_every_kind_used_by_the_library(self):
        expected = {
            "STRING", "CHAR", "INT8", "UINT8", "INT16", "UINT16", "INT32", "UINT32",
            "INT64", "UINT64", "HEX8", "HEX16", "HEX32", "HEX64", "SIZE", "SSIZE",
            "POINTER", "DOUBLE", "ERROR_CODE",
        }
        self.assertEqual(set(gen.ARGUMENT_TYPES), expected)


class FormatSourceTest(unittest.TestCase):
    """clang-format 呼び出しの文字コード指定を確認する。"""

    @mock.patch("string_catalog_gen.shutil.which", return_value="clang-format")
    @mock.patch("string_catalog_gen.subprocess.run")
    def test_uses_utf8_for_clang_format_stdio(self, run_mock, _which_mock):
        run_mock.return_value = mock.Mock(stdout="整形後\n")

        formatted = gen.format_source("日本語\n", "sample.c", Path("/tmp/.clang-format"))

        self.assertEqual(formatted, "整形後\n")
        self.assertEqual(run_mock.call_args.kwargs["encoding"], "utf-8")


class WriteTextLfTest(unittest.TestCase):
    """生成物の文字コードと改行コード指定を確認する。"""

    @mock.patch("pathlib.Path.open", new_callable=mock.mock_open)
    def test_writes_utf8_with_lf(self, open_mock):
        gen.write_text_lf(Path("sample.c"), "1 行目\n2 行目\n")

        open_mock.assert_called_once_with("w", encoding="utf-8", newline="\n")
        open_mock().write.assert_called_once_with("1 行目\n2 行目\n")


if __name__ == "__main__":
    unittest.main()


class GeneratedOutputTest(unittest.TestCase):
    """生成物が特定の app へ依存しないことを確認する。"""

    def setUp(self):
        self.document = minimal_document(module_dir="prod/src/cmd/example")
        self.strings = gen.validate(self.document)

    def test_header_includes_only_library_and_standard_headers(self):
        header = gen.emit_header(self.document, self.strings, "example.jsonc")
        includes = [line for line in header.splitlines() if line.startswith("#include")]
        self.assertEqual(
            includes,
            [
                "#include <cplat/string_catalog/string_catalog.h>",
                "#include <stdarg.h>",
                "#include <stddef.h>",
                "#include <stdint.h>",
            ],
        )

    def test_source_emits_raw_category(self):
        source = gen.emit_source(self.document, self.strings, "example.jsonc")
        self.assertIn("    {SAMPLE_MESSAGES_ID_A,\n     1,\n", source)
        self.assertNotIn("TRACE_LEVEL", source)

    def test_module_dir_appears_in_documentation(self):
        header = gen.emit_header(self.document, self.strings, "example.jsonc")
        self.assertIn("`prod/src/cmd/example/` のモジュール私有ヘッダー", header)
        source = gen.emit_source(self.document, self.strings, "example.jsonc")
        self.assertIn("@file           src/cmd/example/sample_messages.c", source)


class DoxygenGroupTest(unittest.TestCase):
    """生成物ヘッダーの Doxygen グループ記述を確認する。"""

    def setUp(self):
        self.document = minimal_document(module_dir="prod/src/cmd/example")
        self.strings = gen.validate(self.document)

    def test_header_contains_defgroup(self):
        header = gen.emit_header(self.document, self.strings, "example.jsonc")
        self.assertIn("@defgroup       SAMPLE_MESSAGES 文字列カタログ (sample_messages)", header)

    def test_header_contains_group_open_and_close(self):
        header = gen.emit_header(self.document, self.strings, "example.jsonc")
        self.assertIn("@{", header)
        self.assertIn("/** @} */", header)

    def test_header_group_brief_references_definition(self):
        header = gen.emit_header(self.document, self.strings, "example.jsonc")
        self.assertIn("カタログ定義 `example.jsonc` から自動生成された文字列カタログです。", header)

    def test_source_never_contains_defgroup(self):
        source = gen.emit_source(self.document, self.strings, "example.jsonc")
        self.assertNotIn("@defgroup", source)
        self.assertNotIn("/** @} */", source)

