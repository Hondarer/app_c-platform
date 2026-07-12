---
short-title: "argparser"
---

# argparser - コマンド ライン引数パーサー

`com_util_argparser` は、コマンド ライン引数 (argc / argv) を解析する汎用パーサーです。
フラグ、値付きオプション、位置引数を事前に登録し、解析結果を登録時に指定した格納先へ書き込みます。

対応する構文は次のとおりです。

- フラグ: `-v` / `--verbose` (出現回数を格納)
- 値付きオプション: `-o value` / `--option value` / `--option=value`
- 複数回指定できる値付きオプション: 上記構文の繰り返し (出現順に配列へ格納)
- 位置引数: 登録順に割り当て
- 負の位置整数: 次の位置引数が整数型の場合、`-1` などを位置引数として割り当て

次の構文は対応していません。

- 短オプションの連結 (`-abc`)
- 1 つのオプションに複数の値を続ける方式 (`--option value1 value2 value3`)
- `--` 区切り以降を無条件で位置引数扱いにする慣習

宣言は `com_util/argparser/argparser.h` にあります。
API の詳細な引数説明は同ヘッダーの Doxygen コメントを参照してください。
本書ではユース ケース別の使い方をまとめます。

本書で扱うのは、プロセス共有のデフォルト パーサーを暗黙に使用する API (`com_util_argparser_*`) です。
通常のアプリケーションはこの API だけで完結し、パーサー ハンドルを意識する必要はありません。
複数のパーサー インスタンスを同時に扱う場合 (主にテスト) は、明示ハンドル API (`_com_util_argparser_*`) を解説する [argparser.internal.md](argparser.internal.md) を参照してください。

## 基本フロー

パーサーの利用は、初期化、登録、解析、参照という一連の流れで行います。
パーサーの実体はプロセス正常終了時に自動解放されるため、明示的な解放処理は不要です。

```c
#include <com_util/argparser/argparser.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    com_util_console_init();
    com_util_argparser_init("sample program");

    int need_help = 0;
    int count = 1; /* 既定値は解析前に設定する */
    const char *input = NULL;

    com_util_argparser_register_flag("-h", "--help", "ヘルプを表示する", &need_help);
    com_util_argparser_register_option_int("-c", "--count", "N", "繰り返し回数", 0, &count);
    com_util_argparser_register_positional_string("input", "入力ファイル", COM_UTIL_ARGPARSER_REQUIRED, &input);

    if (com_util_argparser_get_register_error_count() > 0)
    { /* オプションの登録に失敗した場合 (コーディング エラーの場合) */
        com_util_argparser_print_register_error_messages(stderr);
        return EXIT_FAILURE;
    }

    int parse_result = com_util_argparser_parse(argc, argv);

    if (need_help != 0)
    {
        /* 必須引数が省略されていても -h, --help を優先する */
        com_util_argparser_print_usage(stdout);
        return EXIT_SUCCESS;
    }

    if (parse_result != COM_UTIL_ARGPARSER_OK)
    {
        com_util_argparser_print_error_messages(stderr);
        com_util_argparser_print_usage(stderr);
        return EXIT_FAILURE;
    }

    printf("count=%d input=%s\n", count, input);

    return EXIT_SUCCESS;
}
```

`com_util_argparser_init()` にはプログラムの説明文を設定することができます (不要な場合は NULL)。
説明文は usage の冒頭に表示されます。
`com_util_argparser_init()` を呼ばずに register 系 API をいきなり呼び出しても、既定のオプションで暗黙に初期化されます。

値付きオプションと位置引数の格納先は、コマンド ラインに出現した場合のみ書き込まれます。
既定値は `com_util_argparser_parse()` を呼ぶ前に呼び出し側で設定してください。

## 登録エラーの確認

register 系 API (`com_util_argparser_register_*()`) は結果コードを戻り値として返さず、エラーを内部に記録します。
呼び出しごとの成否確認を省略し、すべての登録を終えた後に `com_util_argparser_get_register_error_count()` でまとめて判定できます。

