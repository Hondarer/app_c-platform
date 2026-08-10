// テスト対象ソース ファイルの注入用追加ヘッダー
// このヘッダーをテスト プログラムが参照することで
// テスト プログラムからテスト対象ソースの static 変数にアクセスできます
#ifndef TRACER_INJECT_H
#define TRACER_INJECT_H

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /* tracer.c のファイル内 static 変数 s_trace_registry の shutdown 状態を戻すアクセサー。
       shutdown_started は本来一度立つと戻らないため、shutdown を検証したテストの後に
       同一プロセスで実行される他のテストが tracer を生成できるようにする。 */
    extern void test_trace_registry_reset_shutdown_state(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* TRACER_INJECT_H */
