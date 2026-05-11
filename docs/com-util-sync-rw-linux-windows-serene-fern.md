# プロセス横断 RW ロックを「ファイルに依存しない」中央集中型へ移行する設計

## Context

`app/com_util/prod/libsrc/com_util/sync/` の `com_util_interprocess_rwlock_*` は、
Linux/Windows のいずれも **ファイル パスを識別子に取り、当該ファイルに対する
カーネル提供のファイル ロック (`flock` / `LockFileEx`) を共有/排他で取得する** 実装になっている。

- Linux: `open(identity, O_RDWR|O_CREAT|O_CLOEXEC, 0666)` + `flock(fd, LOCK_SH|LOCK_EX)`
  (`sync_linux.c:159, 237, 251, 285`)
- Windows: `CreateFileA(identity, GENERIC_READ|GENERIC_WRITE, ...)` + `LockFileEx`
  (`sync_windows.c:119, 156, 196, 230`)
- 現状この API を呼び出すコードは com_util のテスト/モック以外には存在しない

ユーザー要件 (確認済):

1. **最終的に数千のロックを管理可能にしたい**
2. **プロセス クラッシュ時のロック自動解放は必須**
3. アプリ可視ロック ファイル (a) と FS 名前空間エントリ (b) の両方を排除したい
4. **専用マネージャ プロセスは増やさず、共有メモリ上の集中データ構造で実現する (案 R-1)**

「(a)+(b) を排除しつつ数千ロックを扱う」を 1 対 1 の OS プリミティブ割り当てで実現すると、
Linux 側で FD 上限 (`ulimit -n` 既定 1024) や `/dev/shm/` エントリ膨張という別の壁に当たる。
したがって **「共有メモリ 1 セグメント + その中の論理ロック表」** という構成が必須となる。

---

## アーキテクチャ概要

- 全参加プロセスが対等。専用デーモンは増やさない。
- 「ロック ドメイン」 1 つにつき、共有メモリ セグメント 1 個を持つ。
  - Linux: `shm_open("/com_util_lkdomain_<DOMAIN>", O_RDWR|O_CREAT, 0660)` + `ftruncate` + `mmap`
  - Windows: `CreateFileMappingA(INVALID_HANDLE_VALUE, ..., "Local\\com_util_lkdomain_<DOMAIN>")` +
    `MapViewOfFile`
- セグメント内に「マスター ロック (robust)」と「論理ロック表 (数千 entries)」を置く。
- 待機は条件変数プール (Linux: PROCESS_SHARED `pthread_cond_t` × K 個 /
  Windows: 名前付き auto-reset Event × K 個) を ID ハッシュで割当て。
- 識別子 (`identity` 文字列) はロック ドメイン内の論理名としてハッシュ参照する。
- 参加プロセスはロック ドメインに attach した時点で `ref_count++`、detach 時 `ref_count--`。
  最後の detach 時にセグメント自体を破棄 (`shm_unlink` / 全 HANDLE クローズ)。

---

## ロック ドメインの命名と発見

現 API では `identity` がファイル パスを想定したが、新方式では「ドメイン名 + ロック名」の
2 階層が必要。後方互換のために以下のルールで識別子を解釈する。

- `identity` に `'\0'` 区切りなし: ライブラリ既定ドメイン (環境変数 `COM_UTIL_LOCK_DOMAIN`
  または UID + 実行バイナリ パスのハッシュから計算) を使用し、`identity` 全体を論理ロック名とする
- `identity` が `"DOMAIN/LOCK"` 形式: 明示分離
- (検討項目) パス区切りで悩ましいケースは `identity` 先頭 1 バイトを区切り選択子にする等の運用ルールで吸収

ドメイン名は最終的に OS の名前空間名 (POSIX shm 名 / NT 名前空間名) になるため、
英数記号の制限が出る。ライブラリ内部で sanitize + ハッシュ化する関数を入れる。

---

## 共有メモリ レイアウト

