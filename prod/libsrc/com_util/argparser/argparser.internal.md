# argparser 明示ハンドル API ガイド (内部用)

本書は、`com_util_argparser` の明示ハンドル API (`_com_util_argparser_*`) の利用方法をまとめます。  
明示ハンドル API は、複数のパーサー インスタンスを同時に扱う必要がある場合、主にテストでの独立性検証のために使用します。

通常のアプリケーションでは、プロセス共有のデフォルト パーサーを暗黙に使用する API (`com_util_argparser_*`) を使用してください。  
シングルトン API のユース ケース別の使い方は [README.md](README.md) を参照してください。  
オプションの登録、解析、エラー詳細の取得といった機能面の挙動は両 API で共通であり、本書では明示ハンドル API に固有の事項を中心に説明します。

## ハンドルの取得方法

ハンドルの取得方法は 2 通りあります。

- `_com_util_argparser_create()` / `_com_util_argparser_dispose()`: 独立したハンドルを明示的に生成、解放します。  
  プロセス終了時の自動解放を行わないため、生成したハンドルは必ず `_com_util_argparser_dispose()` で解放してください。
- `_com_util_argparser_default()`: プロセス共有のデフォルト パーサー ハンドルを取得します。  
  シングルトン API (`com_util_argparser_*`) が内部で使用しているハンドルと同一の実体です。  
  ライブラリが所有しプロセス正常終了時に自動解放されるため、返却されたハンドルを `_com_util_argparser_dispose()` に渡さないでください (渡した場合は何も行いません)。

`_com_util_argparser_create()` と `_com_util_argparser_default()` は、生成オプション `com_util_argparser_options` (usage に表示するプログラム名と説明文) を受け取れます。  
既定設定でよい場合は NULL を渡します。  
`_com_util_argparser_default()` の生成オプションは初回呼び出し時のみ有効で、以降の呼び出しでは無視されます。

## 基本フロー (生成と解放)

```c
#include <com_util/argparser/argparser.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    com_util_argparser *parser = _com_util_argparser_create(NULL);

    int count = 1; /* 既定値は解析前に設定する */
    const char *input = NULL;

    _com_util_argparser_register_option_int(parser, "-c", "--count", "N", "繰り返し回数", 0, &count);
    _com_util_argparser_register_positional_string(parser, "input", "入力ファイル", COM_UTIL_ARGPARSER_REQUIRED,
                                                   &input);

    if (_com_util_argparser_parse(parser, argc, argv) != COM_UTIL_OK)
    {
        char message[256];
        _com_util_argparser_get_error_message(parser, message, sizeof(message));
        fprintf(stderr, "error: %s\n", message);
        _com_util_argparser_dispose(parser);
        return 1;
    }

    printf("count=%d input=%s\n", count, input);

    _com_util_argparser_dispose(parser);
    return 0;
}
```

## 登録エラーの確認 (戻り値による個別確認)

シングルトン API の register 系が結果コードを内部に記録するだけなのに対し、明示ハンドル API の `_com_util_argparser_register_*()` は結果コードを戻り値として返します。  
呼び出しごとに成否を確認できるため、テストで特定の登録が失敗することを検証する場合に使用します。

```c
int result = _com_util_argparser_register_flag(parser, "-v", "--verbose", "詳細出力を有効にする", &verbose);

if (result != COM_UTIL_OK)
{
    /* COM_UTIL_ERR_INVALID_ARGUMENT / COM_UTIL_ERR_DUPLICATE_DEFINITION / COM_UTIL_ERR_OUT_OF_MEMORY のいずれか */
}
```

戻り値を都度確認しない場合でも、エラーは内部に記録されます。  
すべての登録を終えた後に `_com_util_argparser_get_register_error_count()` でまとめて判定し、`_com_util_argparser_get_register_error()` 系 API または `_com_util_argparser_print_register_error_messages()` で詳細を取得できます。

## 解析エラー詳細の取得

`_com_util_argparser_parse()` が `COM_UTIL_ERR_PARSE` を返した場合、詳細は次の API で取得します。

- `_com_util_argparser_get_error()`: エラー種別 (`int`)
- `_com_util_argparser_get_error_target()`: エラーの対象名 (オプション名や位置引数名)
- `_com_util_argparser_get_error_index()`: エラーを起こした argv のインデックス (該当なしは -1)
- `_com_util_argparser_get_error_message()`: 人間可読の 1 行メッセージを呼び出し側バッファーへ組み立てる

`_com_util_argparser_get_error_target()` が返す文字列はハンドルが所有し、次回の `_com_util_argparser_parse()` または `_com_util_argparser_dispose()` まで有効です。

定型的な表示には `_com_util_argparser_print_error_messages()` と `_com_util_argparser_print_usage()` の組み合わせが使用できます。

```c
if (_com_util_argparser_parse(parser, argc, argv) != COM_UTIL_OK)
{
    _com_util_argparser_print_error_messages(parser, stderr);
    _com_util_argparser_print_usage(parser, stderr);

    _com_util_argparser_dispose(parser);
    return 1;
}
```

## usage 文字列の必要サイズ問い合わせ

`_com_util_argparser_get_usage()` は、usage を文字列として自前のバッファーへ組み立てます。  
バッファーが不足する場合は `COM_UTIL_ERR_BUFFER_TOO_SMALL` が返り、バッファーには切り詰めた内容が格納されます。  
必要なバイト数を事前に知りたい場合は、`buffer` に `NULL` を渡して `required_size` だけを問い合わせられます。

```c
size_t required_size = 0;

_com_util_argparser_get_usage(parser, NULL, 0, &required_size);
/* required_size バイト分のバッファーを確保してから再度呼び出す */
```

## 同一ハンドルでの再解析

同じパーサー ハンドルで `_com_util_argparser_parse()` を複数回呼び出すことができます。  
テストで複数のコマンド ラインを検証する場合に使えます。

```c
_com_util_argparser_parse(parser, argc1, argv1);
/* ... 1 回目の結果を利用 ... */

_com_util_argparser_parse(parser, argc2, argv2);
/* ... 2 回目の結果を利用 (フラグと複数値オプションの出現数は自動的にリセットされる) ... */
```

呼び出しの開始時に、フラグの格納先を 0 に、複数値オプションの出現数を 0 に初期化し、前回のエラー状態をクリアします。  
値付きオプションと位置引数の格納先は出現時のみ上書きされるため、2 回目の解析前に必要であれば呼び出し側で既定値を設定し直してください。

## スレッド セーフ性

- `_com_util_argparser_create()` はスレッド セーフです。各呼び出しは独立したハンドルを生成します。
- `_com_util_argparser_default()` の初回生成と同一ハンドルの取得は、内部ロックによりスレッド セーフです。
- 取得後のハンドルへの登録、解析呼び出しはスレッド セーフではありません。同一ハンドルへの並行呼び出しは未定義動作です。ハンドルごとに 1 スレッドから使用してください。
- `_com_util_argparser_dispose()` の呼び出し時は、解放対象のハンドルを他スレッドが使用していないことを呼び出し側で保証してください。