登録エラーは、同名オプションの二重登録などのコーディング エラーです。
定型的な報告には `com_util_argparser_print_register_error_messages()` が使用でき、記録されたエラーを発生順にすべて `"error: {メッセージ}\n"` の形式で指定ストリームへ書き出します。
個別のエラー詳細が必要な場合は `com_util_argparser_get_register_error()` 系の API で取得できます。

## 解析エラー処理の方針

本 API はエラーを標準出力・標準エラーへ出力しません。
表示するかどうか、どこへ (stdout / stderr / ログ) 出すかは呼び出し側が決定します。

`com_util_argparser_parse()` が `COM_UTIL_ARGPARSER_PARSE_ERROR` を返した場合、詳細は次の API で取得します。

- `com_util_argparser_get_error()`: エラー種別 (`int`)
- `com_util_argparser_get_error_target()`: エラーの対象名 (オプション名や位置引数名)
- `com_util_argparser_get_error_index()`: エラーを起こした argv のインデックス (該当なしは -1)
- `com_util_argparser_get_error_message()`: 人間可読の 1 行メッセージを呼び出し側バッファーへ組み立てる

いずれも表示は行わず、文字列や値を返すだけです。
定型的なエラー表示で足りる場合は、後述の `com_util_argparser_print_error_messages()` が使用できます。

## ユース ケース別の定義例

### フラグ (複数回指定でカウント)

`-v` を複数回指定して詳細度を上げるような使い方です。
フラグは同一コマンド ラインで複数回指定でき、出現ごとに格納先へ 1 加算されます。

```c
int verbose = 0;

com_util_argparser_register_flag("-v", "--verbose", "詳細出力を有効にする", &verbose);

/* "-v -v --verbose" を解析すると verbose == 3 */
```

`--verbose=1` のように値を指定するとエラー (`COM_UTIL_ARGPARSER_ERROR_UNEXPECTED_VALUE`) になります。

### 必須の値付きオプション (int)

`COM_UTIL_ARGPARSER_REQUIRED` を指定すると、1 回も出現しない場合に `COM_UTIL_ARGPARSER_ERROR_MISSING_REQUIRED` で解析が失敗します。

```c
int port = 0;

com_util_argparser_register_option_int("-p", "--port", "PORT", "待ち受けポート番号",
                                       COM_UTIL_ARGPARSER_REQUIRED, &port);
```

同一オプションを 2 回指定した場合は `COM_UTIL_ARGPARSER_ERROR_DUPLICATE_OPTION` になります。
複数回の指定を許可したい場合は、後述の配列版オプションを使用してください。

### 文字列オプションと値の寿命

文字列オプションの格納先には、argv 内の文字列がそのまま格納されます (コピーしません)。
格納した文字列の寿命は argv の寿命に従うため、argv が有効な間だけ参照してください。

```c
const char *name = NULL;

com_util_argparser_register_option_string("-n", "--name", "NAME", "表示名", 0, &name);

/* "--name=alice" を解析すると name は argv 内の "alice" 部分を指す */
```

### 複数回指定できるオプション

同じオプションを複数回指定して値を積み上げたい場合は、配列版の登録関数を使用します。
呼び出し側は格納先の配列 (`storage`)、その要素数 (`capacity`)、出現数の格納先 (`count`) を渡します。

```c
#define INCLUDE_MAX 8

const char *includes[INCLUDE_MAX];
size_t include_count = 0;

com_util_argparser_register_option_string_array("-i", "--include", "DIR", "インクルード ディレクトリ", 0,
                                                includes, INCLUDE_MAX, &include_count);

/* "-i dir1 --include=dir2" を解析すると
   include_count == 2, includes[0] == "dir1", includes[1] == "dir2" */
```

`capacity` を超える出現は `COM_UTIL_ARGPARSER_ERROR_TOO_MANY_OCCURRENCES` になります。
int 値の複数回指定には `com_util_argparser_register_option_int_array()` を使用します。

```c
int ports[4];
size_t port_count = 0;

com_util_argparser_register_option_int_array("-p", "--port", "PORT", "待ち受けポート番号",
                                             COM_UTIL_ARGPARSER_REQUIRED, ports, 4, &port_count);

/* REQUIRED を付けた場合、1 回も出現しないと MISSING_REQUIRED になる */
```

### 位置引数 (必須と任意)

位置引数はオプションとして解釈されなかったトークンを、登録順に割り当てます。

