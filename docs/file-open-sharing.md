# com_util_file の共有モードを対称化した設計

## 背景

Windows の `CreateFile` は、既定では他プロセスからのアクセスを一切許可しません。  
他プロセスに読み取り/書き込み/削除を許可するには、呼び出し側が `dwShareMode` (`FILE_SHARE_READ` / `FILE_SHARE_WRITE` / `FILE_SHARE_DELETE`) を明示的に指定する必要があります。

一方 Linux の `open()` には、この「他プロセスへの共有可否」を制御する概念自体が存在しません。  
同じファイルは常に他プロセスから自由に `open()` でき、排他制御が必要な場合は `flock()`/`fcntl()` によるアドバイザリ ロックを別途取得します (`com_util_interprocess_lock`/`com_util_interprocess_rwlock` がこれに相当します)。

`com_util_file_open()` は従来、この Windows 固有の概念を `COM_UTIL_FILE_OPEN_SHARE_READ` / `COM_UTIL_FILE_OPEN_SHARE_WRITE` / `COM_UTIL_FILE_OPEN_SHARE_DELETE` という 3 つのフラグで公開していました。しかし呼び出し側の多く (`trace_file.c` の両モード、`mmap` モジュールの両アクセス モード) が、これらを実質的に無条件で指定しており、また `com_util_file_open()` を無指定で呼ぶ既存コードは Windows でのみ排他アクセスになってしまうという非対称な既定動作も抱えていました。

## 決定事項

`COM_UTIL_FILE_OPEN_SHARE_READ` / `SHARE_WRITE` / `SHARE_DELETE` を公開 API (`crt/file.h`) から削除しました。`com_util_file_open()` は Windows でも常に `FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE` を指定してオープンします。  
これにより、Linux の既定動作 (常にフル共有) と Windows の挙動が完全に対称になります。

## トレードオフ

`app/com_util/prod/libsrc/com_util/trace/backends/file/trace_file.c` の単一プロセス モードは、従来この非対称性を「安全機構」として積極的に利用していました。単一プロセス モードでは `COM_UTIL_FILE_OPEN_SHARE_WRITE` を指定しないことで、Windows の排他書き込みにより複数プロセスが誤って同じログ ファイルへ同時書き込みしてしまう事故を OS レベルで防止していました。

今回の変更により、この OS レベルの保護は失われます。Linux では元々この保護が存在しなかったため、「Linux で機能していなかった保護は Windows でも提供しない」という対称性を優先する判断です。

複数プロセスからの同時書き込みを防ぎたい場合は、OS の排他アクセスに頼らず、呼び出し側が `com_util_interprocess_lock`/`com_util_interprocess_rwlock` (`sync.h`) で明示的に排他制御してください。`trace_file.c` の共有モード (`COM_UTIL_TRACE_FILE_SINK_SHARED`) は、もともとローテーション時の調整に `"<path>.lock"` というプロセス間ロック ファイルを使用しており、この仕組みには影響しません。

## 影響を受けたファイル

| パス | 変更内容 |
|---|---|
| `app/com_util/prod/include/com_util/crt/file.h` | `SHARE_*` フラグを削除。残るフラグのビット番号を詰め直し。 |
| `app/com_util/prod/libsrc/com_util/crt/file.c` | Windows の `share_mode` を常時フル共有に固定。 |
| `app/com_util/prod/libsrc/com_util/trace/backends/file/trace_file.c` | `base_open_flags()` から `SHARE_*` 分岐を削除。 |
| `app/com_util/prod/libsrc/com_util/mmap/mmap_linux.c` / `mmap_windows.c` | `open_backing_file()` から `SHARE_*` 指定を削除。 |
