# hashtable 可変長ストレージ アロケーターの計算量改善

> [!NOTE]
> 本書は、`com_util` の hashtable が持つ可変長ストレージの確保と圧縮について、
> 現行実装の計算量を分析し、改善案を検討した提案文書です。
> 現行仕様を確認する場合は、[API チート シート](../api-cheatsheet.md) と
> 公開ヘッダー `prod/include/com_util/hashtable/hashtable.h` を参照してください。

日付: 2026-08-22

## 背景

`prompt/hashtable-assessment.md` は、hashtable の残課題として次の 2 点を挙げています。

- `key_storage_find_free` / `value_storage_find_free` の最悪 O (capacity²)
- `compact_*_storage` の選択ソート相当 O (capacity²)

本書は、この 2 点を掘り下げ、実測できる形で解消する方針をまとめます。

分析の結果、これらは「まれに踏む最悪ケース」ではなく、通常運用で必ず踏む経路であることが分かりました。

## 現行実装の構造

対象は `prod/libsrc/com_util/hashtable/hashtable.c` です。

可変長キーと可変長値は、それぞれ専用のストレージ領域へ NUL 終端文字列として詰めます。  
各レコードは 16 バイトの descriptor (`struct hashtable_string_ref`) を持ち、ストレージ内の `offset` と `length` を指します。

```c
struct hashtable_string_ref
{
    uint64_t offset;
    uint64_t length;
};
```

`length == 0` が未使用を表します。

重要な設計上の特徴は、**空き領域を表すデータを一切持たない** ことです。  
空き領域は、その都度すべての descriptor から導出します。

`hdr->key_storage_used` / `value_storage_used` は使用バイトの総和であり、高水位マーク (high-water mark) ではありません。  
断片化があると `used` の位置より後ろに使用中ブロックが存在しうるため、「末尾へ追記する」高速パスの判定には使えません。

## 現行の計算量

L を可変長ブロックを持つレコード数、N を `capacity` とします。  
L の上限は N です。

| 関数 | 行 | 計算量 |
| --- | --- | --- |
| `key_storage_find_free` | 950-994 | O (L × N) |
| `value_storage_find_free` | 996-1040 | O (L × N) |
| `compact_key_storage` | 795-827 | O (L × N) |
| `compact_value_storage` | 829-861 | O (L × N) |

### 確保の内訳

`key_storage_find_free` は、隙間を 1 個進めるたびに全 descriptor を線形走査します。

```c
while (cursor <= ht->hdr->config.key_storage_size)
{
    const struct hashtable_string_ref *selected = NULL;

    for (i = 0; i < ht->hdr->config.capacity; i++)   /* 毎回 N 件を走査する */
    {
        /* cursor 以降で最小オフセットの使用中ブロックを選び直す */
    }
    if (selected == NULL) { /* 末尾の空きへ収まるか判定して終わる */ }
    if (needed <= (size_t)selected->offset - cursor) { /* この隙間へ収める */ }
    cursor = (size_t)(selected->offset + selected->length);
}
```

外側ループは「使用中ブロックをオフセット昇順に 1 個ずつ辿る」動きになり、最大 L 回まわります。  
内側ループは常に N 件です。

### 圧縮の内訳

`compact_key_storage` は、`selected_offset` を毎回 `UINT64_MAX` に戻して全 descriptor を走査し、次に詰めるブロックを選びます。  
選択ソートそのものです。

### 充填の総コスト

空のテーブルを N 件まで可変長キーで埋める場合を考えます。

削除がなければブロックは先頭から隙間なく並ぶため、i 件目の `add` は既存の i ブロックを順に辿り、各段で N 件を走査します。  
1 件あたり i × N、合計は `(N³ + N²) / 2`、およそ `N³ / 2` です。

descriptor 参照の回数は次のようになります。

| capacity | 充填にかかる descriptor 参照回数 |
| --- | --- |
| 256 | 約 8.4 × 10⁶ |
| 1024 | 約 5.4 × 10⁸ |
| 4096 | 約 3.4 × 10¹⁰ |

キーと値の双方が可変長なら、この 2 倍です。

つまり、**テーブルを埋めるという最も基本的な操作が O (capacity³)** です。

### 移行の総コスト

`hashtable_apply_migration` (3105-3155 行) は、残すレコードを `com_util_hashtable_insert_direct` で新しいテーブルへ 1 件ずつ書き直します。  
`insert_direct` は `key_storage_find_free` と `value_storage_find_free` を呼ぶため、`com_util_hashtable_resize` と `com_util_hashtable_rebuild_into` も同じ O (capacity³) を踏みます。

