#ifndef COM_UTIL_EVENTLOG_REGISTER_H
#define COM_UTIL_EVENTLOG_REGISTER_H

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     *  @brief          使用方法を標準エラー出力に表示します。
     *  @param[in]      argv0  実行ファイル名 (argv[0])。
     */
    void eventlog_register_print_usage(const char *argv0);

    /**
     *  @brief          サブコマンドを解釈し、イベント ソースの登録/削除を実行します。
     *  @param[in]      argc  コマンド ライン引数の数。
     *  @param[in]      argv  コマンド ライン引数の配列。
     *  @return         終了コード (EXIT_SUCCESS / EXIT_FAILURE)。
     *
     *  install / uninstall を解釈します。HKLM への書き込みには管理者権限が必要なため、
     *  Windows では必要に応じて UAC 昇格を行います。\n
     *  登録先は @c HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\com_util.tracer
     *  キーです。ソース名 @c com_util.tracer は @c COM_UTIL_TRACER_DEFAULT_PROVIDER_NAME の値で、
     *  登録状態の確認はこのキーの有無で判断できます。\n
     *  Linux ではイベント ログが存在しないため、案内を表示して EXIT_FAILURE を返します。
     */
    int eventlog_register_run(int argc, char *argv[]);

#ifdef __cplusplus
}
#endif

#endif /* COM_UTIL_EVENTLOG_REGISTER_H */
