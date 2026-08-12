// shutdown.c の static 関数へテストからアクセスするための宣言
#ifndef SHUTDOWN_INJECT_H
#define SHUTDOWN_INJECT_H

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    extern void test_shutdown_signal_handler(int signal_number);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SHUTDOWN_INJECT_H */
