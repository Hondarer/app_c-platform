# argparser 明示ハンドル API ガイド (内部用)

本書は、`com_util_argparser` の明示ハンドル API (`com_util_argparser_handle_*`) の利用方法をまとめます。  
明示ハンドル API は、複数のパーサー インスタンスを同時に扱う必要がある場合、主にテストでの独立性検証のために使用します。

通常のアプリケーションでは、プロセス共有のパーサーを暗黙に使用する API (`com_util_argparser_*`) を使用してください。  
シングルトン API のユース ケース別の使い方は [README.md](README.md) を参照してください。  
オプションの登録、解析、エラー詳細の取得といった機能面の挙動は両 API で共通であり、本書では明示ハンドル API に固有の事項を中心に説明します。

## ハンドルの取得方法

明示ハンドル API では、独立したハンドルを生成して使用します。

- `com_util_argparser_handle_create()` / `com_util_argparser_handle_dispose()`: 独立したハンドルを明示的に生成、解放します。  
  プロセス終了時の自動解放を行わないため、生成したハンドルは必ず `com_util_argparser_handle_dispose()` で解放してください。

`com_util_argparser_handle_create()` は、解析対象の `argc` と `argv` に続けて、生成オプション `com_util_argparser_options` (usage に表示するプログラム名と説明文) を受け取ります。  
生成オプションが既定設定でよい場合は NULL を渡します。  
ハンドルは `argv` を複製せずポインターを保持するため、ハンドルを使用する間は `argv` を有効なまま維持してください。  
`argv[0]` は、生成オプションでプログラム名を指定しない場合の usage 上のプログラム名になります。  

## 基本フロー (生成と解放)

```c
#include <com_util/argparser/argparser.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    com_util_argparser *parser = com_util_argparser_handle_create(argc, argv, NULL);

    int count = 1; /* 既定値は解析前に設定する */
    const char *input = NULL;

    com_util_argparser_handle_register_option_int(parser, "-c", "--count", "N", "繰り返し回数", 0, &count);
    com_util_argparser_handle_register_positional_string(parser, "input", "入力ファイル", COM_UTIL_ARGPARSER_REQUIRED,
                                                   &input);

    if (com_util_argparser_handle_parse(parser) != COM_UTIL_OK)
    {
        char message[256];
        com_util_argparser_handle_get_error_message(parser, message, sizeof(message));
        fprintf(stderr, "error: %s\n", message);
        com_util_argparser_handle_dispose(parser);
        return 1;
    }

    printf("count=%d input=%s\n", count, input);

    com_util_argparser_handle_dispose(parser);
    return 0;
}
```

## 登録エラーの確認 (戻り値による個別確認)

暗黙パーサー版の register 系 (`com_util_argparser_register_*()`) は結果コードを戻り値として返し、エラーを内部にも記録します。  
明示ハンドル API の `com_util_argparser_handle_register_*()` も結果コードを戻り値として返します。  
呼び出しごとに成否を確認できるため、テストで特定の登録が失敗することを検証する場合に使用します。

```c
int result = com_util_argparser_handle_register_flag(parser, "-v", "--verbose", "詳細出力を有効にする", &verbose);

if (result != COM_UTIL_OK)
{
    /* COM_UTIL_ERR_INVALID_ARGUMENT / COM_UTIL_ERR_DUPLICATE_DEFINITION / COM_UTIL_ERR_OUT_OF_MEMORY のいずれか */
}
```

戻り値を都度確認しない場合でも、エラーは内部に記録されます。  
すべての登録を終えた後に `com_util_argparser_handle_get_register_error_count()` でまとめて判定し、`com_util_argparser_handle_get_register_error()` 系 API または `com_util_argparser_handle_print_register_error_messages()` で詳細を取得できます。

## 解析エラー詳細の取得

`com_util_argparser_handle_parse()` が解析エラーのコードを返した場合、詳細は次の API で取得します。

- `com_util_argparser_handle_get_error()`: エラー種別 (`int`)
- `com_util_argparser_handle_get_error_target()`: エラーの対象名 (オプション名や位置引数名)
- `com_util_argparser_handle_get_error_index()`: エラーを起こした argv のインデックス (該当なしは -1)
- `com_util_argparser_handle_get_error_message()`: 人間可読の 1 行メッセージを呼び出し側バッファーへ組み立てる

`com_util_argparser_handle_get_error_target()` が返す文字列はハンドルが所有し、次回の `com_util_argparser_handle_parse()` または `com_util_argparser_handle_dispose()` まで有効です。

定型的な表示には `com_util_argparser_handle_print_error_messages()` と `com_util_argparser_handle_print_usage()` の組み合わせが使用できます。

```c
if (com_util_argparser_handle_parse(parser) != COM_UTIL_OK)
{
    com_util_argparser_handle_print_error_messages(parser, stderr);
    com_util_argparser_handle_print_usage(parser, stderr);

    com_util_argparser_handle_dispose(parser);
    return 1;
}
```

## usage 文字列の必要サイズ問い合わせ

`com_util_argparser_handle_get_usage()` は、usage を文字列として自前のバッファーへ組み立てます。  
バッファーが不足する場合は `COM_UTIL_ERR_BUFFER_TOO_SMALL` が返り、バッファーには切り詰めた内容が格納されます。  
必要なバイト数を事前に知りたい場合は、`buffer` に `NULL` を渡して `required_size` だけを問い合わせられます。

```c
size_t required_size = 0;

com_util_argparser_handle_get_usage(parser, NULL, 0, &required_size);
/* required_size バイト分のバッファーを確保してから再度呼び出す */
```

## 再解析と引数の差し替え

同じパーサー ハンドルで `com_util_argparser_handle_parse()` を複数回呼び出すことができます。  
解析対象は生成時に受け取った `argc` と `argv` であるため、いずれの呼び出しも同じコマンド ラインを解析し直します。

呼び出しの開始時に、フラグの格納先を 0 に、複数値オプションの出現数を 0 に初期化し、前回のエラー状態をクリアします。  
値付きオプションと位置引数の格納先は出現時のみ上書きされるため、2 回目の解析前に必要であれば呼び出し側で既定値を設定し直してください。

異なるコマンド ラインを検証する場合は、ハンドルを生成し直します。  
テストで複数のコマンド ラインを検証する場合がこれに当たります。

```c
com_util_argparser *parser1 = com_util_argparser_handle_create(argc1, argv1, NULL);
register_options(parser1); /* オプションを登録する */
com_util_argparser_handle_parse(parser1);
/* ... 1 回目の結果を利用 ... */
com_util_argparser_handle_dispose(parser1);

com_util_argparser *parser2 = com_util_argparser_handle_create(argc2, argv2, NULL);
register_options(parser2); /* 新しいハンドルへ登録し直す */
com_util_argparser_handle_parse(parser2);
/* ... 2 回目の結果を利用 ... */
com_util_argparser_handle_dispose(parser2);
```

## スレッド セーフ性

- `com_util_argparser_handle_create()` はスレッド セーフです。各呼び出しは独立したハンドルを生成します。
- `com_util_argparser_init()` の初回生成と再初期化は、内部ロックによりスレッド セーフです。
- 取得後のハンドルへの登録、解析呼び出しはスレッド セーフではありません。同一ハンドルへの並行呼び出しは未定義動作です。ハンドルごとに 1 スレッドから使用してください。
- `com_util_argparser_handle_dispose()` の呼び出し時は、解放対象のハンドルを他スレッドが使用していないことを呼び出し側で保証してください。
