# com_util API チート シート

## 概要

本チート シートは、`app/` 配下のコードで生の CRT / POSIX / Win32 API を書こうとしたときに、対応する com_util の代替関数を素早く引くための一覧です。  
同時に、com_util が公開する API 全体を用途から引ける逆引き辞書でもあります。  
規範本文と根拠は [coding-guideline.md](coding-guideline.md) の「[ラッパーの設計方針](coding-guideline.md#ラッパーの設計方針)」「[危険な標準関数の代替](coding-guideline.md#危険な標準関数の代替)」に集約されています。  
本チート シートは対応表の抽出であり、規範としての正本は `coding-guideline.md` です。  
公開 API の宣言とシグネチャは、[`prod/include/`](../prod/include/) 配下のヘッダーを正本とします。  
コード、コーディング規範、または本チート シートに不一致がある場合は、正本を確認したうえで同じ変更の中で一致させてください。

表は 3 種類に分かれます。

- **危険な標準関数の代替**: 境界検査または失敗通知を欠く標準関数で、`app/` 配下の管理対象コードでの直接使用を禁止しています。
- **プラットフォーム抽象ラッパー**: 危険ではないものの、Linux / Windows の挙動差 (共有モード、UTF-8 パス、64bit オフセットなど) を吸収するために com_util 側で提供しているラッパーです。
- **独自機能 API**: 単一の生 API とは 1 対 1 で対応しない com_util 独自の機能です。「生の構文」列には、対応する生の関数がある場合はその名前を、ない場合は用途を記載します。

外部 OSS 由来のコード (`app/lua`、`app/sqlite`、`app/cjson`) はいずれの表も対象外です。

> [!NOTE]
> `memcpy` / `memmove` / `memset` / `strcmp` 系 / 単純な `malloc` 呼出 / `printf` など、両プラットフォームで挙動が同じで境界検査の欠落もない関数には、com_util はラッパーを作りません。
> 詳細は [coding-guideline.md の「ラッパーを作らないもの」](coding-guideline.md#ラッパーを作らないもの) を参照してください。

## 危険な標準関数の代替

対象ヘッダー: `com_util/crt/string.h`、`com_util/crt/stdio.h`、`com_util/crt/stdlib.h`、`com_util/base/error_message.h`

### 文字列操作

| 生 API | 問題 | com_util 代替 |
|---|---|---|
| `strcpy` | コピー先の容量を受け取らず、境界を検査しません。 | `com_util_strcpy(dest, dest_size, src)` |
| `strncpy` | コピー元が `count` 以上のとき null 終端しません。`count` は宛先容量ではなく最大コピー文字数 | 文字列全体をコピーするなら `com_util_strcpy`、意図的に切り詰めるなら `com_util_strncpy(dest, dest_size, src, count)` |
| `strcat` | 連結先の容量を受け取らず、境界を検査しません。 | `com_util_strcat(dest, dest_size, src)` |
| `strncat` | 連結先の容量を受け取らない。`count` は連結元から読む最大文字数であり、終端の 1 バイトも別に必要 | `com_util_strncat(dest, dest_size, src, count)` |
| `wcscpy` | `strcpy` と同じ | `com_util_wcscpy(dest, dest_size, src)` |
| `strdup` / `_strdup` | MSVC では `strdup` が非推奨 (C4996) であり名前が異なります。 | `com_util_strdup(src)` |
| `strtok` | 解析状態をライブラリ内の静的変数に持ち、再入できません。 | `com_util_strtok_r(str, delim, saveptr)` |
| `gets` | 宛先の容量を指定できません。C11 で標準から削除された | `com_util_fgets(dest, dest_size, stream, detail_out)` |

### 書式化・行入力

| 生 API | 問題 | com_util 代替 |
|---|---|---|
| `fgets` | 切り詰めと EOF を戻り値で区別できず、改行の有無を呼び出し側が判定する必要がある | `com_util_fgets(dest, dest_size, stream, detail_out)` |
| `sprintf` / `vsprintf` | 出力先の容量を受け取らず、境界を検査しません。 | `com_util_snprintf(dest, dest_size, format, ...)` / `com_util_vsnprintf` |
| `snprintf` / `vsnprintf` | 境界は検査するが、切り詰めの検出を呼び出し側の戻り値検査に委ねている | `com_util_snprintf` / `com_util_vsnprintf` (戻り値は文字数ではなく共通結果コード) |
| `scanf` / `fscanf` / `sscanf` と各 `v*` 版 | 幅を指定しない `%s` が境界外書き込みを起こす | `com_util_scanf` / `com_util_fscanf` / `com_util_sscanf` と各 `v*` 版 |

### 数値変換

| 生 API | 問題 | com_util 代替 |
|---|---|---|
| `atoi` / `atol` / `atoll` / `atof` | 変換の失敗を通知せず、範囲外の入力が未定義動作になります。 | `com_util_parse_int` / `com_util_parse_int64` / `com_util_parse_double` |
| `strtol` / `strtoll` / `strtoul` / `strtoull` / `strtod` | 完全消費と `errno` の検査を呼び出し側に委ねており、検査を省略しても失敗が表面化しません。 | `com_util_parse_int` / `com_util_parse_int64` / `com_util_parse_uint64` / `com_util_parse_double` |

`com_util_parse_uint64` は先頭の符号 `'-'` を範囲外エラーとして拒否します。  
`strtoull` が負値を符号なしの折り返し値として受け付ける挙動とは異なります。  
幅が処理系に依存する `long` を返す変換 API は提供しません。

### エラー文字列化

| 生 API | 問題 | com_util 代替 |
|---|---|---|
| `strerror` | 戻り値の生存期間が処理系依存で、スレッド セーフとは限らない。再入可能版は `strerror_r` と `strerror_s` で名前と引数が異なります。 | `com_util_error_message(buf, buf_size, &error)` |

### メモリ確保

| 生 API | 問題 | com_util 代替 |
|---|---|---|
| `malloc` | 単一オブジェクト、バイト バッファーの確保に使用します。長さ 0 の戻り値が処理系定義 | `com_util_malloc(size)` (ゼロ初期化しない) / `com_util_malloc_zerofill(size)` (ゼロ初期化する) |
| `calloc` | `malloc(count * size)` は乗算の回り込みを検出しません。 | `com_util_calloc(count, size)` (乗算オーバーフローを検査し、ゼロ初期化する) |
| `realloc` | 失敗時の受け方と長さ 0 の扱いを呼び出し側に委ねている | `com_util_realloc(ptr, count, size)` / `com_util_realloc_zerofill(ptr, old_count, count, size)` |
| `free` | 共有ライブラリの境界をまたぐと、確保側と解放側で C ランタイムのヒープが一致しない場合がある | `com_util_free(ptr)` |

> [!IMPORTANT]
> `com_util_realloc` / `com_util_realloc_zerofill` は要素数とサイズを分けて受け取る 3 引数 (`_zerofill` 版は 4 引数) であり、`realloc(ptr, size)` を機械的に置換すると引数がずれます。
> `com_util_realloc(ptr, 0, size)` は元の領域を解放せずに NULL を返します。標準の `realloc(ptr, 0)` とは異なる扱いです。

## プラットフォーム抽象ラッパー

危険関数には該当しませんが、Linux / Windows の挙動差を吸収するため、`app/` 配下のコードでは生 API ではなくラッパーを使用します。

### 標準 I/O・ファイル操作

対象ヘッダー: `com_util/crt/stdio.h`

| 生 API | com_util 代替 | 差異の要点 |
|---|---|---|
| `fopen` | `com_util_fopen(path, modes, detail_out)` | パスは UTF-8 を受け取り、Windows 内部で `_wfsopen` を `_SH_DENYNO` 指定で呼び出し Linux と同じ共有可の既定にします。 |
| `freopen` | `com_util_freopen(path, modes, stream, detail_out)` | `fopen` と同じ共有モード吸収。テキスト/バイナリ モード フラグは元の `stream` を引き継ぐ |
| `fclose` | `com_util_fclose(stream, detail_out)` | OS エラー詳細を `detail_out` へ格納する以外は元の戻り値規約 (0/EOF) を保持 |
| `fflush` | `com_util_fflush(stream, detail_out)` | 同上 |
| `fread` | `com_util_fread(buffer, size, count, stream, detail_out)` | 同上 |
| `fwrite` | `com_util_fwrite(buffer, size, count, stream, detail_out)` | 同上 |
| `remove` / `_wremove` | `com_util_remove(path, detail_out)` | パスは UTF-8 |
| `rename` / `_wrename` | `com_util_rename(oldpath, newpath, detail_out)` | 同上 |
| `fprintf` | `com_util_fprintf(stream, format, ...)` | 素通しに近いが `detail_out` 系 API との整合のため統一 |
| `vfprintf` | `com_util_vfprintf(stream, format, args)` | 同上 |
| `fseek` | `com_util_fseek(stream, offset, whence)` | `offset` が 64bit (`int64_t`) 対応 |
| `ftell` | `com_util_ftell(stream)` | 戻り値が 64bit (`int64_t`) 対応 |

書式指定パスでファイルを開く/削除する用途には `com_util_fopen_fmt` / `com_util_vfopen_fmt` / `com_util_remove_fmt` / `com_util_vremove_fmt` を使用します。  
一意な一時ファイルの作成には `com_util_fopen_temp(prefix, modes, path_out, path_size, detail_out)` を使用します。  
`stdio` ラッパーとメモリ マップド ファイル (後述) のどちらを使うべきかの選定基準は [fileio-api-selection-guideline.md](fileio-api-selection-guideline.md) を参照してください。

### 低レベル fd 操作

対象ヘッダー: `com_util/crt/fcntl.h`、`com_util/crt/unistd.h`

| 生 API | com_util 代替 | 差異の要点 |
|---|---|---|
| `open` / `_wopen` | `com_util_open(path, flags, mode, detail_out)` | パスは UTF-8。Windows は `_wsopen_s` を `_SH_DENYNO` 指定で呼び出し Linux と同じ既定にします。 |
| `lseek` / `_lseeki64` | `com_util_lseek(fd, offset, whence, detail_out)` | `offset` が 64bit 対応。`fd` が負の場合は OS API を呼ばず -1 を返す |
| `close` / `_close` | `com_util_close(fd, detail_out)` | `fd` が負の場合は OS API を呼ばず -1 を返す |
| `dup` / `_dup` | `com_util_dup(fd, detail_out)` | 同上 |
| `dup2` / `_dup2` | `com_util_dup2(oldfd, newfd, detail_out)` | 成功時の戻り値を Windows の `_dup2` に合わせて常に 0 とする (POSIX `dup2` は `newfd` を返す) |
| `read` / `_read` | `com_util_read(fd, buf, count, detail_out)` | Windows では 1 回の呼び出しで読み取れる上限が `INT_MAX` バイトであり、超過分は切り詰められる |
| `write` / `_write` | `com_util_write(fd, buf, count, detail_out)` | 同上 (書き込み上限) |
| `access` / `_waccess` | `com_util_access(path, mode, detail_out)` | パスは UTF-8 |
| `isatty` | `com_util_isatty(stream)` | 引数がファイル記述子ではなく `com_util_stream` 列挙型。Windows は `GetFileType`/`GetConsoleMode` の組み合わせで判定 |

書式指定パスで開く/アクセス確認する用途には `com_util_open_fmt` / `com_util_vopen_fmt` / `com_util_access_fmt` / `com_util_vaccess_fmt` を使用します。

### ファイル システム

対象ヘッダー: `com_util/crt/sys/stat.h`

| 生 API | com_util 代替 | 差異の要点 |
|---|---|---|
| `stat` / `_wstat64` | `com_util_stat(buf, detail_out, path)` | パスは UTF-8。`buf` の型は `com_util_file_stat_t` (Linux: `struct stat`、Windows: `struct _stat64`) |
| `mkdir` / `_wmkdir` | `com_util_mkdir(path, detail_out)` | パスは UTF-8 |
| (`mkdir -p` 相当) | `com_util_makedirs(path, detail_out)` | 中間ディレクトリを再帰的に作成。既存ディレクトリはべき等に成功扱い |
| `rmdir` / `_wrmdir` | `com_util_rmdir(path, detail_out)` | パスは UTF-8。中間ディレクトリの再帰削除はしません。 |

書式指定パスを使う用途には `com_util_stat_fmt` / `com_util_vstat_fmt` / `com_util_mkdir_fmt` / `com_util_vmkdir_fmt` を使用します。

### 環境変数

対象ヘッダー: `com_util/crt/stdlib.h`

| 生 API | com_util 代替 | 差異の要点 |
|---|---|---|
| `getenv` | `com_util_getenv(name, buf, buf_size, exists_out, detail_out)` | Windows は `_dupenv_s` を使用し MSVC セキュリティ警告を回避。`buf` に NULL を渡すと存在確認のみ行う |
| `setenv` | `com_util_setenv(name, value, overwrite, detail_out)` | Linux は `setenv`、Windows は `_putenv_s` を使用。設定は呼び出し元プロセスにのみ反映 |
| `unsetenv` | `com_util_unsetenv(name, detail_out)` | Linux は `unsetenv`、Windows は値に空文字列を指定した `_putenv_s` を使用 |

### 時刻変換

対象ヘッダー: `com_util/crt/time.h`

| 生 API | com_util 代替 | 差異の要点 |
|---|---|---|
| `gmtime` (非再入版) | `com_util_gmtime(utc_tm, timep)` | Linux は `gmtime_r`、Windows は `gmtime_s` を使用しスレッド セーフにします。 |
| `localtime` (非再入版) | `com_util_localtime(local_tm, timep)` | Linux は `localtime_r`、Windows は `localtime_s` を使用 |
| `ctime` (非再入版) | `com_util_ctime(buf, buf_size, timep)` | Linux は `ctime_r`、Windows は `ctime_s` を使用。`buf_size` は 26 以上が必要 |

### Win32 UTF-8 ラッパー (Windows 専用)

対象ヘッダー: `com_util/win32/win32.h`

Win32 API はネイティブでは ANSI (現在のコード ページ) または UTF-16 のいずれかを扱い、UTF-8 を直接受け取りません。  
`*U` サフィックスのラッパーは、UTF-8 文字列と Win32 の UTF-16 API の境界を吸収します。

| 生 API (ANSI/Wide 版) | com_util 代替 (UTF-8 版) |
|---|---|
| `CreateFileA` / `CreateFileW` | `CreateFileU` |
| `CreateNamedPipeA` / `CreateNamedPipeW` | `CreateNamedPipeU` |
| `GetModuleFileNameA` / `GetModuleFileNameW` | `GetModuleFileNameU` |
| `WriteConsoleA` / `WriteConsoleW` | `WriteConsoleU` |
| `GetVolumePathNameA` / `GetVolumePathNameW` | `GetVolumePathNameU` |
| `GetVolumeInformationA` / `GetVolumeInformationW` | `GetVolumeInformationU` |
| `LoadLibraryA` / `LoadLibraryW` | `LoadLibraryU` |
| `CreateProcessA` / `CreateProcessW` | `CreateProcessU` |
| `OpenSCManagerA` / `OpenSCManagerW` | `OpenSCManagerU` |
| `CreateServiceA` / `CreateServiceW` | `CreateServiceU` |
| `OpenServiceA` / `OpenServiceW` | `OpenServiceU` |
| `ChangeServiceConfig2A` / `ChangeServiceConfig2W` | `ChangeServiceConfig2U` |
| `RegisterServiceCtrlHandlerExA` / `RegisterServiceCtrlHandlerExW` | `RegisterServiceCtrlHandlerExU` |
| `StartServiceCtrlDispatcherA` / `StartServiceCtrlDispatcherW` | `StartServiceCtrlDispatcherU` (サービス エントリは `com_util_service_entry_u` を使用) |

適用範囲は `CreateFileU` などのファイル ハンドル系に限らず、`app/com_util/docs/coding-guideline.md` の該当節を参照してください。

### クロック

対象ヘッダー: `com_util/clock/clock.h`

| 生 API | com_util 代替 | 差異の要点 |
|---|---|---|
| `clock_gettime(CLOCK_MONOTONIC, ...)` (Linux) / `GetTickCount64()` (Windows) | `com_util_get_monotonic_ms()` / `com_util_get_monotonic()` | 単調クロックをミリ秒/ナノ秒精度で返す。Windows の分解能差 (既定 ~15ms 刻み) を吸収 |
| `clock_gettime(CLOCK_REALTIME, ...)` (Linux) / `GetSystemTimeAsFileTime()` (Windows) | `com_util_get_realtime()` | Windows の 1601 年基準 (FILETIME) から Unix epoch 基準への変換を吸収 |

### メモリ マップド ファイル

対象ヘッダー: `com_util/mmap/mmap.h`

| 生 API | com_util 代替 | 差異の要点 |
|---|---|---|
| `mmap()` (POSIX) / `CreateFileMapping` +`MapViewOfFile` (Win32) | `com_util_mmap_attach(...)` | ファイル オープンとマッピングを一括し、新規作成時のサイズ指定と所有ハンドル化を行います。 |
| `munmap()` (POSIX) / `UnmapViewOfFile` +`CloseHandle` (Win32) | `com_util_mmap_detach(...)` | 内包するロックの破棄も合わせて行う |
| `msync(MS_SYNC)` (POSIX) / `FlushViewOfFile` +`FlushFileBuffers` (Win32) | `com_util_mmap_flush(...)` | - |

`stdio` ラッパーとの使い分けは [fileio-api-selection-guideline.md](fileio-api-selection-guideline.md) を参照してください。

### ネットワーク バイト オーダー

本節、「IPv4 アドレス」節、「ソケット」節が扱う `net` カテゴリの規範は [net-api-guideline.md](net-api-guideline.md) を参照してください。

対象ヘッダー: `com_util/net/byteorder.h`

| 生 API | com_util 代替 |
|---|---|
| `htons` | `com_util_hton16` |
| `ntohs` | `com_util_ntoh16` |
| `htonl` | `com_util_hton32` |
| `ntohl` | `com_util_ntoh32` |

`com_util_hton16`/`hton32` 系はシフト演算とバイト列再構成のみで実装され、ソケット ヘッダーへの依存を持ちません。

### IPv4 アドレス

対象ヘッダー: `com_util/net/endpoint.h`

| 生 API | com_util 代替 |
|---|---|
| `inet_pton(AF_INET, ...)` | `com_util_ipv4_parse(...)` |
| `getaddrinfo()` | `com_util_ipv4_resolve(...)` |
| `inet_ntop(AF_INET, ...)` | `com_util_ipv4_to_string(...)` |

`struct sockaddr_in` は公開面に出さず、独自型 `com_util_ipv4_endpoint` (ネットワーク バイト オーダーの address/port) で置換します。

### ソケット

対象ヘッダー: `com_util/net/socket.h`

IPv4 ソケットの生成、接続、送受信、待機は com_util のソケット API を使用します。

| 生 API | com_util 代替 |
|---|---|
| `socket()` | `com_util_socket_open(...)` |
| `close()` (POSIX) / `closesocket()` (Winsock) | `com_util_socket_close(...)` |
| `shutdown(SHUT_RDWR)` | `com_util_socket_shutdown(...)` |
| `bind()` | `com_util_socket_bind(...)` |
| `listen()` | `com_util_socket_listen(...)` |
| `accept()` | `com_util_socket_accept(...)` |
| `connect()` | `com_util_socket_connect(...)` (`EINPROGRESS`/`WSAEWOULDBLOCK` を `COM_UTIL_ERR_IN_PROGRESS` に統一) |
| `getsockopt(SO_ERROR)` | `com_util_socket_get_pending_error(...)` |
| `fcntl(O_NONBLOCK)` (POSIX) / `ioctlsocket(FIONBIO)` (Winsock) | `com_util_socket_set_nonblocking(...)` |
| `setsockopt(SO_REUSEADDR)` | `com_util_socket_set_reuse_address(...)` |
| `setsockopt(SO_BROADCAST)` | `com_util_socket_set_broadcast(...)` |
| `setsockopt(IP_MULTICAST_IF)` | `com_util_socket_set_multicast_interface(...)` |
| `setsockopt(IP_ADD_MEMBERSHIP)` / `IP_DROP_MEMBERSHIP` | `com_util_socket_join_multicast_group(...)` / `com_util_socket_leave_multicast_group(...)` |
| `send()` | `com_util_socket_send(...)` |
| `recv()` | `com_util_socket_recv(...)` |
| `sendto()` | `com_util_socket_sendto(...)` |
| `recvfrom()` | `com_util_socket_recvfrom(...)` |
| `poll()` (Linux) / `WSAPoll()` (Winsock) | `com_util_socket_wait_readable(...)` / `com_util_socket_wait_writable(...)` / `com_util_socket_wait_readable_multi(...)` |

`com_util_socket_send_all`/`com_util_socket_recv_all` は複数回の `send`/`recv` をループさせる合成 API です。  
Linux の `com_util_socket_send`/`com_util_socket_send_all` は、切断済みの接続への送信による SIGPIPE を送信単位で抑制し、送信エラーは結果コードと `com_util_error` で通知します。  
`com_util_socket_shutdown_receive` は Linux (`shutdown(SHUT_RD)`) と Windows (実質 close 相当) で意味論が異なる差異を吸収した複合 API です。

### プロセス内メモリ ロック

対象ヘッダー: `com_util/runtime/memory_lock.h`

| 生 API | com_util 代替 | 差異の要点 |
|---|---|---|
| `mlock()` (POSIX) / `VirtualLock()` (Win32) | `com_util_memory_lock_range(...)` | - |
| `munlock()` (POSIX) / `VirtualUnlock()` (Win32) | `com_util_memory_unlock_range(...)` | - |
| `explicit_bzero()` (glibc) / `SecureZeroMemory()` (Win32) | `com_util_secure_zero(...)` | コンパイラによるデッド ストア除去が起きないことを保証します。単なる `memset` の代替ではありません。 |
| `mlockall()` (Linux) | `com_util_memory_lock_self(...)` | Windows には相当 API が無いため、`VirtualQuery` で列挙し各領域へ `VirtualLock` を適用する合成実装 (参照カウント管理付き) |
| `munlockall()` (Linux) | `com_util_memory_lock_scope_release(...)` | Windows では各範囲へ `VirtualUnlock` を参照カウント管理で適用 |

scope API の設計や結果コードの詳細は [memory-lock.md](memory-lock.md) を参照してください。

### モジュール / プロセス情報

対象ヘッダー: `com_util/runtime/module.h`、`com_util/runtime/process.h`

| 生 API | com_util 代替 | 差異の要点 |
|---|---|---|
| `dladdr()` (POSIX) / `GetModuleHandleEx` +`GetModuleFileName` (Win32) | `com_util_module_get_path(...)` | 関数アドレスから所属モジュールの完全なパスを取得 |
| `readlink("/proc/self/exe")` (Linux) / `GetModuleFileName(NULL, ...)` (Win32) | `com_util_process_get_executable_path(...)` | - |
| `getpid()` (POSIX) / `GetCurrentProcessId()` (Win32) | `com_util_process_get_pid()` | - |
| `fork()` +`execve()` (POSIX) / `CreateProcess()` (Win32) | `com_util_process_start(...)` | stdio リダイレクト、環境変数上書き、作業ディレクトリ指定を共通オプション構造体 `com_util_process_options` に集約 |
| `waitpid()` (POSIX) / `WaitForSingleObject()` (Win32) | `com_util_process_wait(...)` | - |
| `WEXITSTATUS(status)` (POSIX) / `GetExitCodeProcess()` (Win32) | `com_util_process_get_exit_code(...)` | - |
| `kill(SIGKILL)` (POSIX) / `TerminateProcess()` (Win32) | `com_util_process_terminate(...)` | - |

`com_util_process_dispose`/`com_util_process_run_sync` はハンドル破棄、`start` +`wait` +`get_exit_code` の合成 API です。

### 暗号論的乱数

対象ヘッダー: `com_util/crypto/random.h`

| 生 API | com_util 代替 | 差異の要点 |
|---|---|---|
| `RAND_bytes()` (OpenSSL, Linux) / `BCryptGenRandom()` (CNG, Windows) | `com_util_random_bytes(...)` | 全バイト充足を保証 (部分成功なし)。`rand()` のような予測可能な擬似乱数の使用は明示的に禁止 |

### 暗号化・復号

対象ヘッダー: `com_util/crypto/crypto.h`

| 生 API | com_util 代替 | 差異の要点 |
|---|---|---|
| `EVP_EncryptInit_ex` +`EVP_EncryptUpdate` +`EVP_EncryptFinal_ex` (OpenSSL) / `BCryptEncrypt` (CNG) | `com_util_encrypt(...)` | AES-256-GCM 固定のワンショット API に集約し、暗号文+タグ連結フォーマットを標準化 |
| `EVP_DecryptInit_ex` +`EVP_DecryptUpdate` +`EVP_DecryptFinal_ex` (OpenSSL) / `BCryptDecrypt` (CNG) | `com_util_decrypt(...)` | 同上。タグ検証を内包 |

### 正規表現

対象ヘッダー: `com_util/regex/regex.h`

POSIX の照合 3 関数は、UTF-8 文字列を扱う com_util の正規表現 API へ置き換えます。

| 生 API | com_util 代替 |
|---|---|
| `regcomp()` | `com_util_regex_create(...)` |
| `regexec()` | `com_util_regex_search(...)` (部分一致) / `com_util_regex_matches(...)` (全体一致) |
| `regfree()` | `com_util_regex_dispose(...)` |

置換 (`com_util_regex_replace`)、イテレーター (`com_util_regex_iter_*`)、分割 (`com_util_regex_split`) は POSIX regex にない機能で、対応する単一の生 API はありません。  
フラグの対応関係、文字モデル、制限事項の詳細は [regex-guideline.md](regex-guideline.md) を参照してください。

### コンソール制御 (Windows 専用)

対象ヘッダー: `com_util/console/console.h`

| 生 API | com_util 代替 | 差異の要点 |
|---|---|---|
| `SetConsoleOutputCP` +`SetConsoleCP` +`SetConsoleMode(ENABLE_VIRTUAL_TERMINAL_PROCESSING)` (Win32) | `com_util_console_init(...)` | Linux では無処理。UTF-8 出力と VT100 制御シーケンスを有効化する複数 API 呼び出しを集約 |
| `AttachConsole()` (Win32) | `com_util_console_attach_parent(...)` | UAC 昇格後の親コンソール再接続に特化 |
| `WriteConsoleA` (Win32) / `write(fd, ...)` (POSIX) | `com_util_console_write(...)` | CRT ストリーム経由の `printf`/`fprintf` が失敗する既知の問題を回避するための直接書き込み |

`com_util_console_dispose()` は `com_util_console_init` が変更したコード ページ/モードを元に戻す後始末です。

### 圧縮・展開

対象ヘッダー: `com_util/compress/compress.h`

| 生 API | com_util 代替 | 差異の要点 |
|---|---|---|
| `deflateInit2` +`deflate` +`deflateEnd` (zlib, Linux) / `CreateCompressor` +`Compress` (Windows Compression API) | `com_util_compress(...)` | ワンショット API に集約。先頭 4 バイトへ元サイズ (ネットワーク バイト オーダー) を付加する独自フォーマットで統一 |
| `inflateInit2` +`inflate` +`inflateEnd` (zlib, Linux) / `CreateDecompressor` +`Decompress` (Windows Compression API) | `com_util_decompress(...)` | 同上 |

### DLL エクスポート マクロ (Windows 専用)

対象ヘッダー: `com_util/base/dll_exports.h`

関数ではなくプリプロセッサ マクロですが、生のコンパイラ拡張構文を直接使わせず経由させる 1 対 1 の対応です。

| 生の構文 | com_util 代替 |
|---|---|
| `__declspec(dllexport)` / `__declspec(dllimport)` (Win32)、`__attribute__((visibility("default")))` (Linux/GCC) | `COM_UTIL_DLL_EXPORT(prefix)` |
| `__stdcall` (Win32) | `COM_UTIL_DLL_API(prefix)` (Linux では空) |

リンク方式や動的ライブラリの成果物構成は [link-policy.md](link-policy.md) を参照してください。

### プラットフォームとアーキテクチャーの検出

本節と「コンパイラの検出とインライン制御」節が扱う分岐の書き方の規範は [platform-abstraction-guideline.md](platform-abstraction-guideline.md) を参照してください。

対象ヘッダー: `com_util/base/platform.h`

| 生の構文 | com_util 代替 |
|---|---|
| `#if defined(__linux__)` | `#ifdef PLATFORM_LINUX` |
| `#if defined(_WIN32) \|\| defined(_WIN64)` | `#ifdef PLATFORM_WINDOWS` |
| プラットフォーム名を文字列で得たい (`#if` で分岐して個別にリテラルを書く) | `PLATFORM_NAME` ("Windows"/"Linux"/"Unknown") |
| `#if defined(__x86_64__) \|\| defined(_M_X64)` | `#ifdef ARCH_X64` |
| `#if defined(__i386__) \|\| defined(_M_IX86)` | `#ifdef ARCH_X86` |
| アーキテクチャー名を文字列で得たい | `ARCH_NAME` ("x64"/"x86"/"Unknown") |

### コンパイラの検出とインライン制御

対象ヘッダー: `com_util/base/compiler.h`

| 生の構文 | com_util 代替 |
|---|---|
| `#if defined(_MSC_VER)` | `#ifdef COMPILER_MSVC` |
| `#if defined(__GNUC__) && !defined(__clang__)` | `#ifdef COMPILER_GCC` |
| コンパイラ名/バージョンを文字列や数値で得たい | `COMPILER_NAME` / `COMPILER_VERSION` |
| `__attribute__((always_inline))` (GCC) / `__forceinline` (MSVC) | `FORCE_INLINE` |
| `__attribute__((noinline))` (GCC) / `__declspec(noinline)` (MSVC) | `NO_INLINE` |
| `_Thread_local` (C, GCC) / `thread_local` (C++, GCC) / `__declspec(thread)` (MSVC) | `THREAD_LOCAL` |

### Windows SDK の取り込み順序 (Windows 専用)

対象ヘッダー: `com_util/base/windows_sdk.h`

| 生の構文 | com_util 代替 | 差異の要点 |
|---|---|---|
| `#include <windows.h>` を先に取り込む | `#include <com_util/base/windows_sdk.h>` | `winsock2.h`/`ws2tcpip.h` を `windows.h` より先に取り込む正しい順序を保証し、レガシ `winsock.h` との衝突を防ぐ |

## 独自機能 API (単一の生 API と 1 対 1 対応しない)

以下は com_util 独自の機能で、単一の生 API とは 1 対 1 で対応しません。  
「生の構文」列には、対応する生の関数がある場合はその名前を、ない場合は用途を記載します。  
「生の構文」列を主キーとした逆引きはできませんが、com_util が公開する API を用途から引けるようにするため掲載します。

### ハッシュ テーブル

対象ヘッダー: `com_util/hashtable/hashtable.h`

固定レコード数、固定ストレージ容量、遅延削除を持つハッシュ テーブルです。  
単一の標準 API とは対応しません。

キーと値は個別に固定長バイナリ、固定長 NUL 終端文字列、可変長 NUL 終端文字列を選択できます。  
可変長文字列では `key_storage_size` または `value_storage_size` で構築時の容量を指定し、自動拡張しません。  
断片化で連続領域が不足すると、合計空き容量が足りていても `COM_UTIL_ERR_STORAGE_FULL` です。  
必要に応じて `com_util_hashtable_compact` を明示的に呼び出してから再試行します。  
格納先の確保は、空き領域を管理する穴ディレクトリの先着適合で行い、空き領域の個数を H として O(H) です。  
断片化がなければ H は 1 のため、実質 O(1) で完了します。  
格納済みブロックの配置が変わるのは、`com_util_hashtable_compact`、`com_util_hashtable_resize`、`com_util_hashtable_rebuild_into` の 3 つです。  
前者は同一のストレージ内で、後の 2 つは移行先の領域へ詰め直します。  
確保は空き領域だけを使うため、対象レコード以外のブロックを動かしません。  
取得済みの可変長参照が無効になるのは、そのフィールドの更新・回収・再利用、`com_util_hashtable_compact`、`com_util_hashtable_clear`、`com_util_hashtable_resize`、`com_util_hashtable_rebuild_into`、`com_util_hashtable_dispose` です。  
穴ディレクトリは可変長ストレージ 1 個につき `capacity + 1` 要素を占め、必要バッファー サイズに含まれます。  
必要バッファー サイズは `com_util_hashtable_required_size` で求めてください。  
設計の背景は [hashtable 可変長ストレージ アロケーターの計算量改善](proposals/hashtable-storage-allocator.md) を参照してください。  
固定長バイナリ値は `value_size` バイトのバイト列として `memcpy` で授受します。  
値の格納境界は設定の `value_align` で決まります。  
既定の `0` では値を隙間なく並べ、値への参照を返す API も型のアラインメントを保証しないため、型付きポインターとして直接参照せず `memcpy` で取り出します。  
`value_align` に非 0 (2 の冪、`COM_UTIL_HASHTABLE_VALUE_ALIGN_MAX` 以下) を指定すると、値をその境界へ整列させ、`buf_data` にも同じ境界を要求します。  
この場合に限り、値への参照を境界を満たす型のポインターとして直接参照できます。  
可変長値では `value_align` は `0` でなければなりません。

固定長文字列を追加または更新するときは、NUL 終端文字列をそのまま渡します。  
未使用部分は内部で 0 埋めするため、呼び出し側で固定長まで fill する必要はありません。

可変長フィールドへの参照は、そのフィールドの更新、回収、再利用、または `compact`、`clear`、`dispose` まで有効です。  
別のレコードや別のフィールドを更新しただけでは無効になりません。  
安全に保持する場合は、固定長文字列と可変長文字列のどちらも `*_copy` を使います。  
`dest == NULL`、`dest_size == 0` で NUL を含む必要量を照会できます。  
固定長バイナリも `*_copy` で取得でき、必要量は設定した固定サイズです。

テーブルは変更時刻と世代カウンターの両方を持ちます。  
変更時刻は実時刻のため時計の巻き戻しで逆行しますが、世代カウンターは変更のたびに 1 ずつ増える単調な値です。  
変更の前後関係を判定する場合は世代カウンターを比較し、変更時刻は表示と外部システムとの相関に使います。

レコード番号は内部スロットの 1 相対番号であり、有効期間を持つ内部キーです。  
永続化した番号を別の機会に突き合わせる用途は対象外です。  
`com_util_hashtable_delete` で直ちに空へ戻ったレコード、`com_util_hashtable_push_deleted` で寿命に到達したレコード、`com_util_hashtable_purge_deleted` の削除済みレコード、`com_util_hashtable_clear` のすべて、`reuse_deleted` が非 0 のときに `com_util_hashtable_add` が追い出した削除済みレコード、および `com_util_hashtable_dispose` をまたぐと、番号は無効になりえます。  
`com_util_hashtable_find_recno` で得た番号は、これらを挟まない範囲で使います。  
空へ戻ったスロットと追い出されたスロットは、別のキーへ再利用されます。

| 用途 | com_util API |
|---|---|
| 必要バッファー サイズを求める | `com_util_hashtable_required_size` / `com_util_hashtable_buffer_size` |
| 管理中バッファーの先頭を得る | `com_util_hashtable_buffer_ref` |
| 構築 / 再接続 / 破棄 | `com_util_hashtable_create` / `com_util_hashtable_attach` / `com_util_hashtable_dispose` |
| 設定を読む | `com_util_hashtable_get_config_ref` / `com_util_hashtable_get_config_val` |
| 追加 / 更新 / 削除 | `com_util_hashtable_add` / `com_util_hashtable_update` / `com_util_hashtable_update_rec` / `com_util_hashtable_delete` / `com_util_hashtable_delete_rec` |
| 無ければ追加、あれば更新 | `com_util_hashtable_upsert`。新規追加か既存更新かは `inserted_out` で返る。重複も未登録もエラーにしない |
| 削除済み同一キーの追加方針 | `COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE` は新しい値で復活 / `COM_UTIL_HASHTABLE_ADD_DELETED_REVIVE` は削除前の値で復活 |
| 空きがない場合に削除済みを再利用 | 設定 `reuse_deleted` を非 0 にする。既定値は 0 |
| レコード番号を指定して直接書き込む | `com_util_hashtable_insert_direct` |
| 検索 | `com_util_hashtable_find_value_ref` / `com_util_hashtable_find_value_copy` / `com_util_hashtable_find_recno` / `com_util_hashtable_find_timestamp_ref` / `com_util_hashtable_find_timestamp_val` |
| レコード番号から読む | `com_util_hashtable_get_key_ref` / `com_util_hashtable_get_key_copy` / `com_util_hashtable_get_value_ref` / `com_util_hashtable_get_value_copy` / `com_util_hashtable_get_status` / `com_util_hashtable_get_timestamp_ref` / `com_util_hashtable_get_timestamp_val` |
| テーブル横断の変更時刻 | `com_util_hashtable_get_table_timestamp_ref` / `com_util_hashtable_get_table_timestamp_val` |
| 世代カウンターを読む | `com_util_hashtable_get_table_generation` / `com_util_hashtable_get_generation` / `com_util_hashtable_find_generation` |
| タイムスタンプと世代カウンターの粒度 | `COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_TABLE` / `COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_RECORD`。テーブル横断の世代カウンターは粒度に関わらず常に有効 |
| 固定長値の格納境界 | 設定 `value_align`。上限は `COM_UTIL_HASHTABLE_VALUE_ALIGN_MAX` |
| 占有状況 | `com_util_hashtable_count_status` / `com_util_hashtable_count` / `com_util_hashtable_deleted_count` / `com_util_hashtable_empty_count`。永続化領域の保持カウンターから取得するため、スロットを走査せず O(1) で完了 |
| 状態で絞って走査する | `com_util_hashtable_next_record`。`COM_UTIL_HASHTABLE_SCAN_IN_USE` / `_DELETED` / `_EMPTY` のビット和で対象を選ぶ。列挙の終了はエラーではなく `has_record_out` が 0 |
| レコード数とストレージ容量を変える | 内部確保は `com_util_hashtable_resize`、外部領域は `com_util_hashtable_rebuild_into` |
| 削除の加齢と回収 | `com_util_hashtable_push_deleted` / `com_util_hashtable_purge_deleted` / `com_util_hashtable_clear` |
| 可変長ストレージの明示的な圧縮 | `com_util_hashtable_compact`。キーと値を一括して圧縮し、取得済みの可変長参照を無効化。capacity を N、空き領域の個数を H、使用バイト数を U として O(N log H + U) |
| 寿命無限 | `COM_UTIL_HASHTABLE_LIFETIME_INFINITE` |
| 整合性検査 | `com_util_hashtable_validate` |

`com_util_hashtable_add` と `com_util_hashtable_insert_direct` は、既存キーとの衝突を `COM_UTIL_ERR_DUPLICATE_KEY` で通知します。  
`com_util_hashtable_insert_direct` の指定スロットが空でない場合は、`COM_UTIL_ERR_DUPLICATE_DEFINITION` です。

`com_util_hashtable_attach` は、呼び出し引数の不正を `COM_UTIL_ERR_INVALID_ARGUMENT`、領域の容量不足を `COM_UTIL_ERR_BUFFER_TOO_SMALL`、保存ヘッダーの不正を `COM_UTIL_ERR_CORRUPT_DESCRIPTOR` で通知します。

`com_util_hashtable_resize` と `com_util_hashtable_rebuild_into` で変えられるのは、`capacity`、`key_storage_size`、`value_storage_size` の 3 つだけです。  
ほかの項目が現在の設定と異なる場合は `COM_UTIL_ERR_INVALID_ARGUMENT` です。  
拡大ではレコード番号を保存し、縮小では保存しません。縮小後は `com_util_hashtable_find_recno` で取り直します。  
レコードは 1 件も捨てません。使用中が収まらない場合、および `reuse_deleted` が `0` で削除済みが収まらない場合は `COM_UTIL_ERR_LIMIT_EXCEEDED` です。  
`reuse_deleted` が非 0 のときだけ、`com_util_hashtable_add` の追い出しと同じ規則で削除済みを古い順に空へ戻します。  
可変長ストレージは移行時に詰め直し、詰めた後の使用量が新しい容量を超える場合は `COM_UTIL_ERR_STORAGE_FULL` です。  
`com_util_hashtable_resize` は内部確保のテーブル専用で、外部領域のテーブルには `COM_UTIL_ERR_UNSUPPORTED` を返します。

### スレッドと同期プリミティブ

対象ヘッダー: `com_util/sync/sync.h`

| 生の構文 | com_util API | 補足 |
|---|---|---|
| `pthread_mutex_t` (POSIX) / `CRITICAL_SECTION` (Win32) | `com_util_local_lock_create` / `com_util_local_lock_lock` / `com_util_local_lock_try_lock` / `com_util_local_lock_unlock` / `com_util_local_lock_dispose` | プロセス内ミューテックス。`lock` はタイムアウト付き |
| `pthread_cond_t` (POSIX) / `CONDITION_VARIABLE` (Win32) | `com_util_condvar_create` / `com_util_condvar_wait` / `com_util_condvar_signal` / `com_util_condvar_broadcast` / `com_util_condvar_dispose` | プロセス内条件変数 |
| `pthread_rwlock_t` (POSIX) / `SRWLOCK` (Win32) | `com_util_local_rwlock_create` / `com_util_local_rwlock_lock_shared` / `com_util_local_rwlock_lock_exclusive` / `com_util_local_rwlock_try_lock_shared` / `com_util_local_rwlock_try_lock_exclusive` / `com_util_local_rwlock_unlock_shared` / `com_util_local_rwlock_unlock_exclusive` / `com_util_local_rwlock_dispose` | プロセス内読み書きロック |
| `pthread_create` +`pthread_join` +`pthread_detach` (POSIX) / `CreateThread` +`WaitForSingleObject` +`CloseHandle` (Win32) | `com_util_thread_create` / `com_util_thread_join` / `com_util_thread_detach` | スレッド生成・待機・切り離し |
| 名前付き `flock`/セマフォ (POSIX) / 名前付き `Mutex` (Win32) | `com_util_interprocess_lock_open` / `com_util_interprocess_lock_lock` / `com_util_interprocess_lock_try_lock` / `com_util_interprocess_lock_unlock` / `com_util_interprocess_lock_dispose` | プロセス横断ミューテックス。プロセス間受け渡しには `_export_descriptor` / `_import_descriptor` を使用 |
| プロセス横断読み書きロックを取得したい (POSIX/Win32 に単一の生 API なし) | `com_util_interprocess_rwlock_open` / `_lock_shared` / `_lock_exclusive` / `_try_lock_shared` / `_try_lock_exclusive` / `_unlock` / `_destroy` | ロック ファイル バックエンドで実装 |
| `pthread_once` (POSIX) / `InitOnceExecuteOnce` (Win32) | `com_util_call_once(flag, func)` | - |
| `sleep`/`usleep`/`nanosleep` (POSIX) / `Sleep` (Win32) | `com_util_sleep_ms(ms)` | Linux はシグナル割り込み時に残り時間を再計算し継続待機します。 |

### コマンド ライン引数の解析

対象ヘッダー: `com_util/argparser/argparser.h`

`getopt`/`getopt_long` (POSIX) に相当する機能を、Windows でも同じ API で使えるように提供します。

| 用途 | com_util API |
|---|---|
| パーサーを生成/初期化します。 | `com_util_argparser_create` / `com_util_argparser_default_init` / `com_util_argparser_default` / `com_util_argparser_dispose` |
| フラグ/オプション/位置引数を登録します。 | `com_util_argparser_default_register_flag` / `_register_option_int` / `_register_option_int_array` / `_register_option_string` / `_register_option_string_array` / `_register_positional_int` / `_register_positional_int_array` / `_register_positional_string` / `_register_positional_string_array` |
| 登録エラーを確認します。 | `com_util_argparser_default_get_register_error` / `_get_register_error_count` / `_get_register_error_target` / `_get_register_error_message` / `_print_register_error_messages` |
| コマンド ラインを解析します。 | `com_util_argparser_default_parse` |
| 解析エラーを確認します。 | `com_util_argparser_default_get_error` / `_get_error_index` / `_get_error_target` / `_get_error_message` / `_print_error_messages` |
| 使用方法を表示します。 | `com_util_argparser_default_get_usage` / `com_util_argparser_default_print_usage` |

### 対話的プロンプト

対象ヘッダー: `com_util/prompt/prompt.h`、`com_util/prompt/pinned_prompt.h`

`fgets(stdin)`/`readline` (POSIX) や `ReadConsole` (Win32) を直接使う代わりに使用します。

| 用途 | com_util API |
|---|---|
| 1 行の対話的入力を読み取る | `com_util_prompt_create` / `com_util_prompt_readline` / `com_util_prompt_readline_at` / `com_util_prompt_readline_fmt` / `com_util_prompt_readline_fmt_at` / `com_util_prompt_dispose` |
| 画面下部に固定したプロンプトへ入力しつつ、その上にログを流す | `com_util_pinned_prompt_create` / `com_util_pinned_prompt_readline` / `com_util_pinned_prompt_readline_fmt` / `com_util_pinned_prompt_write` / `com_util_pinned_prompt_printf` / `com_util_pinned_prompt_status_enable` / `com_util_pinned_prompt_status_set` / `com_util_pinned_prompt_dispose` |

### トレース

対象ヘッダー: `com_util/trace/tracer.h`、`com_util/trace/trace_file.h`、`com_util/trace/syslog.h`、`com_util/trace/eventlog.h`、`com_util/trace/etw.h`

Linux syslog、Windows イベント ログ、ETW を個別に呼び出す代わりに、共通のトレーサー API へ集約します。

| 用途 | com_util API |
|---|---|
| トレーサーを生成しレベル別に出力を開始/停止します。 | `com_util_tracer_create` / `com_util_tracer_start` / `com_util_tracer_stop` / `com_util_tracer_dispose` |
| 出力先ごとのレベルを設定/取得する (標準エラー、ファイル、OS ログ、ETW) | `com_util_tracer_set_stderr_level` / `_get_stderr_level` / `_set_file_level` / `_get_file_level` / `_set_os_level` / `_get_os_level` / `_set_etw_level` / `_get_etw_level` |
| トレーサーの名前/識別子/ファイル名を設定/取得します。 | `com_util_tracer_set_name` / `_get_name` / `_get_identifier` / `_set_file_name` / `_get_file_name` / `_get_file_identifier` |
| メッセージを出力する (ソース ファイル名/行番号を自動付与するマクロ) | `com_util_tracer_write` / `com_util_tracer_writef` |
| バイナリ データを 16 進数で出力する (同上) | `com_util_tracer_write_hex` / `com_util_tracer_write_hexf` |
| `va_list` 版や、ソース位置を自前で制御したい場合の低レベル関数 | `com_util_tracer_writef_at` / `com_util_tracer_vwritef_at` / `com_util_tracer_write_hexf_at` / `com_util_tracer_vwrite_hexf_at` |
| 出力フックを追加/削除します。 | `com_util_tracer_set_hook` / `com_util_tracer_call_next_hook` / `com_util_tracer_remove_hook` |
| トレーサーの状態を取得します。 | `com_util_tracer_get_state` |
| syslog (RFC5424 系、Linux 専用) の書き込みシンクを扱います。 | `com_util_syslog_sink_create` / `com_util_syslog_sink_write` / `com_util_syslog_sink_rename` / `com_util_syslog_sink_dispose` |
| Windows イベント ログの書き込みシンクを扱います。 | `com_util_eventlog_register_source` / `com_util_eventlog_sink_create` / `com_util_eventlog_sink_write` / `com_util_eventlog_sink_dispose` / `com_util_eventlog_unregister_source` |
| ETW (TraceLogging) の書き込みシンクを扱います。 | `com_util_etw_provider_create` / `com_util_etw_provider_write` / `com_util_etw_provider_dispose` / `com_util_etw_session_start` / `com_util_etw_session_stop` / `com_util_etw_session_check_access` |
| ローテーション付きファイルへ同期書き込みするシンクを扱います。 | `com_util_trace_file_sink_create` / `com_util_trace_file_sink_write` / `com_util_trace_file_sink_dispose` |

### 管理者権限の確認と昇格

対象ヘッダー: `com_util/runtime/elevated_process.h`

`GetTokenInformation(TokenElevation)`/`geteuid() == 0` の判定 (実 UID を返す `getuid() == 0` による誤判定を含む) や、UAC による自己再起動を直接書く代わりに使用します。  
`uid == 0` という判定方法自体が Windows には存在しないため、判定の目的である「管理者権限を保有しているか」に着目し、生 API の違いを個別に吸収するのではなく本 API へ抽象化してください。

| 用途 | com_util API |
|---|---|
| 現在のプロセスが管理者権限で実行されているか確認します。 | `com_util_elevated_process_is_elevated` |
| 必要な場合のみ管理者権限で自己を再起動します。 | `com_util_elevated_process_run_if_needed` / `com_util_elevated_process_run_with_result` |
| 昇格プロセスの実行結果を受け渡しします。 | `com_util_elevated_process_extract_result_target` / `com_util_elevated_process_report_result` |

### プロセス終了時の共通フック

対象ヘッダー: `com_util/runtime/shutdown.h`

`atexit`、シグナル ハンドラー、`SetConsoleCtrlHandler` を個別に登録する代わりに使用します。

| 用途 | com_util API |
|---|---|
| 終了コードを保ったままプロセスを終了します。 | `com_util_exit(code)` |
| 終了時に呼び出されるコールバックを LIFO で登録します。 | `com_util_shutdown_register` |
| Ctrl+C 等の終了要求を受け取るコールバックを登録します。 | `com_util_shutdown_request_register` |

`com_util_shutdown_invoke_for_test` / `com_util_shutdown_request_invoke_for_test` / `com_util_shutdown_reset_for_test` はテスト専用で、本番コードからは呼び出しません。

### 動的シンボル解決

対象ヘッダー: `com_util/runtime/sym_loader.h`

`dlopen` +`dlsym` (POSIX) / `LoadLibrary` +`GetProcAddress` (Win32) を直接呼ぶ代わりに使用します。  
JSON 設定ファイルからのライブラリ名解決、関数ポインターのキャッシュ、スレッド セーフな初期化を内包します。

| 用途 | com_util API |
|---|---|
| 関数を動的に解決します。 | `com_util_sym_loader_resolve` (型安全なラッパー マクロ `com_util_sym_loader_resolve_as`) |
| ローダーを初期化/破棄します。 | `com_util_sym_loader_init` / `com_util_sym_loader_dispose` |
| 既定ローダーかどうか確認します。 | `com_util_sym_loader_is_default` |
| 解決状況の情報を取得します。 | `com_util_sym_loader_info` |

### 共有ライブラリのロード/アンロード フック

対象ヘッダー: `com_util/base/shared_lib_lifecycle.h`

`__attribute__((constructor/destructor))` (Linux/GCC) と `DllMain` (Win32) という異なる仕組みを直接書く代わりに、共通のコールバック規約に統一します。  
エクスポートされる関数はなく、呼び出し側が `onLoad`/`onUnload` を実装するヘッダー オンリーの仕組みです。

## 公開関数名の補助索引

前節までの表や説明でまとめて扱った公開関数を、完全な関数名から検索できるように補足します。  
シグネチャを確認する場合は、関数名に対応する [`prod/include/`](../prod/include/) 配下のヘッダーを参照してください。

- 引数解析: `com_util_argparser_default_register_option_int`、`com_util_argparser_default_register_option_string`、`com_util_argparser_default_register_option_int_array`、`com_util_argparser_default_register_option_string_array`、`com_util_argparser_default_register_positional_int`、`com_util_argparser_default_register_positional_int_array`、`com_util_argparser_default_register_positional_string_array`、`com_util_argparser_default_get_error_target`、`com_util_argparser_default_get_error_index`、`com_util_argparser_default_get_error_message`、`com_util_argparser_default_print_error_messages`、`com_util_argparser_default_get_register_error_count`、`com_util_argparser_default_get_register_error_target`、`com_util_argparser_default_get_register_error_message`、`com_util_argparser_default_print_register_error_messages`
- 時刻: `com_util_format_realtime_iso8601_local`、`com_util_format_realtime_iso8601_utc`、`com_util_get_realtime_utc`、`com_util_get_realtime_deadline_ms`、`com_util_timespec_normalize`、`com_util_timespec_add`、`com_util_timespec_sub`、`com_util_timespec_cmp`、`com_util_timespec_add_ms`、`com_util_timespec_diff_ms`、`com_util_timespec_to_native`、`com_util_timespec_from_native`
- ファイル: `com_util_file_init`、`com_util_file_open`、`com_util_file_write`、`com_util_file_read`、`com_util_file_get_size`、`com_util_file_set_size`、`com_util_file_get_id`、`com_util_file_get_path_id`、`com_util_file_flush`、`com_util_file_close`
- パス: `com_util_normalize_path_sep`、`com_util_path_get_full`、`com_util_paths_equal`、`com_util_get_temp_dir`、`com_util_path_concat_n`、`com_util_vpath_concat_n`、`com_util_path_basename`、`com_util_path_dirname`、`com_util_path_extension`、`com_util_path_strip_extension`、`com_util_path_join_n`、`com_util_vpath_join_n`
- 書式入力: `com_util_vscanf`、`com_util_vfscanf`、`com_util_vsscanf`
- 暗号: `com_util_passphrase_to_key`
- エラー: `com_util_error_clear`、`com_util_error_capture_errno`、`com_util_error_capture_current_errno`、`com_util_error_get_last`、`com_util_error_set_last`、`com_util_error_clear_last`、`com_util_error_is_set`、`com_util_error_get_domain`、`com_util_error_get_errno`、`com_util_error_to_result`、`com_util_error_get_cause`、`com_util_error_is`、`com_util_result_to_string`
- メモリ マップド ファイル: `com_util_mmap_get_address`、`com_util_mmap_get_size`、`com_util_mmap_get_rwlock`
- 正規表現: `com_util_regex_get_group_count`、`com_util_regex_iter_create`、`com_util_regex_iter_next`、`com_util_regex_iter_dispose`
- モジュール: `com_util_module_get_basename`
- プロセス間ロック: `com_util_interprocess_lock_export_descriptor`、`com_util_interprocess_lock_import_descriptor`、`com_util_interprocess_rwlock_export_descriptor`、`com_util_interprocess_rwlock_import_descriptor`、`com_util_interprocess_rwlock_lock_shared`、`com_util_interprocess_rwlock_try_lock_shared`、`com_util_interprocess_rwlock_lock_exclusive`、`com_util_interprocess_rwlock_try_lock_exclusive`、`com_util_interprocess_rwlock_unlock`、`com_util_interprocess_rwlock_dispose`
- トレース: `com_util_tracer_get_name`、`com_util_tracer_get_identifier`、`com_util_tracer_set_file_name`、`com_util_tracer_get_file_name`、`com_util_tracer_get_file_identifier`、`com_util_tracer_get_os_level`、`com_util_tracer_set_os_level`、`com_util_tracer_get_etw_level`、`com_util_tracer_set_etw_level`、`com_util_tracer_get_file_level`、`com_util_tracer_set_file_level`、`com_util_tracer_get_stderr_level`
- Windows 文字列変換: `com_util_utf8_to_wpath`、`com_util_wpath_to_utf8`、`com_util_utf8_to_wstr_alloc`、`com_util_wstr_to_utf8_alloc`
- Windows エラー: `com_util_error_capture_windows_error`、`com_util_error_capture_current_windows_error`、`com_util_error_get_windows_error`

## 検証

対象関数の残存を確認する場合は、`coding-guideline.md` の「[検証](coding-guideline.md#検証)」節にある grep コマンドを使用します。  
危険な標準関数のコマンドはそのまま使用でき、プラットフォーム抽象ラッパー分を追加確認する場合は次のように対象関数を拡張します。

```bash
# プラットフォーム抽象ラッパー対象関数の直接使用確認 (例)
grep -rnE '\b(fopen|freopen|fclose|fflush|fread|fwrite|remove|rename|fprintf|vfprintf|fseek|ftell|open|lseek|close|dup|dup2|read|write|access|isatty|stat|mkdir|rmdir|getenv|setenv|unsetenv|gmtime|localtime|ctime)[[:space:]]*\(' \
  app --include=*.c --include=*.h \
  | grep -vE 'app/(lua|sqlite|cjson)/|/obj/|doxybook2_' \
  | grep -vE 'com_util_(fopen|freopen|fclose|fflush|fread|fwrite|remove|rename|fprintf|vfprintf|fseek|ftell|open|lseek|close|dup|dup2|read|write|access|isatty|stat|mkdir|rmdir|getenv|setenv|unsetenv|gmtime|localtime|ctime)' \
  | grep -vE ':[[:space:]]*\*'
```

com_util 自身のラッパー実装 (`prod/libsrc/com_util/crt/`) は、元関数へ委譲するため検出されます。  
[coding-guideline.md の「ラッパーがある関数の使用」](coding-guideline.md#ラッパーがある関数の使用) の例外に当たるため、対象外として扱います。
