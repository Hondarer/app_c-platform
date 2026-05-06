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
- テストコード側で `EXPECT_CALL` / `ON_CALL` を上書きし、`com_util` 単体および `com_util` を利用するモジュールの単体テストを行うことを基本とします。
- testfw 下では実装がある `delegate_fake` や real と fake の切替 API は作りません。
- 本物の呼び出しは `framework/testfw/include/sharedLibrary.h` の `resolveSharedSymbolOrExit()` を使います。
- ライブラリ探索は `.vscode` で設定された `LD_LIBRARY_PATH` / `PATH` に委ねます。
- attach やシンボル解決に失敗した場合は `stderr` に理由を出し、`exit(1)` で終了します。

## 実装箇所

- 宣言追加: `app/com_util/test/include/mock_com_util.h`
- 既定動作設定: `app/com_util/test/libsrc/mock_com_util/mock_com_util.cc`
- 関数本体: `app/com_util/test/libsrc/mock_com_util/<module>/mock_com_util_<func>.cc`
- 共通ローダ: `framework/testfw/include/sharedLibrary.h`
- 共通ローダ実装: `framework/testfw/libsrc/test_com/sharedLibrary.cc`

## 実装形

- `delegate_real_<func>` は static な関数ポインタを保持し、初回のみ `resolveSharedSymbolOrExit()` で解決します。
- `sharedLibrary.h` で `TESTFW_SHARED_LIBRARY_EXTENSION` を定義し、`mock_com_util.h` では `kLibComUtilName` を `"libcom_util" TESTFW_SHARED_LIBRARY_EXTENSION` で 1 回だけ定義します。
- Windows のライブラリ読み込みは UTF-8 を UTF-16 に変換して `LoadLibraryW` を使います。
- シンボル名は export 名として `GetProcAddress` に渡します。
- 実関数の weak wrapper は、未注入時に `delegate_real_<func>` を呼び、mock 注入時は Mock クラスの指定を優先します。
- Mock クラス側で個別指定がない場合は `ON_CALL` の既定動作により再び `delegate_real_<func>` が使われます。

## `v*` 系関数（va_list 版）の ON_CALL 設定

`com_util_vaccess_fmt` などの `v*` 関数は、実際の API では `va_list` を受け取りますが、
MOCK_METHOD は対応する非 `v` 版と同じシグネチャ（`va_list` なしの整形済み文字列）で定義されています。

```cpp
// 実 API
int com_util_vaccess_fmt(int mode, const char *format, va_list args);

// MOCK_METHOD — va_list なし（整形済み文字列を受け取る）
MOCK_METHOD(int, com_util_vaccess_fmt, (int, const char *));
```

これは、`.cc` ラッパーが `vsnprintf` で事前整形した `buf` を MOCK_METHOD に渡す設計だからです。

この設計により、`delegate_real_com_util_vaccess_fmt` は `(int, const char*, va_list)` を要求するため、
`Invoke(delegate_real_com_util_vaccess_fmt)` は MOCK_METHOD の型 `int(int, const char*)` と不一致になります。

**ON_CALL には対応する非 `v` 版の `delegate_real_` を使います：**

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

対象関数と対応する非 `v` 版 `delegate_real_` の一覧：

| v* 関数 | ON_CALL で使う delegate_real_ |
|---|---|
| `com_util_vaccess_fmt` | `delegate_real_com_util_access_fmt` |
| `com_util_vopen_fmt` | `delegate_real_com_util_open_fmt` |
| `com_util_vfprintf` | `delegate_real_com_util_fprintf` |
| `com_util_vfopen_fmt` | `delegate_real_com_util_fopen_fmt` |
| `com_util_vmkdir_fmt` | `delegate_real_com_util_mkdir_fmt` |
| `com_util_vremove_fmt` | `delegate_real_com_util_remove_fmt` |
| `com_util_vstat_fmt` | `delegate_real_com_util_stat_fmt` |

なお `com_util_vsscanf` は MOCK_METHOD に `va_list` が含まれるため `Invoke(delegate_real_com_util_vsscanf)` を使います。

#### Windows MSVC でのリンク補完

上記の v* 関数は ON_CALL で非 v* の `delegate_real_` を参照するため、v* 関数自身の obj ファイル (`mock_com_util_v*.obj`) が Windows MSVC のリンカーに取り込まれません。
`_mock_impl_com_util_v*` が取り込まれなければ `/ALTERNATENAME` も機能せず、本番コードが v* 関数を呼び出すテストで未解決シンボルエラーが発生します。

**リンク補完の配置先：**
`/INCLUDE` pragma は `mock_com_util.h` ヘッダーに記述します。

```cpp
// mock_com_util.h
#ifndef MOCK_UTIL_H
#define MOCK_UTIL_H

#include <com_util/base/platform.h>
#include <testfw.h>
// ... その他のインクルード

#if defined(COMPILER_MSVC)
#pragma comment(linker, "/INCLUDE:_mock_impl_com_util_vfprintf")
#pragma comment(linker, "/INCLUDE:_mock_impl_com_util_vfopen_fmt")
#pragma comment(linker, "/INCLUDE:_mock_impl_com_util_vaccess_fmt")
#pragma comment(linker, "/INCLUDE:_mock_impl_com_util_vopen_fmt")
#pragma comment(linker, "/INCLUDE:_mock_impl_com_util_vremove_fmt")
#pragma comment(linker, "/INCLUDE:_mock_impl_com_util_vmkdir_fmt")
#pragma comment(linker, "/INCLUDE:_mock_impl_com_util_vstat_fmt")
#endif /* COMPILER_MSVC */

class Mock_com_util
{
    // ...
};
```

これにより、テストコードが `mock_com_util.h` をインクルードする時点でリンカーへの指示が確定し、
実ファイルから v* 関数が呼び出される統合テストや実装テストでもリンク成功が保証されます。

## モックの弱参照対応 (`MOCK_WEAK_IMPL`)

モックにはデフォルトで全関数を定義しておき、テストで必要なソースファイルはテストの際に個別に `TEST_SRCS` で与えます。
この際、Linux, Windows のそれぞれで弱参照を実現する必要があるため、`MOCK_WEAK_IMPL` マクロでプラットフォーム別の弱参照を抽象化しています。

```cpp
MOCK_WEAK_IMPL(com_util_tracer_t *, com_util_tracer_create, void)
{
    // 本体
}
```

`MOCK_WEAK_IMPL` は `framework/testfw/include/testfw.h` に定義されており、`#include <testfw.h>` で利用できます。

## テスト

- 本物への移譲確認は `app/com_util/test/src/libcom_utilTest/` 配下のテストで行います。
- デザインパターン化されているため、未注入時の動作、`Mock_com_util` 注入済み既定動作、`EXPECT_CALL` / `ON_CALL` による上書き、必要に応じてローダ失敗診断は `app/com_util/test/src/libcom_utilTest/compress/compressDelegateRealTest` の代表実施とします。
  この仕組みが正しく動作していない場合は、各テストが期待通り動作しなくなることでじゅうぶんに確認できます。