## 定数項の問題

計算量の次数だけでなく、内側ループ 1 回あたりの重さも問題です。

`hashtable_key_ref_at` (641-647 行) は、呼び出しのたびに `entry_stride_checked` を実行します。  
`entry_stride_checked` はあふれ検査付きの乗算と加算、および境界への切り上げを行うため、内側ループ 1 回はバイト比較 1 回では終わりません。

`compact_key_storage` はさらに、外側ループ 1 回につき `hashtable_key_storage` を 2 回呼びます (821 行)。  
`hashtable_key_storage` は `hashtable_mgmt_layout` 全体を再計算します。

一方、値側の `hashtable_value_ref_at` (690-693 行) は単純な配列添字です。  
このため、同じ次数でもキー側と値側で定数項が大きく異なります。

## 検討した案

### 案 1: 高水位マーク + 満杯時の自動圧縮 (見送り)

ヘッダーへ `key_storage_top` / `value_storage_top` を持ち、確保は先端の押し出しだけで済ませます。  
先端に収まらないが総空き容量は足りるとき、内部で圧縮してから確保します。

確保は償却 O(1) になり、実装も小さく収まります。

**見送りの理由**: 確保が既存ブロックを移動させるため、`com_util_hashtable_find_value_ref` と `com_util_hashtable_get_key_ref` が返した参照が `add` / `upsert` / `update` で無効化されます。  
公開ヘッダー 57-59 行は、可変長フィールドへの参照が無効になる契機を「当該フィールドの更新・回収・再利用、または `compact` / `clear` / `dispose`」と定めています。  
自動圧縮はこの契約を変えるため、利用者の判断なしには採用できません。

なお、自動圧縮を伴わない高水位マーク単独の採用は、現行なら収まる確保が `COM_UTIL_ERR_STORAGE_FULL` になる退行を招くため、成立しません。

### 案 2: 穴へ侵入型のノードを置く空きリスト (見送り)

空き領域そのものへ `{next, size}` を書き込み、連結リストを作ります。  
追加の領域を必要としません。

**見送りの理由**: 2 点あります。

第一に、ノードを置けない大きさの穴が残ります。  
可変長文字列は短いものが多く、ノード サイズ未満の穴は珍しくありません。  
これらを表現できないと確保可能な組み合わせが減り、現行より `STORAGE_FULL` が増えます。

第二に、解放したバイトを 0 埋めする現行の性質と両立しません。  
`purge_zero_fills_released_variable_key_and_value` と `lifetime_expiration_zero_fills_released_variable_storage` が、解放領域の 0 埋めを検証しています。

### 案 3: descriptor へオフセット順のリンクを足す (見送り)

`hashtable_string_ref` へ「次のブロック (オフセット順)」を足し、使用中ブロックをオフセット昇順に辿れるようにします。  
追加は capacity × 8 バイトで、案 4 の半分です。

**見送りの理由**: 先着適合は先頭から辿る必要があるため、確保は O(L) のままです。  
断片化していない状態で充填すると、最初に収まる隙間が常に末尾になり、1 件あたり O(L) を払います。  
充填の総コストは O (capacity²) にとどまり、案 4 の O(capacity) に届きません。

### 案 4: 穴ディレクトリ (採用)

可変長ストレージごとに、空き領域をオフセット昇順の密な配列として明示的に持ちます。

## 採用案: 穴ディレクトリ

### 不変条件

- 要素は `{uint64 offset, uint64 length}` で、`hashtable_string_ref` と同じ 16 バイトです。
- オフセットの昇順に厳密に並びます。
- 隣接する穴は必ず結合済みです。すなわち、前の穴の終端は次の穴の開始より真に小さい値です。
- 穴と使用中ブロックは `[0, storage_size)` を過不足なく分割します。
- 構築直後と `clear` 直後の要素数は 1 で、内容は `{0, storage_size}` です。
- 穴は使用中ブロックで区切られるため、要素数の上限は L + 1、すなわち capacity + 1 です。

### 操作

H を穴の個数とします。

| 操作 | 手順 | 計算量 |
| --- | --- | --- |
| 確保 | 先頭から先着適合で走査し、穴を縮小または分割する | O(H) |
| 解放 | 二分探索で挿入位置を求め、前後の穴と結合する | O(H) |
| 圧縮 | 穴を辿って使用中区間を左詰めし、descriptor を更新する | O(N log H + used) |

