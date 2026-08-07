---
short-title: "crt"
---

# crt - C ランタイム抽象レイヤー

`crt` は、標準 C / CRT API のプラットフォーム差異を吸収するためのレイヤーです。  
標準入口は `com_util/crt/crt.h` です。  
公開ヘッダーは標準ヘッダー名と対応するように `com_util/crt/*.h` に分割しており、  
必要に応じて個別に取り込むこともできます。

## 目的

Linux と Windows では、ファイル パス、セキュア関数、ワイド文字 API、時刻変換などの扱いが異なります。  
このモジュールは、呼び出し側が同じ `com_util_*` API を使えるように、その差異を集約します。

- UTF-8 パスを Windows ではワイド文字 API に変換する
- `stdio.h`、`sys/stat.h`、`fcntl.h`、`unistd.h` 相当の API を分けて提供する
- Windows の `HANDLE` と Linux の `fd` を薄い低レベル I/O API で共通化する
- `string.h` / `time.h` まわりの `_s` 系差異を薄いラッパーで扱う
- printf 形式のパス生成付き API を対応する CRT ヘッダーに配置する

## 公開ヘッダー

- `com_util/crt/crt.h`: `crt` 配下の公開ヘッダーをまとめて取り込む標準入口
- `com_util/crt/stdio.h`: `FILE *` 操作、`remove`、`rename`、`fopen_fmt`、`remove_fmt`、切り詰めを検出する `snprintf` と `fgets`
- `com_util/crt/sys/stat.h`: `stat`、`mkdir`、`stat_fmt`、`mkdir_fmt`
- `com_util/crt/fcntl.h`: `open`、`open_fmt`
- `com_util/crt/file.h`: 書き込み用低レベル ファイル ハンドル、`open`、`write`、`size`、`close`
- `com_util/crt/unistd.h`: `access`、`access_fmt`、`COM_UTIL_ACCESS_FMT_*`
- `com_util/crt/string.h`: 安全な文字列コピー/連結、再入可能な `strtok_r`、`sscanf` 抽象
- `com_util/crt/stdlib.h`: 環境変数の取得と設定、完全消費を検査する文字列から数値への変換
- `com_util/crt/time.h`: UTC / local 時刻変換
- `com_util/crt/path.h`: `PLATFORM_PATH_MAX`

## 実装方針

基礎 API とフォーマット付き API は別の実装ファイルに分けます。  
これは、フォーマット付き API のテストで下位の `com_util_fopen` や `com_util_stat` をモックできるようにするためです。

`*_fmt` API は、内部で `vsnprintf` によりパスを `PLATFORM_PATH_MAX` のバッファーへ展開し、対応する基礎 API を呼び出します。

## 使用例

```c
#include <com_util/crt/crt.h>

FILE *fp = com_util_fopen_fmt("w", NULL, "./logs/app_%03d.txt", 7);
if (fp != NULL)
{
    (void)com_util_fprintf(fp, "hello\n");
    (void)com_util_fclose(fp, NULL);
}
```

```c
#include <com_util/crt/crt.h>

if (com_util_access_fmt(COM_UTIL_ACCESS_FMT_F_OK, "./logs/app_%03d.txt", 7) != 0)
{
    (void)com_util_mkdir_fmt("./logs");
}
```
