#!/usr/bin/env python3
"""check_functional_spec.py の単体テスト。"""

from __future__ import annotations

import importlib.util
from pathlib import Path
import sys
import tempfile
import unittest


SCRIPT_PATH = Path(__file__).resolve().parent / "check_functional_spec.py"
SPEC = importlib.util.spec_from_file_location("check_functional_spec", SCRIPT_PATH)
assert SPEC is not None
assert SPEC.loader is not None
CHECKER = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = CHECKER
SPEC.loader.exec_module(CHECKER)

UUID_1 = "123e4567-e89b-42d3-a456-426614174000"
UUID_2 = "223e4567-e89b-42d3-b456-426614174001"
UUID_3 = "323e4567-e89b-42d3-8456-426614174002"


class CheckFunctionalSpecTest(unittest.TestCase):
    def _write(self, root: Path, relative_path: str, content: str) -> None:
        path = root / relative_path
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")

    def _write_spec(self, root: Path, rows: list[str]) -> None:
        body = "\n".join(rows)
        self._write(
            root,
            "docs/functional-spec/clock.md",
            "# clock 機能仕様\n\n"
            "## 機能要件\n\n"
            "| 要件 ID | cplat の要件 |\n"
            "|---|---|\n"
            f"{body}\n",
        )

    @staticmethod
    def _row(requirement_id: str, requirement_uuid: str) -> str:
        return CheckFunctionalSpecTest._row_with_body(
            requirement_id,
            requirement_uuid,
            "cplat の時計・時刻機能は、時計を提供します。",
        )

    @staticmethod
    def _row_with_body(
        requirement_id: str, requirement_uuid: str, body: str
    ) -> str:
        return (
            f"| `{requirement_id}` "
            f"<!-- cplat-req: uuid={requirement_uuid} --> "
            f"| {body} |"
        )

    def test_accepts_all_reference_formats(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            requirement_id = "CPLAT-CLOCK-FUNC-001"
            self._write_spec(root, [self._row(requirement_id, UUID_1)])
            self._write(
                root,
                "docs/design.md",
                f"要件: `{requirement_id}` "
                f"<!-- cplat-req: uuid={UUID_1} -->\n",
            )
            self._write(
                root,
                "prod/sample.c",
                f"/* cplat-req: id={requirement_id}; uuid={UUID_1} */\n"
                "/** 時計を取得する。 */\n"
                "void get_clock(void);\n",
            )
            test_blocks = ""
            for macro in ("TEST", "TEST_F", "TEST_P", "TYPED_TEST"):
                test_blocks += (
                    "// 単調増加クロックを確認する。\n"
                    f"// cplat-req: id={requirement_id}; uuid={UUID_1}\n"
                    f"{macro}(clockTest, monotonic_clock)\n"
                    "{\n"
                    "    // Arrange\n"
                    "}\n"
                )
            self._write(
                root,
                "test/sample.cpp",
                test_blocks,
            )

            result = CHECKER.check_repository(root)

            self.assertEqual([], result.errors)
            self.assertEqual(1, result.requirement_count)
            self.assertEqual(6, result.reference_count)

    def test_rejects_duplicate_id_and_uuid(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            row = self._row("CPLAT-CLOCK-FUNC-001", UUID_1)
            self._write_spec(root, [row, row])

            result = CHECKER.check_repository(root)

            self.assertTrue(any("要件 ID が重複" in error for error in result.errors))
            self.assertTrue(any("UUID が重複" in error for error in result.errors))

    def test_rejects_malformed_uuid(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self._write_spec(
                root,
                [
                    "| `CPLAT-CLOCK-FUNC-001` "
                    "<!-- cplat-req: uuid=not-a-uuid --> "
                    "| cplat の時計・時刻機能は、時計を提供します。 |"
                ],
            )

            result = CHECKER.check_repository(root)

            self.assertTrue(any("機能要件行の書式が不正" in error for error in result.errors))

    def test_rejects_unknown_uuid_and_stale_id(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            current_id = "CPLAT-CLOCK-FUNC-001"
            self._write_spec(root, [self._row(current_id, UUID_1)])
            self._write(
                root,
                "docs/design.md",
                f"`{current_id}` <!-- cplat-req: uuid={UUID_2} -->\n"
                f"`CPLAT-CLOCK-FUNC-002` "
                f"<!-- cplat-req: uuid={UUID_1} -->\n",
            )

            result = CHECKER.check_repository(root)

            self.assertTrue(any("正本に存在しない UUID" in error for error in result.errors))
            self.assertTrue(any("現在の要件 ID" in error for error in result.errors))

    def test_rejects_unpaired_and_legacy_ids(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            current_id = "CPLAT-CLOCK-FUNC-001"
            self._write_spec(root, [self._row(current_id, UUID_1)])
            self._write(
                root,
                "docs/design.md",
                f"要件: `{current_id}`\n旧要件: `CPLAT-CLOCK-001`\n",
            )

            result = CHECKER.check_repository(root)

            self.assertTrue(any("UUID が併記されていません" in error for error in result.errors))
            self.assertTrue(any("旧形式の要件 ID" in error for error in result.errors))

    def test_rejects_test_marker_without_description_or_adjacent_macro(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            requirement_id = "CPLAT-CLOCK-FUNC-001"
            self._write_spec(root, [self._row(requirement_id, UUID_1)])
            self._write(
                root,
                "test/sample.cpp",
                f"// cplat-req: id={requirement_id}; uuid={UUID_1}\n"
                "\n"
                "TEST(clockTest, monotonic_clock)\n"
                "{\n"
                "    // Arrange\n"
                "}\n",
            )

            result = CHECKER.check_repository(root)

            self.assertTrue(any("テスト項目の説明がありません" in error for error in result.errors))
            self.assertTrue(any("直後にテスト マクロがありません" in error for error in result.errors))

    def test_rejects_comment_style_for_the_wrong_medium(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            requirement_id = "CPLAT-CLOCK-FUNC-001"
            self._write_spec(root, [self._row(requirement_id, UUID_1)])
            self._write(
                root,
                "prod/sample.c",
                f"// cplat-req: id={requirement_id}; uuid={UUID_1}\n",
            )
            self._write(
                root,
                "test/sample.cpp",
                "// 単調増加クロックを確認する。\n"
                f"/* cplat-req: id={requirement_id}; uuid={UUID_1} */\n"
                "TEST(clockTest, monotonic_clock)\n"
                "{\n"
                "    // Arrange\n"
                "}\n",
            )

            result = CHECKER.check_repository(root)

            self.assertTrue(any("製品コードの要件コメントが不正" in error for error in result.errors))
            self.assertTrue(any("テストの要件コメントが不正" in error for error in result.errors))

    def test_rejects_wrong_or_omitted_requirement_subject(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self._write_spec(
                root,
                [
                    self._row_with_body(
                        "CPLAT-CLOCK-FUNC-001",
                        UUID_1,
                        "cplat の基盤機能は、時計を提供します。",
                    ),
                    self._row_with_body(
                        "CPLAT-CLOCK-FUNC-002",
                        UUID_2,
                        "cplat は、時計を提供します。",
                    ),
                    self._row_with_body(
                        "CPLAT-CLOCK-FUNC-003",
                        UUID_3,
                        "時計を提供します。",
                    ),
                ],
            )

            result = CHECKER.check_repository(root)

            subject_errors = [
                error for error in result.errors if "要件文の主語" in error
            ]
            self.assertEqual(3, len(subject_errors))


if __name__ == "__main__":
    unittest.main()
