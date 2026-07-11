# com_util_argparser 利用ガイド

`com_util_argparser` は、コマンド ライン引数 (argc / argv) を解析する汎用パーサーです。
フラグ、値付きオプション、位置引数を事前に登録し、解析結果を登録時に指定した格納先へ書き込みます。

対応する構文は次のとおりです。

- フラグ: `-v` / `--verbose` (出現回数を格納)
- 値付きオプション: `-o value` / `--option value` / `--option=value`
- 複数回指定できる値付きオプション: 上記構文の繰り返し (出現順に配列へ格納)
- 位置引数: 登録順に割り当て

次の構文は対応していません。

- 短オプションの連結 (`-abc`)
- `--` 区切り以降を無条件で位置引数扱いにする慣習

本 API はエラーを標準出力・標準エラーへ出力しません。
解析エラーの詳細は `com_util_argparser_get_error()` 系の API で取得し、表示は呼び出し側で行います。

宣言は `com_util/argparser/argparser.h` にあります。
API の詳細な引数説明は同ヘッダーの Doxygen コメントを参照してください。
本書ではユース ケース別の使い方をまとめます。

## 基本フロー

パーサーの利用は、生成、登録、解析、参照、解放という一連の流れになります。
ハンドルの取得方法は用途に応じて 2 通りあります。

- 通常のアプリケーション: `com_util_argparser_default()` でプロセス共有のハンドルを取得します。
  `com_util_console` と同様にプロセス正常終了時に自動解放されるため、返却されたハンドルを `com_util_argparser_dispose()` に渡さないでください。
- 複数インスタンスを同時に扱う場合 (主にテスト): `com_util_argparser_create()` / `com_util_argparser_dispose()` の対で明示的に管理します。
  こちらはプロセス終了時の自動解放を行わないため、生成したハンドルは必ず `com_util_argparser_dispose()` で解放してください。

### デフォルト インスタンスを使う場合 (通常のアプリケーション)

```c
#include <com_util/argparser/argparser.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    com_util_argparser *parser = com_util_argparser_default(NULL);

    int count = 1; /* 既定値は解析前に設定する */
    const char *input = NULL;

    com_util_argparser_register_option_int(parser, "-c", "--count", "N", "繰り返し回数", 0, &count);
    com_util_argparser_register_positional_string(parser, "input", "入力ファイル", COM_UTIL_ARGPARSER_REQUIRED,
                                                &input);

    if (com_util_argparser_parse(parser, argc, argv) != COM_UTIL_ARGPARSER_OK)
    {
        char message[256];
        com_util_argparser_get_error_message(parser, message, sizeof(message));
        fprintf(stderr, "error: %s\n", message);
        return 1;
    }

    printf("count=%d input=%s\n", count, input);

    return 0;
}
```

### 明示的な生成と解放を使う場合 (複数インスタンス管理が必要な場合)

```c
#include <com_util/argparser/argparser.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    com_util_argparser *parser = com_util_argparser_create(NULL);

    int count = 1; /* 既定値は解析前に設定する */
    const char *input = NULL;

    com_util_argparser_register_option_int(parser, "-c", "--count", "N", "繰り返し回数", 0, &count);
    com_util_argparser_register_positional_string(parser, "input", "入力ファイル", COM_UTIL_ARGPARSER_REQUIRED,
                                                &input);

    if (com_util_argparser_parse(parser, argc, argv) != COM_UTIL_ARGPARSER_OK)
    {
        char message[256];
        com_util_argparser_get_error_message(parser, message, sizeof(message));
        fprintf(stderr, "error: %s\n", message);
        com_util_argparser_dispose(parser);
        return 1;
    }

    printf("count=%d input=%s\n", count, input);

    com_util_argparser_dispose(parser);
    return 0;
}
```

値付きオプションと位置引数の格納先は、コマンド ラインに出現した場合のみ書き込まれます。
既定値は `parse()` を呼ぶ前に呼び出し側で設定してください。

`parse()` は同一ハンドルで繰り返し呼び出せます。
呼び出しの開始時に、フラグの格納先を 0 に、複数値オプションの出現数を 0 に初期化し、前回のエラー状態をクリアします。
詳細は「同一ハンドルでの再解析」の節を参照してください。

## エラー処理の方針

