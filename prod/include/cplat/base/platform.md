# platform - OS / アーキテクチャー差異の吸収

`platform.h` は、ビルド対象の OS と CPU アーキテクチャーを共通マクロへ正規化するための基盤ヘッダーです。  
Windows と Linux、x86 と x64 のような環境差異を、呼び出し側が処理系依存マクロに直接触れずに扱えるようにします。

## 目的

クロスプラットフォームの C コードでは、OS やアーキテクチャーによって次のような差異が発生します。

- 利用するシステム API
- パス区切りや共有ライブラリの扱い
- 低レベル最適化やバイナリ互換性に関わる分岐

こうした条件を `_WIN32` や `__x86_64__` で直接書き始めると、機能コードの意図が見えにくくなります。  
`platform.h` は、環境判定を 1 か所にまとめ、業務ロジック側を読みやすく保つための仕組みです。

## 代表マクロ

### プラットフォーム判定

- `PLATFORM_WINDOWS`
- `PLATFORM_LINUX`
- `PLATFORM_UNKNOWN`
- `PLATFORM_NAME`

OS ごとの API 差異や動作差異を切り替えるときに使います。

### アーキテクチャー判定

- `ARCH_X64`
- `ARCH_X86`
- `ARCH_UNKNOWN`
- `ARCH_NAME`

アーキテクチャーに依存した処理を切り替えるときに使います。

## 設計の考え方

`platform.h` は、OS 判定とアーキテクチャー判定を分けて扱います。  
Windows か Linux かという違いと、x86 か x64 かという違いは別の軸であり、両方を独立して扱える方が拡張しやすいためです。

また、このヘッダーは `compiler.h` を取り込み、環境差異を扱う基盤ヘッダー群として並べて使えるようにしています。  
コンパイラ差異は `compiler.h`、OS / CPU 差異は `platform.h` へ寄せる、と整理すると分岐の責務が明確になります。

## 使い方

### OS ごとに処理を分ける

```c
#include <cplat/base/platform.h>

void init_platform_layer(void)
{
#if defined(PLATFORM_LINUX)
    /* Linux 向け初期化 */
#elif defined(PLATFORM_WINDOWS)
    /* Windows 向け初期化 */
#else
    /* 未対応環境向けの保守的な処理 */
#endif
}
```

### アーキテクチャーごとに処理を分ける

```c
#include <cplat/base/platform.h>

void select_code_path(void)
{
#if defined(ARCH_X64)
    /* x64 向け処理 */
#elif defined(ARCH_X86)
    /* x86 向け処理 */
#else
    /* 汎用処理 */
#endif
}
```

### 診断情報へ環境名を出す

```c
#include <cplat/base/platform.h>
#include <stdio.h>

void print_runtime_info(void)
{
    printf("platform=%s arch=%s\n", PLATFORM_NAME, ARCH_NAME);
}
```

## 利用の目安

このヘッダーは、次のような場面で先に参照すると整理しやすくなります。

- OS ごとに呼ぶ API を切り替えたい
- ビルド ログや診断に対象環境名を出したい
- アーキテクチャー依存の分岐を局所化したい

一方で、インライン属性やコンパイラ固有拡張を扱う場合は `compiler.h` を使います。

## 関連ヘッダー

- `compiler.h`: コンパイラ差異と属性差異の吸収
