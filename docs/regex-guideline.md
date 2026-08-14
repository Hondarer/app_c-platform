# 正規表現 API (regex) の利用指針

`com_util/regex/regex.h` が提供する正規表現 API の設計方針と、POSIX `<regex.h>` との対応を示します。

com_util が公開する API 全体の一覧は [com_util API チート シート](api-cheatsheet.md) を参照してください。

## 実装の背景

正規表現は C 標準に存在せず、POSIX `<regex.h>` は Linux の libc には含まれますが Windows (MSVC) には存在しません。  
本 API は C++ 標準ライブラリの `std::basic_regex` を単一の実装として使用し、両プラットフォームで同一の照合結果を得られるようにしています。外部ライブラリへの依存は追加していません。

文字特性 (`std::regex_traits` 相当) は com_util 側で実装しています。  
標準の `std::regex_traits` はロケールの `ctype` / `collate` ファセットに依存し、環境設定によって照合結果が変わるためです。自前の実装により、照合結果はロケールに依存しません。

## 文字モデル

照合は **UTF-16 コード単位 1 個を 1 文字** として行います。  
公開 API が受け渡す文字列はすべて UTF-8 であり、マッチ位置は UTF-8 のバイト オフセットで返します。

| 文字の範囲 | 例 | 照合上の文字数 |
|---|---|---|
| ASCII | `a` | 1 |
| 基本多言語面 (BMP) | `あ`、`漢` | 1 |
| BMP 外 (追加面) | 絵文字、CJK 拡張 B 以降 | 2 |

BMP 外の文字が 2 文字として扱われるのは、ECMAScript の `u` フラグを指定しない正規表現、.NET、Java と同じモデルです。  
`.` 1 個では BMP 外の文字全体に一致せず、`.{2}` に一致します。

Linux の `wchar_t` は 32 ビットですが、内部表現にはあえて UTF-16 コード単位を格納しています。  
`wchar_t` が 16 ビットである Windows と照合セマンティクスを完全に一致させるためです。ネイティブの `wchar_t` をそのまま使うと、Linux では BMP 外の文字が 1 文字、Windows では 2 文字となり、プラットフォーム間で結果が食い違います。

マッチ境界がサロゲート ペアの内側に落ちた場合、返すバイト オフセットはコード ポイント境界へ丸めます (開始位置は前方、終了位置は後方)。  
このため、返却された範囲で切り出した部分文字列が不正な UTF-8 になることはありません。

## POSIX <regex.h> との対応

| POSIX | 本 API | 備考 |
|---|---|---|
| `regcomp()` | `com_util_regex_create()` | 生成したハンドルは `com_util_regex_dispose()` で破棄します。 |
| `regexec()` | `com_util_regex_search()` | 全体一致の判定には `com_util_regex_matches()` を使用します。 |
| `regfree()` | `com_util_regex_dispose()` | |
| `regerror()` | (なし) | 診断文字列は返さない。結果コードで区別します。 |
| `regmatch_t` | `com_util_regex_match` | `rm_so` / `rm_eo` は `begin` / `end` に対応します。 |
| `re_nsub` | `com_util_regex_get_group_count()` | 全体マッチの 1 を含めた要素数を返す |
| `nmatch` | `matches_capacity` | 不足時は先頭から切り捨てる (POSIX と同じ) |
| `REG_EXTENDED` | `COM_UTIL_REGEX_EXTENDED` | 無指定時の既定は ECMAScript |
| `REG_ICASE` | `COM_UTIL_REGEX_ICASE` | 畳み込みは ASCII 範囲のみ |
| `REG_NOSUB` | `COM_UTIL_REGEX_NOSUB` | |
| `REG_NEWLINE` | (なし) | 後述 |
| `REG_NOTBOL` | `COM_UTIL_REGEX_MATCH_NOTBOL` | |
| `REG_NOTEOL` | `COM_UTIL_REGEX_MATCH_NOTEOL` | |
| `REG_NOMATCH` | `matched_out` に 0 | 戻り値ではなく出力引数で表す |
| `-1` などの位置 | `COM_UTIL_REGEX_NPOS` | 不参加の捕捉グループを表す |

