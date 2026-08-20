/**
 *******************************************************************************
 *  @file           hashtable.c
 *  @brief          固定長スロットと遅延削除を持つハッシュ テーブルを実装します。
 *  @author         Tetsuo Honda
 *  @date           2026/08/18
 *  @version        1.0.0
 *
 *  公開契約は hashtable.h を正とします。\n
 *  以下の配置は実装の説明であり、公開 ABI ではありません。
 *
 *  @section        hashtable_buffer_layout 管理領域とデータ領域の構成
 *
 *  テーブルは、識別子・ヘッダー・バケット配列・エントリ配列からなる「管理領域」と、
 *  値配列 (data[N]) からなる「データ領域」の 2 ブロックで構成されます。\n
 *  両者は連続している必要がありません。データ領域は、
 *  メモリマップド ファイル (@c com_util_mmap) など管理領域と独立した領域を
 *  想定しています。\n
 *  呼び出し側が両方 NULL を渡した場合 (@ref com_util_hashtable_create) は、
 *  内部で 1 回の確保にまとめ、データ領域は管理領域の直後に連続配置します。
 *  この場合に限り、解放も @ref com_util_hashtable_destroy の 1 回で両方が
 *  片付きます。\n
 *  呼び出し側が両方を明示的に渡した場合は、それぞれ独立した領域として扱い、
 *  解放は呼び出し側の責務です。片方だけ NULL の指定は
 *  @ref COM_UTIL_ERR_INVALID_ARGUMENT です。\n
 *  各領域の開始は、直前領域の末尾を指定アラインメントへ切り上げた位置です。\n
 *  @p timestamp_scope はエントリ配置と管理領域サイズに使います。\n
 *  @p key_type と @p lifetime はサイズ計算に使いません。\n
 *  必要バイト数の計算入口は @ref com_util_hashtable_required_size です。\n
 *  構築済みテーブルの各領域の先頭は @ref com_util_hashtable_buffer_ref 、
 *  バイト数は @ref com_util_hashtable_buffer_size で取得できます。
 *
    @code
    管理領域:
    +----------------------+
    | magic (4B)           |  // COM_UTIL_HASHTABLE_MAGIC (0x48544142)
    +----------------------+
    | version (4B)         |  // COM_UTIL_HASHTABLE_VERSION (1)
    +----------------------+
    | header               |  // data(ポインター), config, owns_buffer, pad, next_empty, table_timestamp
    +----------------------+
    | pad to uint64        |  // sizeof(struct) が未整列のときだけ
    +----------------------+
    | bucket_head[N]       |  // N x sizeof(uint64_t)
    +----------------------+
    | pad to uint64        |
    +----------------------+
    | entries[N]           |  // N x entry_stride
    +----------------------+

    データ領域 (管理領域と連続しない、独立したブロック):
    +----------------------+
    | data[N]              |  // N x record_size
    +----------------------+
    @endcode
 *
 *  N は @p capacity です。レコード番号は 1 相対で、内部添字は record - 1 です。
 *
 *  @subsection     hashtable_ident マジックと版番号
 *
 *  管理領域先頭 8 バイトは識別子です。ヘッダーより前に置きます。\n
 *  マジックは 0x48544142、版番号は 1 です。\n
 *  @ref com_util_hashtable_attach は両者を検証し、不一致なら失敗します。
 *
 *  @subsection     hashtable_header ヘッダー
 *
 *  版番号の直後から、データ領域先頭ポインター、設定の複製、所有フラグ、詰め物、
 *  @p next_empty 、テーブル横断の変更時刻が続きます。\n
 *  データ領域先頭ポインターは実行時のみ有効な値であり、管理領域バイト列を
 *  ファイルなどへ永続化しても意味を持ちません。\n
 *  @ref com_util_hashtable_attach は、呼び出し側が渡したデータ領域アドレスで
 *  必ずこの値を上書きします。\n
 *  @p next_empty は 1 相対の最小空きスロットです。満杯のときは 0 です。\n
 *  テーブル時刻は最後にキーまたは値が変わった実時刻です。構築直後は 0 です。\n
 *  テーブル時刻は @p timestamp_scope に関わらず常に持ちます。\n
 *  ヘッダー全体の大きさ、および暗黙パディングは処理系依存です。\n
 *  管理領域バッファーは struct com_util_hashtable のアラインメント境界が必要です。\n
 *  データ領域バッファーは値配列への @c memcpy 経由アクセスのみのため、
 *  アラインメント要件はありません。\n
 *  @ref com_util_hashtable_attach が書き換えるのは、データ領域先頭ポインターと
 *  所有フラグだけです。
 *
 *  @subsection     hashtable_buckets バケット先頭
 *
 *  @p capacity 個の @c uint64_t です。\n
 *  値は 1 相対のレコード番号です。0 は後続が無いことを示します。
 *
 *  @subsection     hashtable_entries エントリ
 *
 *  キーのオフセットとストライドは @p timestamp_scope で変わります。\n
 *  SCOPE_RECORD のストライドは 8 + 1 + 7 + sizeof(com_util_timespec) + key_size を
 *  uint64_t 境界へ切り上げた値です。\n
 *  SCOPE_TABLE のストライドは 8 + 1 + key_size を uint64_t 境界へ切り上げた値です。
 *
    @code
    SCOPE_RECORD:
    +--------+--------+-------+------------+-------------+-------+
    | next   | status | pad   | timestamp  | key         | pad   |
    | 8B     | 1B     | 7B    | timespec   | key_size    | to 8B |
    +--------+--------+-------+------------+-------------+-------+

    SCOPE_TABLE:
    +--------+--------+-------------+-------+
    | next   | status | key         | pad   |
    | 8B     | 1B     | key_size    | to 8B |
    +--------+--------+-------------+-------+
    @endcode
 *
 *  @p next は同一バケットの次レコード (1 相対) です。0 は終端です。\n
 *  @p status は 0 が空、1 が実装中、2 以上が削除済みの加齢です。\n
 *  @p timestamp は実時刻の変更時刻です。status が 0 のときは 0 埋めです。\n
 *  文字列キーは NUL までを格納し、残りを 0 埋めします。
 *
 *  @subsection     hashtable_data 値配列 (データ領域)
 *
 *  各値は @p record_size バイトを隙間なく並べます。\n
 *  内部確保 (両方 NULL) のときは、データ領域が管理領域の直後に連続配置され、
 *  先頭は 8 境界です。\n
 *  外部指定のときは、データ領域は独立したブロックのため、境界の制約はありません。\n
 *  2 件目以降のアラインメントは @p record_size 次第です。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include <com_util/hashtable/hashtable.h>

#include <com_util/clock/clock.h>
#include <com_util/clock/timespec.h>
#include <com_util/crt/stdlib.h>
#include <stdint.h>
#include <string.h>

/**
 *  @brief          エントリの実装状況です。
 *
 *  2 以上は削除済みの加齢カウンターです。255 は無限寿命の終端です。
 */
enum
{
    REC_EMPTY = 0,  /**< 空きスロットです。 */
    REC_IN_USE = 1, /**< 実装中です。 */
    REC_DELETED = 2 /**< 削除直後の加齢開始値です。 */
};

enum
{
    ENTRY_TIMESTAMP_PAD = 7 /**< status の直後から timespec を 8 境界へ置く詰め物です。 */
};

