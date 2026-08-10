// テスト対象ソース ファイルの注入用追加ヘッダー
// このヘッダーをテスト プログラムが参照することで
// テスト プログラムからテスト対象ソースの static 関数にアクセスできます
#ifndef MODULE_INJECT_H
#define MODULE_INJECT_H

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /* module.c のファイル内 static 関数 find_shared_lib_extension_cut へのアクセサー。
       公開 API 経由では実行中のモジュール名に依存するため、
       .so.<version> や .dylib の分岐へ到達できない。 */
    extern const char *test_find_shared_lib_extension_cut(const char *s);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MODULE_INJECT_H */
