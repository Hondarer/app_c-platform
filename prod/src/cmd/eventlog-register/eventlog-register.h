#ifndef COM_UTIL_EVENTLOG_REGISTER_H
#define COM_UTIL_EVENTLOG_REGISTER_H

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     *  @brief          サブコマンドを解釈し、イベント ソースの登録/削除を実行します。
     *  @param[in]      command  サブコマンド ("install" または "uninstall")。
     *  @return         正常終了時は 0、異常終了時は 0 以外を返します。
     *
     *  install / uninstall を解釈します。HKLM への書き込みには管理者権限が必要なため、
     *  Windows では必要に応じて UAC 昇格を行います。\n
     *  登録先は `HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\EventLog\Application\com_util.tracer`
     *  キーです。ソース名 `com_util.tracer` は `COM_UTIL_TRACER_DEFAULT_PROVIDER_NAME` の値で、
     *  登録状態の確認はこのキーの有無で判断できます。\n
     *  Linux ではイベント ログが存在しないため、案内を表示して 0 以外を返します。
     */
    int eventlog_register_run(const char *command);

#ifdef __cplusplus
}
#endif

#endif /* COM_UTIL_EVENTLOG_REGISTER_H */