POSIX に対応がなく本 API が追加している機能は、置換 (`com_util_regex_replace()`)、反復列挙 (`com_util_regex_iter_*()`)、分割 (`com_util_regex_split()`) です。

### 「一致しなかった」を戻り値で表さない理由

com_util の戻り値規約では、非 0 は「要求した操作が完遂されなかった」ことを表します。  
照合が完了して一致が無かった場合、操作自体は成功しているため、結果は出力引数 `matched_out` で表します。  
既存の `com_util_paths_equal()` と同じ形です。

### REG_NEWLINE を提供しない理由

POSIX の `REG_NEWLINE` は、(a) `^` と `$` を行境界にも一致させる、(b) `.` と否定文字クラスを改行に一致させない、の 2 つを同時に意味します。  
`std::regex` には (b) を実現する手段がなく、(a) に対応する `std::regex_constants::multiline` も libstdc++ では GCC 11 以降でしか利用できません。  
片方だけを `REG_NEWLINE` の名前で提供すると誤解を招くため、本 API はこのフラグを提供していません。行単位の扱いが必要な場合は、入力を行ごとに分割してから照合するか、`[^\n]` のようにパターン側で明示してください。

## 制限事項

- **大小の畳み込みは ASCII 範囲のみ** です。`Ä` と `ä` は一致しません。Unicode のケース フォールディング表は保持していません。
- **文字クラスは ASCII 定義** です。`\w` と `\d` は ECMAScript の仕様自体が ASCII 定義のため仕様どおりですが、`\s` は仕様より狭くなります。
- **Unicode 正規化は行いません**。`"が"` (U+304C) と `"か" + 濁点` (U+304B U+3099) は一致しません。
- **照合要素 `[[.x.]]` と等価クラス `[[=x=]]` はサポートしません**。
- 不正な UTF-8 (オーバー ロング表現、単独サロゲート、U+10FFFF 超、途中で切れた列) は `COM_UTIL_ERR_INVALID_ENCODING` で拒否します。
- パターンと入力のバイト数は `COM_UTIL_REGEX_MAX_LENGTH` (1 MiB) までです。

## 信頼できない入力を扱わないこと

`std::regex` は再帰的なバックトラッキングで実装されています。  
`(a+)+b` のような病的なパターンと長い入力を組み合わせると、照合が指数的な時間を要します。  
さらに libstdc++ はスタックの枯渇を検出しないため、深い再帰でプロセスが異常終了する可能性があります。MSVC の標準ライブラリは複雑度の上限を持ち `COM_UTIL_ERR_LIMIT_EXCEEDED` を返すため、同じ入力でもプラットフォームによって結果が異なります。

パターンは自プログラムが用意した定数を使用し、外部から与えられた文字列をパターンとして受け付けないでください。

## 結果コード

| 結果コード | 主な発生条件 |
|---|---|
| `COM_UTIL_ERR_INVALID_ARGUMENT` | NULL 引数、未定義のフラグ ビット、`EXTENDED` と `BASIC` の同時指定、コード ポイント境界を指さない `start_offset` |
| `COM_UTIL_ERR_INVALID_PATTERN` | パターンの構文エラー |
| `COM_UTIL_ERR_INVALID_ENCODING` | パターンまたは入力が不正な UTF-8 |
| `COM_UTIL_ERR_BUFFER_TOO_SMALL` | 置換結果または分割結果の格納先が不足 |
| `COM_UTIL_ERR_LIMIT_EXCEEDED` | 入力長の上限超過、照合の複雑度の上限超過 |
| `COM_UTIL_ERR_OUT_OF_MEMORY` | 内部バッファーの確保に失敗 |

いずれも OS 呼び出しに由来しない失敗のため、`detail_out` には詳細を格納しません。  
詳細は [コーディング規範](coding-guideline.md) の「OS エラー詳細の抽象化」を参照してください。
