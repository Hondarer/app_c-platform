# prompt 系テストの現状と引き継ぎ

`app/com_util/prod/libsrc/com_util/prompt/` 配下 5 本のソースに対するテストの状況と、未着手部分を進めるための手順をまとめます。

規約は [framework/testfw/docs/how-to-test.md](../../../../../../framework/testfw/docs/how-to-test.md) の「カバレッジの基準」と「テストの構成単位」に従います。カバレッジの原則は条件網羅 (C2) で、計測は Linux (gcov) を正本とします。

## 対応状況

| 対象ソース | テスト ディレクトリ | 行 | C2 | テスト数 |
|---|---|---|---|---|
| `prompt_edit.c` | `promptEditTest` | 98% | 100% | 18 |
| `prompt_linux.c` | `promptLinuxTest` | 98% | 100% | 11 |
| `prompt.c` | `promptTest` | 85% | 91% | 30 |
| `pinned_prompt.c` | `pinnedPromptTest` | 1% | 0% | 1 |
| `prompt_windows.c` | `promptWindowsTest` | - | - | 11 |

`pinned_prompt.c` (806 行) が未着手です。`prompt_windows.c` は Windows 専用実装のため、行/C2 の数値は Linux (gcov) では計測されません。

## テストの構成

### promptTest — プラットフォーム層を fake で差し替える

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

### promptLinuxTest — 標準入力の差し替えと inject

`prompt_linux.c` は `STDIN_FILENO` を直接扱うため、`dup2()` で標準入力をパイプまたは疑似端末へ差し替えて検証します。

- パイプ — `tcgetattr` が失敗する経路、`read` の通常読み取りと EOF、`select` のタイムアウト
- 疑似端末 (`posix_openpt`) — raw モードへの移行と復帰、SIGWINCH ハンドラーの登録と復元

`s_resize_pending` と `s_sigwinch_installed` はファイル内 `static` のため、`prompt_linux.inject.c` でアクセサーを通しています。リサイズ通知の経路 (`read` が EINTR で失敗し、かつリサイズ待ちがある) は `Mock_unistd` で `read` に EINTR を注入して到達させます。

### promptWindowsTest — Win32 API を Mock_windows でモックする

`prompt_windows.c` は `GetStdHandle`/`GetConsoleMode`/`SetConsoleMode`/`WaitForSingleObject`/`ReadFile` を直接呼び出します。これらは `framework/testfw` の `mock_windows.h`/`Mock_windows` ( `include_override/windows.h` による差し替え ) でモック化しています。`GetTickCount64` などの既存モックと同じ仕組みで、実コンソールや実プロセスを一切必要とせず、`SetConsoleMode` の失敗経路や `WaitForSingleObject` のタイムアウト経路も含めて全分岐を決定的に再現できます。

```cpp
Mock_windows mock_windows;

EXPECT_CALL(mock_windows, GetStdHandle(_, _, _, STD_INPUT_HANDLE)).WillOnce(Return(dummy_handle));
EXPECT_CALL(mock_windows, GetConsoleMode(_, _, _, dummy_handle, _))
    .WillOnce(DoAll(SetArgPointee<4>(orig_mode), Return(TRUE)));
```

`MOCK_METHOD` の各引数は `(__FILE__, __LINE__, __func__, 実引数...)` の順であるため、実引数の位置は 4 番目以降になります ( `SetArgPointee<4>` などのインデックスに注意 ) 。モック未注入時 (`_mock_windows == nullptr`) は自動的に実 API へ委譲されるため、`console.c`/`process.c`/`sync_windows.c` など他の `TEST_SRCS` が同じ関数を呼んでいても挙動は変化しません。

## 未到達として残している箇所

| 箇所 | 理由 |
|---|---|
| `prompt_linux.c:52` | `tcsetattr` の失敗経路。`tcsetattr` のモックが testfw にない |
| `prompt.c` の `redisplay` 周辺 | 画面制御の出力のみで分岐がない行、および履歴ブラウズ中の一部境界 |
| `prompt.c` の `find_or_create_ctx` の確保失敗 | `realloc` の失敗注入が必要。`mock_stdlib` は `calloc` と `getenv` のみ |
| `prompt_edit.c:98` | `realloc` の失敗経路。同上 |

`realloc` のモックを testfw へ追加すれば、`prompt.c` と `prompt_edit.c` の確保失敗経路をまとめて到達させられます。追加方法は `.claude/skills/create-testfw-mock/SKILL.md` に従ってください。

## 未着手分の進め方

### pinned_prompt.c (806 行)

現状の `pinnedPromptTest` は `com_util_pinned_prompt_status_*` の引数検証 1 件のみです。`prompt.c` と同様にプラットフォーム層を持つ構造であれば、`promptPlatformFake.cc` と同じ方式で差し替えられます。着手前に依存の切り口を確認してください。

`prompt_edit.c` を `ADD_SRCS` で取り込む構成は `pinnedPromptTest/makepart.mk` に反映済みです。fake を追加する場合は、そのディレクトリへ `.cc` を置くだけで自動収集されます。

### prompt_windows.c (89 行) — 対応済み

`promptWindowsTest` として実装済みです。`prompt_linux.c` と対になる 4 関数が extern リンクのため、`promptLinuxTest` のテスト構成をそのまま写像しています。標準入力の差し替えは、実コンソールではなく `Mock_windows` ( 「promptWindowsTest — Win32 API を Mock_windows でモックする」を参照 ) による決定的なモックで行っています。実コンソールの確保 (`AllocConsole`) や `GTEST_SKIP()` は使用していません。

`Mock_windows` への 5 関数 (`GetStdHandle`/`GetConsoleMode`/`SetConsoleMode`/`WaitForSingleObject`/`ReadFile`) の追加は `framework/testfw/include/mock_windows.h`、`framework/testfw/libsrc/mock_libc/mock_windows.cc`、`framework/testfw/libsrc/mock_libc/mock_<Func>.cc` ( 関数ごと ) で行いました。`framework/testfw` は別 git ルートのため、変更する際は `framework/testfw/AGENTS.md` に従ってください。

Windows CI での初回確認はまだ実施できていません。

同様に Windows 専用で未着手のソースは com_util 全体で 8 本あります。`trace_etw_session.c`、`trace_eventlog.c`、`win32/` 配下 4 本、`wchar_conv.c`、`crypto_windows.c` です。

## 作業時の注意

- テスト ディレクトリを改名した場合は `obj/` を削除してから再ビルドしてください。`.gcno` に埋め込まれた旧パスと実体がずれ、テストは通るのにカバレッジが 0% と表示されます
- `ADD_SRCS` に指定したソースは override ヘッダーの対象外です。モックを前提とするテストがある場合、実装を `ADD_SRCS` で取り込むと実物が優先されてテストが壊れます
- `ADD_SRCS` は自ディレクトリ外のソースを引き込むための指定です。テスト ディレクトリ直下に置いた補助ソースは自動収集されるため、記載すると重複になります
- `prompt_internal.h` には `extern "C"` を追加済みです。テスト コード (C++) からプラットフォーム層を差し替えるために必要でした
