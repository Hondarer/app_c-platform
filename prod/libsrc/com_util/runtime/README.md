---
short-title: "runtime"
---

# runtime - 実行時補助ユーティリティ

`runtime` は、実行時にモジュール自身の情報を取得したり、外部ライブラリの関数を動的に解決したりするためのユーティリティ群です。  
公開面は大きく `com_util/runtime/module.h` と `com_util/runtime/sym_loader.h` の 2 つに分かれます。

## 目的

共有ライブラリやプラグイン型の構成では、実行時に「自分がどこからロードされたか」「どの関数実装を外部ライブラリへ委譲するか」を知りたい場面があります。  
このモジュールは、そのための最低限の共通部品を提供します。

- 関数アドレスから所属モジュールのパスや basename を取得できる
- 設定ファイルに基づいて関数シンボルを動的に解決できる
- 解決結果をキャッシュし、同じ関数を繰り返し高速に呼べる
- Linux と Windows のローダー API 差異を吸収できる

## 構成

### module_info

`module_info` は、指定した関数アドレスが属するモジュールの情報を取得する機能です。

- `com_util_module_get_path`: モジュールの絶対パスを取得する
- `com_util_module_get_basename`: モジュールの basename を取得する

典型的には、ロード中の共有ライブラリ自身の名前から設定ファイル名やログ識別子を組み立てる用途で使います。

### process_info

`process_info` は、現在のプロセスの実行ファイル本体の情報取得と、Linux/Windows での子プロセス起動を担う機能です。管理者権限確認や昇格起動の責務は持たず、コンソール コンポーネントにも依存しません。

- `com_util_process_get_executable_path`: プロセスの実行ファイル絶対パスを取得する
- `com_util_process_start` / `com_util_process_wait` / `com_util_process_get_exit_code` / `com_util_process_terminate` / `com_util_process_destroy`: 子プロセスの起動・待機・終了コード取得・強制終了・破棄
- `com_util_process_run_sync`: 子プロセスを起動し、終了まで同期的に待機する

`com_util_module_get_path()` は関数アドレスが属するモジュールを返すため、Windows では DLL を指しうます。`com_util_process_get_executable_path()` は常にプロセス本体 (`.exe`) のパスを返します。  
典型的には、サービス登録時の `ExecStart` や SCM の `binPath` 設定に使います。

### elevated_process

`elevated_process` は、管理者/root 権限の確認と、必要に応じた昇格プロセスの起動を担う機能です。プロセスの待機・終了コード取得・破棄は `process_info` を再利用しますが、`process_info` 側が `elevated_process` に依存することはありません。

- `com_util_elevated_process_is_elevated`: 現在のプロセスが管理者/root 権限で動作しているかを確認する
- `com_util_elevated_process_run_if_needed`: 管理者/root 権限が必要な処理のため、必要に応じて昇格実行する
- `com_util_elevated_process_run_with_result`: 同様に昇格実行し、昇格プロセスが報告した結果メッセージを取得する
- `com_util_elevated_process_extract_result_target` / `com_util_elevated_process_report_result`: 昇格プロセス側で結果メッセージを報告する

`com_util_elevated_process_run_if_needed()` は、権限が必要な処理の入口で呼び出します。Windows では未昇格の場合に UAC を要求して現在の実行ファイルを再起動し、Linux では実効ユーザー ID が root でなければ失敗します。Windows で親にコンソールがある場合は、昇格プロセスのコマンド ラインへ親プロセス ID と親コンソールの window ハンドルを引き継ぎフラグとして付与します。昇格プロセス側は `com_util_console_attach_parent()` でこれを検出し、親コンソールへ確実に再接続したことを確認したうえで出力を元のコンソールへ戻します。

ただし、UAC 昇格直後の親コンソール再割り当ては、実機調査の結果、`AttachConsole` 後の安定待ちを満たしてもなお間欠的に書き込み不能 (`ERROR_INVALID_HANDLE`) になることがあり、原因を特定できていません。確実に結果を表示したい場合は `com_util_elevated_process_run_with_result()` を使ってください。こちらは昇格プロセスのコンソールを一切引き継がず、結果メッセージを一時ファイル経由で受け渡します。昇格プロセス側は起動直後に `com_util_elevated_process_extract_result_target()` を呼び出し、処理結果を `com_util_elevated_process_report_result()` で報告します。呼び出し元プロセス (常に未昇格、かつ自分自身の正常なコンソールを保持している) が、そのメッセージを `printf`/`fprintf` で表示します。

### sym_loader

`sym_loader` は、設定ファイルで指定した `lib_name` / `func_name` を使って関数ポインターを解決する機構です。  
オーバーライド可能な関数や、実行環境によって差し替える関数の呼び出しに向いています。

