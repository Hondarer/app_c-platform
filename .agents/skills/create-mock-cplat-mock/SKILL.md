---
name: create-mock-cplat-mock
description: app/c-platform の mock_cplat に関数を追加・変更し、実 libcplat への既定委譲を維持します。
---

# mock_cplat の変更

`app/c-platform/docs/mock-cplat-guideline.md` の変更する関数形式とリンク保持の節を、公開ヘッダーおよび同種 mock と照合してください。

宣言、`MOCK_METHOD`、`ON_CALL`、real delegate、関数ラッパーを同じシグネチャで更新してください。  
ラッパーには `MOCK_WEAK_IMPL` を使用し、可変長引数関数は対応する `va_list` 版へ委譲してください。  
Windows のリンク保持が必要なら `mock_cplat.h` の `MOCK_CPLAT_LINK_IMPL` を更新してください。  
`WEAK_ATR` の直接付与と、個別 `.cc` への `/INCLUDE` pragma の重複記載は行わないでください。

変更した関数の mock と実委譲を対象に、app/c-platform の局所テストで確認してください。