```
+--------------------------------------------------------------+
| segment_header  (cacheline aligned, 固定 1 個)               |
|   magic            : "CUSL"                                  |
|   layout_version   : 1                                       |
|   total_size       : sizeof(segment) byte                    |
|   ref_count        : 参加プロセス数 (atomic uint32)          |
|   manager_mutex    : robust pthread_mutex / Win Mutex name   |
|   condvar_count    : K (例: 64)                              |
|   condvar_pool[K]  : 待機用                                  |
|   table_size       : N (例: 4096)                            |
|   free_list_head   : 空きエントリの先頭 index                |
|   hash_buckets[B]  : 識別子ハッシュ → entry index (B≒N/2)    |
|   string_pool_size : 識別子文字列領域サイズ                  |
+--------------------------------------------------------------+
| lock_table[N]                                                |
|   per-entry (固定サイズ):                                    |
|     state          : FREE / IN_USE                           |
|     identity_hash  : uint64                                  |
|     identity_off   : string_pool 内オフセット                |
|     identity_len   : uint16                                  |
|     reader_count   : uint32                                  |
|     writer_pid     : pid_t / DWORD (0 なら writer 不在)      |
|     writer_serial  : プロセス起動シリアル (PID リユース判定) |
|     readers[R_MAX] : (pid, serial) の小配列 (R_MAX 例:8)     |
|     readers_overflow_off : 拡張領域 off (8 を超える場合)     |
|     waiter_count   : 統計用                                  |
|     cond_var_idx   : 0..K-1                                  |
|     hash_next      : ハッシュ チェーン                       |
|     refcount       : このエントリを open しているハンドル数  |
+--------------------------------------------------------------+
| string_pool / overflow_pool (slab allocator)                 |
+--------------------------------------------------------------+
```

固定サイズで設計し、`mmap` / `MapViewOfFile` 後はそのまま使う。サイズ パラメータ (N, K, B,
R_MAX, string_pool) は環境変数または初回作成時の hint で決定し、ヘッダーに記録する
(後で attach するプロセスはヘッダーを信用)。

---

## マスター ロックと復旧

### Linux

- `manager_mutex` を `pthread_mutexattr_setpshared(PTHREAD_PROCESS_SHARED)` +
  `pthread_mutexattr_setrobust(PTHREAD_MUTEX_ROBUST)` で初期化
- 取得時:
  - `pthread_mutex_lock(&manager_mutex)`
  - 返値 `EOWNERDEAD` → 前保持者が死亡。スイープ (後述) を実行し
    `pthread_mutex_consistent(&manager_mutex)` で整合化
  - 返値 `ENOTRECOVERABLE` → セグメント全体を放棄 (新 generation で再作成)
- 条件変数も `pthread_condattr_setpshared(PTHREAD_PROCESS_SHARED)` で PROCESS_SHARED に。
  `pthread_condattr_setclock(CLOCK_MONOTONIC)` を併用してタイムアウト計算を `CLOCK_MONOTONIC` に統一
  (現状の `monotonic_deadline` と整合)

### Windows

- `manager_mutex` は名前付き Mutex (`CreateMutexA("Local\\com_util_lkdomain_<DOMAIN>_mutex")`)
- 取得時:
  - `WaitForSingleObject(manager_mutex, ms)` の結果が `WAIT_ABANDONED_0` → 前保持者死亡。
    スイープ実行後、所有を取って進める (Windows は自動的に正常所有状態に戻る)
- 待機は名前付き auto-reset Event のプール (`CreateEventA` で K 個)。
  公平性: 起床通知時に「全部 set してから cond を再評価」(thundering herd) または
  「FIFO 待機 ID キューを共有メモリに持ち、特定 Event を狙って set」 のどちらかを選択。
  当面は前者の単純実装で良い。

---

## プロセス生存検査

論理ロックの owner として記録された PID が、マスター ロック取得後の検査時点で
本当に生きているかを確認する必要がある。検査は (a) スイープ時、(b) ロック取得待ちが
タイムアウトに近づいた時、に行う。

### Linux

- 第一選択: `pidfd_open(pid, 0)` (Linux 5.3+)
  - 成功 → `poll({.fd=pidfd, .events=POLLIN}, 1, 0)` で 0 返却 = 生存、
    POLLIN 立つ = 終了済
  - `pidfd_open` 失敗 (`ESRCH`) = 死亡
- フォールバック (古いカーネル): `/proc/<pid>/stat` から開始時刻 (`starttime` フィールド) を
  読み、エントリに記録した `writer_serial` (= 開始時刻) と一致するか確認
  - PID リユース耐性のために `writer_serial` には必ず開始時刻を入れる
- 検査結果のキャッシュ: マスター ロック保持中は同一 PID を複数回検査することがあるので
  一時マップに乗せる (関数呼び出しスタック内のローカル領域)