_Static_assert(((sizeof(uint64_t) + sizeof(unsigned char) + 7u) % _Alignof(com_util_timespec)) == 0,
               "entry time offset must satisfy alignof(com_util_timespec)");

#define COM_UTIL_HASHTABLE_MAGIC   0x48544142u /**< 管理領域先頭の識別子です。 */
#define COM_UTIL_HASHTABLE_VERSION 1u          /**< 配置の版番号です。 */

/**
 *  @brief          管理領域先頭の内部ヘッダーです。
 *
 *  識別子、データ領域先頭ポインター、設定の複製、所有フラグ、空きヒント、
 *  テーブル時刻をまとめています。\n
 *  配置の図は本ファイル先頭の説明を参照してください。
 */
struct com_util_hashtable
{
    uint32_t magic;                    /**< 識別子。@c COM_UTIL_HASHTABLE_MAGIC 。 */
    uint32_t version;                  /**< 配置の版。@c COM_UTIL_HASHTABLE_VERSION 。 */
    unsigned char *data;               /**< データ領域(値配列)の先頭です。実行時のみ有効で、
                                             永続化バイト列としての意味は持ちません。
                                             @ref com_util_hashtable_create と
                                             @ref com_util_hashtable_attach が、
                                             呼び出し側の渡した領域で必ず上書きします。 */
    com_util_hashtable_config config;  /**< 構築時の設定の複製です。 */
    unsigned char owns_buffer;         /**< 1 なら @ref com_util_hashtable_destroy が解放します。 */
    unsigned char pad[7];              /**< owns_buffer のあとの明示パディングです。 */
    uint64_t next_empty;               /**< 1 相対の最小空きです。満杯のときは 0 です。 */
    com_util_timespec table_timestamp; /**< 最後にキーまたは値が変わった実時刻です。 */
};

/* ヘッダー サイズは hashtable_mgmt_layout の整列前提(uint64_t 境界)を満たす。 */
_Static_assert(sizeof(struct com_util_hashtable) % _Alignof(uint64_t) == 0,
               "com_util_hashtable header size must be a multiple of _Alignof(uint64_t)");

/**
 *  @brief          加算のあふれを検出します。
 *  @param[in]      a    加数。
 *  @param[in]      b    加数。
 *  @param[out]     sum_out  和の格納先。あふれ時は書きません。
 *  @return         成功なら 0、あふれなら 1 です。
 */
static int add_checked(size_t a, size_t b, size_t *sum_out)
{
    if (b > SIZE_MAX - a)
    {
        return 1;
    }
    *sum_out = a + b;
    return 0;
}

/**
 *  @brief          乗算のあふれを検出します。
 *  @param[in]      a    乗数。
 *  @param[in]      b    乗数。
 *  @param[out]     product_out  積の格納先。あふれ時は書きません。
 *  @return         成功なら 0、あふれなら 1 です。
 */
static int mul_checked(size_t a, size_t b, size_t *product_out)
{
    /* a が 0 なら積は 0 で、SIZE_MAX / a は未定義になるため検査しない。 */
    if ((a != 0) && (b > SIZE_MAX / a))
    {
        return 1;
    }
    *product_out = a * b;
    return 0;
}

/**
 *  @brief          オフセットを指定境界へ切り上げます。
 *  @param[in]      offset     元のオフセット。
 *  @param[in]      alignment  1 以上の境界。
 *  @param[out]     aligned_out  切り上げ後の格納先。あふれ時は書きません。
 *  @return         成功なら 0、あふれなら 1 です。
 */
static int align_up_checked(size_t offset, size_t alignment, size_t *aligned_out)
{
    size_t rem = offset % alignment;
    size_t pad = (rem == 0) ? 0 : (alignment - rem);

    return add_checked(offset, pad, aligned_out);
}

/**
 *  @brief          エントリ先頭からキーまでのオフセットを返します。
 *  @param[in]      config  設定。NULL を渡してはなりません。
 *  @return         キーのバイト オフセットです。
 *
 *  SCOPE_RECORD では next + status + pad + timespec のあとです。\n
 *  SCOPE_TABLE では next + status の直後です。
 */
static size_t entry_key_offset(const com_util_hashtable_config *config)
{
    size_t off = sizeof(uint64_t) + sizeof(unsigned char);

    if (config->timestamp_scope == COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_RECORD)
    {
        off += (size_t)ENTRY_TIMESTAMP_PAD + sizeof(com_util_timespec);
    }
    return off;
}

/**
 *  @brief          1 エントリのストライドを求めます。
 *  @param[in]      config  設定。NULL を渡してはなりません。
 *  @param[out]     stride_out  ストライドの格納先。あふれ時は書きません。
 *  @return         成功なら 0、あふれなら 1 です。
 *
 *  中身は next + status + (SCOPE_RECORD なら pad と timespec) + key を
 *  uint64_t 境界へ切り上げた値です。
 */
static int entry_stride_checked(const com_util_hashtable_config *config, size_t *stride_out)
{
    size_t raw;

    if (add_checked(entry_key_offset(config), config->key_size, &raw) != 0)
    {
        return 1;
    }
    return align_up_checked(raw, _Alignof(uint64_t), stride_out);
}

/**
 *  @brief          レコードごとの変更時刻を持つかを返します。
 *  @param[in]      ht  対象。NULL を渡してはなりません。
 *  @return         持つなら 1、持たないなら 0 です。
 */
static int hashtable_has_record_timestamp(const com_util_hashtable *ht)
{
    return ht->config.timestamp_scope == COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_RECORD;
}

/**
 *  @brief          管理領域内の領域オフセットと総バイト数を求めます。
 *  @param[in]      config           設定。NULL を渡してはなりません。
 *  @param[out]     off_bucket_head  バケット先頭のオフセット。不要なら NULL を渡せます。
 *  @param[out]     off_entries      エントリ配列のオフセット。不要なら NULL を渡せます。
 *  @param[out]     mgmt_size_out    管理領域の総バイト数。不要なら NULL を渡せます。
 *  @return         成功なら 0、あふれなら -1 です。
 *
 *  管理領域は識別子・ヘッダー・バケット配列・エントリ配列からなり、
 *  値配列(データ領域)は含みません。\n
 *  @p timestamp_scope はエントリ配置に使います。@p key_type と @p lifetime は使いません。\n
 *  構築済みテーブルではあふれません。呼び出し側は戻り値を捨てて構いません。
 */
static int hashtable_mgmt_layout(const com_util_hashtable_config *config, size_t *off_bucket_head, size_t *off_entries,
                                 size_t *mgmt_size_out)
{
    size_t offset = sizeof(struct com_util_hashtable);
    size_t region_size;
    size_t entry_stride;

    /* ヘッダー サイズは uint64_t 境界の倍数(構造体定義直後の _Static_assert で保証)。整列不要。 */

    if (off_bucket_head != NULL)
    {
        *off_bucket_head = offset;
    }
    if (mul_checked(config->capacity, sizeof(uint64_t), &region_size) != 0)
    {
        return -1;
    }
    if (add_checked(offset, region_size, &offset) != 0)
    {
        return -1;
    }

    if (entry_stride_checked(config, &entry_stride) != 0)
    {
        return -1;
    }

    /* 直前の offset は uint64_t 境界の倍数同士の和のため、既に境界上にある。整列不要。 */

    if (off_entries != NULL)
    {
        *off_entries = offset;
    }
    if (mul_checked(config->capacity, entry_stride, &region_size) != 0)
    {
        return -1;
    }
    if (add_checked(offset, region_size, &offset) != 0)
    {
        return -1;
    }

    if (mgmt_size_out != NULL)
    {
        *mgmt_size_out = offset;
    }
    return 0;
}

