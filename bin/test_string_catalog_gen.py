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
                "key": "SAMPLE_MESSAGES_KEY_A",
                "id": "ID_0001",
                "category": 1,
                "brief": "あ。",
                "details": "あを組み立てます。",
                "arguments": [{"kind": "STRING", "name": "path", "description": "パス。"}],
                "texts": {"neutral": "{0}"},
                "notes": {"neutral": ""},
            }
        ],
    }
    document.update(overrides)
    return document


def trace_document(**overrides):
    """トレース種別の最小の定義を組み立てる。"""
    document = {
        "kind": "trace",
        "module_prefix": "sample_trace",
        "strings": [
            {
                "key": "SAMPLE_TRACE_KEY_A",
                "id": "ID_0001",
                "level": "ERROR",
                "brief": "あ。",
                "arguments": [{"kind": "STRING", "name": "path", "description": "パス。"}],
                "texts": {"neutral": "{0}"},
                "notes": {"neutral": ""},
            }
        ],
    }
    document.update(overrides)
    return document


class CatalogKindTest(unittest.TestCase):
    """カタログ種別の既定値と検査を確認する。"""

    def test_defaults_to_message(self):
        self.assertEqual(gen.catalog_kind(minimal_document()), "message")
        self.assertFalse(gen.is_trace(minimal_document()))

    def test_accepts_trace(self):
        self.assertTrue(gen.is_trace(trace_document()))
        self.assertEqual(len(gen.validate(trace_document())), 1)

    def test_rejects_unknown_kind(self):
        with self.assertRaises(gen.DefinitionError):
            gen.validate(minimal_document(kind="warning"))

    def test_trace_level_maps_to_category(self):
        self.assertEqual(gen.trace_level_value("CRITICAL"), 0)
        self.assertEqual(gen.trace_level_value("ERROR"), 1)
        self.assertEqual(gen.trace_level_value("NONE"), 6)


class TraceValidateTest(unittest.TestCase):
    """トレース種別に固有の検査を確認する。"""

    def test_requires_id(self):
        document = trace_document()
        del document["strings"][0]["id"]
        with self.assertRaises(gen.DefinitionError):
            gen.validate(document)

    def test_requires_level(self):
        document = trace_document()
        del document["strings"][0]["level"]
        with self.assertRaises(gen.DefinitionError):
            gen.validate(document)

    def test_rejects_unknown_level(self):
        document = trace_document()
        document["strings"][0]["level"] = "FATAL"
        with self.assertRaises(gen.DefinitionError):
            gen.validate(document)

    def test_rejects_category(self):
        document = trace_document()
        document["strings"][0]["category"] = 1
        with self.assertRaises(gen.DefinitionError):
            gen.validate(document)

    def test_rejects_level_in_message_kind(self):
        document = minimal_document()
        document["strings"][0]["level"] = "ERROR"
        with self.assertRaises(gen.DefinitionError):
            gen.validate(document)

    def test_rejects_argument_name_colliding_with_context(self):
        document = trace_document()
        document["strings"][0]["arguments"][0]["name"] = "source_line"
        with self.assertRaises(gen.DefinitionError):
            gen.validate(document)

    def test_limits_user_arguments_to_context_base(self):
        document = trace_document()
        argument = document["strings"][0]["arguments"][0]
        document["strings"][0]["arguments"] = [dict(argument) for _ in range(gen.CONTEXT_ARGUMENT_BASE)]
        self.assertEqual(len(gen.validate(document)), 1)

        document["strings"][0]["arguments"].append(dict(argument))
        with self.assertRaises(gen.DefinitionError):
            gen.validate(document)

    def test_allows_placeholder_for_context_argument(self):
        document = trace_document()
        document["strings"][0]["texts"]["neutral"] = "{0} {45}"
        self.assertEqual(len(gen.validate(document)), 1)

    def test_rejects_placeholder_for_unused_index(self):
        document = trace_document()
        document["strings"][0]["texts"]["neutral"] = "{1}"
        with self.assertRaises(gen.DefinitionError):
            gen.validate(document)

    def test_rejects_placeholder_beyond_context_arguments(self):
        document = trace_document()
        document["strings"][0]["texts"]["neutral"] = "{46}"
        with self.assertRaises(gen.DefinitionError):
            gen.validate(document)