### Windows

- `OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid)`
- ハンドル取得失敗 = 死亡
- 取得できたら `GetExitCodeProcess(handle, &code)` で `code != STILL_ACTIVE` を判定。
  ただし STILL_ACTIVE(259) の終了コード偽陽性を避けるため、`WaitForSingleObject(handle, 0)` で
  `WAIT_OBJECT_0` (シグナル) を確認する方が確実
- PID リユース対策: `GetProcessTimes` で取得した `CreationTime` を `writer_serial` に格納
  (FILETIME を 64bit にして比較)

---

## スイープ ロジック

マスター ロック保持中に呼ばれる。

```
for each entry in lock_table where state == IN_USE:
    if entry.writer_pid != 0:
        if !is_alive(entry.writer_pid, entry.writer_serial):
            entry.writer_pid = 0
            wakeup_waiters(entry.cond_var_idx)
    for each reader (pid, serial) in entry.readers:
        if !is_alive(pid, serial):
            remove from entry.readers
            entry.reader_count--
            if entry.reader_count == 0 and entry.writer_pid == 0 and waiter_count > 0:
                wakeup_waiters(entry.cond_var_idx)
    if entry.reader_count == 0 and entry.writer_pid == 0 and entry.refcount == 0:
        return to free_list
```

スイープを毎回の `lock` で実行するとコストが高い。最適化:

- **遅延スイープ**: 取得待ちでタイムアウトに近づいたとき / 取得に明確に失敗したとき
  のみスイープ
- **`EOWNERDEAD` 経路では必ず全表スイープ** (マスター ロック自体の保持者が死んでいるため)
- **エントリ単位のスイープ**: ロック取得対象のエントリだけ生存検査

reader 配列が固定長 R_MAX (例 8) を超える場合は overflow 領域へリンク。
通常運用では R_MAX で十分のはず (1 ロックを 8 プロセスが同時 read することは稀)。

---

## API 互換性

ヘッダー `sync.h` の関数シグネチャはすべて維持可能。

- `com_util_interprocess_rwlock_open(identity, &lock)` — identity 解釈を上記ルールで変える
- `lock_shared / lock_exclusive / unlock` — 内部実装のみ変更
- `export_descriptor / import_descriptor` — 識別子文字列をシリアライズする現方式を維持
  (新方式でも identity 文字列で一意特定できるため)
- `com_util_interprocess_sync_backend_t` の `COM_UTIL_INTERPROCESS_SYNC_BACKEND_LOCK_FILE` は
  enum 値を残しつつ、新規 `COM_UTIL_INTERPROCESS_SYNC_BACKEND_SHARED_TABLE` を追加して
  既定をこちらに切り替える (バックエンド選択 API が将来必要なら拡張ポイントになる)

エラー コード:

- 新規 `COM_UTIL_SYNC_DEAD_OWNER_RECOVERED` (`EOWNERDEAD` 起因で復旧した直後の通知。
  オプション。クライアントが「以前保持されていた状態が一貫しているか」を再検査するヒント)
- 既存 `COM_UTIL_SYNC_CORRUPT_DESCRIPTOR` を `ENOTRECOVERABLE` 経路でも使用

---

## ファイル変更計画

| パス | 変更概要 |
|---|---|
| `app/com_util/prod/include/com_util/sync/sync.h` | enum 拡張、エラー コード追加 (互換維持) |
| `app/com_util/prod/include_internal/com_util/sync/lock_manager_internal.h` (新規) | 共有メモリ レイアウト構造体、内部 API 宣言 |
| `app/com_util/prod/libsrc/com_util/sync/lock_manager.c` (新規) | OS 非依存ロジック (ハッシュ、スイープ、API アダプタ) |
| `app/com_util/prod/libsrc/com_util/sync/lock_manager_linux.c` (新規) | shm_open/mmap、robust mutex、pidfd、pthread_cond |
| `app/com_util/prod/libsrc/com_util/sync/lock_manager_windows.c` (新規) | CreateFileMapping、名前付き Mutex/Event、OpenProcess |
| `app/com_util/prod/libsrc/com_util/sync/sync_linux.c` | `interprocess_rwlock_*` を lock_manager 呼び出しへ置換。`interprocess_lock_*` (排他のみ) も同じバックエンドの degenerate ケースとして実装 (writer のみ使う) |
| `app/com_util/prod/libsrc/com_util/sync/sync_windows.c` | 同上 |
| `app/com_util/prod/libsrc/com_util/sync/makefile` | 新規ソースの追加 |
| `app/com_util/test/libsrc/mock_com_util/` 各 .c | delegate_real_ 系の差し替え不要 (API シグネチャ維持のため)。動作変更による期待値見直し |
| `app/com_util/test/src/` (新規テスト) | 数千スケール / クラッシュ復旧 / PID リユース のテスト |

