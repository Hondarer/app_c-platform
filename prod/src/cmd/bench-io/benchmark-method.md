# ベンチマークの測定方法

`bench-io` コマンドが何をどう測っているかと、Linux および Windows での実行手順をまとめます。  
測定結果から導いた API の選び分けは [API 選定基準](https://github.com/Hondarer/app_com_util/blob/main/docs/fileio-api-selection-guideline.md) を参照してください。

## 測定対象

固定レコード長バイナリ ファイル (1 レコード 64 バイトの構造体配列) に対して、次の 2 系統の API を同一条件で比較します。

- `stdio` ラッパー API - `com_util_fopen` / `com_util_fread` / `com_util_fwrite` / `com_util_fseek` / `com_util_fclose`
- メモリ マップド ファイル API - `com_util_mmap_attach` / `com_util_mmap_get_address` / `com_util_mmap_flush` / `com_util_mmap_detach`

レコード型はパディングが入らないよう 64 バイトちょうどに揃えています。

## 測定軸

### API 形態

| ID | 内容 |
|----|------|
| `stdio-rec` | ファイルを開き、レコード単位で読み書きして閉じる |
| `stdio-blk` | ファイルを開き、1024 レコード (64 KB 相当) 単位でまとめて読み書きして閉じる |
| `mmap-once` | アタッチを測定ループの外で 1 回だけ行い、各反復ではマップ済み領域へのアクセスだけを行います。 |
| `mmap-each` | 反復ごとにアタッチ、アクセス、デタッチを行います。 |
| `mmap-lock` | `mmap-each` に加えて `com_util_mmap_get_rwlock()` によるロックの取得と解放を行います。 |

`+sync` が付く形態は、書き込み後にディスクへの反映を要求した条件です。  
`stdio` 側は `com_util_fflush` による CRT バッファーの掃き出しまで、mmap 側は `com_util_mmap_flush` による `msync(MS_SYNC)` (Windows は `FlushViewOfFile` + `FlushFileBuffers`) までを行います。  
両者は耐久性の水準が異なるため、`+sync` 同士の直接比較はできません。この非対称性は結果の解釈で考慮してください。

### アクセス パターン

| ID | 内容 | 1 反復のアクセス レコード数 |
|----|------|------|
| `seq-read` | 全レコードを先頭から読み、フィールドを集計します。 | 全レコード |
| `seq-write` | 全レコードを先頭から書き込む | 全レコード |
| `rand-read` | 固定シードの乱数順にレコードを読む | 最大 65536 |
| `rand-update` | 同じ順にレコードを read-modify-write します。 | 最大 65536 |
| `point-lookup` | ファイルを開いて 1 レコードだけ読み、閉じる | 1 |
| `open-close` | ファイルを開いて閉じるだけ | 1 |

`open-close` は、オープンに伴う固定コストを他のパターンから分離するためのマイクロ測定です。  
`stdio-rec` との組み合わせが `fopen` から `fclose` まで、`mmap-each` との組み合わせがアタッチからデタッチまで、`mmap-lock` との組み合わせがこれにロックの取得解放を加えたコストに対応します。

### ファイル サイズ

既定は 4 KB / 64 KB / 1 MB / 16 MB / 256 MB です。  
`--huge` を指定すると 1 GB を追加します。

### 組み合わせの除外

次の組み合わせは意味を持たないため測定しません。

- `mmap-once` と `open-close` - アタッチが測定ループの外にあるため、オープン コストを分離できません。
- `stdio-blk` と逐次以外のパターン - ブロック単位のまとめ読み書きは逐次アクセスでのみ成立します。
- 書き込みを伴わないパターンの `+sync` 条件

## 計測の仕組み

Windows の単調増加クロック `com_util_get_monotonic()` は内部で `GetTickCount64()` を使用しており、分解能はハードウェア依存でおおむね 15 ms です。  
1 回の操作を直接測ると分解能に埋もれるため、次の手順で反復回数を自動調整します。

1. ウォーム アップとして測定対象を 1 回実行する (この結果は測定に含めない)
2. 測定区間が `--min-ms` (既定 500 ms) 以上になるまで、反復回数を 1 から倍増させる
3. 確定した反復回数で `--trials` (既定 5) 回の試行を行います。
4. 1 反復あたりの所要時間について、中央値、最小値、最大値を記録します。

経過時間は `com_util_timespec_sub()` の結果からナノ秒で算出します。  
`com_util_timespec_diff_ms()` はミリ秒単位のため使用しません。

この方式により、Windows でも中央値の相対誤差はおおむね 15 ms / 500 ms = 3% 以内に収まります。

## ページ キャッシュの温冷

既定は warm です。  
ウォーム アップの実行によって対象ファイルがページ キャッシュに載った状態で測定します。

`--cold` を指定すると、試行ごとに `/proc/sys/vm/drop_caches` へ 3 を書き込んでページ キャッシュを破棄し、1 反復だけを測定します。  
反復の繰り返しもウォーム アップも行わないため、1 反復の所要時間がクロック分解能に対して十分長い条件でのみ意味を持ちます。

`--cold` は Linux 専用で root 権限が必要です。  
Windows には同等の手段がないため、`--cold` を指定するとエラー終了します。

**WSL2 では `--cold` を使用しないでください。**

WSL2 の仮想ディスク上では `sync()` と `/proc/sys/vm/drop_caches` への書き込みが長時間ブロックし、  
プロセスが uninterruptible sleep (`D` 状態) に入って `kill -9` でも停止できなくなる事象を確認しています。  
cold の測定はネイティブ Linux 環境で実施してください。

## 実行手順

### Linux

```bash
make sync-app-env
make -C app/com_util
app/com_util/prod/cbin/bench-io --dir /var/tmp/bench-io --csv app/com_util/prod/src/cmd/bench-io/measurements/linux.csv
```

`--dir` には測定用ファイルを置くディレクトリを指定します。  
測定対象のファイル システムが結果を左右するため、比較する環境どうしで同じ種別のファイル システムを指定してください。  
指定したディレクトリのファイル システム種別は CSV のコメント行に記録されます。

ページ キャッシュを落とした測定を行う場合は次のようにします。

```bash
sudo app/com_util/prod/cbin/bench-io --dir /var/tmp/bench-io --cold --trials 5 --csv app/com_util/prod/src/cmd/bench-io/measurements/linux-cold.csv
```

### Windows

`Start-VSCode-With-Env.cmd` で GNU Make と MSVC の環境を整えたうえで実行します。

```
make -C app/com_util
app\com_util\prod\cbin\bench-io.exe --dir C:\Temp\bench-io --csv app\com_util\prod\src\cmd\bench-io\measurements\windows.csv
```

## 出力

既定では人間可読のテーブルを標準出力へ書き出します。  
`--csv` を指定すると、同じ内容を CSV としても出力します。  
CSV は Linux と Windows の結果を突き合わせる際の正本として使用します。

CSV の列は次のとおりです。

| 列 | 内容 |
|----|------|
| `os` | OS 種別 |
| `cpu_model` | CPU の型名 |
| `fs_type` | 対象ディレクトリのファイル システム種別 |
| `cache_state` | `warm` または `cold` |
| `api` | API 形態 |
| `pattern` | アクセス パターン |
| `file_size_bytes` | 対象ファイルのサイズ |
| `record_bytes` | 1 レコードのサイズ |
| `records_touched` | 1 反復でアクセスしたレコード数 |
| `iterations` | 1 試行あたりの反復回数 |
| `trial_median_ns` | 1 反復あたりの所要時間の中央値 |
| `trial_min_ns` | 同じく最小値 |
| `trial_max_ns` | 同じく最大値 |
| `ns_per_record` | 1 レコードあたりの所要時間 |
| `mib_per_sec` | スループット |

`point-lookup` と `open-close` は `records_touched` が 1 のため、`mib_per_sec` は意味を持ちません。  
これらの条件では `trial_median_ns` を直接比較してください。

## 主なオプション

| オプション | 既定値 | 内容 |
|------------|--------|------|
| `--dir PATH` | `.` | 測定用ファイルを置くディレクトリ |
| `--csv PATH` | なし | CSV の出力先 |
| `--sizes LIST` | `4K,64K,1M,16M,256M` | 測定するファイル サイズ |
| `--apis LIST` | 全形態 | 測定する API 形態の絞り込み |
| `--patterns LIST` | 全パターン | 測定するアクセス パターンの絞り込み |
| `--min-ms MS` | 500 | 1 試行の測定区間の下限 |
| `--trials N` | 5 | 1 条件あたりの試行回数 |
| `--huge` | 無効 | 1 GB のケースを追加します。 |
| `--cold` | 無効 | ページ キャッシュを落として測定します。 |
| `--keep` | 無効 | 測定用ファイルを削除せずに残す |

`--apis` と `--patterns` には `+sync` を付けない基本名を指定します。  
たとえば `--apis mmap-once,mmap-each` を指定すると、`mmap-once+sync` と `mmap-each+sync` も測定対象に含まれます。