class TraceOutputTest(unittest.TestCase):
    """トレース種別の生成物を確認する。"""

    def setUp(self):
        self.document = trace_document(module_dir="prod/src/cmd/example")
        self.strings = gen.validate(self.document)

    def test_header_includes_trace_and_runtime_headers(self):
        header = gen.emit_header(self.document, self.strings, "example.jsonc")
        includes = [line for line in header.splitlines() if line.startswith("#include")]
        self.assertEqual(
            includes,
            [
                "#include <cplat/string_catalog/string_catalog.h>",
                "#include <cplat/trace/tracer.h>",
                "#include <cplat/crt/path.h>",
                "#include <cplat/runtime/process.h>",
                "#include <stdarg.h>",
                "#include <stddef.h>",
                "#include <stdint.h>",
            ],
        )

    def test_header_declares_write(self):
        header = gen.emit_header(self.document, self.strings, "example.jsonc")
        self.assertIn("int sample_trace_write(int string_key, ...);", header)
        self.assertIn("void sample_trace_set_tracer(cplat_tracer *tracer);", header)
        self.assertIn("cplat_tracer *sample_trace_get_tracer(void);", header)

    def test_wrapper_takes_tracer_and_source_location(self):
        wrapper = gen.emit_wrapper(self.document, self.strings[0])
        self.assertIn(
            "static inline int sample_trace_key_a_with_source(const char *path, "
            "const char *source_file_path, const char *source_file_name, const int32_t source_line, "
            "const char *function_name)",
            wrapper,
        )

    def test_wrapper_obtains_runtime_identifiers_inside(self):
        wrapper = gen.emit_wrapper(self.document, self.strings[0])
        self.assertIn("cplat_process_get_pid(), cplat_process_get_tid()", wrapper)

    def test_macro_passes_call_site(self):
        wrapper = gen.emit_wrapper(self.document, self.strings[0])
        self.assertIn(
            "#define sample_trace_key_a(path) "
            "sample_trace_key_a_with_source((path), __FILE__, "
            "cplat_path_basename(__FILE__), __LINE__, __func__)",
            wrapper,
        )

    def test_source_emits_context_arguments_at_base(self):
        source = gen.emit_source(self.document, self.strings, "example.jsonc")
        self.assertIn(
            '    [40] = {CPLAT_STRING_CATALOG_ARGUMENT_KIND_STRING, 0, "source_file_path", ',
            source,
        )
        self.assertIn(
            '    [45] = {CPLAT_STRING_CATALOG_ARGUMENT_KIND_UINT32, 0, "thread_id", ',
            source,
        )

    def test_source_emits_level_as_category_and_full_argument_count(self):
        source = gen.emit_source(self.document, self.strings, "example.jsonc")
        self.assertIn("    {SAMPLE_TRACE_KEY_A,\n     1,\n     46,\n", source)

    def test_source_emits_write_function(self):
        source = gen.emit_source(self.document, self.strings, "example.jsonc")
        self.assertIn("int sample_trace_write(const int string_key, ...)", source)
        self.assertIn("cplat_tracer_write_at(s_tracer, (cplat_trace_level)sample_trace_category(string_key)", source)

    def test_source_holds_tracer_and_rejects_unset(self):
        source = gen.emit_source(self.document, self.strings, "example.jsonc")
        self.assertIn("static cplat_tracer *s_tracer = NULL;", source)
        self.assertIn("void sample_trace_set_tracer(cplat_tracer *tracer)", source)
        # 出力先が未設定なら、組み立てを行わずに失敗を返す
        self.assertIn("    if (s_tracer == NULL)\n    {\n        return CPLAT_ERR_INVALID_ARGUMENT;", source)

    def test_wrapper_for_entry_without_arguments_takes_no_parameter(self):
        document = trace_document()
        document["strings"][0]["arguments"] = []
        document["strings"][0]["texts"]["neutral"] = "開始しました。"
        strings = gen.validate(document)
        wrapper = gen.emit_wrapper(document, strings[0])
        self.assertIn("#define sample_trace_key_a() sample_trace_key_a_with_source(__FILE__, ", wrapper)

    def test_message_kind_does_not_emit_write(self):
        document = minimal_document()
        strings = gen.validate(document)
        self.assertNotIn("sample_messages_write", gen.emit_source(document, strings, "example.jsonc"))
        self.assertNotIn("sample_messages_write", gen.emit_header(document, strings, "example.jsonc"))


