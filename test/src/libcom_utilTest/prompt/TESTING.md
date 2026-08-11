# prompt 系テストの現状と引き継ぎ

`app/com_util/prod/libsrc/com_util/prompt/` 配下 5 本のソースに対するテストの状況と、未着手部分を進めるための手順をまとめます。

規約は [framework/testfw/docs/how-to-test.md](../../../../../../framework/testfw/docs/how-to-test.md) の「カバレッジの基準」と「テストの構成単位」に従います。カバレッジの原則は条件網羅 (C2) で、最新値の計測は Linux (gcov) を正本とします。

## 対応状況

| 対象ソース | テスト ディレクトリ | テスト数 | 現状 |
|---|---|---:|---|
| `prompt_edit.c` | `promptEditTest` | 19 | 基本経路と `realloc` 失敗経路をテスト実装済み |
| `prompt_linux.c` | `promptLinuxTest` | 12 | `tcsetattr` 失敗経路を含む主要経路をテスト実装済み |
| `prompt.c` | `promptTest` | 37 | `realloc` 失敗経路を含む主要経路をテスト実装済み |
| `pinned_prompt.c` | `pinnedPromptTest` | 1 | ステータス API の NULL 引数だけを確認。拡充が必要 |
| `prompt_windows.c` | `promptWindowsTest` | 13 | Windows API の主要経路をテスト実装済み |

`pinned_prompt.c` (1,860 行) のテストは未完了です。`prompt_windows.c` は Windows 専用実装のため、Linux ではテストが 0 件となり、行/C2 の数値も計測されません。

## カバレッジ計測結果

2026-08-11 に Linux (gcov) で `make test` を実行した結果です。行カバレッジと C2 カバレッジは、各テストでリンクした対象ソースに対する値です。

| 対象ソース | テスト数 | 行カバレッジ | C2 カバレッジ | 状況 |
|---|---:|---:|---:|---|
| `prompt_edit.c` | 19 | 100% (64/64) | 100% (54/54) | 到達済み |
| `prompt_linux.c` | 12 | 100% (48/48) | 100% (22/22) | 到達済み |
| `prompt.c` | 37 | 91% (310/340) | 94% (170/181) | 未到達行あり |
| `pinned_prompt.c` | 1 | 1% (6/806) | 0% (2/473) | テスト拡充が必要 |
| `prompt_windows.c` | 0 (Linux) | 計測対象外 | 計測対象外 | Windows で実行 |

`promptWindowsTest` には Windows 用のテストを 13 件定義しています。Linux 実行時は Windows 専用コードがコンパイル対象外となるため、テスト数は 0 件です。

計測を再現する場合は、次のディレクトリで実行します。

```bash
cd app/com_util/test/src/libcom_utilTest/prompt
make test
```

## テストの構成

### promptTest: プラットフォーム層を fake で差し替える

`prompt.c` は端末制御を `prompt_platform_enter_raw` / `leave_raw` / `read_char` / `read_char_nb` の 4 関数へ委ねています。実機の端末を必要とせずキー入力を再現するため、`promptPlatformFake.cc` でこの 4 関数を差し替えています。

```makefile
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/prompt/prompt.c

ADD_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/prompt/prompt_edit.c
```

`prompt_linux.c` / `prompt_windows.c` はリンクしません。

`promptPlatformFake.cc` は `makepart.mk` に現れません。テスト ディレクトリ直下のソースは `SRCS_CPP` として自動収集されるためです。`ADD_SRCS` は自ディレクトリ外のソースを引き込むための指定であり、同ディレクトリのファイルに使う必要はありません。

テストからは `promptFakeSetInput()` へバイト列を渡し、`com_util_prompt_readline_at()` を呼び出します。列を消費し切ると EOF (-1) を返します。

```cpp
promptFakeSetInput("abc\r");                 // "abc" と Enter
promptFakeSetInput("abc\x1B[D\x1B[3~\r");    // "abc"、左矢印、Delete、Enter
```

`enter_raw` / `leave_raw` の呼び出し回数は `promptFakeEnterRawCount()` / `promptFakeLeaveRawCount()` で取得できます。

### TTY 状態の扱い

`com_util_prompt_readline_at()` は `p->is_tty` が 0 のとき `fgets()` フォールバックへ分岐します。CI では標準入力が端末ではないため、対話パスを通すにはハンドル生成後に直接立てます。

```cpp
prompt_ = com_util_prompt_create(NULL);
prompt_->is_tty = 1;
```

構造体の実体は `prod/include_internal/com_util/prompt/prompt_internal.h` にあり、テストから参照できます。

### promptLinuxTest: 標準入力の差し替えと inject

`prompt_linux.c` は `STDIN_FILENO` を直接扱うため、`dup2()` で標準入力をパイプまたは疑似端末へ差し替えて検証します。

