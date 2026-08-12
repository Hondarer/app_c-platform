# ネットワーク API ガイドライン

## 概要

`com_util` の `net` カテゴリは、BSD ソケット (Linux) と Winsock (Windows) の差異を吸収し、共通の通信 API を提供します。

本書は `net` カテゴリ固有の規範を定めます。  
プラットフォーム分岐の書き方は [プラットフォーム抽象化ガイドライン](platform-abstraction-guideline.md)、結果コードと命名は [コーディング規範](coding-guideline.md) に従います。

利用側のコードは、`socket`、`bind`、`listen`、`accept`、`connect`、`send`、`recv`、`sendto`、`recvfrom`、`poll`、`WSAPoll`、`setsockopt`、`getsockopt`、`shutdown`、`closesocket`、`ioctlsocket`、`htons`、`ntohs`、`htonl`、`ntohl` を直接呼び出しません。

## 公開ヘッダーにシステム ヘッダーを漏らさない

`net` の公開ヘッダーは、`winsock2.h`、`ws2tcpip.h`、`netinet/in.h`、`sys/socket.h`、`arpa/inet.h`、`netdb.h` のいずれも include しません。  
`PLATFORM_*` による分岐も置きません。

> [!NOTE]
> 暗号 API が OpenSSL のヘッダーを公開面へ出していないのと同じ方針です。
> 利用側のライブラリがシステムのソケット ヘッダーに依存しなくなるため、モックだけで通信経路を差し替えた単体テストが成立します。

## ソケット ハンドル

ソケット ハンドルは `com_util_socket` で表します。  
実体は `intptr_t` であり、Linux のファイル記述子 (`int`) と Windows の `SOCKET` (`UINT_PTR`) の双方を可逆に格納できます。

無効値は `COM_UTIL_INVALID_SOCKET` を使用します。  
数値リテラルの `-1` や `INVALID_SOCKET` との比較は行いません。

> [!NOTE]
> Windows の `INVALID_SOCKET` は `(SOCKET)~0` であり、`intptr_t` へ変換すると `-1` になります。
> Linux の無効な fd も `-1` であるため、単一の定数で両プラットフォームを表現できます。

## エンドポイント

アドレスとポートの受け渡しには `com_util_ipv4_endpoint` を使用し、`struct sockaddr_in` を API 境界へ出しません。  
`struct sockaddr_in` への変換は `net` の実装ファイル内部でのみ行います。

`com_util_ipv4_endpoint` のアドレスとポートは、いずれもネットワーク バイト オーダーで保持します。  
ホスト バイト オーダーとの混在を避けるため、フィールドへ直接代入する場合もバイト オーダー変換 API を経由します。

## バイト オーダー変換

バイト オーダー変換は `com_util/net/byteorder.h` の `static inline` 関数を使用します。  
実装はシフト演算とバイト列の再構成で行い、`htons` などの OS API を呼び出しません。

> [!NOTE]
> OS API への依存が消えるため、変換の検証にモックが不要になり、条件網羅も自明に満たせます。

## ソケット オプション

ソケット オプションは、用途ごとの名前付き API で提供します。  
`level` と `optname` を引数に取る汎用の `setsockopt` 相当 API は公開しません。

マルチキャスト グループへの参加と離脱は対で提供します。  
`com_util_socket_join_multicast_group()` で参加したグループは、`com_util_socket_leave_multicast_group()` で明示的に離脱できます。  
ソケットを閉じれば参加中のグループからは自動的に離脱するため、離脱 API は閉じる前に明示的に通知する場合に使用します。

システム ヘッダーが定義する `SOL_SOCKET`、`SO_REUSEADDR`、`SO_BROADCAST`、`IPPROTO_IP`、`IP_MULTICAST_IF`、`IP_ADD_MEMBERSHIP`、`IP_DROP_MEMBERSHIP` などの定数は、`net` の実装ファイル内部でのみ使用します。

## ライブラリの初期化と終了

Winsock の初期化と終了は公開 API にしません。  
`net` の内部で初回利用時に初期化し、共有ライブラリのアンロード時に終了します。