class ValidateTest(unittest.TestCase):
    """定義の検査を確認する。"""

    def test_accepts_minimal(self):
        self.assertEqual(len(gen.validate(minimal_document())), 1)

    def test_allows_missing_details(self):
        document = minimal_document()
        del document["strings"][0]["details"]
        self.assertEqual(len(gen.validate(document)), 1)

    def test_does_not_require_derived_keys(self):
        # 生成器が持つ名前と仕様は、カタログ定義へ書かない
        document = minimal_document()
        for key in ("library_prefix", "key_enum", "module_dir", "languages"):
            self.assertNotIn(key, document)
        self.assertEqual(len(gen.validate(document)), 1)

    def test_accepts_details_as_list(self):
        document = minimal_document()
        document["strings"][0]["details"] = ["あを", "組み立てます。"]
        strings = gen.validate(document)
        source = gen.emit_source(document, strings, "example.jsonc")
        self.assertIn('"あを 組み立てます。"', source)

    def test_rejects_details_with_non_string_element(self):
        document = minimal_document()
        document["strings"][0]["details"] = ["あ", 1]
        with self.assertRaises(gen.DefinitionError):
            gen.validate(document)

    def test_rejects_duplicate_key(self):
        document = minimal_document()
        document["strings"].append(dict(document["strings"][0]))
        document["strings"][1]["id"] = "ID_0002"
        with self.assertRaises(gen.DefinitionError):
            gen.validate(document)

    def test_allows_duplicate_id(self):
        # id は処理で項目を識別しないため、重複を検査しない
        document = minimal_document()
        duplicated = dict(document["strings"][0])
        duplicated["key"] = "SAMPLE_MESSAGES_KEY_B"
        document["strings"].append(duplicated)
        self.assertEqual(len(gen.validate(document)), 2)

    def test_allows_missing_id(self):
        document = minimal_document()
        del document["strings"][0]["id"]
        self.assertEqual(len(gen.validate(document)), 1)

    def test_rejects_missing_key(self):
        document = minimal_document()
        del document["strings"][0]["key"]
        with self.assertRaises(gen.DefinitionError):
            gen.validate(document)

    def test_rejects_non_string_id(self):
        document = minimal_document()
        document["strings"][0]["id"] = 1
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

    def test_rejects_non_string_metadata(self):
        document = minimal_document()
        document["strings"][0]["details"] = 1
        with self.assertRaises(gen.DefinitionError):
            gen.validate(document)

    def test_rejects_non_string_argument_metadata(self):
        document = minimal_document()
        document["strings"][0]["arguments"][0]["description"] = 1
        with self.assertRaises(gen.DefinitionError):
            gen.validate(document)

    def test_rejects_non_string_remarks_item(self):
        document = minimal_document()
        document["strings"][0]["remarks"] = ["補足", 1]
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
        # 文字列キーの小文字形が同じ生成物の簡易関数名と衝突しうる。
        # 衝突対象は ACCESSOR_DECLARATIONS (および SOURCE_TAIL) が実際に @MODULE@_ に続けて
        # 出力する名前一覧 (gen.MODULE_FUNCTION_SUFFIXES) のすべてで確認する。
        for suffix in gen.MODULE_FUNCTION_SUFFIXES:
            with self.subTest(suffix=suffix):
                document = minimal_document()
                document["strings"][0]["key"] = f"SAMPLE_MESSAGES_{suffix.upper()}"
                with self.assertRaises(gen.DefinitionError):
                    gen.validate(document)

    def test_accepts_key_that_does_not_collide(self):
        # 通常の文字列キーでは衝突検査に引っかからないことを確認する (回帰防止)。
        document = minimal_document()
        self.assertEqual(len(gen.validate(document)), 1)


