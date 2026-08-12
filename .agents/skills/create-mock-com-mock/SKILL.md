---
name: create-mock-com-mock
description: |
  com_util のモックである mock_com_util を実装・保守するときに使うスキルです。
  libcom_util の動的ロード、既定動作、本物未接続時の扱い、失敗時終了方針をまとめます。
when_to_use: |
  - app/com_util/test 配下で mock_com_util の関数を追加・更新するとき
  - mock_com_util の既定動作や未注入時動作を確認したいとき
---

# mock_com_util 向け real delegate

このスキルは `app/com_util/test/` 配下の `mock_com_util` に適用します。  
通常の app mock 作成ルールとは異なるため、`create-mock` の一般ルールよりこちらを優先します。  
`mock_com_util` は全関数でこの方針を採用します。

## 方針

- `mock_com_util` では全関数について `delegate_real_<func>` を用意します。
- `_mock_com_util == nullptr` のときは全関数で本物の `libcom_util` へ移譲します。
- `Mock_com_util` が注入されていても、全関数の `ON_CALL` 既定動作は `Invoke(delegate_real_<func>)` にします。
- テスト コード側で `EXPECT_CALL` / `ON_CALL` を上書きし、`com_util` 単体および `com_util` を利用するモジュールの単体テストを行うことを基本とします。
- testfw 下では実装がある `delegate_fake` や real と fake の切替 API は作りません。
- 本物の呼び出しは `framework/testfw/include/sharedLibrary.h` の `resolveSharedSymbolOrExit()` を使います。
- ライブラリ探索は `.vscode` で設定された `LD_LIBRARY_PATH` / `PATH` に委ねます。
- attach やシンボル解決に失敗した場合は `stderr` に理由を出し、`exit(1)` で終了します。

## 実装箇所

- 宣言追加: `app/com_util/test/include/mock_com_util.h`
- 既定動作設定: `app/com_util/test/libsrc/mock_com_util/mock_com_util.cc`
- 関数本体: `app/com_util/test/libsrc/mock_com_util/<module>/mock_com_util_<func>.cc`
- 共通ローダー: `framework/testfw/include/sharedLibrary.h`
- 共通ローダー実装: `framework/testfw/libsrc/test_com/sharedLibrary.cc`

## 実装形

- `delegate_real_<func>` は static な関数ポインターを保持し、初回のみ `resolveSharedSymbolOrExit()` で解決します。
- `sharedLibrary.h` で `TESTFW_SHARED_LIBRARY_EXTENSION` を定義し、`mock_com_util.h` では `kLibComUtilName` を `"libcom_util" TESTFW_SHARED_LIBRARY_EXTENSION` で 1 回だけ定義します。
- Windows のライブラリ読み込みは UTF-8 を UTF-16 に変換して `LoadLibraryW` を使います。
- シンボル名は export 名として `GetProcAddress` に渡します。
- 実関数の weak wrapper は、未注入時に `delegate_real_<func>` を呼び、mock 注入時は Mock クラスの指定を優先します。
- Mock クラス側で個別指定がない場合は `ON_CALL` の既定動作により再び `delegate_real_<func>` が使われます。

## v* 系関数 (va_list 版) の ON_CALL 設定

`com_util_vaccess_fmt` などの `v*` 関数は、実際の API では `va_list` を受け取りますが、  
MOCK_METHOD は対応する非 `v` 版と同じシグネチャ (`va_list` なしの整形済み文字列) で定義されています。

```cpp
// 実 API
int com_util_vaccess_fmt(int mode, const char *format, va_list args);

// MOCK_METHOD — va_list なし (整形済み文字列を受け取る)
MOCK_METHOD(int, com_util_vaccess_fmt, (int, const char *));
```

これは、`.cc` ラッパーが `vsnprintf` で事前整形した `buf` を MOCK_METHOD に渡す設計だからです。

