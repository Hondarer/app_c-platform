# mock_com_util の実装規則

## 対象

この文書は、`app/com_util/test/` の `mock_com_util` を実装または変更するときの規則を示します。  
app 向け mock の一般的な配置と仕組みは [テスト チュートリアル](../../general/docs/testing-tutorial.md) と [testfw の mock](../../../framework/testfw/docs/how-to-mock.md) を参照してください。

## 既定動作

すべての関数に `delegate_real_<func>` を用意します。  
`Mock_com_util` が未注入の場合は、実 `libcom_util` へ委譲します。  
mock 注入後の `ON_CALL` も、既定では real delegate を呼び出します。

実シンボルは `resolveSharedSymbolOrExit()` で解決します。  
ライブラリを開けない場合やシンボルが見つからない場合は、理由を標準エラーへ出力して終了します。

## 実装箇所

- 宣言と `MOCK_COM_UTIL_LINK_IMPL`: `test/include/mock_com_util.h`
- Mock クラスと `ON_CALL`: `test/libsrc/mock_com_util/mock_com_util.cc`
- 関数ラッパー: `test/libsrc/mock_com_util/<module>/mock_com_util_<func>.cc`

関数ラッパーは `MOCK_WEAK_IMPL` を使用します。  
`WEAK_ATR` を関数へ直接付けません。

## Windows のリンク保持

MSVC では、弱リンク実装を含むオブジェクトが静的ライブラリから取り込まれない場合があります。  
real delegate から直接参照されない関数は、`mock_com_util.h` の `MOCK_COM_UTIL_LINK_IMPL(func)` に追加します。

`MOCK_COM_UTIL_LINK_IMPL` は `/INCLUDE:_mock_impl_<func>` を生成します。  
pragma を個別の `.cc` へ重複して記載しません。

## 可変長引数

可変長引数関数の `ON_CALL` は、対応する `va_list` 版へ委譲します。  
可変長引数を Google Mock のメソッドへ直接渡しません。

## 変更時の確認

- 公開 API の追加または削除に合わせ、宣言、`MOCK_METHOD`、`ON_CALL`、関数ラッパーを確認します。
- real delegate と mock ラッパーのシグネチャが公開ヘッダーと一致することを確認します。
- 必要な `MOCK_COM_UTIL_LINK_IMPL` がヘッダーにあることを確認します。
- `app/com_util` の局所テストを実行します。