class WrapperNameTest(unittest.TestCase):
    """関数名の導出規則を確認する。"""

    def test_lowercases_whole_key(self):
        self.assertEqual(
            gen.wrapper_name("SAMPLE_MESSAGES_KEY_FILE_OPEN_FAILED"),
            "sample_messages_key_file_open_failed",
        )

    def test_has_no_double_underscore(self):
        self.assertNotIn("__", gen.wrapper_name("SAMPLE_MESSAGES_KEY_A"))

    def test_fits_into_the_caller_namespace(self):
        # ライブラリ側の接頭辞は前置しない。文字列キーに含まれるモジュール接頭辞だけで
        # 利用側の名前空間に収まる。
        name = gen.wrapper_name("SAMPLE_MESSAGES_KEY_A")
        self.assertFalse(name.startswith(f"{gen.LIBRARY_PREFIX}_"))
        self.assertTrue(name.startswith("sample_messages_"))

    def test_follows_the_module_prefix_embedded_in_the_key(self):
        # 名前空間は、文字列キーに含まれるモジュール接頭辞がそのまま担う
        self.assertEqual(gen.wrapper_name("APP_KEY_A"), "app_key_a")


class DerivedNameTest(unittest.TestCase):
    """定義に書かない名前の導出を確認する。"""

    def test_module_prefix_comes_from_the_file_name(self):
        self.assertEqual(gen.derive_module_prefix(Path("/tmp/app/sample_messages.jsonc")), "sample_messages")

    def test_module_prefix_rejects_a_name_that_is_not_an_identifier(self):
        for name in ("Sample_Messages.jsonc", "sample-messages.jsonc", "1st.jsonc", "サンプル.jsonc"):
            with self.subTest(name=name):
                with self.assertRaises(gen.DefinitionError):
                    gen.derive_module_prefix(Path(f"/tmp/app/{name}"))

    def test_key_enum_follows_the_module_prefix(self):
        self.assertEqual(gen.key_enum_name(minimal_document()), "sample_messages_key")
        self.assertEqual(gen.key_enum_name({"module_prefix": "app_messages"}), "app_messages_key")

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

    def test_typed_wrapper_calls_module_format(self):
        header = gen.emit_header(self.document, self.strings, "example.jsonc")
        self.assertIn(
            "        return sample_messages_format(dest, dest_size, SAMPLE_MESSAGES_KEY_A, path);",
            header,
        )

    def test_typed_wrapper_does_not_depend_on_catalog_object(self):
        # ラッパーは呼び出し側のコンパイル単位で展開されるため、cplat の関数と
        # cplat_string_catalog のレイアウトへ依存させない。
        header = gen.emit_header(self.document, self.strings, "example.jsonc")
        wrappers = header[header.index("static inline int sample_messages_key_a") :]
        self.assertNotIn("cplat_string_catalog_format", wrappers)
        self.assertNotIn("sample_messages_catalog()", wrappers)

    def test_source_emits_raw_category(self):
        source = gen.emit_source(self.document, self.strings, "example.jsonc")
        self.assertIn("    {SAMPLE_MESSAGES_KEY_A,\n     1,\n", source)
        self.assertNotIn("TRACE_LEVEL", source)

    def test_source_emits_entry_metadata(self):
        source = gen.emit_source(self.document, self.strings, "example.jsonc")
        self.assertIn('"あ。"', source)
        self.assertIn('"あを組み立てます。"', source)
        self.assertIn('{CPLAT_STRING_CATALOG_ARGUMENT_KIND_STRING, 0, "path", "パス。"}', source)

    def test_source_emits_null_for_missing_details(self):
        document = minimal_document()
        del document["strings"][0]["details"]
        strings = gen.validate(document)
        source = gen.emit_source(document, strings, "example.jsonc")
        self.assertIn('     "ID_0001",\n     "あ。",\n     NULL,', source)

    def test_source_emits_null_for_missing_id(self):
        document = minimal_document()
        del document["strings"][0]["id"]
        strings = gen.validate(document)
        source = gen.emit_source(document, strings, "example.jsonc")
        self.assertIn('     s_arguments_0,\n     NULL,\n     "あ。",', source)

    def test_typed_formatter_emits_brief_and_details_separately(self):
        header = gen.emit_header(self.document, self.strings, "example.jsonc")
        self.assertIn("     *  @brief          あ。", header)
        self.assertIn("     *  あを組み立てます。", header)

    def test_typed_formatter_separates_details_from_parameters(self):
        wrapper = gen.emit_wrapper(self.document, self.strings[0])
        lines = wrapper.splitlines()
        details_index = lines.index("     *  あを組み立てます。")
        param_index = next(index for index, line in enumerate(lines) if "@param[out]" in line)

        self.assertEqual(details_index + 1, param_index - 1)
        self.assertEqual("     *", lines[details_index + 1])

    def test_typed_formatter_puts_no_argument_note_in_details(self):
        document = minimal_document()
        document["strings"][0]["arguments"] = []
        del document["strings"][0]["details"]
        document["strings"][0]["texts"] = {"neutral": "started"}
        strings = gen.validate(document)
        wrapper = gen.emit_wrapper(document, strings[0])
        lines = wrapper.splitlines()
        no_argument_index = lines.index("     *  この文字列は引数を必要としません。")
        param_index = next(index for index, line in enumerate(lines) if "@param[out]" in line)

        self.assertLess(no_argument_index, param_index)
        self.assertEqual("     *", lines[no_argument_index + 1])

    def test_typed_formatter_does_not_add_no_argument_note_for_arguments(self):
        wrapper = gen.emit_wrapper(self.document, self.strings[0])
        self.assertNotIn("この文字列は引数を必要としません。", wrapper)

    def test_typed_formatter_emits_remarks_as_remark(self):
        document = minimal_document()
        document["strings"][0]["remarks"] = "あを組み立てます。"
        strings = gen.validate(document)
        wrapper = gen.emit_wrapper(document, strings[0])
        lines = wrapper.splitlines()
        details_index = lines.index("     *  あを組み立てます。")
        return_index = next(index for index, line in enumerate(lines) if "@return" in line)
        remark_index = lines.index("     *  @remark         あを組み立てます。")
        format_index = next(index for index, line in enumerate(lines) if "@par            書式" in line)

        self.assertLess(details_index, return_index)
        self.assertLess(return_index, remark_index)
        self.assertLess(remark_index, format_index)
        self.assertEqual(return_index + 1, remark_index)
        self.assertEqual(remark_index + 1, format_index)

    def test_source_emits_remarks(self):
        self.document["strings"][0]["remarks"] = ["補足", "説明"]
        strings = gen.validate(self.document)
        source = gen.emit_source(self.document, strings, "example.jsonc")
        self.assertIn('"補足 説明"', source)

    def test_header_and_source_emit_entry_accessor(self):
        header = gen.emit_header(self.document, self.strings, "example.jsonc")
        source = gen.emit_source(self.document, self.strings, "example.jsonc")
        self.assertIn("const cplat_string_catalog_entry *sample_messages_entry(int string_key);", header)
        self.assertIn("const cplat_string_catalog_entry *sample_messages_entry(const int string_key)", source)

    def test_header_and_source_emit_id_accessor(self):
        header = gen.emit_header(self.document, self.strings, "example.jsonc")
        source = gen.emit_source(self.document, self.strings, "example.jsonc")
        self.assertIn("const char *sample_messages_id(int string_key);", header)
        self.assertIn("const char *sample_messages_id(const int string_key)", source)

    def test_source_macros_use_the_module_prefix(self):
        source = gen.emit_source(self.document, self.strings, "example.jsonc")

        self.assertIn("#define SAMPLE_MESSAGES_ENTRY_COUNT", source)
        self.assertIn("#define SAMPLE_MESSAGES_KEY_INDEX_ABSENT", source)
        self.assertIn("#define SAMPLE_MESSAGES_KEY_INDEX_COUNT", source)
        self.assertNotIn("#define ENTRY_COUNT", source)
        self.assertNotIn("#define KEY_INDEX_ABSENT", source)
        self.assertNotIn("#define KEY_INDEX_COUNT", source)

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

    def test_typed_formatters_are_in_a_nested_group(self):
        header = gen.emit_header(self.document, self.strings, "example.jsonc")

        child_group = "@defgroup       SAMPLE_MESSAGES_TYPED_FORMATTERS 文字列キーごとの型付き組み立て関数"
        child_group_start = header.index(child_group)
        child_group_close = header.index("/** @} */", child_group_start)
        parent_group_close = header.rindex("/** @} */")

        self.assertIn("@ingroup        SAMPLE_MESSAGES", header[child_group_start:child_group_close])
        self.assertLess(header.index("int sample_messages_format"), child_group_start)
        self.assertLess(child_group_start, header.index("static inline int sample_messages_key_a"))
        self.assertLess(child_group_close, parent_group_close)

    def test_typed_formatter_group_has_consistent_indentation(self):
        header = gen.emit_header(self.document, self.strings, "example.jsonc")

        child_group = "@defgroup       SAMPLE_MESSAGES_TYPED_FORMATTERS 文字列キーごとの型付き組み立て関数"
        lines = header.splitlines()
        group_index = next(index for index, line in enumerate(lines) if child_group in line)

        self.assertEqual("/**", lines[group_index - 1])
        self.assertEqual(
            " *  @defgroup       SAMPLE_MESSAGES_TYPED_FORMATTERS 文字列キーごとの型付き組み立て関数",
            lines[group_index],
        )
        self.assertEqual(" */", lines[group_index + 4])

    def test_source_never_contains_defgroup(self):
        source = gen.emit_source(self.document, self.strings, "example.jsonc")
        self.assertNotIn("@defgroup", source)
        self.assertNotIn("/** @} */", source)


