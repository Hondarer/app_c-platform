# リンク方式の規約

## 概要

`com_util` は `LIB_TYPE = both` で共有ライブラリと静的ライブラリの両方を生成します。

| 成果物 | Linux | Windows |
|---|---|---|
| 共有ライブラリ | `libcom_util.so` | `com_util.dll` / `com_util.lib` (import library) |
| 静的ライブラリ | `libcom_util_static.a` | `com_util_static.lib` |

どちらを選ぶかは利用側の裁量ではなく、本文書の規約に従います。  
`com_util` はプロセス グローバルな状態を持つため、リンク方式を誤ると単一プロセス内に状態が複数生成され、原因の特定が困難な不具合につながります。

## 基本ルール

### 静的リンクは末端の実行可能ファイルに限る

**静的リンク (`com_util_static`) が許されるのは、他の `com_util` 利用共有ライブラリをロードしない末端の実行可能ファイルだけです。**

共有ライブラリ (`LIB_TYPE = shared` または `both`) からは、必ず共有ライブラリ版へ動的リンクしてください。

### 同一プロセス内で静的版と動的版を同居させない

同一プロセスが `com_util` の静的版と動的版の両方を抱えると、`com_util` の状態が 2 セット生成されます。

Linux では、実行可能ファイルは既定でシンボルをエクスポートしません (`-rdynamic` を指定しない限り)。  
このため共有ライブラリ側の `com_util` 呼び出しが実行可能ファイル内の静的コピーへ解決されることはなく、シンボル インターポジションによる救済は期待できません。  
静的版と動的版は独立した状態を持ったまま並存します。

分裂する状態には次のものがあります。

| 状態 | 定義箇所 | 分裂したときの症状 |
|---|---|---|
| トレース レジストリ | `trace/tracer.c` の `s_trace_registry` | トレース プロバイダーの登録が分裂し、一方の登録が他方から見えない |
| トレース参照カウント | `trace/tracer.c` の `s_trace_ref` | 参照カウントが二重管理になり、解放のタイミングが狂う |
| 既定パーサー | `argparser/argparser.c` の `s_default_parser` | 既定ハンドルが 2 つ生成される |
| コンソール初期状態 | `console/console.c` の `s_orig_output_cp` ほか | Windows のコード ページとコンソール モードの復元が二重に走る |
| スレッド ローカル記憶域 | `THREAD_LOCAL` を伴う変数 | スレッドごとの状態が実体ごとに分かれ、記録した側と参照する側が食い違う |

いずれもコンパイル時にもリンク時にも検出されず、実行時に不可解な挙動として現れます。

### 動的シンボル解決を追加するときはリンク方式を見直す

静的リンクしている実行可能ファイルへプラグイン機構や `dlopen` / `LoadLibrary` による動的ロードを追加する場合は、ロード対象が `com_util` を利用する可能性を検討し、該当するなら動的リンクへ切り替えてください。

`com_util` 自身が `com_util/runtime/sym_loader.h` で動的シンボル解決を提供しており、`app/override-sample` がその利用例です。  
実行時ロードは、この規約に抵触しやすい機能です。

## 静的リンクが妥当な典型

末端の実行可能ファイルのうち、次のものは静的リンクが妥当です。  
いずれも「ライブラリ探索パスに依存せず単一ファイルで完結させたい」という配布上の要求が動機です。

### 単一ファイルで配布する CLI ツール

`com_util` に同梱する CLI ツールや、単体で配布・実行するサンプルの実行可能ファイルが該当します。  
ライブラリ探索パスを設定しないまま実行できることが利点です。

### OS のサービスとして登録・起動される実行可能ファイル

systemd や Windows SCM に登録され、サービスとして起動される実行可能ファイルが該当します。

**systemd から起動されるプロセスは、開発者のシェル環境の `LD_LIBRARY_PATH` を継承しません。**

このため動的リンクでは共有ライブラリを解決できず、次のいずれかが必要になります。いずれも配布形態としての制約が大きくなります。

- unit ファイルへ `Environment=LD_LIBRARY_PATH=...` をビルド ツリーのパス込みで埋め込む
- 共有ライブラリをシステムのライブラリ ディレクトリへインストールする
- `-Wl,-rpath,$ORIGIN/../lib` を付与し、実行可能ファイルとライブラリの相対配置を固定する

静的リンクはこれらを回避する選択です。  
ただし、サービスがプラグインを動的ロードする設計になった時点で本規約に抵触するため、そのときは動的リンクへ切り替えたうえで探索パスの解決方法を選び直してください。

## makepart.mk の記述

静的リンクする場合は `LIBS += com_util_static` を指定します。  
Windows では、`LIB_TYPE = both` の静的ライブラリにも `dllexport` 付きのシンボルが含まれるため、リンク時に `.exp` と import library が生成されます。これを抑止する指定もあわせて記述します。

```make
# ライブラリの指定 (static library を利用)
# 本実行可能ファイルは他の com_util 利用共有ライブラリをロードしないため、静的リンクとする。
# see: app/com_util/docs/link-policy.md
LIBS += com_util_static

ifdef PLATFORM_WINDOWS
    CFLAGS   += /DCOM_UTIL_STATIC
    CXXFLAGS += /DCOM_UTIL_STATIC
    # libcom_util は both 生成で、static 側にも dllexport 付きシンボルを含む。
    # そのまま exe をリンクすると .exp と import lib も生成されるため、抑止する。
    LDFLAGS  += /NOEXP /NOIMPLIB
endif
```

`COM_UTIL_STATIC` は Windows で `COM_UTIL_EXPORT` を空へ展開するためのマクロです。  
静的リンクする翻訳単位すべてに定義する必要があるため、テスト側の `makepart.mk` にも同じ定義が必要になる場合があります。

共有ライブラリ版 (`LIBS += com_util`) は `libcjson.so` または `libcjson.dll` に動的リンクするため、利用側での `cjson` の追加リンクは不要です。ただし、実行時には `com_util` と `cjson` の両方をライブラリ探索パスから解決できるようにしてください。

動的リンクする場合は `LIBS += com_util` を指定し、`COM_UTIL_STATIC` は定義しません。

## 確認方法

Linux では `ldd` で、実行可能ファイルおよび共有ライブラリが `libcom_util.so` を参照しているかを確認できます。

```bash
# com_util を利用する共有ライブラリが動的リンクになっていること
ldd <利用側>/prod/lib/lib<name>.so | grep com_util

# 静的リンクの実行可能ファイルが libcom_util.so を参照していないこと
ldd <利用側>/prod/cbin/<name> | grep com_util
```

同一プロセスで両方が現れる構成になっていないかを、依存関係の追加時に確認してください。

## 関連ドキュメント

- [コーディング規範 (特化事項)](coding-guideline.md)
- [プラットフォーム抽象化ガイドライン](platform-abstraction-guideline.md)
- [makepart.mk の記述](../../../framework/makefw/docs/makeparts.md)