```c
const char *input = NULL;
const char *output = NULL;

com_util_argparser_register_positional_string("input", "入力ファイル", COM_UTIL_ARGPARSER_REQUIRED, &input);
com_util_argparser_register_positional_string("output", "出力ファイル", 0, &output);

/* "in.txt" だけを渡すと input == "in.txt"、output は未変更 (既定値のまま) */
```

任意 (`COM_UTIL_ARGPARSER_REQUIRED` なし) の位置引数を登録した後に、必須の位置引数を登録することはできません (登録エラーになります)。
割り当てが曖昧になるためです。
必須の位置引数は先に登録してください。

登録数を超える位置引数トークンが出現した場合は `COM_UTIL_ARGPARSER_ERROR_TOO_MANY_POSITIONALS` になります。

### ヘルプ表示との組み合わせ

`--help` のようなフラグと `com_util_argparser_print_usage()` を組み合わせると、必須引数が省略されていてもヘルプを優先して表示できます。

```c
int need_help = 0;

com_util_argparser_register_flag("-h", "--help", "ヘルプを表示する", &need_help);

int result = com_util_argparser_parse(argc, argv);

if (need_help != 0)
{
    com_util_argparser_print_usage(stdout);
    return EXIT_SUCCESS;
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
if (com_util_argparser_parse(argc, argv) != COM_UTIL_ARGPARSER_OK)
{
    com_util_argparser_print_error_messages(stderr);
    com_util_argparser_print_usage(stderr);
    return EXIT_FAILURE;
}
```

`com_util_argparser_print_error_messages()` は、内部でエラー メッセージを組み立て、`"error: {メッセージ}\n"` の形式で書き出したあと、区切りの空行を出力します。
エラーがない場合や対象がない場合は何も出力しません。

### usage を文字列として組み立てる場合

出力先ストリームへ直接書き出すのではなく、usage を文字列として自前のバッファーで扱いたい場合は、`com_util_argparser_get_usage()` を使用します。

必要なバイト数を事前に知りたい場合は、`buffer` に `NULL` を渡して `required_size` だけを問い合わせられます。

```c
size_t required_size = 0;

com_util_argparser_get_usage(NULL, 0, &required_size);
/* required_size バイト分のバッファーを確保してから再度呼び出す */
```

### 再解析

`com_util_argparser_parse()` は繰り返し呼び出すことができます。
対話的に複数回コマンド ラインを受け付ける場合に使えます。

```c
com_util_argparser_parse(argc1, argv1);
/* ... 1 回目の結果を利用 ... */

com_util_argparser_parse(argc2, argv2);
/* ... 2 回目の結果を利用 (フラグと複数値オプションの出現数は自動的にリセットされる) ... */
```

呼び出しの開始時に、フラグの格納先を 0 に、複数値オプションの出現数を 0 に初期化し、前回のエラー状態をクリアします。
値付きオプションと位置引数の格納先は出現時のみ上書きされるため、2 回目の解析前に必要であれば呼び出し側で既定値を設定し直してください。

## 参考実装

全種別 (フラグ、必須/任意オプション、複数回指定オプション、位置引数) を組み合わせた実例は、サンプル コマンド `argparser-sample` にあります。

- `app/com_util/prod/src/cmd/argparser-sample/argparser-sample.c`
- コマンドの概要は同ディレクトリの [README.md](../../../src/cmd/argparser-sample/README.md) を参照してください

サンプルでは、解析結果の格納先を `argparser_sample_options` 構造体に集約し、登録処理を `register_argparser()` という別関数に分離しています。
これは登録内容が多いコマンドで構成を分かりやすくするための一例であり、必須の作法ではありません。
軽量なプログラムでは、本書の各例のように main 内のローカル変数を格納先として直接登録すれば十分です。

## 複数インスタンスを扱う場合 (主にテスト)

デフォルト パーサーはプロセス内で 1 つだけです。
テストでの独立性検証など、複数のパーサー インスタンスを同時に扱う必要がある場合は、明示ハンドル API (`_com_util_argparser_create()` / `_com_util_argparser_dispose()` など) を使用します。
詳細は [argparser.internal.md](argparser.internal.md) を参照してください。