class KeyValueTest(unittest.TestCase):
    """文字列キーの値を定義で固定する扱いを確認する。"""

    @staticmethod
    def document_with(*values):
        """指定した値を持つ 2 件以上の定義を組み立てる。値が None の項目は value を書かない。"""
        document = minimal_document()
        template = document["strings"][0]
        document["strings"] = []
        for position, value in enumerate(values):
            entry = dict(template)
            entry["key"] = f"SAMPLE_MESSAGES_KEY_{chr(ord('A') + position)}"
            if value is not None:
                entry["value"] = value
            document["strings"].append(entry)
        return document

    def test_defaults_to_definition_order(self):
        document = self.document_with(None, None)
        self.assertEqual(gen.key_values(gen.validate(document)), [1, 2])

    def test_uses_declared_value(self):
        document = self.document_with(10, 3)
        self.assertEqual(gen.key_values(gen.validate(document)), [10, 3])

    def test_header_emits_declared_value(self):
        document = self.document_with(10, 3)
        header = gen.emit_header(document, gen.validate(document), "example.jsonc")
        self.assertIn("SAMPLE_MESSAGES_KEY_A = 10,", header)
        self.assertIn("SAMPLE_MESSAGES_KEY_B = 3 ", header)

    def test_key_index_marks_absent_values(self):
        document = self.document_with(1, 3)
        source = gen.emit_source(document, gen.validate(document), "example.jsonc")
        self.assertIn("SAMPLE_MESSAGES_KEY_INDEX_ABSENT, /* 2: 欠番 */", source)

    def test_static_assert_uses_largest_value(self):
        document = self.document_with(10, 3)
        source = gen.emit_source(document, gen.validate(document), "example.jsonc")
        self.assertIn("KEY_INDEX_COUNT > SAMPLE_MESSAGES_KEY_A,", source)

    def test_rejects_duplicate_value(self):
        with self.assertRaises(gen.DefinitionError):
            gen.validate(self.document_with(2, 2))

    def test_rejects_value_below_one(self):
        with self.assertRaises(gen.DefinitionError):
            gen.validate(self.document_with(0, 1))

    def test_rejects_value_above_maximum(self):
        with self.assertRaises(gen.DefinitionError):
            gen.validate(self.document_with(gen.KEY_VALUE_MAX + 1, 1))

    def test_rejects_boolean_value(self):
        with self.assertRaises(gen.DefinitionError):
            gen.validate(self.document_with(True, 2))

    def test_rejects_partially_declared_values(self):
        with self.assertRaises(gen.DefinitionError):
            gen.validate(self.document_with(1, None))