`com_util_argparser_parse()` が `COM_UTIL_ARGPARSER_PARSE_ERROR` を返した場合、詳細は次の API で取得します。

- `com_util_argparser_get_error()`: エラー種別 (`com_util_argparser_error_t`)
- `com_util_argparser_get_error_target()`: エラーの対象名 (オプション名や位置引数名)
- `com_util_argparser_get_error_index()`: エラーを起こした argv のインデックス (該当なしは -1)
- `com_util_argparser_get_error_message()`: 人間可読の 1 行メッセージを呼び出し側バッファーへ組み立てる

いずれも表示は行わず、文字列や値を返すだけです。
表示するかどうか、どこへ (stdout / stderr / ログ) 出すかは呼び出し側が決定します。

## ユース ケース別の定義例

### フラグ (複数回指定でカウント)

`-v` を複数回指定して詳細度を上げるような使い方です。
フラグは同一コマンド ラインで複数回指定でき、出現ごとに格納先へ 1 加算されます。

```c
int verbose = 0;

com_util_argparser_register_flag(parser, "-v", "--verbose", "詳細出力を有効にする", &verbose);

/* "-v -v --verbose" を解析すると verbose == 3 */
```

`--verbose=1` のように値を指定するとエラー (`COM_UTIL_ARGPARSER_ERROR_UNEXPECTED_VALUE`) になります。

### 必須の値付きオプション (int)

`COM_UTIL_ARGPARSER_REQUIRED` を指定すると、1 回も出現しない場合に `COM_UTIL_ARGPARSER_ERROR_MISSING_REQUIRED` で解析が失敗します。

```c
int port = 0;

com_util_argparser_register_option_int(parser, "-p", "--port", "PORT", "待ち受けポート番号",
                                     COM_UTIL_ARGPARSER_REQUIRED, &port);
```

同一オプションを 2 回指定した場合は `COM_UTIL_ARGPARSER_ERROR_DUPLICATE_OPTION` になります。
複数回の指定を許可したい場合は、後述の配列版オプションを使用してください。

### 文字列オプションと値の寿命

文字列オプションの格納先には、argv 内の文字列がそのまま格納されます (コピーしません)。
格納した文字列の寿命は argv の寿命に従うため、argv が有効な間だけ参照してください。

```c
const char *name = NULL;

com_util_argparser_register_option_string(parser, "-n", "--name", "NAME", "表示名", 0, &name);

/* "--name=alice" を解析すると name は argv 内の "alice" 部分を指す */
```

### 複数回指定できるオプション

同じオプションを複数回指定して値を積み上げたい場合は、配列版の登録関数を使用します。
呼び出し側は格納先の配列 (`storage`)、その要素数 (`capacity`)、出現数の格納先 (`count`) を渡します。

```c
#define INCLUDE_MAX 8

const char *includes[INCLUDE_MAX];
size_t include_count = 0;

com_util_argparser_register_option_string_array(parser, "-i", "--include", "DIR", "インクルード ディレクトリ", 0,
                                              includes, INCLUDE_MAX, &include_count);

/* "-i dir1 --include=dir2" を解析すると
   include_count == 2, includes[0] == "dir1", includes[1] == "dir2" */
```

`capacity` を超える出現は `COM_UTIL_ARGPARSER_ERROR_TOO_MANY_OCCURRENCES` になります。
int 値の複数回指定には `com_util_argparser_register_option_int_array()` を使用します。

```c
int ports[4];
size_t port_count = 0;

com_util_argparser_register_option_int_array(parser, "-p", "--port", "PORT", "待ち受けポート番号",
                                           COM_UTIL_ARGPARSER_REQUIRED, ports, 4, &port_count);

/* REQUIRED を付けた場合、1 回も出現しないと MISSING_REQUIRED になる */
```

### 位置引数 (必須と任意)

位置引数はオプションとして解釈されなかったトークンを、登録順に割り当てます。

```c
const char *input = NULL;
const char *output = NULL;

com_util_argparser_register_positional_string(parser, "input", "入力ファイル", COM_UTIL_ARGPARSER_REQUIRED, &input);
com_util_argparser_register_positional_string(parser, "output", "出力ファイル", 0, &output);

/* "in.txt" だけを渡すと input == "in.txt"、output は未変更 (既定値のまま) */
```