/**
 *  @brief          データ領域(値配列)の総バイト数を求めます。
 *  @param[in]      config         設定。NULL を渡してはなりません。
 *  @param[out]     data_size_out  データ領域の総バイト数。NULL を渡してはなりません。
 *  @return         成功なら 0、あふれなら -1 です。
 *
 *  データ領域は管理領域と独立したブロックのため、オフセット加算は発生しません。
 */
static int hashtable_data_region_size(const com_util_hashtable_config *config, size_t *data_size_out)
{
    return (mul_checked(config->capacity, config->record_size, data_size_out) != 0) ? -1 : 0;
}

/**
 *  @brief          管理領域をバイト列として見ます。
 *  @param[in]      ht  対象。NULL を渡してはなりません。
 *  @return         管理領域先頭です。
 *
 *  const の @p ht から領域ポインターを共用するため、const を外します。
 */
static unsigned char *hashtable_bytes(const com_util_hashtable *ht)
{
    return (unsigned char *)(uintptr_t)ht;
}

/**
 *  @brief          バケット先頭配列を返します。
 *  @param[in]      ht  対象。NULL を渡してはなりません。
 *  @return         @p capacity 個の 1 相対レコード番号です。0 は空です。
 *
 *  範囲検査はしません。構築済みの @p ht を渡してください。
 */
static uint64_t *hashtable_bucket_head(const com_util_hashtable *ht)
{
    size_t off_bucket_head;

    (void)hashtable_mgmt_layout(&ht->config, &off_bucket_head, NULL, NULL);
    return (uint64_t *)(hashtable_bytes(ht) + off_bucket_head);
}

/**
 *  @brief          エントリ配列の先頭を返します。
 *  @param[in]      ht  対象。NULL を渡してはなりません。
 *  @return         エントリ配列の先頭です。
 *
 *  範囲検査はしません。構築済みの @p ht を渡してください。
 */
static unsigned char *hashtable_entries(const com_util_hashtable *ht)
{
    size_t off_entries;

    (void)hashtable_mgmt_layout(&ht->config, NULL, &off_entries, NULL);
    return hashtable_bytes(ht) + off_entries;
}

/**
 *  @brief          指定スロットの next へのポインターを返します。
 *  @param[in]      ht   対象。NULL を渡してはなりません。
 *  @param[in]      rec  0 相対のスロット添字。capacity 未満であること。
 *  @return         1 相対の次レコード番号です。0 は終端です。
 */
static uint64_t *hashtable_entry_next(const com_util_hashtable *ht, size_t rec)
{
    size_t stride = 0;

    (void)entry_stride_checked(&ht->config, &stride);
    return (uint64_t *)(hashtable_entries(ht) + rec * stride);
}

/**
 *  @brief          指定スロットの status へのポインターを返します。
 *  @param[in]      ht   対象。NULL を渡してはなりません。
 *  @param[in]      rec  0 相対のスロット添字。capacity 未満であること。
 *  @return         実装状況へのポインターです。
 */
static unsigned char *hashtable_entry_status(const com_util_hashtable *ht, size_t rec)
{
    size_t stride = 0;

    (void)entry_stride_checked(&ht->config, &stride);
    return hashtable_entries(ht) + rec * stride + sizeof(uint64_t);
}

/**
 *  @brief          指定スロットの変更時刻へのポインターを返します。
 *  @param[in]      ht   対象。NULL を渡してはなりません。
 *  @param[in]      rec  0 相対のスロット添字。capacity 未満であること。
 *  @return         変更時刻へのポインターです。
 *
 *  @p timestamp_scope が @c SCOPE_RECORD のときだけ呼べます。
 */
static com_util_timespec *hashtable_entry_timestamp(const com_util_hashtable *ht, size_t rec)
{
    size_t stride = 0;

    (void)entry_stride_checked(&ht->config, &stride);
    return (com_util_timespec *)(hashtable_entries(ht) + rec * stride + sizeof(uint64_t) + sizeof(unsigned char) +
                                 (size_t)ENTRY_TIMESTAMP_PAD);
}

/**
 *  @brief          指定スロットのキーへのポインターを返します。
 *  @param[in]      ht   対象。NULL を渡してはなりません。
 *  @param[in]      rec  0 相対のスロット添字。capacity 未満であること。
 *  @return         key_size バイトのキーです。
 */
static char *hashtable_entry_key(const com_util_hashtable *ht, size_t rec)
{
    size_t stride = 0;

    (void)entry_stride_checked(&ht->config, &stride);
    return (char *)(hashtable_entries(ht) + rec * stride + entry_key_offset(&ht->config));
}

/**
 *  @brief          値配列の先頭を返します。
 *  @param[in]      ht  対象。NULL を渡してはなりません。
 *  @return         値配列の先頭です。
 *
 *  データ領域は管理領域と独立したブロックのため、@p ht->data をそのまま返します。
 */
static unsigned char *hashtable_data(const com_util_hashtable *ht)
{
    return ht->data;
}

/**
 *  @brief          指定スロットの値へのポインターを返します。
 *  @param[in]      ht   対象。NULL を渡してはなりません。
 *  @param[in]      rec  0 相対のスロット添字。capacity 未満であること。
 *  @return         record_size バイトの値です。
 */
static unsigned char *hashtable_data_at(const com_util_hashtable *ht, size_t rec)
{
    return hashtable_data(ht) + rec * ht->config.record_size;
}

/**
 *  @brief          キーのバケット番号を求めます。
 *  @param[in]      ht   対象。NULL を渡してはなりません。
 *  @param[in]      key  キー。NULL を渡してはなりません。
 *  @return         0 以上 capacity 未満のバケット番号です。
 *
 *  djb2 を使い、最後に capacity で割った余りを返します。
 */
static unsigned long hash_key(const com_util_hashtable *ht, const void *key)
{
    unsigned long hash = 5381;
    const unsigned char *p = (const unsigned char *)key;

    if (ht->config.key_type == COM_UTIL_HASHTABLE_KEY_STRING)
    {
        int c;

        /* 文字列は先頭の NUL までだけを混ぜ、残り 0 埋めはハッシュに入れません。 */
        while ((c = *p++) != 0)
        {
            hash = ((hash << 5) + hash) + (unsigned long)c;
        }
    }
    else
    {
        size_t i;

        for (i = 0; i < ht->config.key_size; i++)
        {
            hash = ((hash << 5) + hash) + p[i];
        }
    }
    return hash % ht->config.capacity;
}

/**
 *  @brief          格納済みキーと探索キーが同じかを判定します。
 *  @param[in]      ht          対象。NULL を渡してはなりません。
 *  @param[in]      stored_key  スロット上のキー。
 *  @param[in]      key         探索キー。
 *  @return         一致なら 1、不一致なら 0 です。
 */