def export_document(scope="api", **overrides):
    """公開する設定を持つ定義を組み立てる。"""
    document = minimal_document(module_dir="prod/libsrc/example")
    document["settings"] = "catalog_settings.jsonc"
    document["export"] = scope
    document[gen.SETTINGS_KEY] = {
        "export": {"prefix": "SAMPLECATALOG", "header": "samplecatalog/samplecatalog_export.h"}
    }
    document.update(overrides)
    return document


class ExportValidateTest(unittest.TestCase):
    """カタログを外部へ公開する設定の検査を確認する。"""

    def test_accepts_api_and_full(self):
        for scope in gen.EXPORT_SCOPES:
            self.assertEqual(len(gen.validate(export_document(scope))), 1)

    def test_rejects_unknown_scope(self):
        with self.assertRaises(gen.DefinitionError):
            gen.validate(export_document("public"))

    def test_requires_settings_export_section(self):
        document = export_document()
        del document[gen.SETTINGS_KEY]
        with self.assertRaises(gen.DefinitionError):
            gen.validate(document)

    def test_requires_prefix_and_header(self):
        for key in ("prefix", "header"):
            document = export_document()
            del document[gen.SETTINGS_KEY]["export"][key]
            with self.assertRaises(gen.DefinitionError):
                gen.validate(document)

    def test_rejects_lowercase_prefix(self):
        document = export_document()
        document[gen.SETTINGS_KEY]["export"]["prefix"] = "samplecatalog"
        with self.assertRaises(gen.DefinitionError):
            gen.validate(document)

    def test_allows_missing_export(self):
        document = minimal_document()
        self.assertEqual(len(gen.validate(document)), 1)