この設計により、`delegate_real_com_util_vaccess_fmt` は `(int, const char*, va_list)` を要求するため、  
`Invoke(delegate_real_com_util_vaccess_fmt)` は MOCK_METHOD の型 `int(int, const char*)` と不一致になります。

**ON_CALL には対応する非 `v` 版の `delegate_real_` を使います:**

```cpp
// NG: va_list が必要なため型エラー
ON_CALL(*this, com_util_vaccess_fmt(_, _))
    .WillByDefault(Invoke(delegate_real_com_util_vaccess_fmt));

// OK: 非 v 版は (int, const char*, ...) — 整形済み文字列を "%s" で実関数に渡す
ON_CALL(*this, com_util_vaccess_fmt(_, _))
    .WillByDefault(Invoke(delegate_real_com_util_access_fmt));
```

`delegate_real_com_util_access_fmt` の内部は `real_fn(mode, "%s", format)` で整形済み文字列をそのまま渡すため、  
動作は等価です。

対象関数と対応する非 `v` 版 `delegate_real_` の一覧:

| v* 関数 | ON_CALL で使う delegate_real_ |
|---|---|
| `com_util_vaccess_fmt` | `delegate_real_com_util_access_fmt` |
| `com_util_vopen_fmt` | `delegate_real_com_util_open_fmt` |
| `com_util_vfprintf` | `delegate_real_com_util_fprintf` |
| `com_util_vfopen_fmt` | `delegate_real_com_util_fopen_fmt` |
| `com_util_vmkdir_fmt` | `delegate_real_com_util_mkdir_fmt` |
| `com_util_vremove_fmt` | `delegate_real_com_util_remove_fmt` |
| `com_util_vstat_fmt` | `delegate_real_com_util_stat_fmt` |
| `com_util_vsnprintf` | `delegate_real_com_util_snprintf` |

なお `com_util_vsscanf` は MOCK_METHOD に `va_list` が含まれるため `Invoke(delegate_real_com_util_vsscanf)` を使います。

#### Windows MSVC でのリンク補完 (v* 関数の例)

上記の v* 関数は ON_CALL で非 v* の `delegate_real_` を参照するため、v* 関数自身の obj ファイル (`mock_com_util_v*.obj`) が Windows MSVC のリンカーに取り込まれません。  
`_mock_impl_com_util_v*` が取り込まれなければ `/ALTERNATENAME` も機能せず、本番コードが v* 関数を呼び出すテストで未解決シンボル エラーが発生します。

これは v* 関数に限った現象ではなく、「その関数を Mock 経由でしか使わないテストで、他のどの翻訳単位からも `delegate_real_<func>` 等が参照されない」場合に共通して起こります。次項の `MOCK_COM_UTIL_LINK_IMPL` で機械的に対処します。

## モックの弱参照対応 (MOCK_WEAK_IMPL) と MOCK_COM_UTIL_LINK_IMPL

モックにはデフォルトで全関数を定義しておき、テストで必要なソース ファイルはテストの際に個別に `TEST_SRCS` で与えます。  
この際、Linux, Windows のそれぞれで弱参照を実現する必要があるため、`MOCK_WEAK_IMPL` マクロでプラットフォーム別の弱参照を抽象化しています。

```cpp
MOCK_WEAK_IMPL(com_util_tracer_t *, com_util_tracer_create, void)
{
    // 本体
}
```

`MOCK_WEAK_IMPL` は `framework/testfw/include/testfw.h` に定義されており、`#include <testfw.h>` で利用できます。  
Linux (GCC) では `__attribute__((weak))` により同名の弱シンボルとして定義されますが、Windows (MSVC) では `_mock_impl_<func>` という別名の強シンボルと `/ALTERNATENAME` pragma の組み合わせで実現されます。`/ALTERNATENAME` は、その obj ファイル自体がリンクへ取り込まれて初めて有効になります。

### MOCK_COM_UTIL_LINK_IMPL の網羅義務