- `COM_UTIL_SYM_LOADER_ENTRY_INIT`: 静的エントリ初期化
- `com_util_sym_loader_init`: 設定ファイル読み込み
- `com_util_sym_loader_resolve_as`: 型付きで関数ポインター取得
- `com_util_sym_loader_is_default`: 明示的デフォルト設定か確認
- `com_util_sym_loader_info`: 現在状態のダンプ
- `com_util_sym_loader_dispose`: 後始末

## 設計の要点

### module_info

`module_info` は、関数アドレスを手掛かりにして所属モジュールを特定します。

- Linux では `dladdr()` と `realpath()` を使う
- Windows では `GetModuleHandleEx()` と `GetModuleFileNameW()` を使う
- basename 取得時は拡張子を取り除く

そのため、設定ファイル名を `<basename>_extdef.txt` のように組み立てる用途と相性が良い構成です。

### sym_loader

`sym_loader` は、`func_key` ごとの解決情報を `com_util_sym_loader_entry` に保持します。  
初回解決時にライブラリ ロードとシンボル探索を行い、その結果をキャッシュします。

- Linux では `dlopen` / `dlsym`
- Windows では `LoadLibrary` / `GetProcAddress`
- 解決処理は内部ロックで保護される
- 1 度解決した結果はエントリ内へ保持される

## sym_loader の利用手順

### エントリを静的定義する

```c
static com_util_sym_loader_entry sfo_sample_func =
    COM_UTIL_SYM_LOADER_ENTRY_INIT("sample_func", sample_func_t);
```

### エントリ配列を用意する

```c
com_util_sym_loader_entry *const fobj_array[] = {
    &sfo_sample_func,
};
```

### ロード時に設定ファイルを読む

```c
com_util_sym_loader_init(fobj_array, fobj_length, configpath);
```

### 呼び出し時に型付きで解決する

```c
sample_func_t fp = com_util_sym_loader_resolve_as(&sfo_sample_func, sample_func_t);
if (fp != NULL) {
    return fp(a, b, result);
}
```

### アンロード時に解放する

```c
com_util_sym_loader_dispose(fobj_array, fobj_length);
```

## 設定ファイル形式

`com_util_sym_loader_init` が読む設定ファイルは、1 行ごとに `func_key lib_name func_name` を並べる単純な形式です。

```text
# comment
sample_func sample_override sample_func_impl
```

- 行頭 `#` はコメント
- 行中の `#` 以降もコメント扱い
- 3 フィールドそろわない行は無視
- `func_key` が一致したエントリに対して設定が反映される

`lib_name` と `func_name` の両方に `default` を指定した場合は、明示的にデフォルト実装を使う設定として扱われます。

## 使い方

### module_info

共有ライブラリ自身の basename を取得して、設定ファイル名を組み立てる例です。

```c
#include <com_util/runtime/module.h>

char basename[256] = {0};
if (com_util_module_get_basename(basename, sizeof(basename), (const void *)onLoad) == 0) {
    /* basename を使って設定パスを決める */
}
```

### sym_loader

関数を外部実装へ委譲できるようにする基本形です。

```c
#include <com_util/runtime/sym_loader.h>

typedef int (*sample_func_t)(int a, int b, int *result);

static com_util_sym_loader_entry sfo_sample_func =
    COM_UTIL_SYM_LOADER_ENTRY_INIT("sample_func", sample_func_t);

int sample_func(int a, int b, int *result)
{
    sample_func_t fp = com_util_sym_loader_resolve_as(&sfo_sample_func, sample_func_t);
    if (fp != NULL) {
        return fp(a, b, result);
    }

    *result = a + b;
    return 0;
}
```

## プラットフォームごとの動作

### Windows

- `module_info` は Win32 API ベースで DLL パスを取得する
- `process_info` は UAC を使った昇格再起動に対応する
- `sym_loader` は `.dll` を内部で補完してロードする
- ロックは `com_util_local_lock` を使う

### Linux / 非 Windows

- `module_info` は `dladdr()` ベースで `.so` の位置を特定する
- `process_info` は root 権限の確認を行う
- `sym_loader` は `.so` を内部で補完してロードする
- ロックは `com_util_local_lock` を使う

## 注意点

- `com_util_sym_loader_init` と `com_util_sym_loader_dispose` は constructor / destructor や `DllMain` から呼ぶ前提です
- `com_util_sym_loader_dispose` はその前提に合わせてロックを取らずに解放します
- `sym_loader` はライブラリ名に拡張子を含めず設定します
- 解決失敗時は `com_util_sym_loader_resolve_as` が `NULL` を返すため、呼び出し側でデフォルト処理を持つ設計が基本です
- オーバーライド実装側から元の関数を再帰的に呼ぶ構成は避けてください

## 関連ヘッダー

- `com_util/runtime/module.h`: モジュール情報取得
- `com_util/runtime/process.h`: プロセス情報取得
- `com_util/runtime/sym_loader.h`: 動的シンボル解決