class ExportOutputTest(unittest.TestCase):
    """公開するカタログの生成物を確認する。"""

    @staticmethod
    def header(scope):
        document = export_document(scope) if scope else minimal_document(module_dir="prod/libsrc/example")
        return gen.emit_header(document, gen.validate(document), "example.jsonc")

    def test_includes_export_header(self):
        self.assertIn("#include <samplecatalog/samplecatalog_export.h>", self.header("api"))

    def test_does_not_include_export_header_when_not_exported(self):
        self.assertNotIn("samplecatalog_export.h", self.header(None))

    def test_api_scope_decorates_behavior_functions(self):
        header = self.header("api")
        self.assertIn(
            "SAMPLECATALOG_EXPORT int SAMPLECATALOG_API sample_messages_format(", header
        )
        self.assertIn(
            "SAMPLECATALOG_EXPORT const char *SAMPLECATALOG_API sample_messages_id(", header
        )

    def test_api_scope_leaves_structure_returning_functions_undecorated(self):
        # 構造体を返す関数を公開すると、利用側が cplat のレイアウトへ依存する。
        header = self.header("api")
        self.assertIn("    const cplat_string_catalog *sample_messages_catalog(void);", header)
        self.assertIn("    const cplat_string_catalog_entry *sample_messages_entries(void);", header)
        self.assertIn("    int sample_messages_verify(", header)

    def test_full_scope_decorates_every_function(self):
        header = self.header("full")
        self.assertIn(
            "SAMPLECATALOG_EXPORT const cplat_string_catalog *SAMPLECATALOG_API sample_messages_catalog(",
            header,
        )
        self.assertIn("SAMPLECATALOG_EXPORT int SAMPLECATALOG_API sample_messages_verify(", header)

    def test_no_marker_remains_when_not_exported(self):
        header = self.header(None)
        self.assertNotIn("@EXPORT", header)
        self.assertNotIn("@API", header)
        self.assertIn("    int sample_messages_format(", header)

    def test_trace_write_functions_are_decorated(self):
        document = export_document("api", **trace_document(module_dir="prod/libsrc/example"))
        header = gen.emit_header(document, gen.validate(document), "example.jsonc")
        self.assertIn(
            "SAMPLECATALOG_EXPORT void SAMPLECATALOG_API sample_trace_set_tracer(", header
        )
        self.assertIn("SAMPLECATALOG_EXPORT int SAMPLECATALOG_API sample_trace_write(", header)

if __name__ == "__main__":
    unittest.main()