static int key_equal(const com_util_hashtable *ht, const char *stored_key, const void *key)
{
    if (ht->config.key_type == COM_UTIL_HASHTABLE_KEY_STRING)
    {
        return strcmp(stored_key, (const char *)key) == 0;
    }
    return memcmp(stored_key, key, ht->config.key_size) == 0;
}

/**
 *  @brief          キーが key_size に収まるかを判定します。
 *  @param[in]      ht   対象。NULL を渡してはなりません。
 *  @param[in]      key  キー。NULL を渡してはなりません。
 *  @return         収まれば 1、収まらなければ 0 です。
 */
static int key_fits(const com_util_hashtable *ht, const void *key)
{
    /* バイナリは常に key_size バイトなので検査しません。 */
    if (ht->config.key_type != COM_UTIL_HASHTABLE_KEY_STRING)
    {
        return 1;
    }
    return memchr(key, '\0', ht->config.key_size) != NULL;
}

/**
 *  @brief          キーをスロットへ書き込みます。
 *  @param[in]      ht   対象。NULL を渡してはなりません。
 *  @param[out]     dst  スロット上のキー領域。
 *  @param[in]      key  書き込むキー。@ref key_fits を満たすこと。
 *
 *  文字列は NUL までを写し、残りを 0 埋めします。
 */
static void key_store(const com_util_hashtable *ht, char *dst, const void *key)
{
    if (ht->config.key_type == COM_UTIL_HASHTABLE_KEY_STRING)
    {
        size_t copy_len = strlen((const char *)key) + 1u;

        memset(dst, 0, ht->config.key_size);
        memcpy(dst, key, copy_len);
    }
    else
    {
        memcpy(dst, key, ht->config.key_size);
    }
}

/**
 *  @brief          start_idx 以降の最小空きスロットを 1 相対で返します。
 *  @param[in]      ht         対象。NULL を渡してはなりません。
 *  @param[in]      start_idx  走査開始の 0 相対添字。
 *  @return         1 相対のレコード番号です。無ければ 0 です。
 */
static uint64_t scan_next_empty(com_util_hashtable *ht, size_t start_idx)
{
    size_t i;

    for (i = start_idx; i < ht->config.capacity; i++)
    {
        if (*hashtable_entry_status(ht, i) == REC_EMPTY)
        {
            return (uint64_t)(i + 1);
        }
    }
    return 0;
}

/**
 *  @brief          スロットへ現在の実時刻を刻みます。
 *  @param[in]      ht   対象。NULL を渡してはなりません。
 *  @param[in]      rec  0 相対のスロット添字。
 */
static void stamp_record(com_util_hashtable *ht, size_t rec)
{
    com_util_timespec now;

    com_util_get_realtime(&now);
    if (hashtable_has_record_timestamp(ht) != 0)
    {
        *hashtable_entry_timestamp(ht, rec) = now;
    }
    ht->table_timestamp = now;
}

/**
 *  @brief          テーブル横断の変更時刻へ現在の実時刻を刻みます。
 *  @param[in]      ht  対象。NULL を渡してはなりません。
 */