任意 (`COM_UTIL_ARGPARSER_REQUIRED` なし) の位置引数を登録した後に、必須の位置引数を登録することはできません (`COM_UTIL_ARGPARSER_INVALID_ARGUMENT` になります)。
割り当てが曖昧になるためです。
必須の位置引数は先に登録してください。

登録数を超える位置引数トークンが出現した場合は `COM_UTIL_ARGPARSER_ERROR_TOO_MANY_POSITIONALS` になります。

### ヘルプ表示との組み合わせ

`--help` のようなフラグと `com_util_argparser_print_usage()` を組み合わせると、
必須引数が省略されていてもヘルプを優先して表示できます。

```c
int help_flag = 0;

com_util_argparser_register_flag(parser, "-h", "--help", "ヘルプを表示する", &help_flag);

com_util_argparser_result_t result = com_util_argparser_parse(parser, argc, argv);

if (help_flag != 0)
{
    com_util_argparser_print_usage(parser, stdout);
    com_util_argparser_dispose(parser);
    return 0;
}

if (result != COM_UTIL_ARGPARSER_OK)
{
    /* 通常のエラー処理 (次節を参照) */
}
```

`com_util_argparser_print_usage()` は、内部で必要サイズを問い合わせてから usage 文字列を組み立て、指定したストリームへ書き出す便利関数です。
固定長バッファーによる切り詰めは発生しません。
解析の成否とは独立に、登録が完了していればいつでも呼び出せます。

### エラー時のメッセージと usage をまとめて表示する

解析に失敗した場合の定型的なエラー表示は、`com_util_argparser_print_error_messages()` と `com_util_argparser_print_usage()` の組み合わせでまとめられます。

```c
if (com_util_argparser_parse(parser, argc, argv) != COM_UTIL_ARGPARSER_OK)
{
    com_util_argparser_print_error_messages(parser, stderr);
    com_util_argparser_print_usage(parser, stderr);

    com_util_argparser_dispose(parser);
    return 1;
}
```

`com_util_argparser_print_error_messages()` は、内部で `com_util_argparser_get_error_message()` を用いてエラー メッセージを組み立て、`"error: {メッセージ}\n"` の形式で書き出したあと、区切りの空行を出力します。
エラーがない場合や対象がない場合は何も出力しません。

### usage を文字列として組み立てる場合

出力先ストリームへ直接書き出すのではなく、usage を文字列として自前のバッファーで扱いたい場合は、
引き続き `com_util_argparser_get_usage()` を使用します。

バッファーが不足する場合は `COM_UTIL_ARGPARSER_BUFFER_TOO_SMALL` が返り、
バッファーには切り詰めた内容が格納されます。
必要なバイト数を事前に知りたい場合は、`buffer` に `NULL` を渡して `required_size` だけを問い合わせられます。

```c
size_t required_size = 0;

com_util_argparser_get_usage(parser, NULL, 0, &required_size);
/* required_size バイト分のバッファーを確保してから再度呼び出す */
```

### 同一ハンドルでの再解析

同じパーサー ハンドルで `parse()` を複数回呼び出すことができます。
テストで複数のコマンド ラインを検証する場合や、対話的に複数回コマンドを受け付ける場合に使えます。

```c
com_util_argparser_parse(parser, argc1, argv1);
/* ... 1 回目の結果を利用 ... */

com_util_argparser_parse(parser, argc2, argv2);
/* ... 2 回目の結果を利用 (フラグと複数値オプションの出現数は自動的にリセットされる) ... */
```

値付きオプションと位置引数の格納先は出現時のみ上書きされるため、
2 回目の解析前に必要であれば呼び出し側で既定値を設定し直してください。
`com_util_argparser_default()` で取得したハンドルも同様に、`parse()` の度に出現数がリセットされます。

## 参考実装

全種別 (フラグ、必須/任意オプション、複数回指定オプション、位置引数) を組み合わせた実例は、
サンプル コマンド `argparser-sample` にあります。

- `app/com_util/prod/src/cmd/argparser-sample/argparser-sample.c`

`make -C app/com_util` でビルドすると `app/com_util/prod/cbin/argparser-sample` が生成されます。
`--help` を付けて実行すると、登録内容から組み立てた usage を確認できます。