- パイプ: `tcgetattr` が失敗する経路、`read` の通常読み取りと EOF、`select` のタイムアウト
- 疑似端末 (`posix_openpt`): raw モードへの移行と復帰、`tcsetattr` の失敗、SIGWINCH ハンドラーの登録と復元

`s_resize_pending` と `s_sigwinch_installed` はファイル内 `static` のため、`prompt_linux.inject.c` でアクセサーを通しています。リサイズ通知の経路 (`read` が EINTR で失敗し、かつリサイズ待ちがある) は `Mock_unistd` で `read` に EINTR を注入して到達させます。

### promptWindowsTest: Win32 API を Mock_windows でモックする

`prompt_windows.c` は `GetStdHandle`/`GetConsoleMode`/`SetConsoleMode`/`WaitForSingleObject`/`ReadFile` を直接呼び出します。これらは `framework/testfw` の `mock_windows.h`/`Mock_windows` (`include_override/windows.h` による差し替え) でモック化しています。`GetTickCount64` などの既存モックと同じ仕組みで、実コンソールや実プロセスを一切必要とせず、`SetConsoleMode` の失敗経路や `WaitForSingleObject` のタイムアウト経路も含めて全分岐を決定的に再現できます。

```cpp
Mock_windows mock_windows;

EXPECT_CALL(mock_windows, GetStdHandle(_, _, _, STD_INPUT_HANDLE)).WillOnce(Return(dummy_handle));
EXPECT_CALL(mock_windows, GetConsoleMode(_, _, _, dummy_handle, _))
    .WillOnce(DoAll(SetArgPointee<4>(orig_mode), Return(TRUE)));
```

`MOCK_METHOD` の各引数は `(__FILE__, __LINE__, __func__, 実引数...)` の順であるため、実引数の位置は 4 番目以降になります (`SetArgPointee<4>` などのインデックスに注意)。モック未注入時 (`_mock_windows == nullptr`) は自動的に実 API へ委譲されるため、`console.c`/`process.c`/`sync_windows.c` など他の `TEST_SRCS` が同じ関数を呼んでいても挙動は変化しません。

## 未到達として残している箇所

| 箇所 | 理由 |
|---|---|
| `prompt.c` の入力解析 | 未知のエスケープ シーケンス、リサイズ通知、制御文字の未到達経路が残っています (55, 101, 126, 175, 199, 206, 214 行)。 |
| `prompt.c` の履歴処理 | NULL エントリ、容量不足、履歴の境界条件が未到達です (246, 251, 255, 268, 278, 283, 296, 300 行)。 |
| `prompt.c` のフォールバックと書式付き API | TTY でない場合とコンテキスト作成失敗時の出力、改行除去、NULL 書式、書式化失敗、再確保成功の経路が未到達です (336, 471-472, 478-479, 496-497, 679, 689-690, 705-706 行)。 |
| `pinned_prompt.c` | 806 行中 6 行の実行にとどまり、473 分岐中 2 分岐だけを実行しています。固定表示の描画、端末制御、履歴、解放処理などのテストが必要です。 |

`tcsetattr` と `realloc` の失敗経路は、対応する testfw のモックとテストを追加済みです。

## 今後の対応

### pinned_prompt.c (1,860 行)

現状の `pinnedPromptTest` は `com_util_pinned_prompt_status_*` の NULL 引数検証 1 件のみです。まず `pinned_prompt.c` の static 関数と OS 関連 API を分類し、static 関数は inject、端末や標準 OS 関数は testfw のモックで差し替えます。

`prompt.c` は未到達行を個別に確認し、入力解析と履歴境界のテストを追加します。画面制御の出力だけで分岐を持たない行は、C2 の対象として必要かをソースの条件単位で判断します。

`prompt_edit.c` を `ADD_SRCS` で取り込む構成は `pinnedPromptTest/makepart.mk` に反映済みです。fake を追加する場合は、そのディレクトリへ `.cc` を置くだけで自動収集されます。

## 作業時の注意

- テスト ディレクトリを改名した場合は `obj/` を削除してから再ビルドしてください。`.gcno` に埋め込まれた旧パスと実体がずれ、テストは通るのにカバレッジが 0% と表示されます
- `ADD_SRCS` に指定したソースは override ヘッダーの対象外です。モックを前提とするテストがある場合、実装を `ADD_SRCS` で取り込むと実物が優先されてテストが壊れます
- `ADD_SRCS` は自ディレクトリ外のソースを引き込むための指定です。テスト ディレクトリ直下に置いた補助ソースは自動収集されるため、記載すると重複になります
- `prompt_internal.h` には `extern "C"` を追加済みです。テスト コード (C++) からプラットフォーム層を差し替えるために必要でした