参照する既存ユーティリティ:

- 現 `monotonic_ms` / `monotonic_deadline` (`sync_linux.c:80-98`) は流用
- 現 `map_wait_rc` (`sync_linux.c:119-134`) も流用
- Windows 側の `dup_string` (`sync_windows.c`) も流用

---

## Open Questions (実装着手前に確定)

1. **「数千」の正確な目安**: 同時アクティブ ロック上限 (table_size N) と、それを超えた
   ときの挙動 (拡張 / エラー) の方針。
2. **ロック ドメインの分離単位**: 「全アプリで 1 つ」「実行バイナリごと」「ユーザーごと」 のどれか。
3. **公平性ポリシ**: ライター優先 / リーダー優先 / FIFO のどれを既定にするか。
   現 `flock` (Linux) は実装依存、`LockFileEx` も公平性は保証なし。新実装ではどう決めるか。
4. **タイムアウト時の挙動**: 「待機列に居続けるが時間切れで離脱」と単純な polling、
   どちらを採るか (条件変数 + timed wait で前者にできる)。
5. **`interprocess_lock` (排他のみ) を共通バックエンドに統合してよいか**: 同じ table に
   exclusive-only モードのエントリとして同居させる方針 (推奨)。
6. **環境変数による override (`COM_UTIL_LOCK_DOMAIN`, `COM_UTIL_LOCK_TABLE_SIZE` 等) の命名規約**。

---

## Verification

### ユニット テスト

- `cd app/com_util && make test` で全テスト通過
- 既存の `interprocess_rwlock_*` 単純シナリオ (shared/exclusive 競合、タイムアウト、
  export/import) が新バックエンドでも同等動作

### スケール テスト (新規)

- N=1, 10, 100, 1000, 5000 の論理ロックを同時にオープン → 取得/解放
- Linux: `lsof -p $$` で FD 数が O(1) であること (shm 1 個 + 必要最小限)
- Linux: `ls /dev/shm` でロック ドメイン用エントリが 1 個のみ
- Windows: `Handle.exe` でハンドル数が O(1) (shm + master mutex + cond event プール)

### クラッシュ復旧テスト (新規)

- writer 保持中の SIGKILL → 別プロセスが取得可能になる (タイムアウト ≤ 1s)
- reader 多数のうち 1 プロセス SIGKILL → 残り reader 影響なし、writer 待機者が
  reader 0 になった時点で取得
- マスター ロック保持中の SIGKILL → 次取得者が `EOWNERDEAD` 経由で復旧
- マネージャ自身の `ENOTRECOVERABLE` を擬似発火 → セグメント破棄と再構築の経路確認

### PID リユース耐性テスト (新規)

- writer プロセス死亡 → 同じ PID を高速に再使用 (PID 空間が小さい場合) →
  `writer_serial` (開始時刻) によって別プロセスと判定されることを確認

### 性能ベースライン

- 単一ロックでの shared/exclusive スループット (現方式 vs 新方式)
- ロック取得待ち→取得のレイテンシ分布
- スイープ コスト (table_size を変えての acquire レイテンシ最悪値)

---

## 実装規模見積もり

| 部位 | 行数目安 |
|---|---|
| 共有メモリ初期化、attach、ref count (両 OS) | 400 |
| robust mutex 取得/復旧 (Linux) | 150 |
| WAIT_ABANDONED 復旧 (Windows) | 100 |
| 論理ロック表 + ハッシュ + free list | 400 |
| 取得/解放/待機ロジック | 350 |
| owner 生存検査 (Linux pidfd + フォールバック) | 200 |
| owner 生存検査 (Windows OpenProcess + 時刻照合) | 150 |
| スイープ | 200 |
| 既存 API へのアダプタ + 既存 sync_*.c の差し替え | 300 |
| テスト (スケール、クラッシュ、リユース) | 600 |

合計: 約 2,800 行の新規 + 200 行程度の削除/変更。
