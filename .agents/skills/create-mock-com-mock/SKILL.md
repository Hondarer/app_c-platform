---
name: create-mock-com-mock
description: app/com_util の mock_com_util に関数を追加または変更するときに使用します。実 libcom_util への既定委譲、動的シンボル解決、MOCK_WEAK_IMPL、MSVC のリンク保持を扱います。
---

# mock_com_util の変更

1. `app/com_util/docs/mock-com-util-guideline.md` を読んでください。
2. 対応する公開ヘッダーと既存の同種 mock を確認してください。
3. 宣言、`MOCK_METHOD`、`ON_CALL`、real delegate、関数ラッパーを同じシグネチャで更新してください。
4. 関数ラッパーには `MOCK_WEAK_IMPL` を使用してください。
5. Windows でリンク保持が必要なら、`mock_com_util.h` の `MOCK_COM_UTIL_LINK_IMPL` を更新してください。
6. `app/com_util` の局所テストを実行してください。

可変長引数関数は、対応する `va_list` 版へ委譲してください。  
`WEAK_ATR` の直接付与と、個別 `.cc` への `/INCLUDE` pragma の重複記載は行わないでください。