static void stamp_table(com_util_hashtable *ht)
{
    com_util_get_realtime(&ht->table_timestamp);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_hashtable_required_size(const com_util_hashtable_config *config, size_t *mgmt_size_out,
                                     size_t *data_size_out)
{
    size_t mgmt_size;
    size_t data_size;

    if ((config == NULL) || ((mgmt_size_out == NULL) && (data_size_out == NULL)))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if (hashtable_mgmt_layout(config, NULL, NULL, &mgmt_size) != 0)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if (hashtable_data_region_size(config, &data_size) != 0)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if (mgmt_size_out != NULL)
    {
        *mgmt_size_out = mgmt_size;
    }
    if (data_size_out != NULL)
    {
        *data_size_out = data_size;
    }
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_hashtable_create(const com_util_hashtable_config *config, void *buf_mgmt, size_t buf_mgmt_size,
                              void *buf_data, size_t buf_data_size, com_util_hashtable **ht_out)
{
    size_t mgmt_size;
    size_t data_size;
    com_util_hashtable *ht;
    unsigned char owns_buffer;
    unsigned char *data;

    if (ht_out != NULL)
    {
        *ht_out = NULL;
    }
    if ((ht_out == NULL) || (config == NULL))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if (config->capacity == 0)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if ((config->key_type != COM_UTIL_HASHTABLE_KEY_STRING) && (config->key_type != COM_UTIL_HASHTABLE_KEY_BINARY))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if ((config->timestamp_scope != COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_TABLE) &&
        (config->timestamp_scope != COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_RECORD))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if ((config->key_size == 0) || (config->record_size == 0))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    /* REC_DELETED が 2 なので、寿命は削除直後に空へ戻す 2 以上が必要。 */
    if (config->lifetime < 2)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    /* 管理領域とデータ領域は、両方 NULL(内部確保)か両方非 NULL(外部指定)のいずれかに限る。 */
    if ((buf_mgmt == NULL) != (buf_data == NULL))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }

    if (hashtable_mgmt_layout(config, NULL, NULL, &mgmt_size) != 0)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if (hashtable_data_region_size(config, &data_size) != 0)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }

    if (buf_mgmt == NULL)
    {
        size_t total_size;

        if (add_checked(mgmt_size, data_size, &total_size) != 0)
        {
            return COM_UTIL_ERR_INVALID_ARGUMENT;
        }
        ht = (com_util_hashtable *)com_util_calloc(1, total_size);
        if (ht == NULL)
        {
            return COM_UTIL_ERR_OUT_OF_MEMORY;
        }
        data = hashtable_bytes(ht) + mgmt_size;
        owns_buffer = 1;
    }
    else
    {
        if (buf_mgmt_size < mgmt_size)
        {
            return COM_UTIL_ERR_BUFFER_TOO_SMALL;
        }
        /* 不足と同じコードにする。領域へ触れる前に戻る。 */
        if (((uintptr_t)buf_mgmt % _Alignof(struct com_util_hashtable)) != 0)
        {
            return COM_UTIL_ERR_BUFFER_TOO_SMALL;
        }
        /* データ領域は memcpy 経由でしかアクセスしないため、アラインメント要件はありません。 */
        if (buf_data_size < data_size)
        {
            return COM_UTIL_ERR_BUFFER_TOO_SMALL;
        }
        memset(buf_mgmt, 0, mgmt_size);
        memset(buf_data, 0, data_size);
        ht = (com_util_hashtable *)buf_mgmt;
        data = (unsigned char *)buf_data;
        owns_buffer = 0;
    }

    ht->magic = COM_UTIL_HASHTABLE_MAGIC;
    ht->version = COM_UTIL_HASHTABLE_VERSION;
    ht->data = data;
    ht->config.capacity = config->capacity;
    ht->config.key_type = config->key_type;
    ht->config.timestamp_scope = config->timestamp_scope;
    ht->config.key_size = config->key_size;
    ht->config.record_size = config->record_size;
    ht->config.lifetime = config->lifetime;
    ht->owns_buffer = owns_buffer;
    ht->next_empty = 1; /* 全スロット空きなので、最小空きはレコード 1。 */

    *ht_out = ht;
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_hashtable_get_config_ref(const com_util_hashtable *ht, const com_util_hashtable_config **config_out)
{
    if ((ht == NULL) || (config_out == NULL))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    *config_out = &ht->config;
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_hashtable_get_config_val(const com_util_hashtable *ht, com_util_hashtable_config *config_out)
{
    const com_util_hashtable_config *src;
    int ret;

    if (config_out == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    ret = com_util_hashtable_get_config_ref(ht, &src);
    if (ret != COM_UTIL_OK)
    {
        return ret;
    }
    *config_out = *src;
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_hashtable_buffer_size(const com_util_hashtable *ht, size_t *mgmt_size_out, size_t *data_size_out)
{
    if ((ht == NULL) || ((mgmt_size_out == NULL) && (data_size_out == NULL)))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if (mgmt_size_out != NULL)
    {
        (void)hashtable_mgmt_layout(&ht->config, NULL, NULL, mgmt_size_out);
    }
    if (data_size_out != NULL)
    {
        (void)hashtable_data_region_size(&ht->config, data_size_out);
    }
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_hashtable_buffer_ref(const com_util_hashtable *ht, const void **mgmt_out, const void **data_out)
{
    if ((ht == NULL) || ((mgmt_out == NULL) && (data_out == NULL)))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if (mgmt_out != NULL)
    {
        *mgmt_out = hashtable_bytes(ht);
    }
    if (data_out != NULL)
    {
        *data_out = hashtable_data(ht);
    }
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_hashtable_attach(void *buf_mgmt, size_t buf_mgmt_size, void *buf_data, size_t buf_data_size,
                              com_util_hashtable **ht_out)
{
    com_util_hashtable *ht;
    size_t mgmt_size;
    size_t data_size;

    if (ht_out != NULL)
    {
        *ht_out = NULL;
    }
    if ((ht_out == NULL) || (buf_mgmt == NULL) || (buf_data == NULL))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    /* マジックを読むために、まず内部ヘッダー分だけを要求する。 */
    if (buf_mgmt_size < sizeof(struct com_util_hashtable))
    {
        return COM_UTIL_ERR_BUFFER_TOO_SMALL;
    }
    if (((uintptr_t)buf_mgmt % _Alignof(struct com_util_hashtable)) != 0)
    {
        return COM_UTIL_ERR_BUFFER_TOO_SMALL;
    }

    ht = (com_util_hashtable *)buf_mgmt;

    if (ht->magic != COM_UTIL_HASHTABLE_MAGIC)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if (ht->version != COM_UTIL_HASHTABLE_VERSION)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if ((ht->config.key_type != COM_UTIL_HASHTABLE_KEY_STRING) &&
        (ht->config.key_type != COM_UTIL_HASHTABLE_KEY_BINARY))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if ((ht->config.timestamp_scope != COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_TABLE) &&
        (ht->config.timestamp_scope != COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_RECORD))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if ((ht->config.key_size == 0) || (ht->config.record_size == 0))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if (ht->config.lifetime < 2)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if (ht->config.capacity == 0)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    /* 0 は満杯の正当値。capacity 超えだけを拒否する。 */
    if (ht->next_empty > ht->config.capacity)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }

    if (hashtable_mgmt_layout(&ht->config, NULL, NULL, &mgmt_size) != 0)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if (buf_mgmt_size < mgmt_size)
    {
        return COM_UTIL_ERR_BUFFER_TOO_SMALL;
    }
    if (hashtable_data_region_size(&ht->config, &data_size) != 0)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if (buf_data_size < data_size)
    {
        return COM_UTIL_ERR_BUFFER_TOO_SMALL;
    }

    /* data は実行時のみ有効なアドレスのため、管理領域バイト列に残る値は信用せず必ず上書きする。 */
    ht->data = (unsigned char *)buf_data;
    /* 再接続後の所有は常に呼び出し側。内部確保の印は残さない。 */
    ht->owns_buffer = 0;
    *ht_out = ht;
    return COM_UTIL_OK;
}

/**
 *  @brief          チェーンと空きヒントの整合を検査します。
 *  @param[in]      ht       対象。NULL を渡してはなりません。
 *  @param[in,out]  visited  capacity バイトの作業領域。呼び出し側が 0 埋めします。
 *  @return         整合なら 0、破損なら -1 です。
 */
static int hashtable_validate_impl(const com_util_hashtable *ht, unsigned char *visited)
{
    size_t capacity = ht->config.capacity;
    uint64_t *bucket_head = hashtable_bucket_head(ht);
    size_t idx;
    size_t min_empty = 0;
    size_t i;

    for (idx = 0; idx < capacity; idx++)
    {
        uint64_t cur = bucket_head[idx];
        size_t hops = 0;

        if (cur > capacity)
        {
            return -1;
        }
        while (cur != 0)
        {
            size_t rec;
            char *stored_key;
            uint64_t next;

            /* capacity を超える追跡は循環。 */
            if (hops++ >= capacity)
            {
                return -1;
            }
            rec = (size_t)(cur - 1);
            /* 同一スロットが複数チェーンに載ってはなりません。 */
            if (visited[rec] != 0)
            {
                return -1;
            }
            /* 空きはチェーンから外れているはず。 */
            if (*hashtable_entry_status(ht, rec) == REC_EMPTY)
            {
                return -1;
            }
            stored_key = hashtable_entry_key(ht, rec);
            if (!key_fits(ht, stored_key))
            {
                return -1;
            }
            /* キーのハッシュ先と、載っているバケットが一致すること。 */
            if (hash_key(ht, stored_key) != idx)
            {
                return -1;
            }
            visited[rec] = 1;

            next = *hashtable_entry_next(ht, rec);
            if (next > capacity)
            {
                return -1;
            }
            cur = next;
        }
    }

    for (i = 0; i < capacity; i++)
    {
        unsigned char status = *hashtable_entry_status(ht, i);

        if (status == REC_EMPTY)
        {
            if (min_empty == 0)
            {
                min_empty = i + 1;
            }
        }
        else if (visited[i] == 0)
        {
            /* 実装中または削除済みなのに、どのバケットからも辿れない。 */
            return -1;
        }
    }

    if (ht->next_empty != min_empty)
    {
        return -1;
    }
    return 0;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_hashtable_validate(const com_util_hashtable *ht)
{
    unsigned char *visited;
    int result;

    if (ht == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    visited = (unsigned char *)com_util_calloc(ht->config.capacity, 1);
    if (visited == NULL)
    {
        return COM_UTIL_ERR_OUT_OF_MEMORY;
    }
    result = hashtable_validate_impl(ht, visited);
    com_util_free(visited);
    return (result == 0) ? COM_UTIL_OK : COM_UTIL_ERR_CORRUPT_DESCRIPTOR;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_hashtable_add(com_util_hashtable *ht, const void *key, const void *value)
{
    unsigned long idx;
    uint64_t *bucket_head;
    uint64_t cur;
    uint64_t rec_no;
    size_t rec;

    if ((ht == NULL) || (key == NULL) || (value == NULL))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if (!key_fits(ht, key))
    {
        return COM_UTIL_ERR_OUT_OF_RANGE;
    }

    idx = hash_key(ht, key);
    bucket_head = hashtable_bucket_head(ht);
    cur = bucket_head[idx];
    while (cur != 0)
    {
        rec = (size_t)(cur - 1);
        if (key_equal(ht, hashtable_entry_key(ht, rec), key))
        {
            if (*hashtable_entry_status(ht, rec) == REC_IN_USE)
            {
                return COM_UTIL_ERR_DUPLICATE_DEFINITION;
            }
            /* 削除済みの同一キーは、同じレコードを再利用する。 */
            memcpy(hashtable_data_at(ht, rec), value, ht->config.record_size);
            *hashtable_entry_status(ht, rec) = REC_IN_USE;
            stamp_record(ht, rec);
            return COM_UTIL_OK;
        }
        cur = *hashtable_entry_next(ht, rec);
    }

    rec_no = ht->next_empty;
    if (rec_no == 0)
    {
        return COM_UTIL_ERR_LIMIT_EXCEEDED;
    }

    rec = (size_t)(rec_no - 1);
    memcpy(hashtable_data_at(ht, rec), value, ht->config.record_size);
    *hashtable_entry_status(ht, rec) = REC_IN_USE;
    key_store(ht, hashtable_entry_key(ht, rec), key);
    stamp_record(ht, rec);
    /* 新しいレコードをチェーン先頭へ挿す。 */
    *hashtable_entry_next(ht, rec) = bucket_head[idx];
    bucket_head[idx] = rec_no;
    ht->next_empty = scan_next_empty(ht, rec + 1);
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_hashtable_insert_direct(com_util_hashtable *ht, uint64_t record, const void *key, int status,
                                     const void *value, const com_util_timespec *timestamp)
{
    unsigned long idx;
    uint64_t *bucket_head;
    uint64_t cur;
    size_t rec;

    if ((ht == NULL) || (key == NULL) || (value == NULL))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if (hashtable_has_record_timestamp(ht) != 0)
    {
        if (timestamp == NULL)
        {
            return COM_UTIL_ERR_INVALID_ARGUMENT;
        }
    }
    else if (timestamp != NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if ((status < REC_IN_USE) || (status > COM_UTIL_HASHTABLE_LIFETIME_INFINITE))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if (!key_fits(ht, key))
    {
        return COM_UTIL_ERR_OUT_OF_RANGE;
    }
    if ((record == 0) || (record > ht->config.capacity))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    /* 有限寿命では、加齢後に存在し得ない status は書かずに省略する。 */
    if ((status >= REC_DELETED) && (ht->config.lifetime != COM_UTIL_HASHTABLE_LIFETIME_INFINITE) &&
        (status >= (int)ht->config.lifetime))
    {
        return COM_UTIL_SKIPPED;
    }

    rec = (size_t)(record - 1);
    if (*hashtable_entry_status(ht, rec) != REC_EMPTY)
    {
        return COM_UTIL_ERR_DUPLICATE_DEFINITION;
    }

    idx = hash_key(ht, key);
    bucket_head = hashtable_bucket_head(ht);
    cur = bucket_head[idx];
    while (cur != 0)
    {
        size_t existing = (size_t)(cur - 1);

        if (key_equal(ht, hashtable_entry_key(ht, existing), key))
        {
            return COM_UTIL_ERR_DUPLICATE_DEFINITION;
        }
        cur = *hashtable_entry_next(ht, existing);
    }

    memcpy(hashtable_data_at(ht, rec), value, ht->config.record_size);
    key_store(ht, hashtable_entry_key(ht, rec), key);
    *hashtable_entry_status(ht, rec) = (unsigned char)status;
    if (hashtable_has_record_timestamp(ht) != 0)
    {
        *hashtable_entry_timestamp(ht, rec) = *timestamp;
        if (com_util_timespec_cmp(timestamp, &ht->table_timestamp) > 0)
        {
            ht->table_timestamp = *timestamp;
        }
    }
    *hashtable_entry_next(ht, rec) = bucket_head[idx];
    bucket_head[idx] = record;
    /* 最小空きを使ったときだけ、次の空きを探し直す。 */
    if (ht->next_empty == record)
    {
        ht->next_empty = scan_next_empty(ht, rec + 1);
    }
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_hashtable_update(com_util_hashtable *ht, const void *key, const void *value)
{
    unsigned long idx;
    uint64_t *bucket_head;
    uint64_t cur;

    if ((ht == NULL) || (key == NULL) || (value == NULL))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if (!key_fits(ht, key))
    {
        return COM_UTIL_ERR_OUT_OF_RANGE;
    }

    idx = hash_key(ht, key);
    bucket_head = hashtable_bucket_head(ht);
    cur = bucket_head[idx];
    while (cur != 0)
    {
        size_t rec = (size_t)(cur - 1);

        if (key_equal(ht, hashtable_entry_key(ht, rec), key))
        {
            /* 削除済みは更新対象にしない。add のような再利用はしない。 */
            if (*hashtable_entry_status(ht, rec) != REC_IN_USE)
            {
                return COM_UTIL_ERR_NOT_FOUND;
            }
            memcpy(hashtable_data_at(ht, rec), value, ht->config.record_size);
            stamp_record(ht, rec);
            return COM_UTIL_OK;
        }
        cur = *hashtable_entry_next(ht, rec);
    }
    return COM_UTIL_ERR_NOT_FOUND;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_hashtable_update_rec(com_util_hashtable *ht, uint64_t record, const void *value)
{
    size_t rec;

    if ((ht == NULL) || (value == NULL))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if ((record == 0) || (record > ht->config.capacity))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    rec = (size_t)(record - 1);
    if (*hashtable_entry_status(ht, rec) != REC_IN_USE)
    {
        return COM_UTIL_ERR_NOT_FOUND;
    }
    memcpy(hashtable_data_at(ht, rec), value, ht->config.record_size);
    stamp_record(ht, rec);
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_hashtable_find_value_ref(const com_util_hashtable *ht, const void *key, const void **value_out)
{
    unsigned long idx;
    uint64_t *bucket_head;
    uint64_t cur;

    if ((ht == NULL) || (key == NULL) || (value_out == NULL))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if (!key_fits(ht, key))
    {
        return COM_UTIL_ERR_OUT_OF_RANGE;
    }

    idx = hash_key(ht, key);
    bucket_head = hashtable_bucket_head(ht);
    cur = bucket_head[idx];
    while (cur != 0)
    {
        size_t rec = (size_t)(cur - 1);

        if (key_equal(ht, hashtable_entry_key(ht, rec), key))
        {
            /* 削除済みは見つからない扱い。キーは残っていても返さない。 */
            if (*hashtable_entry_status(ht, rec) == REC_IN_USE)
            {
                *value_out = hashtable_data_at(ht, rec);
                return COM_UTIL_OK;
            }
            return COM_UTIL_ERR_NOT_FOUND;
        }
        cur = *hashtable_entry_next(ht, rec);
    }
    return COM_UTIL_ERR_NOT_FOUND;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_hashtable_find_value_val(const com_util_hashtable *ht, const void *key, void *value_out)
{
    const void *src;
    int ret;

    if (value_out == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    ret = com_util_hashtable_find_value_ref(ht, key, &src);
    if (ret != COM_UTIL_OK)
    {
        return ret;
    }
    memcpy(value_out, src, ht->config.record_size);
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_hashtable_find_recno(const com_util_hashtable *ht, const void *key, uint64_t *record_out)
{
    unsigned long idx;
    uint64_t *bucket_head;
    uint64_t cur;

    if ((ht == NULL) || (key == NULL) || (record_out == NULL))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if (!key_fits(ht, key))
    {
        return COM_UTIL_ERR_OUT_OF_RANGE;
    }

    idx = hash_key(ht, key);
    bucket_head = hashtable_bucket_head(ht);
    cur = bucket_head[idx];
    while (cur != 0)
    {
        size_t rec = (size_t)(cur - 1);

        if (key_equal(ht, hashtable_entry_key(ht, rec), key))
        {
            /* 削除済みはレコード番号も返さない。 */
            if (*hashtable_entry_status(ht, rec) == REC_IN_USE)
            {
                *record_out = cur;
                return COM_UTIL_OK;
            }
            return COM_UTIL_ERR_NOT_FOUND;
        }
        cur = *hashtable_entry_next(ht, rec);
    }
    return COM_UTIL_ERR_NOT_FOUND;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_hashtable_find_timestamp_ref(const com_util_hashtable *ht, const void *key,
                                          const com_util_timespec **timestamp_out)
{
    unsigned long idx;
    uint64_t *bucket_head;
    uint64_t cur;

    if ((ht == NULL) || (key == NULL) || (timestamp_out == NULL))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if (hashtable_has_record_timestamp(ht) == 0)
    {
        return COM_UTIL_ERR_UNSUPPORTED;
    }
    if (!key_fits(ht, key))
    {
        return COM_UTIL_ERR_OUT_OF_RANGE;
    }

    idx = hash_key(ht, key);
    bucket_head = hashtable_bucket_head(ht);
    cur = bucket_head[idx];
    while (cur != 0)
    {
        size_t rec = (size_t)(cur - 1);

        if (key_equal(ht, hashtable_entry_key(ht, rec), key))
        {
            if (*hashtable_entry_status(ht, rec) == REC_IN_USE)
            {
                *timestamp_out = hashtable_entry_timestamp(ht, rec);
                return COM_UTIL_OK;
            }
            return COM_UTIL_ERR_NOT_FOUND;
        }
        cur = *hashtable_entry_next(ht, rec);
    }
    return COM_UTIL_ERR_NOT_FOUND;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_hashtable_find_timestamp_val(const com_util_hashtable *ht, const void *key,
                                          com_util_timespec *timestamp_out)
{
    const com_util_timespec *src;
    int ret;

    if (timestamp_out == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    ret = com_util_hashtable_find_timestamp_ref(ht, key, &src);
    if (ret != COM_UTIL_OK)
    {
        return ret;
    }
    *timestamp_out = *src;
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_hashtable_get_key_ref(const com_util_hashtable *ht, uint64_t record, const void **key_out)
{
    size_t rec;

    if ((ht == NULL) || (key_out == NULL))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if ((record == 0) || (record > ht->config.capacity))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    rec = (size_t)(record - 1);
    /* 空は失敗。削除済みは削除直前のキーを返す。 */
    if (*hashtable_entry_status(ht, rec) == REC_EMPTY)
    {
        return COM_UTIL_ERR_NOT_FOUND;
    }
    *key_out = hashtable_entry_key(ht, rec);
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_hashtable_get_key_val(const com_util_hashtable *ht, uint64_t record, void *key_out)
{
    const void *src;
    int ret;

    if (key_out == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    ret = com_util_hashtable_get_key_ref(ht, record, &src);
    if (ret != COM_UTIL_OK)
    {
        return ret;
    }
    memcpy(key_out, src, ht->config.key_size);
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_hashtable_get_value_ref(const com_util_hashtable *ht, uint64_t record, const void **value_out)
{
    size_t rec;

    if ((ht == NULL) || (value_out == NULL))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if ((record == 0) || (record > ht->config.capacity))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    rec = (size_t)(record - 1);
    /* 空は失敗。削除済みは削除直前の値を返す。 */
    if (*hashtable_entry_status(ht, rec) == REC_EMPTY)
    {
        return COM_UTIL_ERR_NOT_FOUND;
    }
    *value_out = hashtable_data_at(ht, rec);
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_hashtable_get_value_val(const com_util_hashtable *ht, uint64_t record, void *value_out)
{
    const void *src;
    int ret;

    if (value_out == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    ret = com_util_hashtable_get_value_ref(ht, record, &src);
    if (ret != COM_UTIL_OK)
    {
        return ret;
    }
    memcpy(value_out, src, ht->config.record_size);
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_hashtable_get_status(const com_util_hashtable *ht, uint64_t record, int *status_out)
{
    if ((ht == NULL) || (status_out == NULL))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if ((record == 0) || (record > ht->config.capacity))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    *status_out = (int)*hashtable_entry_status(ht, (size_t)(record - 1));
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_hashtable_get_timestamp_ref(const com_util_hashtable *ht, uint64_t record,
                                         const com_util_timespec **timestamp_out)
{
    size_t rec;

    if ((ht == NULL) || (timestamp_out == NULL))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if (hashtable_has_record_timestamp(ht) == 0)
    {
        return COM_UTIL_ERR_UNSUPPORTED;
    }
    if ((record == 0) || (record > ht->config.capacity))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    rec = (size_t)(record - 1);
    if (*hashtable_entry_status(ht, rec) == REC_EMPTY)
    {
        return COM_UTIL_ERR_NOT_FOUND;
    }
    *timestamp_out = hashtable_entry_timestamp(ht, rec);
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_hashtable_get_timestamp_val(const com_util_hashtable *ht, uint64_t record,
                                         com_util_timespec *timestamp_out)
{
    const com_util_timespec *src;
    int ret;

    if (timestamp_out == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    ret = com_util_hashtable_get_timestamp_ref(ht, record, &src);
    if (ret != COM_UTIL_OK)
    {
        return ret;
    }
    *timestamp_out = *src;
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_hashtable_get_table_timestamp_ref(const com_util_hashtable *ht, const com_util_timespec **timestamp_out)
{
    if ((ht == NULL) || (timestamp_out == NULL))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    *timestamp_out = &ht->table_timestamp;
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_hashtable_get_table_timestamp_val(const com_util_hashtable *ht, com_util_timespec *timestamp_out)
{
    const com_util_timespec *src;
    int ret;

    if (timestamp_out == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    ret = com_util_hashtable_get_table_timestamp_ref(ht, &src);
    if (ret != COM_UTIL_OK)
    {
        return ret;
    }
    *timestamp_out = *src;
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_hashtable_count_status(const com_util_hashtable *ht, size_t *in_use_out, size_t *deleted_out,
                                    size_t *empty_out)
{
    size_t in_use = 0;
    size_t deleted = 0;
    size_t empty = 0;
    size_t i;

    if (ht == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }

    for (i = 0; i < ht->config.capacity; i++)
    {
        unsigned char status = *hashtable_entry_status(ht, i);

        if (status == REC_IN_USE)
        {
            in_use++;
        }
        else if (status == REC_EMPTY)
        {
            empty++;
        }
        else
        {
            /* 2 以上は加齢中の削除済み。255 も含める。 */
            deleted++;
        }
    }
    if (in_use_out != NULL)
    {
        *in_use_out = in_use;
    }
    if (deleted_out != NULL)
    {
        *deleted_out = deleted;
    }
    if (empty_out != NULL)
    {
        *empty_out = empty;
    }
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_hashtable_count(const com_util_hashtable *ht, size_t *count_out)
{
    if ((ht == NULL) || (count_out == NULL))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    return com_util_hashtable_count_status(ht, count_out, NULL, NULL);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_hashtable_deleted_count(const com_util_hashtable *ht, size_t *count_out)
{
    if ((ht == NULL) || (count_out == NULL))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    return com_util_hashtable_count_status(ht, NULL, count_out, NULL);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_hashtable_empty_count(const com_util_hashtable *ht, size_t *count_out)
{
    if ((ht == NULL) || (count_out == NULL))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    return com_util_hashtable_count_status(ht, NULL, NULL, count_out);
}

/**
 *  @brief          スロットを空へ戻し、キーと値と変更時刻を消します。
 *  @param[in]      ht   対象。NULL を渡してはなりません。
 *  @param[in]      rec  0 相対のスロット添字。
 *
 *  チェーンからの切り離しはしません。呼び出し側が外します。\n
 *  SCOPE_RECORD のときは変更時刻も 0 埋めします。
 */
static void hashtable_expire_record(com_util_hashtable *ht, size_t rec)
{
    *hashtable_entry_status(ht, rec) = REC_EMPTY;
    memset(hashtable_data_at(ht, rec), 0, ht->config.record_size);
    memset(hashtable_entry_key(ht, rec), 0, ht->config.key_size);
    if (hashtable_has_record_timestamp(ht) != 0)
    {
        memset(hashtable_entry_timestamp(ht, rec), 0, sizeof(com_util_timespec));
    }
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_hashtable_delete(com_util_hashtable *ht, const void *key)
{
    unsigned long idx;
    uint64_t *link;

    if ((ht == NULL) || (key == NULL))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if (!key_fits(ht, key))
    {
        return COM_UTIL_ERR_OUT_OF_RANGE;
    }

    idx = hash_key(ht, key);
    link = &hashtable_bucket_head(ht)[idx];
    while (*link != 0)
    {
        size_t rec = (size_t)(*link - 1);

        if (key_equal(ht, hashtable_entry_key(ht, rec), key))
        {
            if (*hashtable_entry_status(ht, rec) != REC_IN_USE)
            {
                return COM_UTIL_ERR_NOT_FOUND;
            }
            *hashtable_entry_status(ht, rec) = REC_DELETED;
            stamp_record(ht, rec);
            /* lifetime が 2 のときは、削除済みのまま置けないので直ちに空へ戻す。 */
            if (REC_DELETED >= ht->config.lifetime)
            {
                uint64_t rec_no = (uint64_t)(rec + 1);

                hashtable_expire_record(ht, rec);
                *link = *hashtable_entry_next(ht, rec);
                *hashtable_entry_next(ht, rec) = 0;
                if ((ht->next_empty == 0) || (rec_no < ht->next_empty))
                {
                    ht->next_empty = rec_no;
                }
            }
            return COM_UTIL_OK;
        }
        link = hashtable_entry_next(ht, rec);
    }
    return COM_UTIL_ERR_NOT_FOUND;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_hashtable_delete_rec(com_util_hashtable *ht, uint64_t record)
{
    size_t rec;

    if (ht == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if ((record == 0) || (record > ht->config.capacity))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    rec = (size_t)(record - 1);
    if (*hashtable_entry_status(ht, rec) != REC_IN_USE)
    {
        return COM_UTIL_ERR_NOT_FOUND;
    }
    return com_util_hashtable_delete(ht, hashtable_entry_key(ht, rec));
}

/**
 *  @brief          空きになったスロットを全バケットのチェーンから外します。
 *  @param[in]      ht  対象。NULL を渡してはなりません。
 *
 *  expire はチェーンを触らないため、加齢や一括回収のあとで呼びます。
 */
static void hashtable_unlink_empty_chains(com_util_hashtable *ht)
{
    uint64_t *bucket_head = hashtable_bucket_head(ht);
    size_t i;

    for (i = 0; i < ht->config.capacity; i++)
    {
        uint64_t *link = &bucket_head[i];

        while (*link != 0)
        {
            size_t rec = (size_t)(*link - 1);

            if (*hashtable_entry_status(ht, rec) == REC_EMPTY)
            {
                *link = *hashtable_entry_next(ht, rec);
                *hashtable_entry_next(ht, rec) = 0;
            }
            else
            {
                link = hashtable_entry_next(ht, rec);
            }
        }
    }
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_hashtable_push_deleted(com_util_hashtable *ht)
{
    uint64_t new_next_empty = 0;
    size_t i;

    if (ht == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }

    for (i = 0; i < ht->config.capacity; i++)
    {
        unsigned char *status = hashtable_entry_status(ht, i);

        /* 255 は増やさない。無限寿命では空へ戻さない。 */
        if ((*status >= REC_DELETED) && (*status < COM_UTIL_HASHTABLE_LIFETIME_INFINITE))
        {
            (*status)++;
            if ((*status >= ht->config.lifetime) && (ht->config.lifetime != COM_UTIL_HASHTABLE_LIFETIME_INFINITE))
            {
                hashtable_expire_record(ht, i);
            }
        }
        if ((new_next_empty == 0) && (*status == REC_EMPTY))
        {
            new_next_empty = (uint64_t)(i + 1);
        }
    }

    hashtable_unlink_empty_chains(ht);
    ht->next_empty = new_next_empty;
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_hashtable_purge_deleted(com_util_hashtable *ht)
{
    uint64_t new_next_empty = 0;
    size_t i;

    if (ht == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }

    for (i = 0; i < ht->config.capacity; i++)
    {
        /* 加齢せず、削除済みをすべて空へ戻す。255 も含める。 */
        if (*hashtable_entry_status(ht, i) >= REC_DELETED)
        {
            hashtable_expire_record(ht, i);
        }
        if ((new_next_empty == 0) && (*hashtable_entry_status(ht, i) == REC_EMPTY))
        {
            new_next_empty = (uint64_t)(i + 1);
        }
    }

    hashtable_unlink_empty_chains(ht);
    ht->next_empty = new_next_empty;
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_hashtable_clear(com_util_hashtable *ht)
{
    size_t off_bucket_head;
    size_t mgmt_size = 0;
    size_t data_size = 0;

    if (ht == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    (void)hashtable_mgmt_layout(&ht->config, &off_bucket_head, NULL, &mgmt_size);
    (void)hashtable_data_region_size(&ht->config, &data_size);
    /* 識別子、設定、所有フラグ、テーブル時刻、data ポインターは残し、バケットとエントリだけを消す。 */
    memset((unsigned char *)ht + off_bucket_head, 0, mgmt_size - off_bucket_head);
    /* 値配列(データ領域)も 0 埋めする。ポインター自体(ht->data)は変更しない。 */
    memset(ht->data, 0, data_size);
    ht->next_empty = 1;
    stamp_table(ht);
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_hashtable_destroy(com_util_hashtable *ht)
{
    if (ht == NULL)
    {
        return;
    }
    /* 外部バッファーや attach 済みは呼び出し側の所有のまま。 */
    if (ht->owns_buffer == 0)
    {
        return;
    }
    com_util_free(ht);
}