`mock_com_util.h` の `#if defined(COMPILER_MSVC)` ブロックにある `MOCK_COM_UTIL_LINK_IMPL(func)` は、`/INCLUDE:_mock_impl_<func>` をリンカーへ渡し、「どこからも参照されない mock 実装 obj」を Windows でも強制的にリンクへ含めます。

**`mock_com_util/<dir>/` 配下に新しい `mock_com_util_<func>.cc` (または既存集約ファイルへの関数追加) を作成したら、同じタイミングで `mock_com_util.h` に `MOCK_COM_UTIL_LINK_IMPL(<func>)` を必ず追加してください。** 追加位置は `mock_com_util/` のディレクトリ構成 (`argparser / clock / compress / console / crt / crypto / net / prompt / runtime / sync / trace / win32`) に沿ったコメント区切りの中に、対応する `.cc` ファイル名の並びで挿入します。Linux 専用の関数 (`com_util_syslog_sink_*` 等、`#if defined(PLATFORM_LINUX)` でのみ公開される関数) は Windows では実体が存在しないため、この Windows 専用ブロックには追加しません。

#### 過去の失敗例: file.c を ADD_SRCS で持ち込む回避策

`mmapTest` (`app/com_util/test/src/libcom_utilTest/mmap/mmapTest`) では、`com_util_file_open` / `com_util_file_get_size` / `com_util_file_set_size` / `com_util_file_close` の 4 関数が `MOCK_COM_UTIL_LINK_IMPL` に未登録だったため、Windows でリンク エラーが発生していました。この際、正規の対処 (該当 4 関数を `MOCK_COM_UTIL_LINK_IMPL` に追加) ではなく、`crt/file.c` (実体) を `ADD_SRCS` へ直接追加して回避する対処が取られました。

この回避策は Windows のリンク エラーを止めましたが、副作用として Linux で `mmapFailureInjectionTest` の失敗注入が効かなくなりました。GNU ld は強シンボルと弱シンボルが同名で競合しても多重定義エラーにはせず、黙って強シンボル (ADD_SRCS で取り込んだ file.c の実体) を優先するため、`EXPECT_CALL(mock_com_util, com_util_file_*)` によるテストダブルへの差し替えが無視され、常に本物の実装が呼ばれてしまったためです。

**教訓**: mock 対象関数のリンク エラーを、実体ソースの `ADD_SRCS`/`TEST_SRCS` 取り込みで回避しないこと。原因は `MOCK_COM_UTIL_LINK_IMPL` の登録漏れであることがほとんどで、対処は登録漏れの関数を `MOCK_COM_UTIL_LINK_IMPL` に追加することです。実体ソースを取り込む回避策は、GCC (Linux) では強シンボルが弱シンボルを静かに上書きするため、Mock を前提にした失敗注入テストを無効化する副作用を伴います。

なお、上記の是正時に `MOCK_COM_UTIL_LINK_IMPL` を監査した結果、`mock_com_util/` 配下で `MOCK_WEAK_IMPL` により実装されている 304 関数のうち 221 関数が未登録であることが判明しました (`com_util_file_*` の 4 関数はその一部)。これは新規関数追加のたびに `MOCK_COM_UTIL_LINK_IMPL` への追加が徹底されていなかったためです。既存分は網羅的に追加済みですが、今後の新規追加でも同じ登録漏れが起きないよう、本項の手順を徹底してください。

## テスト

- 本物への移譲確認は `app/com_util/test/src/libcom_utilTest/` 配下のテストで行います。
- デザイン パターン化されているため、未注入時の動作、`Mock_com_util` 注入済み既定動作、`EXPECT_CALL` / `ON_CALL` による上書き、必要に応じてローダー失敗診断は `app/com_util/test/src/libcom_utilTest/compress/compressDelegateRealTest` の代表実施とします。  
  この仕組みが正しく動作していない場合は、各テストが期待通り動作しなくなることでじゅうぶんに確認できます。
