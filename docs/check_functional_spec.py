#!/usr/bin/env python3
"""cplat の要件 ID、UUID、下流成果物の参照を検査する。"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import re
import sys


ID_TEXT = r"CPLAT-[A-Z0-9]+-(?:FUNC|QUAL|COMP|CONS)-[0-9]{3,}"
UUID_TEXT = (
    r"[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-"
    r"[89ab][0-9a-f]{3}-[0-9a-f]{12}"
)

ID_RE = re.compile(
    r"^CPLAT-(?P<category>[A-Z0-9]+)-"
    r"(?P<kind>FUNC|QUAL|COMP|CONS)-(?P<number>[0-9]{3,})$"
)
ID_FIND_RE = re.compile(rf"\b{ID_TEXT}\b")
LEGACY_ID_RE = re.compile(
    r"\bCPLAT-[A-Z0-9]+-(?!(?:FUNC|QUAL|COMP|CONS)-)[0-9]{3,}\b"
)
CANONICAL_ROW_RE = re.compile(
    rf"^\| `(?P<id>{ID_TEXT})` "
    rf"<!-- cplat-req: uuid=(?P<uuid>{UUID_TEXT}) --> "
    r"\| (?P<body>.+) \|$"
)
MARKDOWN_REF_RE = re.compile(
    rf"`(?P<id>{ID_TEXT})` "
    rf"<!-- cplat-req: uuid=(?P<uuid>{UUID_TEXT}) -->"
)
SOURCE_REF_RE = re.compile(
    rf"^\s*/\* cplat-req: id=(?P<id>{ID_TEXT}); "
    rf"uuid=(?P<uuid>{UUID_TEXT}) \*/\s*$"
)
TEST_REF_RE = re.compile(
    rf"^\s*// cplat-req: id=(?P<id>{ID_TEXT}); "
    rf"uuid=(?P<uuid>{UUID_TEXT})\s*$"
)
TEST_DESCRIPTION_RE = re.compile(r"^\s*// (?!cplat-req:).+\S\s*$")
TEST_MACRO_RE = re.compile(r"^\s*(?:TEST|TEST_F|TEST_P|TYPED_TEST)\s*\(")

SOURCE_SUFFIXES = {
    ".c",
    ".cc",
    ".cpp",
    ".cxx",
    ".h",
    ".hh",
    ".hpp",
    ".hxx",
}
SCAN_SUFFIXES = SOURCE_SUFFIXES | {".md"}
EXCLUDED_DIRECTORY_NAMES = {
    ".git",
    "doxybook2_internal",
    "doxybook2_public",
}

CATEGORY_SUBJECTS = {
    "ARGPARSER": "cplat のコマンド ライン引数解析機能",
    "BASE": "cplat の基盤機能",
    "CLOCK": "cplat の時計・時刻機能",
    "COMPRESS": "cplat の圧縮機能",
    "CONSOLE": "cplat のコンソール機能",
    "CRT": "cplat の C ランタイム抽象機能",
    "CRYPTO": "cplat の暗号機能",
    "HASHTABLE": "cplat のハッシュ テーブル機能",
    "MMAP": "cplat のメモリ マップド ファイル機能",
    "NET": "cplat のネットワーク機能",
    "PROMPT": "cplat のプロンプト機能",
    "REGEX": "cplat の正規表現機能",
    "RUNTIME": "cplat の実行時支援機能",
    "SYNC": "cplat の同期機能",
    "TRACE": "cplat のトレース機能",
    "WIN32": "cplat の Win32 UTF-8 ラッパー機能",
}


@dataclass(frozen=True)
class Requirement:
    requirement_id: str
    uuid: str
    path: Path
    line: int


@dataclass(frozen=True)
class Reference:
    requirement_id: str
    uuid: str
    path: Path
    line: int


@dataclass(frozen=True)
class CheckResult:
    errors: list[str]
    requirement_count: int
    reference_count: int


def _relative(path: Path, root: Path) -> str:
    try:
        return path.relative_to(root).as_posix()
    except ValueError:
        return path.as_posix()


def _error(path: Path, line: int, root: Path, message: str) -> str:
    return f"{_relative(path, root)}:{line}: {message}"


def _read_lines(path: Path, root: Path, errors: list[str]) -> list[str]:
    try:
        return path.read_text(encoding="utf-8").splitlines()
    except (OSError, UnicodeError) as exc:
        errors.append(_error(path, 1, root, f"ファイルを読み取れません: {exc}"))
        return []


def _is_excluded(path: Path, root: Path) -> bool:
    relative = path.relative_to(root)
    return any(part in EXCLUDED_DIRECTORY_NAMES for part in relative.parts)


def _is_test_path(path: Path, root: Path) -> bool:
    return path.relative_to(root).parts[0] == "test"


def _covered_id_spans(matches: list[re.Match[str]]) -> set[tuple[int, int]]:
    return {match.span("id") for match in matches}


def _check_unpaired_ids(
    line: str,
    valid_matches: list[re.Match[str]],
    path: Path,
    line_number: int,
    root: Path,
    errors: list[str],
) -> None:
    covered = _covered_id_spans(valid_matches)
    for match in ID_FIND_RE.finditer(line):
        if match.span() not in covered:
            errors.append(
                _error(
                    path,
                    line_number,
                    root,
                    f"要件 ID {match.group(0)} に UUID が併記されていません",
                )
            )


def _check_legacy_ids(
    line: str,
    path: Path,
    line_number: int,
    root: Path,
    errors: list[str],
) -> None:
    for match in LEGACY_ID_RE.finditer(line):
        errors.append(
            _error(
                path,
                line_number,
                root,
                f"旧形式の要件 ID が残っています: {match.group(0)}",
            )
        )


def _load_requirements(
    root: Path, errors: list[str]
) -> tuple[dict[str, Requirement], dict[str, Requirement], set[Path]]:
    spec_directory = root / "docs" / "functional-spec"
    if not spec_directory.is_dir():
        errors.append(f"docs/functional-spec: 機能仕様ディレクトリがありません")
        return {}, {}, set()

    requirements_by_id: dict[str, Requirement] = {}
    requirements_by_uuid: dict[str, Requirement] = {}
    spec_paths: set[Path] = set()

    for path in sorted(spec_directory.glob("*.md")):
        if path.name == "README.md":
            continue
        spec_paths.add(path)
        category = path.stem.upper()
        lines = _read_lines(path, root, errors)
        if "| 要件 ID | cplat の要件 |" not in lines:
            errors.append(_error(path, 1, root, "機能要件表の見出しがありません"))
        if "|---|---|" not in lines:
            errors.append(_error(path, 1, root, "機能要件表の区切りがありません"))

        previous_numbers: dict[str, int] = {}
        row_count = 0
        for line_number, line in enumerate(lines, start=1):
            if not line.startswith("| `CPLAT-"):
                continue
            row_count += 1
            match = CANONICAL_ROW_RE.fullmatch(line)
            if match is None:
                errors.append(
                    _error(path, line_number, root, "機能要件行の書式が不正です")
                )
                continue

            requirement_id = match.group("id")
            requirement_uuid = match.group("uuid")
            id_match = ID_RE.fullmatch(requirement_id)
            assert id_match is not None
            if id_match.group("category") != category:
                errors.append(
                    _error(
                        path,
                        line_number,
                        root,
                        f"要件 ID のカテゴリが文書名と一致しません: {requirement_id}",
                    )
                )

            expected_subject = CATEGORY_SUBJECTS.get(category)
            if expected_subject is None:
                errors.append(
                    _error(
                        path,
                        line_number,
                        root,
                        f"カテゴリの正式主語が定義されていません: {category}",
                    )
                )
            elif not match.group("body").startswith(f"{expected_subject}は、"):
                errors.append(
                    _error(
                        path,
                        line_number,
                        root,
                        f"要件文の主語は「{expected_subject}」でなければなりません",
                    )
                )

            kind = id_match.group("kind")
            number = int(id_match.group("number"))
            previous_number = previous_numbers.get(kind, 0)
            if number <= previous_number:
                errors.append(
                    _error(
                        path,
                        line_number,
                        root,
                        f"{kind} の連番が掲載順に増加していません: {requirement_id}",
                    )
                )
            previous_numbers[kind] = number

            requirement = Requirement(
                requirement_id, requirement_uuid, path, line_number
            )
            duplicate_id = requirements_by_id.get(requirement_id)
            if duplicate_id is not None:
                errors.append(
                    _error(
                        path,
                        line_number,
                        root,
                        f"要件 ID が重複しています: {requirement_id} "
                        f"({_relative(duplicate_id.path, root)}:{duplicate_id.line})",
                    )
                )
            else:
                requirements_by_id[requirement_id] = requirement

            duplicate_uuid = requirements_by_uuid.get(requirement_uuid)
            if duplicate_uuid is not None:
                errors.append(
                    _error(
                        path,
                        line_number,
                        root,
                        f"UUID が重複しています: {requirement_uuid} "
                        f"({_relative(duplicate_uuid.path, root)}:{duplicate_uuid.line})",
                    )
                )
            else:
                requirements_by_uuid[requirement_uuid] = requirement

        if row_count == 0:
            errors.append(_error(path, 1, root, "機能要件がありません"))

    if not spec_paths:
        errors.append("docs/functional-spec: 機能仕様がありません")
    return requirements_by_id, requirements_by_uuid, spec_paths


def _validate_reference(
    reference: Reference,
    requirements_by_uuid: dict[str, Requirement],
    root: Path,
    errors: list[str],
) -> None:
    requirement = requirements_by_uuid.get(reference.uuid)
    if requirement is None:
        errors.append(
            _error(
                reference.path,
                reference.line,
                root,
                f"正本に存在しない UUID です: {reference.uuid}",
            )
        )
        return
    if requirement.requirement_id != reference.requirement_id:
        errors.append(
            _error(
                reference.path,
                reference.line,
                root,
                f"UUID に対応する現在の要件 ID は "
                f"{requirement.requirement_id} です: {reference.requirement_id}",
            )
        )


def _scan_markdown(
    path: Path,
    lines: list[str],
    root: Path,
    errors: list[str],
    skip_canonical_rows: bool = False,
) -> list[Reference]:
    references: list[Reference] = []
    for line_number, line in enumerate(lines, start=1):
        if skip_canonical_rows and CANONICAL_ROW_RE.fullmatch(line) is not None:
            continue
        _check_legacy_ids(line, path, line_number, root, errors)
        matches = list(MARKDOWN_REF_RE.finditer(line))
        if "cplat-req:" in line and not matches:
            errors.append(_error(path, line_number, root, "要件コメントの書式が不正です"))
        _check_unpaired_ids(line, matches, path, line_number, root, errors)
        references.extend(
            Reference(match.group("id"), match.group("uuid"), path, line_number)
            for match in matches
        )
    return references


def _scan_source(
    path: Path,
    lines: list[str],
    root: Path,
    errors: list[str],
) -> list[Reference]:
    references: list[Reference] = []
    for line_number, line in enumerate(lines, start=1):
        _check_legacy_ids(line, path, line_number, root, errors)
        match = SOURCE_REF_RE.fullmatch(line)
        matches = [match] if match is not None else []
        if "cplat-req:" in line and match is None:
            errors.append(_error(path, line_number, root, "製品コードの要件コメントが不正です"))
        _check_unpaired_ids(line, matches, path, line_number, root, errors)
        if match is not None:
            references.append(
                Reference(match.group("id"), match.group("uuid"), path, line_number)
            )
    return references


def _scan_test(
    path: Path,
    lines: list[str],
    root: Path,
    errors: list[str],
) -> list[Reference]:
    references: list[Reference] = []
    marker_indexes: list[int] = []
    for index, line in enumerate(lines):
        line_number = index + 1
        _check_legacy_ids(line, path, line_number, root, errors)
        match = TEST_REF_RE.fullmatch(line)
        matches = [match] if match is not None else []
        if "cplat-req:" in line and match is None:
            errors.append(_error(path, line_number, root, "テストの要件コメントが不正です"))
        _check_unpaired_ids(line, matches, path, line_number, root, errors)
        if match is not None:
            marker_indexes.append(index)
            references.append(
                Reference(match.group("id"), match.group("uuid"), path, line_number)
            )

    marker_index_set = set(marker_indexes)
    for index in marker_indexes:
        if index - 1 in marker_index_set:
            continue
        if index == 0 or TEST_DESCRIPTION_RE.fullmatch(lines[index - 1]) is None:
            errors.append(
                _error(
                    path,
                    index + 1,
                    root,
                    "要件コメントの直前にテスト項目の説明がありません",
                )
            )

        last_index = index
        while last_index + 1 in marker_index_set:
            last_index += 1
        if (
            last_index + 1 >= len(lines)
            or TEST_MACRO_RE.match(lines[last_index + 1]) is None
        ):
            errors.append(
                _error(
                    path,
                    last_index + 1,
                    root,
                    "要件コメントの直後にテスト マクロがありません",
                )
            )
    return references


def _iter_downstream_files(root: Path, spec_paths: set[Path]) -> list[Path]:
    guideline_path = root / "docs" / "functional-spec-guideline.md"
    paths: list[Path] = []
    for path in root.rglob("*"):
        if not path.is_file() or path.suffix.lower() not in SCAN_SUFFIXES:
            continue
        if _is_excluded(path, root) or path in spec_paths or path == guideline_path:
            continue
        paths.append(path)
    return sorted(paths)


def check_repository(root: Path) -> CheckResult:
    root = root.resolve()
    errors: list[str] = []
    requirements_by_id, requirements_by_uuid, spec_paths = _load_requirements(
        root, errors
    )

    references: list[Reference] = []
    for path in sorted(spec_paths):
        lines = _read_lines(path, root, errors)
        references.extend(
            _scan_markdown(
                path,
                lines,
                root,
                errors,
                skip_canonical_rows=True,
            )
        )

    for path in _iter_downstream_files(root, spec_paths):
        lines = _read_lines(path, root, errors)
        if path.suffix.lower() == ".md":
            references.extend(_scan_markdown(path, lines, root, errors))
        elif _is_test_path(path, root):
            references.extend(_scan_test(path, lines, root, errors))
        else:
            references.extend(_scan_source(path, lines, root, errors))

    for reference in references:
        _validate_reference(reference, requirements_by_uuid, root, errors)

    return CheckResult(
        errors=errors,
        requirement_count=len(requirements_by_id),
        reference_count=len(references),
    )


def _parse_args(argv: list[str]) -> argparse.Namespace:
    default_root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(
        description="cplat の要件 ID、UUID、下流成果物の参照を検査します。"
    )
    parser.add_argument(
        "--root",
        type=Path,
        default=default_root,
        help="c-platform のルート ディレクトリ",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = _parse_args(sys.argv[1:] if argv is None else argv)
    result = check_repository(args.root)
    if result.errors:
        for error in result.errors:
            print(error, file=sys.stderr)
        print(f"ERROR: {len(result.errors)} 件の問題があります", file=sys.stderr)
        return 1

    print(
        f"OK: 要件 {result.requirement_count} 件、"
        f"下流参照 {result.reference_count} 件"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