断片化していないとき H は 1 です。  
このため、確保と解放は実質 O(1) で完了します。

### 改善後の計算量

| 操作 | 現行 | 採用案 |
| --- | --- | --- |
| `add` / `update` / `insert_direct` 1 件 | O (N²) | O(H)、断片化なしで O(1) |
| capacity 件の充填 | O (N³) | O(N) |
| `resize` / `rebuild_into` | O (N³) | O(N) |
| `compact` | O (N²) | O(N log H + used) |

### ブロックが移動する操作

格納済みブロックの配置が変わるのは、`com_util_hashtable_compact`、`com_util_hashtable_resize`、`com_util_hashtable_rebuild_into` の 3 つです。

`com_util_hashtable_compact` は同一のストレージ内で詰め直します。
`com_util_hashtable_resize` と `com_util_hashtable_rebuild_into` は、残すレコードを移行先の領域へ 1 件ずつ書き直します。
容量を縮める場合も同じで、詰め直した結果が新しい容量へ収まらなければ `COM_UTIL_ERR_STORAGE_FULL` です。

確保は空き領域だけを使うため、対象レコード以外のブロックを動かしません。
`com_util_hashtable_update` などは、対象レコード自身のブロックだけを再配置しえます。

参照が無効になる契機は、そのフィールドの更新・回収・再利用、`com_util_hashtable_compact`、`com_util_hashtable_clear`、`com_util_hashtable_resize`、`com_util_hashtable_rebuild_into`、`com_util_hashtable_dispose` です。

### 現行動作との等価性

現行の `find_free` は、隙間をオフセット昇順に辿り、最初に収まる隙間を返します。

穴ディレクトリが昇順かつ結合済みであれば、列挙される隙間の列は現行と完全に一致します。  
したがって、**返るオフセットは現行実装と同一** になり、成功と失敗の判定も変わりません。

この性質により、既存テストは無修正で通ります。  
`hashtableVariableStringTest.cc` の `fragmented_update_returns_storage_full_and_preserves_value` (82 行) と `explicit_compaction_enables_fragmented_add_and_invalidates_moved_reference` (110 行) が該当します。

`needed` が旧 `length` 以下のときに自ブロックへその場で書き込む近道は、配置が現行と変わるため採用しません。

### replace の扱い

現行の `replace` 引数は「自スロットの旧ブロックだけを空き扱いにして探索する」フラグです (971 行、1017 行)。

穴ディレクトリでは次の順序で再現します。

1. 自ブロックの区間を穴ディレクトリへ返却します。バイトの 0 埋めはまだ行いません。
2. 先着適合で確保します。
3. 失敗したら同じ区間を元どおり確保し直し、テーブルを無変更のまま `COM_UTIL_ERR_STORAGE_FULL` を返します。

手順 2 と手順 3 の間に他の確保が挟まらないため、手順 3 は必ず成功します。

`release_key` / `release_value` (863-895 行) が行う 0 埋めと `*_storage_used` の更新は、現行どおり確保成功後に行います。

## 領域レイアウトへの影響

穴ディレクトリは、それぞれの可変長ストレージの直前へ置きます。

```
管理領域 (可変長キーのとき):
    ...
    | entries[N]              |
    | key hole directory      |  // {offset, length} x (N + 1)
    | key storage             |

データ領域 (可変長値のとき):
    | value refs[N]           |
    | value hole directory    |  // {offset, length} x (N + 1)
    | value storage           |
```

穴の個数は永続化ヘッダーへ `key_hole_count` と `value_hole_count` として持ちます。  
いずれも永続化して意味のある状態であり、実行時のみ有効なアドレスや一時フラグではありません。

配置の版番号 `COM_UTIL_HASHTABLE_VERSION` を 3 から 4 へ上げます。  
`com_util_hashtable_attach` は、従来どおり版番号の不一致を拒否します。

## メモリ コスト

穴ディレクトリは、可変長ストレージ 1 個あたり `16 × (capacity + 1)` バイトを追加します。

| 構成 | capacity 1024 | capacity 4096 |
| --- | --- | --- |
| 可変長キーのみ | 約 16 KiB | 約 64 KiB |
| 可変長キーと可変長値 | 約 32 KiB | 約 128 KiB |