利用側は初期化の呼び出し順序を意識せず、任意のタイミングで `net` の API を呼び出せます。

> [!NOTE]
> 暗号 API と同じく、プロセス グローバルな初期化を公開面へ出さない方針です。
> 初期化漏れによる `WSANOTINITIALISED` を、利用側の責務から外すことを意図しています。

## エラーの伝達

戻り値は共通結果コード (`COM_UTIL_OK` および `COM_UTIL_ERR_*`) とします。  
OS 由来の詳細は `com_util_error *detail_out` へ格納します。

`errno` と `WSAGetLastError()` の値を、公開 API の引数または戻り値でそのまま受け渡すことはしません。

### エラー ドメイン

ソケット関連のエラーは、次の 3 つのドメインを使い分けます。

| ドメイン | 用途 |
|---|---|
| `COM_UTIL_ERROR_DOMAIN_SOCKET_ERRNO` | Linux のソケット API が設定した `errno` |
| `COM_UTIL_ERROR_DOMAIN_WINSOCK` | Windows の `WSAGetLastError()` が返す値 |
| `COM_UTIL_ERROR_DOMAIN_GAI` | `getaddrinfo` が返す `EAI_*` |

一般の `errno` を扱う `COM_UTIL_ERROR_DOMAIN_ERRNO` とは区別します。  
ソケット操作の `EAGAIN` は非ブロッキング操作の待機を意味しますが、`fork()` や `pthread_create()` が返す `EAGAIN` は資源の上限超過を意味し、両者は同じ `errno` 値でも要因が異なります。  
ドメインを分けることで、ソケット操作の `errno` だけを `COM_UTIL_CAUSE_WOULD_BLOCK` として解釈できます。

Winsock のエラー番号空間は Win32 の `GetLastError()` と異なるため、`COM_UTIL_ERROR_DOMAIN_WINDOWS` を使用しません。  
`getaddrinfo` の `EAI_*` はさらに別の体系であるため、Winsock ドメインとも区別します。

### エラー要因

プラットフォーム非依存の判定には `com_util_error_cause` を使用します。  
`errno` および `WSAE*` の値を利用側で直接比較することはしません。

`com_util_error_cause` の値は ABI として固定します。新しい要因は末尾へ追加します。

## ブロッキングと非ブロッキング

ブロッキング モードの切り替えは `com_util_socket_set_nonblocking()` の 1 本で行い、有効と無効を引数で指定します。

非ブロッキングの `connect` が完了待ちになった場合は、共通の要因 `COM_UTIL_CAUSE_IN_PROGRESS` で表します。  
Linux の `EINPROGRESS` と Windows の `WSAEWOULDBLOCK` の差はここで吸収します。

## シグナルによる中断

Linux のソケット API がシグナルで中断された場合の扱いは、[コーディング規範](coding-guideline.md) の「シグナル割り込み (EINTR) の扱い」に従います。  
`net` の API 契約として固有の規則は設けません。

## 非対称な動作の明示

プラットフォームで意味論を完全に揃えられない API は、共通契約をヘッダーの Doxygen へ明記します。  
利用側が差を意識せずに済む形へ、引数と戻り値を設計します。

受信方向の半クローズ (`com_util_socket_shutdown_receive()`) が該当します。  
Linux は読み取り方向を停止してハンドルを保持し、Windows はソケットを閉じます。  
呼び出し側がハンドルの生存を判断しなくて済むように、ソケットを入出力引数で受け取り、閉じた場合は `COM_UTIL_INVALID_SOCKET` を書き戻します。

## 参照

- [プラットフォーム抽象化ガイドライン](platform-abstraction-guideline.md)
- [コーディング規範](coding-guideline.md)
- [Windows Sockets Error Codes](https://learn.microsoft.com/en-us/windows/win32/winsock/windows-sockets-error-codes-2)
- [getaddrinfo (POSIX)](https://pubs.opengroup.org/onlinepubs/9699919799/functions/getaddrinfo.html)