`com_util_hashtable_required_size` と `hashtable-required-size` コマンドの出力は、この分だけ増えます。  
外部バッファーを使う利用側は、必要バイト数を取り直してください。

要素を `{uint32, uint32}` の 8 バイトにすれば半減できますが、`storage_size` に 4 GiB の上限が生じます。  
`hashtable_string_ref` が `uint64` である形式の一貫性を優先し、既定では採りません。

## 検証の範囲

`hashtable_validate_impl` (1500-1695 行) へ、穴ディレクトリの検証を追加します。

- 個数が capacity + 1 以下であること
- オフセット昇順、長さが非 0、`[0, storage_size)` の内側であること
- 隣接する穴が結合済みであること
- 穴の長さ合計が `storage_size - *_storage_used` と一致すること
- 各使用中ブロックが、いずれの穴とも重ならないこと

使用中ブロック同士の重なりは検出しません。  
これは現行の検証でも同じで、検出するには領域サイズに比例したビットマップか、descriptor の整列が必要になるためです。

## 測定結果

`prod/src/cmd/bench-hashtable` で測定しました。

測定環境は次のとおりです。

| 項目 | 内容 |
| --- | --- |
| CPU | Intel Xeon Gold 6254 @ 3.10GHz |
| OS | Red Hat Enterprise Linux 8.10 (WSL2、カーネル 6.6.87.2) |
| コンパイラ | GCC 8.5.0、`-O2` |
| 測定日 | 2026-08-22 |

値は 5 試行の中央値で、1 反復あたりのミリ秒です。

### capacity 256

| 操作 | 改善前 | 改善後 | 倍率 |
| --- | --- | --- | --- |
| `fill` | 120.447 | 0.156 | 772 |
| `update` | 15.825 | 0.109 | 145 |
| `churn` | 61.006 | 0.081 | 753 |
| `compact` | 0.463 | 0.014 | 33 |
| `resize` | 235.149 | 0.094 | 2501 |

### capacity 1024

| 操作 | 改善前 | 改善後 | 倍率 |
| --- | --- | --- | --- |
| `fill` | 7656.482 | 0.590 | 12977 |
| `update` | 999.811 | 0.474 | 2109 |
| `churn` | 3885.230 | 0.380 | 10224 |
| `compact` | 7.348 | 0.063 | 117 |
| `resize` | 15219.464 | 0.430 | 35394 |

改善前は capacity を 4 倍にすると所要時間が約 64 倍になり、capacity の 3 乗で伸びていました。
capacity 4096 以上は現実的な時間で測定できないため、改善後だけを載せます。

### 改善後の 1 件あたりの所要時間 (ナノ秒)

| 操作 | 256 | 1024 | 4096 | 16384 |
| --- | --- | --- | --- | --- |
| `fill` | 608 | 577 | 579 | 581 |
| `update` | 427 | 463 | 445 | 452 |
| `churn` | 635 | 742 | 1168 | 2959 |
| `compact` | 56 | 62 | 66 | 72 |
| `resize` | 367 | 420 | 434 | 456 |

`fill`、`update`、`compact`、`resize` は 1 件あたりの所要時間が capacity によらずほぼ一定です。
全体としては capacity に比例します。

### churn だけが capacity に比例する理由

`churn` は 1 件おきに回収した状態、すなわち穴が capacity / 2 個ある状態からの確保です。
先着適合は穴を先頭から辿るため、1 件あたりの所要時間は H に比例します。

ブロックを移動しない方針を採ったため、この振る舞いは避けられません。
断片化した状態での確保は O(H) であり、H が大きいほど遅くなります。

利用側の対処は `com_util_hashtable_compact` です。
圧縮すると H が 1 に戻り、以降の確保は再び実質 O(1) になります。

改善前の `churn` は capacity 1024 で 3885 ミリ秒でした。
改善後は capacity 16384 でも 24 ミリ秒であり、断片化した最悪の条件でも実用上の問題は残りません。

## 測定の実施

`prod/src/cmd/bench-hashtable/` を追加し、`prod/src/cmd/bench-io/` の構成と作法に合わせています。

測定軸は次のとおりです。

- capacity 別の充填時間
- 断片化させた状態での `add` と `update` の 1 件あたり時間
- `compact` の所要時間
- `resize` の所要時間

`AGENTS.md` の `bench-io` の規則にならい、測定結果自体は管理対象外とします。  
共有する数値と測定環境は本書へ記載します。
