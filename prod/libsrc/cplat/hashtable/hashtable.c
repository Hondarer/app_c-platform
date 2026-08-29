/**
 *******************************************************************************
 *  @file           hashtable.c
 *  @brief          固定レコード数と可変長文字列ストレージを持つハッシュ テーブルを実装します。
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
 *  メモリマップド ファイル (@c cplat_mmap) など管理領域と独立した領域を
 *  想定しています。\n
 *  管理領域とデータ領域は、いずれも永続化して意味のある状態だけで構成し、
 *  アドレス情報や一時的なフラグの類は含みません。@ref cplat_hashtable_create /
 *  @ref cplat_hashtable_attach は、呼び出しのたびに管理領域・データ領域とは別の
 *  「内部管理データ」を確保して不透明ハンドルとして返します。内部管理データは
 *  実行時のみ有効な参照であり、永続化してはならず、また呼び出し方によらず
 *  @ref cplat_hashtable_dispose で必ず解放してください。\n
 *  呼び出し側が両方 NULL を渡した場合 (@ref cplat_hashtable_create) は、
 *  内部で 1 回の確保にまとめ、データ領域は管理領域の直後に連続配置します。
 *  この場合に限り、解放も @ref cplat_hashtable_dispose の 1 回で両方が
 *  片付きます。\n
 *  呼び出し側が両方を明示的に渡した場合は、それぞれ独立した領域として扱い、
 *  解放は呼び出し側の責務です。片方だけ NULL の指定は
 *  @ref CPLAT_ERR_INVALID_ARGUMENT です。\n
 *  各領域の開始は、直前領域の末尾を指定アラインメントへ切り上げた位置です。\n
 *  @p timestamp_scope はエントリ配置と管理領域サイズに使います。\n
 *  @p key_type と @p lifetime はサイズ計算に使いません。\n
 *  必要バイト数の計算入口は @ref cplat_hashtable_required_size です。\n
 *  構築済みテーブルの各領域の先頭は @ref cplat_hashtable_buffer_ref 、
 *  バイト数は @ref cplat_hashtable_buffer_size で取得できます。
 *
    @code
    管理領域 (永続化してよい。アドレス・一時フラグを含まない):
    +----------------------+
    | magic (4B)           |  // CPLAT_HASHTABLE_MAGIC (0x48544142)
    +----------------------+
    | version (4B)         |  // CPLAT_HASHTABLE_VERSION (4)
    +----------------------+
    | header               |  // config, next_empty, in_use_count, deleted_count, table_timestamp,
    |                      |  // key_storage_used, value_storage_used, key_free_count, value_free_count
    +----------------------+
    | pad to uint64        |  // sizeof(struct) が未整列のときだけ
    +----------------------+
    | bucket_head[N]       |  // N x sizeof(uint64_t)
    +----------------------+
    | pad to uint64        |
    +----------------------+
    | entries[N]           |  // N x entry_stride
    +----------------------+
    | key free list[N+1]   |  // 可変長キー時だけ (N+1) x (offset + length)
    +----------------------+
    | key storage          |  // 可変長キー時だけ key_storage_size バイト
    +----------------------+

    データ領域 (管理領域と連続しない、独立したブロック):
    +----------------------+
    | value refs[N]        |  // 可変長値時だけ offset + length
    +----------------------+
    | value free list[N+1] |  // 可変長値時だけ (N+1) x (offset + length)
    +----------------------+
    | value storage        |  // 可変長値時だけ value_storage_size バイト
    +----------------------+
    | data[N]              |  // 固定長値時だけ N x value_size
    +----------------------+

    内部管理データ (create/attach のたびに別途確保し、dispose で解放する。永続化しない):
    +----------------------+
    | hdr(ポインター)       |  // 管理領域先頭への参照
    +----------------------+
    | data(ポインター)      |  // データ領域先頭への参照
    +----------------------+
    | owns_buffer           |  // 1 なら dispose が管理領域とデータ領域を解放する
    +----------------------+
    @endcode
 *
 *  N は @p capacity です。レコード番号は 1 相対で、内部添字は record - 1 です。
 *
 *  @subsection     hashtable_ident マジックと版番号
 *
 *  管理領域先頭 8 バイトは識別子です。ヘッダーより前に置きます。\n
 *  マジックは 0x48544142、版番号は 4 です。\n
 *  @ref cplat_hashtable_attach は両者を検証し、不一致なら失敗します。
 *
 *  @subsection     hashtable_header ヘッダー
 *
 *  版番号の直後から、設定の複製、@p next_empty 、実装中件数、削除済み件数、
 *  可変長ストレージの使用バイト数、空きリストの要素数、
 *  テーブル横断の変更時刻が続きます。\n
 *  いずれも永続化して意味のある状態です。アドレスや一時フラグの類は管理領域に
 *  含みません(それらは内部管理データ側が持ちます)。\n
 *  @p next_empty は 1 相対の最小空きスロットです。満杯のときは 0 です。\n
 *  実装中件数・削除済み件数は @ref cplat_hashtable_count_status 等が
 *  そのまま返す走査済みの値で、各更新 API が差分更新します。\n
 *  テーブル時刻は最後にキーまたは値が変わった実時刻です。構築直後は 0 です。\n
 *  テーブル時刻は @p timestamp_scope に関わらず常に持ちます。\n
 *  ヘッダー全体の大きさ、および暗黙パディングは処理系依存です。\n
 *  管理領域バッファーは永続化ヘッダーのアラインメント境界が必要です。\n
 *  データ領域バッファーは値配列への @c memcpy 経由アクセスのみのため、
 *  アラインメント要件はありません。
 *
 *  @subsection     hashtable_buckets バケット先頭
 *
 *  @p capacity 個の @c uint64_t です。\n
 *  値は 1 相対のレコード番号です。0 は後続が無いことを示します。
 *
 *  @subsection     hashtable_entries エントリ
 *
 *  キーのオフセットとストライドは @p timestamp_scope と @p key_type で変わります。\n
 *  可変長キーは 16 バイトの永続 descriptor を置くため、キーのオフセットを
 *  uint64_t 境界へ切り上げます。固定長キーでは切り上げません。\n
 *  ストライドは、キーの終端を uint64_t 境界へ切り上げた値です。
 *
    @code
    SCOPE_RECORD:
    +--------+--------+-------+------------+------------+-------------+-------+
    | next   | status | pad   | timestamp  | generation | key         | pad   |
    | 8B     | 1B     | 7B    | timespec   | 8B         | key_size    | to 8B |
    +--------+--------+-------+------------+------------+-------------+-------+

    SCOPE_TABLE (固定長キー):
    +--------+--------+-------------+-------+
    | next   | status | key         | pad   |
    | 8B     | 1B     | key_size    | to 8B |
    +--------+--------+-------------+-------+

    SCOPE_TABLE (可変長キー):
    +--------+--------+-------+------------------+
    | next   | status | pad   | key descriptor   |
    | 8B     | 1B     | 7B    | 16B              |
    +--------+--------+-------+------------------+
    @endcode
 *
 *  @p next は同一バケットの次レコード (1 相対) です。0 は終端です。\n
 *  @p status は 0 が空、1 が実装中、2 以上が削除済みの加齢です。\n
 *  @p timestamp は実時刻の変更時刻です。status が 0 のときは 0 埋めです。\n
 *  @p generation は変更のたびに 1 ずつ増える単調な値です。status が 0 のときは 0 です。\n
 *  文字列キーは NUL までを格納し、残りを 0 埋めします。
 *
 *  @subsection     hashtable_free_list 空きリスト
 *
 *  可変長ストレージ 1 個につき、空き領域を表す descriptor の配列を 1 本持ちます。\n
 *  キー側はキー ストレージの直前、値側は値ストレージの直前に置きます。\n
 *  要素は @c offset と @c length の対で、オフセットの昇順に並べます。\n
 *  隣接する空きブロックは必ず結合し、空きブロックが常に極大であるように保ちます。\n
 *  空きブロックと使用中ブロックは、ストレージ全体を過不足なく分割します。\n
 *  空きブロックは使用中ブロックで区切られるため、要素数の上限は capacity + 1 です。\n
 *  構築直後は要素数 1 で、ストレージ全体が 1 個の空きブロックです。\n
 *  確保は先頭からの先着適合で、要素数を H として O(H) です。断片化がなければ
 *  H は 1 のため、実質 O(1) で完了します。\n
 *  要素数は永続化ヘッダーの @c key_free_count と @c value_free_count が持ちます。
 *
 *  @subsection     hashtable_data 値配列 (データ領域)
 *
 *  各値は、@p value_size を @p value_align へ切り上げた幅で並べます。\n
 *  @p value_align が 0 のときは切り上げず、@p value_size バイトを隙間なく並べます。
 *  この場合、2 件目以降のアラインメントは @p value_size 次第です。\n
 *  内部確保 (両方 NULL) のときは、管理領域の直後をデータ領域が必要とする境界へ
 *  切り上げた位置にデータ領域を置きます。\n
 *  外部指定のときは、データ領域が必要とする境界を呼び出し側が満たします。\n
 *  データ領域が必要とする境界は、可変長値では descriptor の境界、固定長値では
 *  @p value_align です。@p value_align が 0 なら境界の制約はありません。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include <cplat/hashtable/hashtable.h>

#include <cplat/clock/clock.h>
#include <cplat/clock/timespec.h>
#include <cplat/crt/stdlib.h>
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

_Static_assert(((sizeof(uint64_t) + sizeof(unsigned char) + 7u) % _Alignof(cplat_timespec)) == 0,
               "entry time offset must satisfy alignof(cplat_timespec)");

_Static_assert(((sizeof(uint64_t) + sizeof(unsigned char) + 7u + sizeof(cplat_timespec)) % _Alignof(uint64_t)) == 0,
               "entry generation offset must satisfy alignof(uint64_t)");

#define CPLAT_HASHTABLE_MAGIC   0x48544142u /**< 管理領域先頭の識別子です。 */
#define CPLAT_HASHTABLE_VERSION 4u          /**< 配置の版番号です。 */

struct hashtable_string_ref
{
    uint64_t offset;
    uint64_t length;
};

/**
 *  @brief          可変長ストレージの空き領域 1 個を表します。
 *
 *  空きリストの要素です。オフセットの昇順に隙間なく並べ、隣接する空きブロックは
 *  必ず結合します。使用中ブロックと空きブロックは、ストレージ全体を過不足なく分割します。
 */
struct hashtable_free_block
{
    uint64_t offset; /**< 空き領域の先頭オフセットです。 */
    uint64_t length; /**< 空き領域のバイト数です。0 にはなりません。 */
};

/**
 *  @brief          永続化領域先頭のヘッダーです。
 *
 *  識別子、設定の複製、空きヒント、実装中・削除済み件数、テーブル時刻を
 *  まとめています。\n
 *  永続化して意味を持つ情報だけで構成し、実行時のみ有効なアドレスや
 *  一時的なフラグは含みません。\n
 *  配置の図は本ファイル先頭の説明を参照してください。
 */
struct hashtable_persist_header
{
    uint32_t magic;                    /**< 識別子。@c CPLAT_HASHTABLE_MAGIC 。 */
    uint32_t version;                  /**< 配置の版。@c CPLAT_HASHTABLE_VERSION 。 */
    cplat_hashtable_config config;  /**< 構築時の設定の複製です。 */
    uint64_t next_empty;               /**< 1 相対の最小空きです。満杯のときは 0 です。 */
    uint64_t in_use_count;             /**< 実装中の件数です。 */
    uint64_t deleted_count;            /**< 削除済み(加齢中および終端 255 を含む)の件数です。 */
    uint64_t key_storage_used;         /**< 可変長キー ストレージの使用バイト数です。 */
    uint64_t value_storage_used;       /**< 可変長値ストレージの使用バイト数です。 */
    uint64_t key_free_count;           /**< 可変長キー ストレージの空きリストの要素数です。 */
    uint64_t value_free_count;         /**< 可変長値ストレージの空きリストの要素数です。 */
    cplat_timespec table_timestamp; /**< 最後にキーまたは値が変わった実時刻です。 */
    uint64_t table_generation;         /**< 変更のたびに 1 ずつ増える単調な値です。 */
};

/* ヘッダー サイズは hashtable_mgmt_layout の整列前提(uint64_t 境界)を満たす。 */
_Static_assert(sizeof(struct hashtable_persist_header) % _Alignof(uint64_t) == 0,
               "hashtable_persist_header size must be a multiple of _Alignof(uint64_t)");

/**
 *  @brief          ハッシュ テーブルの内部管理データです。
 *
 *  @ref cplat_hashtable_create と @ref cplat_hashtable_attach が
 *  呼び出しのたびに新規確保し、@ref cplat_hashtable_dispose が解放します。\n
 *  永続化領域・データ領域とは別の割り当てで、実行時のみ有効な参照だけを持ちます。
 */
struct cplat_hashtable
{
    struct hashtable_persist_header *hdr; /**< 永続化領域の先頭です。実行時のみ有効な内部参照です。 */
    unsigned char *data;                  /**< データ領域(値配列)の先頭です。実行時のみ有効です。 */
    unsigned char owns_buffer;            /**< 1 なら @ref cplat_hashtable_dispose が永続化領域と
                                     データ領域をあわせて解放します。 */
    unsigned char pad[7];                 /**< owns_buffer のあとの明示パディングです。 */
};

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
 *  SCOPE_RECORD では next + status + pad + timespec + generation のあとです。\n
 *  SCOPE_TABLE では next + status の直後です。\n
 *  可変長キーは 16 バイトの永続 descriptor を置くため、uint64_t 境界へ切り上げます。
 *  切り上げないと descriptor が境界から外れ、未整列アクセスになります。
 */
static size_t entry_key_offset(const cplat_hashtable_config *config)
{
    size_t off = sizeof(uint64_t) + sizeof(unsigned char);
    size_t rem;

    if (config->timestamp_scope == CPLAT_HASHTABLE_TIMESTAMP_SCOPE_RECORD)
    {
        off += (size_t)ENTRY_TIMESTAMP_PAD + sizeof(cplat_timespec) + sizeof(uint64_t);
    }
    if (config->key_type != CPLAT_HASHTABLE_FIELD_VARIABLE_STRING)
    {
        return off;
    }
    /* off は最大でも 40 のため、切り上げであふれません。 */
    rem = off % _Alignof(struct hashtable_string_ref);
    if (rem != 0)
    {
        off += _Alignof(struct hashtable_string_ref) - rem;
    }
    return off;
}

/**
 *  @brief          エントリ先頭から世代カウンターまでのオフセットを返します。
 *  @return         バイト オフセットです。
 *
 *  SCOPE_RECORD のときだけ意味を持ちます。
 */
static size_t entry_generation_offset(void)
{
    return sizeof(uint64_t) + sizeof(unsigned char) + (size_t)ENTRY_TIMESTAMP_PAD + sizeof(cplat_timespec);
}

static int field_is_variable(cplat_hashtable_field_type type)
{
    return type == CPLAT_HASHTABLE_FIELD_VARIABLE_STRING;
}

static size_t key_slot_size(const cplat_hashtable_config *config)
{
    return (field_is_variable(config->key_type) != 0) ? sizeof(struct hashtable_string_ref) : config->key_size;
}

/**
 *  @brief          固定長値 1 件が占めるバイト数を求めます。
 *  @param[in]      config      設定。NULL を渡してはなりません。
 *  @param[out]     stride_out  ストライドの格納先。あふれ時は書きません。
 *  @return         成功なら 0、あふれなら 1 です。
 *
 *  value_align が 0 のときは value_size をそのまま返し、値を隙間なく並べます。\n
 *  非 0 のときは value_size をその境界へ切り上げ、各値を境界へ整列させます。
 */
static int value_stride_checked(const cplat_hashtable_config *config, size_t *stride_out)
{
    if (config->value_align == 0)
    {
        *stride_out = config->value_size;
        return 0;
    }
    return align_up_checked(config->value_size, config->value_align, stride_out);
}

/**
 *  @brief          データ領域の先頭に必要なアラインメント境界を返します。
 *  @param[in]      config  設定。NULL を渡してはなりません。
 *  @return         1 以上の境界です。1 は制約なしと同じです。
 *
 *  可変長値では永続 descriptor を置くためその境界、固定長値では value_align です。
 */
static size_t data_region_align(const cplat_hashtable_config *config)
{
    if (field_is_variable(config->value_type) != 0)
    {
        return _Alignof(struct hashtable_string_ref);
    }
    if (config->value_align == 0)
    {
        return 1u;
    }
    return config->value_align;
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
static int entry_stride_checked(const cplat_hashtable_config *config, size_t *stride_out)
{
    size_t raw;

    if (add_checked(entry_key_offset(config), key_slot_size(config), &raw) != 0)
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
static int hashtable_has_record_timestamp(const cplat_hashtable *ht)
{
    return ht->hdr->config.timestamp_scope == CPLAT_HASHTABLE_TIMESTAMP_SCOPE_RECORD;
}

/**
 *  @brief          空きリストのバイト数を求めます。
 *  @param[in]      config    設定。NULL を渡してはなりません。
 *  @param[out]     size_out  バイト数の格納先。NULL を渡してはなりません。
 *  @return         成功なら 0、あふれなら -1 です。
 *
 *  空きブロックは使用中ブロックで区切られるため、個数の上限は capacity + 1 です。\n
 *  可変長フィールドのときだけ領域を確保します。
 */
static int hashtable_free_list_region_size(const cplat_hashtable_config *config, size_t *size_out)
{
    size_t count;

    if (add_checked(config->capacity, 1u, &count) != 0)
    {
        return -1;
    }
    if (mul_checked(count, sizeof(struct hashtable_free_block), size_out) != 0)
    {
        return -1;
    }
    return 0;
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
 *  @p timestamp_scope はエントリ配置に使います。@p lifetime は使いません。\n
 *  @p key_type は、可変長のとき空きリストを置くために使います。\n
 *  構築済みテーブルではあふれません。呼び出し側は戻り値を捨てて構いません。
 */
static int hashtable_mgmt_layout(const cplat_hashtable_config *config, size_t *off_bucket_head, size_t *off_entries,
                                 size_t *mgmt_size_out)
{
    size_t offset = sizeof(struct hashtable_persist_header);
    size_t region_size;
    size_t entry_stride;

    /* ヘッダー サイズは uint64_t 境界の倍数(構造体定義直後の _Static_assert で保証)。整列不要。 */

    /* あふれ検査より前に仮値で初期化し、早期 return でも呼び出し側へ未初期化値を渡さない。
       呼び出し側の大半は戻り値を捨てるため、この関数の契約として out パラメーターは
       常に定義済みの値を持つ必要がある。 */
    if (off_bucket_head != NULL)
    {
        *off_bucket_head = offset;
    }
    if (off_entries != NULL)
    {
        *off_entries = offset;
    }
    if (mgmt_size_out != NULL)
    {
        *mgmt_size_out = offset;
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
    /* 空きリストはキー ストレージの直前に置く。キー ストレージは管理領域の末尾のままとする。 */
    if (field_is_variable(config->key_type) != 0)
    {
        if (hashtable_free_list_region_size(config, &region_size) != 0)
        {
            return -1;
        }
        if (add_checked(offset, region_size, &offset) != 0)
        {
            return -1;
        }
    }

    if (add_checked(offset, config->key_storage_size, &offset) != 0)
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
static int hashtable_data_region_size(const cplat_hashtable_config *config, size_t *data_size_out)
{
    size_t size;
    size_t free_list_size;

    if (field_is_variable(config->value_type) == 0)
    {
        size_t stride;

        if (value_stride_checked(config, &stride) != 0)
        {
            return -1;
        }
        if (mul_checked(config->capacity, stride, data_size_out) != 0)
        {
            return -1;
        }
        return 0;
    }
    if (mul_checked(config->capacity, sizeof(struct hashtable_string_ref), &size) != 0)
    {
        return -1;
    }
    /* 空きリストは値ストレージの直前に置く。値ストレージはデータ領域の末尾のままとする。 */
    if (hashtable_free_list_region_size(config, &free_list_size) != 0)
    {
        return -1;
    }
    if (add_checked(size, free_list_size, &size) != 0)
    {
        return -1;
    }
    if (add_checked(size, config->value_storage_size, data_size_out) != 0)
    {
        return -1;
    }
    return 0;
}

/**
 *  @brief          永続化領域をバイト列として見ます。
 *  @param[in]      ht  対象。NULL を渡してはなりません。
 *  @return         永続化領域先頭です。
 *
 *  const の @p ht から領域ポインターを共用するため、const を外します。
 */
static unsigned char *hashtable_bytes(const cplat_hashtable *ht)
{
    return (unsigned char *)(uintptr_t)ht->hdr;
}

/**
 *  @brief          バケット先頭配列を返します。
 *  @param[in]      ht  対象。NULL を渡してはなりません。
 *  @return         @p capacity 個の 1 相対レコード番号です。0 は空です。
 *
 *  範囲検査はしません。構築済みの @p ht を渡してください。
 */
static uint64_t *hashtable_bucket_head(const cplat_hashtable *ht)
{
    size_t off_bucket_head;

    (void)hashtable_mgmt_layout(&ht->hdr->config, &off_bucket_head, NULL, NULL);
    return (uint64_t *)(hashtable_bytes(ht) + off_bucket_head);
}

/**
 *  @brief          エントリ配列の先頭を返します。
 *  @param[in]      ht  対象。NULL を渡してはなりません。
 *  @return         エントリ配列の先頭です。
 *
 *  範囲検査はしません。構築済みの @p ht を渡してください。
 */
static unsigned char *hashtable_entries(const cplat_hashtable *ht)
{
    size_t off_entries;

    (void)hashtable_mgmt_layout(&ht->hdr->config, NULL, &off_entries, NULL);
    return hashtable_bytes(ht) + off_entries;
}

/**
 *  @brief          指定スロットの next へのポインターを返します。
 *  @param[in]      ht   対象。NULL を渡してはなりません。
 *  @param[in]      rec  0 相対のスロット添字。capacity 未満であること。
 *  @return         1 相対の次レコード番号です。0 は終端です。
 */
static uint64_t *hashtable_entry_next(const cplat_hashtable *ht, size_t rec)
{
    size_t stride = 0;

    (void)entry_stride_checked(&ht->hdr->config, &stride);
    return (uint64_t *)(hashtable_entries(ht) + rec * stride);
}

/**
 *  @brief          指定スロットの status へのポインターを返します。
 *  @param[in]      ht   対象。NULL を渡してはなりません。
 *  @param[in]      rec  0 相対のスロット添字。capacity 未満であること。
 *  @return         実装状況へのポインターです。
 */
static unsigned char *hashtable_entry_status(const cplat_hashtable *ht, size_t rec)
{
    size_t stride = 0;

    (void)entry_stride_checked(&ht->hdr->config, &stride);
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
static cplat_timespec *hashtable_entry_timestamp(const cplat_hashtable *ht, size_t rec)
{
    size_t stride = 0;

    (void)entry_stride_checked(&ht->hdr->config, &stride);
    return (cplat_timespec *)(hashtable_entries(ht) + rec * stride + sizeof(uint64_t) + sizeof(unsigned char) +
                                 (size_t)ENTRY_TIMESTAMP_PAD);
}

/**
 *  @brief          指定スロットの世代カウンターへのポインターを返します。
 *  @param[in]      ht   対象。NULL を渡してはなりません。
 *  @param[in]      rec  0 相対のスロット添字。capacity 未満であること。
 *  @return         世代カウンターへのポインターです。
 *
 *  @p timestamp_scope が @c SCOPE_RECORD のときだけ呼べます。
 */
static uint64_t *hashtable_entry_generation(const cplat_hashtable *ht, size_t rec)
{
    size_t stride = 0;

    (void)entry_stride_checked(&ht->hdr->config, &stride);
    return (uint64_t *)(void *)(hashtable_entries(ht) + rec * stride + entry_generation_offset());
}

/**
 *  @brief          指定スロットのキーへのポインターを返します。
 *  @param[in]      ht   対象。NULL を渡してはなりません。
 *  @param[in]      rec  0 相対のスロット添字。capacity 未満であること。
 *  @return         key_size バイトのキーです。
 */
static char *hashtable_entry_key(const cplat_hashtable *ht, size_t rec)
{
    size_t stride = 0;

    (void)entry_stride_checked(&ht->hdr->config, &stride);
    unsigned char *slot = hashtable_entries(ht) + rec * stride + entry_key_offset(&ht->hdr->config);

    if (field_is_variable(ht->hdr->config.key_type) != 0)
    {
        struct hashtable_string_ref *ref = (struct hashtable_string_ref *)slot;
        size_t mgmt_size = 0;

        (void)hashtable_mgmt_layout(&ht->hdr->config, NULL, NULL, &mgmt_size);
        return (char *)(hashtable_bytes(ht) + mgmt_size - ht->hdr->config.key_storage_size + (size_t)ref->offset);
    }
    return (char *)slot;
}

static struct hashtable_string_ref *hashtable_key_ref_at(const cplat_hashtable *ht, size_t rec)
{
    size_t stride = 0;

    (void)entry_stride_checked(&ht->hdr->config, &stride);
    return (struct hashtable_string_ref *)(hashtable_entries(ht) + rec * stride + entry_key_offset(&ht->hdr->config));
}

static unsigned char *hashtable_key_storage(const cplat_hashtable *ht)
{
    size_t mgmt_size = 0;

    (void)hashtable_mgmt_layout(&ht->hdr->config, NULL, NULL, &mgmt_size);
    return hashtable_bytes(ht) + mgmt_size - ht->hdr->config.key_storage_size;
}

/**
 *  @brief          値配列の先頭を返します。
 *  @param[in]      ht  対象。NULL を渡してはなりません。
 *  @return         値配列の先頭です。
 *
 *  データ領域は管理領域と独立したブロックのため、@p ht->data をそのまま返します。
 */
static unsigned char *hashtable_data(const cplat_hashtable *ht)
{
    return ht->data;
}

/**
 *  @brief          可変長値ストレージの先頭を返します。
 *  @param[in]      ht  対象。NULL を渡してはなりません。
 *  @return         値ストレージ先頭です。
 *
 *  データ領域は、値 descriptor 配列、空きリスト、値ストレージの順に並びます。
 */
static unsigned char *hashtable_value_storage(const cplat_hashtable *ht)
{
    size_t free_list_size = 0;

    (void)hashtable_free_list_region_size(&ht->hdr->config, &free_list_size);
    return hashtable_data(ht) + ht->hdr->config.capacity * sizeof(struct hashtable_string_ref) + free_list_size;
}

/**
 *  @brief          指定スロットの値へのポインターを返します。
 *  @param[in]      ht   対象。NULL を渡してはなりません。
 *  @param[in]      rec  0 相対のスロット添字。capacity 未満であること。
 *  @return         value_size バイトの値です。
 */
static unsigned char *hashtable_data_at(const cplat_hashtable *ht, size_t rec)
{
    size_t stride = 0;

    if (field_is_variable(ht->hdr->config.value_type) != 0)
    {
        struct hashtable_string_ref *refs = (struct hashtable_string_ref *)hashtable_data(ht);

        return hashtable_value_storage(ht) + (size_t)refs[rec].offset;
    }
    (void)value_stride_checked(&ht->hdr->config, &stride);
    return hashtable_data(ht) + rec * stride;
}

static struct hashtable_string_ref *hashtable_value_ref_at(const cplat_hashtable *ht, size_t rec)
{
    return &((struct hashtable_string_ref *)hashtable_data(ht))[rec];
}

/**
 *  @brief          可変長キーの空きリスト先頭を返します。
 *  @param[in]      ht  対象。NULL を渡してはなりません。
 *  @return         空きリスト先頭です。
 *
 *  空きリストはキー ストレージの直前に置きます。
 */
static struct hashtable_free_block *hashtable_key_free_list(const cplat_hashtable *ht)
{
    size_t free_list_size = 0;

    (void)hashtable_free_list_region_size(&ht->hdr->config, &free_list_size);
    return (struct hashtable_free_block *)(void *)(hashtable_key_storage(ht) - free_list_size);
}

/**
 *  @brief          可変長値の空きリスト先頭を返します。
 *  @param[in]      ht  対象。NULL を渡してはなりません。
 *  @return         空きリスト先頭です。
 *
 *  空きリストは値ストレージの直前に置きます。
 */
static struct hashtable_free_block *hashtable_value_free_list(const cplat_hashtable *ht)
{
    size_t free_list_size = 0;

    (void)hashtable_free_list_region_size(&ht->hdr->config, &free_list_size);
    return (struct hashtable_free_block *)(void *)(hashtable_value_storage(ht) - free_list_size);
}

/**
 *  @brief          キーのバケット番号を求めます。
 *  @param[in]      ht   対象。NULL を渡してはなりません。
 *  @param[in]      key  キー。NULL を渡してはなりません。
 *  @return         0 以上 capacity 未満のバケット番号です。
 *
 *  djb2 を使い、最後に capacity で割った余りを返します。\n
 *  アキュムレータは幅を uint64_t に固定しています。`unsigned long` は
 *  Linux/GCC (LP64, 64bit) と Windows/MSVC (LLP64, 32bit) で幅が異なり、
 *  同じキーでも環境によってバケット番号がずれるためです。\n
 *  capacity は両対象環境で size_t (64bit) のため、幅を size_t に合わせています。
 */
static size_t hash_key(const cplat_hashtable *ht, const void *key)
{
    uint64_t hash = 5381;
    const unsigned char *p = (const unsigned char *)key;

    if (ht->hdr->config.key_type != CPLAT_HASHTABLE_FIELD_FIXED_BINARY)
    {
        int c;

        /* 文字列は先頭の NUL までだけを混ぜ、残り 0 埋めはハッシュに入れません。 */
        while ((c = *p++) != 0)
        {
            hash = ((hash << 5) + hash) + (uint64_t)c;
        }
    }
    else
    {
        size_t i;

        for (i = 0; i < ht->hdr->config.key_size; i++)
        {
            hash = ((hash << 5) + hash) + p[i];
        }
    }
    return (size_t)hash % ht->hdr->config.capacity;
}

/**
 *  @brief          格納済みキーと探索キーが同じかを判定します。
 *  @param[in]      ht          対象。NULL を渡してはなりません。
 *  @param[in]      stored_key  スロット上のキー。
 *  @param[in]      key         探索キー。
 *  @return         一致なら 1、不一致なら 0 です。
 */
static int key_equal(const cplat_hashtable *ht, const char *stored_key, const void *key)
{
    if (ht->hdr->config.key_type != CPLAT_HASHTABLE_FIELD_FIXED_BINARY)
    {
        return strcmp(stored_key, (const char *)key) == 0;
    }
    return memcmp(stored_key, key, ht->hdr->config.key_size) == 0;
}

/**
 *  @brief          キーが key_size に収まるかを判定します。
 *  @param[in]      ht   対象。NULL を渡してはなりません。
 *  @param[in]      key  キー。NULL を渡してはなりません。
 *  @return         収まれば 1、収まらなければ 0 です。
 */
static int key_fits(const cplat_hashtable *ht, const void *key)
{
    if (ht->hdr->config.key_type == CPLAT_HASHTABLE_FIELD_FIXED_BINARY)
    {
        return 1;
    }
    if (ht->hdr->config.key_type == CPLAT_HASHTABLE_FIELD_VARIABLE_STRING)
    {
        return strlen((const char *)key) < ht->hdr->config.key_storage_size;
    }
    return memchr(key, '\0', ht->hdr->config.key_size) != NULL;
}

/**
 *  @brief          入力フィールドの保存バイト数を返します。
 *  @param[in]      type        フィールド形式。
 *  @param[in]      fixed_size  固定長フィールドの保存バイト数。
 *  @param[in]      value       入力値。NULL を渡してはなりません。
 *  @return         可変長文字列では NUL を含む長さ、それ以外では @p fixed_size です。
 */
static size_t field_input_size(cplat_hashtable_field_type type, size_t fixed_size, const void *value)
{
    if (type == CPLAT_HASHTABLE_FIELD_VARIABLE_STRING)
    {
        return strlen((const char *)value) + 1u;
    }
    return fixed_size;
}

static int fixed_string_fits(cplat_hashtable_field_type type, size_t fixed_size, const void *value)
{
    return (type != CPLAT_HASHTABLE_FIELD_FIXED_STRING) || (memchr(value, '\0', fixed_size) != NULL);
}

/**
 *  @brief          descriptor 取得関数の型です。
 *
 *  キーと値で descriptor の並びが異なるため、共通処理へ取得手段を渡します。
 */
typedef struct hashtable_string_ref *(*hashtable_ref_fn)(const cplat_hashtable *ht, size_t rec);

/**
 *  @brief          可変長ストレージ 1 個分の操作対象をまとめた一時的な束ねです。
 *
 *  キーと値では、ストレージ先頭、空きリスト先頭、容量だけが異なります。\n
 *  実行時のみ有効な参照であり、永続化しません。
 */
struct hashtable_arena
{
    unsigned char *storage;       /**< ストレージ先頭です。 */
    struct hashtable_free_block *free_list; /**< 空きリスト先頭です。 */
    uint64_t *free_count;              /**< 空きブロックの個数への参照です。永続化ヘッダー内を指します。 */
    size_t storage_size;          /**< ストレージのバイト数です。 */
};

/**
 *  @brief          可変長キーの操作対象を組み立てます。
 *  @param[in]      ht     対象。NULL を渡してはなりません。
 *  @param[out]     arena  組み立て先。NULL を渡してはなりません。
 *
 *  可変長キーのときだけ呼べます。
 */
static void hashtable_key_arena(const cplat_hashtable *ht, struct hashtable_arena *arena)
{
    arena->storage = hashtable_key_storage(ht);
    arena->free_list = hashtable_key_free_list(ht);
    arena->free_count = &ht->hdr->key_free_count;
    arena->storage_size = ht->hdr->config.key_storage_size;
}

/**
 *  @brief          可変長値の操作対象を組み立てます。
 *  @param[in]      ht     対象。NULL を渡してはなりません。
 *  @param[out]     arena  組み立て先。NULL を渡してはなりません。
 *
 *  可変長値のときだけ呼べます。
 */
static void hashtable_value_arena(const cplat_hashtable *ht, struct hashtable_arena *arena)
{
    arena->storage = hashtable_value_storage(ht);
    arena->free_list = hashtable_value_free_list(ht);
    arena->free_count = &ht->hdr->value_free_count;
    arena->storage_size = ht->hdr->config.value_storage_size;
}

/**
 *  @brief          空きリストを未使用の初期状態へ戻します。
 *  @param[in,out]  arena  操作対象。NULL を渡してはなりません。
 *
 *  ストレージ全体が 1 個の空きブロックになります。
 */
static void arena_reset(struct hashtable_arena *arena)
{
    arena->free_list[0].offset = 0;
    arena->free_list[0].length = (uint64_t)arena->storage_size;
    *arena->free_count = 1;
}

/**
 *  @brief          指定の空きブロックが、置き換え対象ブロックと隣接するかを判定します。
 *  @param[in]      arena       操作対象。NULL を渡してはなりません。
 *  @param[in]      index       空きブロックの添字。
 *  @param[in]      own_offset  置き換え対象ブロックの先頭オフセット。
 *  @param[in]      own_length  置き換え対象ブロックのバイト数。0 なら対象なしです。
 *  @return         隣接するなら 1、しないなら 0 です。
 */
static int arena_free_block_adjoins(const struct hashtable_arena *arena, uint64_t index, uint64_t own_offset,
                              uint64_t own_length)
{
    if (own_length == 0)
    {
        return 0;
    }
    return (((arena->free_list[index].offset + arena->free_list[index].length) == own_offset) ||
            (arena->free_list[index].offset == (own_offset + own_length)))
               ? 1
               : 0;
}

/**
 *  @brief          先着適合で空き領域を探します。
 *  @param[in]      arena       操作対象。NULL を渡してはなりません。
 *  @param[in]      own_offset  置き換え対象ブロックの先頭オフセット。
 *  @param[in]      own_length  置き換え対象ブロックのバイト数。0 なら対象なしです。
 *  @param[in]      needed      必要バイト数。0 を渡してはなりません。
 *  @param[out]     offset_out  見つかった先頭オフセットの格納先。
 *  @return         見つかれば 1、見つからなければ 0 です。
 *
 *  空きリストを変更しません。@p own_length が非 0 のときは、その区間を
 *  前後の隣接する空きブロックと結合したうえで空きとみなします。\n
 *  結合した空きも、オフセットの昇順の位置で評価します。
 */
static int arena_find_fit(const struct hashtable_arena *arena, uint64_t own_offset, uint64_t own_length, size_t needed,
                          size_t *offset_out)
{
    uint64_t count = *arena->free_count;
    uint64_t merged_offset = own_offset;
    uint64_t merged_length = own_length;
    uint64_t i;
    int merged_pending = (own_length != 0) ? 1 : 0;

    if (own_length != 0)
    {
        for (i = 0; i < count; i++)
        {
            if (arena_free_block_adjoins(arena, i, own_offset, own_length) != 0)
            {
                merged_length += arena->free_list[i].length;
                if (arena->free_list[i].offset < merged_offset)
                {
                    merged_offset = arena->free_list[i].offset;
                }
            }
        }
    }
    for (i = 0; i < count; i++)
    {
        if ((merged_pending != 0) && (merged_offset < arena->free_list[i].offset))
        {
            if ((uint64_t)needed <= merged_length)
            {
                *offset_out = (size_t)merged_offset;
                return 1;
            }
            merged_pending = 0;
        }
        if (arena_free_block_adjoins(arena, i, own_offset, own_length) != 0)
        {
            continue; /* 結合済みのため、単独では評価しない。 */
        }
        if ((uint64_t)needed <= arena->free_list[i].length)
        {
            *offset_out = (size_t)arena->free_list[i].offset;
            return 1;
        }
    }
    if ((merged_pending != 0) && ((uint64_t)needed <= merged_length))
    {
        *offset_out = (size_t)merged_offset;
        return 1;
    }
    return 0;
}

/**
 *  @brief          指定区間を空きリストから取り除きます。
 *  @param[in,out]  arena   操作対象。NULL を渡してはなりません。
 *  @param[in]      offset  取り除く区間の先頭オフセット。
 *  @param[in]      length  取り除く区間のバイト数。0 を渡してはなりません。
 *
 *  区間全体が 1 個の空きブロックに収まっていることが前提です。\n
 *  途中を取り除く場合は空きブロックが 2 個へ分かれますが、同時に使用中ブロックが 1 個増えるため、
 *  要素数の上限 capacity + 1 は超えません。
 */
static void arena_take(struct hashtable_arena *arena, size_t offset, size_t length)
{
    uint64_t count = *arena->free_count;
    uint64_t start = (uint64_t)offset;
    uint64_t end = start + (uint64_t)length;
    uint64_t i;

    for (i = 0; i < count; i++)
    {
        uint64_t free_end = arena->free_list[i].offset + arena->free_list[i].length;

        if ((arena->free_list[i].offset > start) || (end > free_end))
        {
            continue;
        }
        if ((arena->free_list[i].offset == start) && (end == free_end))
        {
            memmove(&arena->free_list[i], &arena->free_list[i + 1u],
                    (size_t)(count - i - 1u) * sizeof(struct hashtable_free_block));
            *arena->free_count = count - 1u;
        }
        else if (arena->free_list[i].offset == start)
        {
            arena->free_list[i].offset = end;
            arena->free_list[i].length = free_end - end;
        }
        else if (end == free_end)
        {
            arena->free_list[i].length = start - arena->free_list[i].offset;
        }
        else
        {
            memmove(&arena->free_list[i + 2u], &arena->free_list[i + 1u],
                    (size_t)(count - i - 1u) * sizeof(struct hashtable_free_block));
            arena->free_list[i].length = start - arena->free_list[i].offset;
            arena->free_list[i + 1u].offset = end;
            arena->free_list[i + 1u].length = free_end - end;
            *arena->free_count = count + 1u;
        }
        return;
    }
}

/**
 *  @brief          指定区間を空きリストへ返します。
 *  @param[in,out]  arena   操作対象。NULL を渡してはなりません。
 *  @param[in]      offset  返す区間の先頭オフセット。
 *  @param[in]      length  返す区間のバイト数。0 なら何もしません。
 *
 *  前後に隣接する空きブロックがあれば結合し、空きブロックが常に極大であるように保ちます。
 */
static void arena_give(struct hashtable_arena *arena, size_t offset, size_t length)
{
    uint64_t count = *arena->free_count;
    uint64_t start = (uint64_t)offset;
    uint64_t end = start + (uint64_t)length;
    uint64_t i = 0;
    int merge_prev;
    int merge_next;

    if (length == 0)
    {
        return;
    }
    while ((i < count) && (arena->free_list[i].offset < start))
    {
        i++;
    }
    merge_prev = ((i > 0u) && ((arena->free_list[i - 1u].offset + arena->free_list[i - 1u].length) == start)) ? 1 : 0;
    merge_next = ((i < count) && (arena->free_list[i].offset == end)) ? 1 : 0;
    if ((merge_prev != 0) && (merge_next != 0))
    {
        arena->free_list[i - 1u].length += (uint64_t)length + arena->free_list[i].length;
        memmove(&arena->free_list[i], &arena->free_list[i + 1u], (size_t)(count - i - 1u) * sizeof(struct hashtable_free_block));
        *arena->free_count = count - 1u;
    }
    else if (merge_prev != 0)
    {
        arena->free_list[i - 1u].length += (uint64_t)length;
    }
    else if (merge_next != 0)
    {
        arena->free_list[i].offset = start;
        arena->free_list[i].length += (uint64_t)length;
    }
    else
    {
        memmove(&arena->free_list[i + 1u], &arena->free_list[i], (size_t)(count - i) * sizeof(struct hashtable_free_block));
        arena->free_list[i].offset = start;
        arena->free_list[i].length = (uint64_t)length;
        *arena->free_count = count + 1u;
    }
}

/**
 *  @brief          使用中ブロックを先頭へ詰め直します。
 *  @param[in,out]  ht       対象。NULL を渡してはなりません。
 *  @param[in,out]  arena    操作対象。NULL を渡してはなりません。
 *  @param[in]      get_ref  descriptor の取得手段。NULL を渡してはなりません。
 *  @param[in]      used     使用中バイト数の合計。
 *
 *  空きブロックを飛ばしながら左詰めするため、移動はオフセットの昇順に起き、上書きは生じません。\n
 *  descriptor の新しいオフセットは、自分より前にある空きブロックのバイト数の累積で決まります。
 *  累積和を空きリスト上へ一時的に作り、二分探索で引きます。
 */
static void arena_compact(cplat_hashtable *ht, struct hashtable_arena *arena, hashtable_ref_fn get_ref,
                          uint64_t used)
{
    uint64_t count = *arena->free_count;
    uint64_t shift = 0;
    uint64_t prev_end = 0;
    uint64_t running = 0;
    uint64_t i;
    size_t rec;

    for (i = 0; i < count; i++)
    {
        uint64_t run_length = arena->free_list[i].offset - prev_end;

        if ((run_length != 0) && (shift != 0))
        {
            memmove(arena->storage + (size_t)(prev_end - shift), arena->storage + (size_t)prev_end,
                    (size_t)run_length);
        }
        shift += arena->free_list[i].length;
        prev_end = arena->free_list[i].offset + arena->free_list[i].length;
    }
    if ((prev_end < (uint64_t)arena->storage_size) && (shift != 0))
    {
        memmove(arena->storage + (size_t)(prev_end - shift), arena->storage + (size_t)prev_end,
                (size_t)((uint64_t)arena->storage_size - prev_end));
    }

    /* 空きブロックの長さを累積和へ置き換える。以降、この配列は移動量の索引としてだけ使う。 */
    for (i = 0; i < count; i++)
    {
        running += arena->free_list[i].length;
        arena->free_list[i].length = running;
    }
    for (rec = 0; rec < ht->hdr->config.capacity; rec++)
    {
        struct hashtable_string_ref *ref = get_ref(ht, rec);
        uint64_t low = 0;
        uint64_t high = count;

        if (ref->length == 0)
        {
            continue;
        }
        while (low < high)
        {
            uint64_t mid = low + ((high - low) / 2u);

            if (arena->free_list[mid].offset < ref->offset)
            {
                low = mid + 1u;
            }
            else
            {
                high = mid;
            }
        }
        if (low != 0u)
        {
            ref->offset -= arena->free_list[low - 1u].length;
        }
    }

    memset(arena->storage + (size_t)used, 0, arena->storage_size - (size_t)used);
    if ((uint64_t)arena->storage_size > used)
    {
        arena->free_list[0].offset = used;
        arena->free_list[0].length = (uint64_t)arena->storage_size - used;
        *arena->free_count = 1;
    }
    else
    {
        *arena->free_count = 0;
    }
}

static void release_key(cplat_hashtable *ht, size_t rec)
{
    if (field_is_variable(ht->hdr->config.key_type) != 0)
    {
        struct hashtable_string_ref *ref = hashtable_key_ref_at(ht, rec);
        struct hashtable_arena arena;

        hashtable_key_arena(ht, &arena);
        memset(hashtable_key_storage(ht) + (size_t)ref->offset, 0, (size_t)ref->length);
        arena_give(&arena, (size_t)ref->offset, (size_t)ref->length);
        ht->hdr->key_storage_used -= ref->length;
        ref->offset = 0;
        ref->length = 0;
    }
    else
    {
        memset(hashtable_entry_key(ht, rec), 0, ht->hdr->config.key_size);
    }
}

static void release_value(cplat_hashtable *ht, size_t rec)
{
    if (field_is_variable(ht->hdr->config.value_type) != 0)
    {
        struct hashtable_string_ref *ref = hashtable_value_ref_at(ht, rec);
        struct hashtable_arena arena;

        hashtable_value_arena(ht, &arena);
        memset(hashtable_value_storage(ht) + (size_t)ref->offset, 0, (size_t)ref->length);
        arena_give(&arena, (size_t)ref->offset, (size_t)ref->length);
        ht->hdr->value_storage_used -= ref->length;
        ref->offset = 0;
        ref->length = 0;
    }
    else
    {
        memset(hashtable_data_at(ht, rec), 0, ht->hdr->config.value_size);
    }
}

static void key_store(cplat_hashtable *ht, size_t rec, const void *key, size_t storage_offset)
{
    if (field_is_variable(ht->hdr->config.key_type) != 0)
    {
        size_t length = strlen((const char *)key) + 1u;
        struct hashtable_string_ref *ref;
        struct hashtable_arena arena;

        hashtable_key_arena(ht, &arena);
        arena_take(&arena, storage_offset, length);
        ref = hashtable_key_ref_at(ht, rec);
        ref->offset = (uint64_t)storage_offset;
        ref->length = (uint64_t)length;
        memcpy(hashtable_key_storage(ht) + (size_t)ref->offset, key, length);
        ht->hdr->key_storage_used += (uint64_t)length;
    }
    else if (ht->hdr->config.key_type == CPLAT_HASHTABLE_FIELD_FIXED_STRING)
    {
        size_t copy_len = strlen((const char *)key) + 1u;
        char *dst = hashtable_entry_key(ht, rec);

        memset(dst, 0, ht->hdr->config.key_size);
        memcpy(dst, key, copy_len);
    }
    else
    {
        memcpy(hashtable_entry_key(ht, rec), key, ht->hdr->config.key_size);
    }
}

static void value_store(cplat_hashtable *ht, size_t rec, const void *value, size_t storage_offset)
{
    if (field_is_variable(ht->hdr->config.value_type) != 0)
    {
        size_t length = strlen((const char *)value) + 1u;
        struct hashtable_string_ref *ref;
        struct hashtable_arena arena;

        hashtable_value_arena(ht, &arena);
        arena_take(&arena, storage_offset, length);
        ref = hashtable_value_ref_at(ht, rec);
        ref->offset = (uint64_t)storage_offset;
        ref->length = (uint64_t)length;
        memcpy(hashtable_value_storage(ht) + (size_t)ref->offset, value, length);
        ht->hdr->value_storage_used += (uint64_t)length;
    }
    else if (ht->hdr->config.value_type == CPLAT_HASHTABLE_FIELD_FIXED_STRING)
    {
        size_t copy_len = strlen((const char *)value) + 1u;

        memset(hashtable_data_at(ht, rec), 0, ht->hdr->config.value_size);
        memcpy(hashtable_data_at(ht, rec), value, copy_len);
    }
    else
    {
        memcpy(hashtable_data_at(ht, rec), value, ht->hdr->config.value_size);
    }
}

/**
 *  @brief          空きリストの構造を検査します。
 *  @param[in]      arena     操作対象。NULL を渡してはなりません。
 *  @param[in]      used      使用中バイト数の合計。
 *  @param[in]      capacity  スロット数。
 *  @return         妥当なら 0、壊れているなら -1 です。
 *
 *  個数の上限、長さの非 0、範囲、オフセットの昇順、隣接する空きブロックが結合済みであること、
 *  および空きブロックの合計が未使用バイト数と一致することを確かめます。
 */
static int arena_validate(const struct hashtable_arena *arena, uint64_t used, size_t capacity)
{
    uint64_t count = *arena->free_count;
    uint64_t total = 0;
    uint64_t prev_end = 0;
    uint64_t i;

    if (count > (uint64_t)capacity + 1u)
    {
        return -1;
    }
    for (i = 0; i < count; i++)
    {
        if (arena->free_list[i].length == 0)
        {
            return -1;
        }
        if (arena->free_list[i].offset > (uint64_t)arena->storage_size)
        {
            return -1;
        }
        if (arena->free_list[i].length > ((uint64_t)arena->storage_size - arena->free_list[i].offset))
        {
            return -1;
        }
        /* 2 個目以降は、直前の空きブロックの終端より後ろから始まること。等しい場合は未結合で不正。 */
        if ((i != 0u) && (arena->free_list[i].offset <= prev_end))
        {
            return -1;
        }
        prev_end = arena->free_list[i].offset + arena->free_list[i].length;
        total += arena->free_list[i].length;
    }
    if (total != ((uint64_t)arena->storage_size - used))
    {
        return -1;
    }
    return 0;
}

/**
 *  @brief          使用中ブロックが空きブロックと重ならないことを確かめます。
 *  @param[in]      arena   操作対象。NULL を渡してはなりません。
 *  @param[in]      offset  使用中ブロックの先頭オフセット。
 *  @param[in]      length  使用中ブロックのバイト数。
 *  @return         重ならないなら 0、重なるなら -1 です。
 *
 *  空きブロックは昇順かつ互いに素のため、直前と直後の空きブロックだけを見れば足ります。\n
 *  @ref arena_validate が昇順を確かめた後に呼んでください。
 */
static int arena_validate_used_block(const struct hashtable_arena *arena, uint64_t offset, uint64_t length)
{
    uint64_t count = *arena->free_count;
    uint64_t low = 0;
    uint64_t high = count;

    while (low < high)
    {
        uint64_t mid = low + ((high - low) / 2u);

        if (arena->free_list[mid].offset <= offset)
        {
            low = mid + 1u;
        }
        else
        {
            high = mid;
        }
    }
    if ((low != 0u) && ((arena->free_list[low - 1u].offset + arena->free_list[low - 1u].length) > offset))
    {
        return -1;
    }
    if ((low < count) && ((offset + length) > arena->free_list[low].offset))
    {
        return -1;
    }
    return 0;
}

/**
 *  @brief          可変長ストレージの空きリストを未使用の初期状態へ戻します。
 *  @param[in,out]  ht  対象。NULL を渡してはなりません。
 *
 *  可変長フィールドを持つ側だけを対象にします。
 */
static void hashtable_reset_arenas(cplat_hashtable *ht)
{
    struct hashtable_arena arena;

    if (field_is_variable(ht->hdr->config.key_type) != 0)
    {
        hashtable_key_arena(ht, &arena);
        arena_reset(&arena);
    }
    if (field_is_variable(ht->hdr->config.value_type) != 0)
    {
        hashtable_value_arena(ht, &arena);
        arena_reset(&arena);
    }
}

/**
 *  @brief          可変長キーの格納先を探します。
 *  @param[in]      ht          対象。NULL を渡してはなりません。
 *  @param[in]      rec         0 相対のスロット添字。capacity 未満であること。
 *  @param[in]      replace     非 0 なら、@p rec が現在使っているブロックも空きとみなします。
 *  @param[in]      key         格納するキー。NULL を渡してはなりません。
 *  @param[out]     offset_out  格納先オフセットの格納先。成功時だけ書きます。
 *  @return         見つかれば 1、見つからなければ 0 です。
 *
 *  固定長キーでは 0 を返して常に成功します。\n
 *  空きリストを変更しません。実際の確保は @ref key_store が行います。\n
 *  @p replace が非 0 のときは、呼び出し側が @ref key_store の前に
 *  @ref release_key を呼ぶ前提です。
 */
static int key_storage_find_free(const cplat_hashtable *ht, size_t rec, int replace, const void *key,
                                 size_t *offset_out)
{
    struct hashtable_arena arena;
    uint64_t own_offset = 0;
    uint64_t own_length = 0;
    size_t needed;

    if (field_is_variable(ht->hdr->config.key_type) == 0)
    {
        *offset_out = 0;
        return 1;
    }
    if (replace != 0)
    {
        const struct hashtable_string_ref *own = hashtable_key_ref_at(ht, rec);

        own_offset = own->offset;
        own_length = own->length;
    }
    hashtable_key_arena(ht, &arena);
    needed = field_input_size(ht->hdr->config.key_type, 0, key);
    return arena_find_fit(&arena, own_offset, own_length, needed, offset_out);
}

/**
 *  @brief          可変長値の格納先を探します。
 *  @param[in]      ht          対象。NULL を渡してはなりません。
 *  @param[in]      rec         0 相対のスロット添字。capacity 未満であること。
 *  @param[in]      replace     非 0 なら、@p rec が現在使っているブロックも空きとみなします。
 *  @param[in]      value       格納する値。NULL を渡してはなりません。
 *  @param[out]     offset_out  格納先オフセットの格納先。成功時だけ書きます。
 *  @return         見つかれば 1、見つからなければ 0 です。
 *
 *  固定長値では 0 を返して常に成功します。\n
 *  空きリストを変更しません。実際の確保は @ref value_store が行います。\n
 *  @p replace が非 0 のときは、呼び出し側が @ref value_store の前に
 *  @ref release_value を呼ぶ前提です。
 */
static int value_storage_find_free(const cplat_hashtable *ht, size_t rec, int replace, const void *value,
                                   size_t *offset_out)
{
    struct hashtable_arena arena;
    uint64_t own_offset = 0;
    uint64_t own_length = 0;
    size_t needed;

    if (field_is_variable(ht->hdr->config.value_type) == 0)
    {
        *offset_out = 0;
        return 1;
    }
    if (replace != 0)
    {
        const struct hashtable_string_ref *own = hashtable_value_ref_at(ht, rec);

        own_offset = own->offset;
        own_length = own->length;
    }
    hashtable_value_arena(ht, &arena);
    needed = field_input_size(ht->hdr->config.value_type, 0, value);
    return arena_find_fit(&arena, own_offset, own_length, needed, offset_out);
}

static int input_points_into_table(const cplat_hashtable *ht, const void *input)
{
    uintptr_t p = (uintptr_t)input;
    uintptr_t mgmt = (uintptr_t)hashtable_bytes(ht);
    uintptr_t data = (uintptr_t)hashtable_data(ht);
    size_t mgmt_size = 0;
    size_t data_size = 0;

    (void)hashtable_mgmt_layout(&ht->hdr->config, NULL, NULL, &mgmt_size);
    (void)hashtable_data_region_size(&ht->hdr->config, &data_size);
    return ((p >= mgmt) && (p - mgmt < mgmt_size)) || ((p >= data) && (p - data < data_size));
}

/**
 *  @brief          start_idx 以降の最小空きスロットを 1 相対で返します。
 *  @param[in]      ht         対象。NULL を渡してはなりません。
 *  @param[in]      start_idx  走査開始の 0 相対添字。
 *  @return         1 相対のレコード番号です。無ければ 0 です。
 */
static uint64_t scan_next_empty(cplat_hashtable *ht, size_t start_idx)
{
    size_t i;

    for (i = start_idx; i < ht->hdr->config.capacity; i++)
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
static void stamp_record(cplat_hashtable *ht, size_t rec)
{
    cplat_timespec now;

    cplat_get_realtime(&now);
    /* 実時刻は時計の巻き戻しで逆行しうるため、順序判定は世代カウンターで行う。 */
    ht->hdr->table_generation++;
    if (hashtable_has_record_timestamp(ht) != 0)
    {
        *hashtable_entry_timestamp(ht, rec) = now;
        *hashtable_entry_generation(ht, rec) = ht->hdr->table_generation;
    }
    ht->hdr->table_timestamp = now;
}

/**
 *  @brief          テーブル横断の変更時刻へ現在の実時刻を刻みます。
 *  @param[in]      ht  対象。NULL を渡してはなりません。
 */
static void stamp_table(cplat_hashtable *ht)
{
    cplat_get_realtime(&ht->hdr->table_timestamp);
    ht->hdr->table_generation++;
}

/**
 *  @brief          設定の意味的な妥当性を検査します。
 *  @param[in]      config  検査対象。NULL を渡してはなりません。
 *  @return         妥当なら 0、不正なら -1 です。
 *
 *  @ref cplat_hashtable_required_size と @ref cplat_hashtable_create が
 *  同じ基準で検査するための共通実装です。レイアウト計算に使わないフィールド
 *  (@p key_type 、 @p lifetime) も含めて検査します。
 */
static int hashtable_validate_config(const cplat_hashtable_config *config)
{
    if (config->capacity == 0)
    {
        return -1;
    }
    if ((config->key_type < CPLAT_HASHTABLE_FIELD_FIXED_BINARY) ||
        (config->key_type > CPLAT_HASHTABLE_FIELD_VARIABLE_STRING) ||
        (config->value_type < CPLAT_HASHTABLE_FIELD_FIXED_BINARY) ||
        (config->value_type > CPLAT_HASHTABLE_FIELD_VARIABLE_STRING))
    {
        return -1;
    }
    if ((config->timestamp_scope != CPLAT_HASHTABLE_TIMESTAMP_SCOPE_TABLE) &&
        (config->timestamp_scope != CPLAT_HASHTABLE_TIMESTAMP_SCOPE_RECORD))
    {
        return -1;
    }
    if (((field_is_variable(config->key_type) != 0) && ((config->key_size != 0) || (config->key_storage_size == 0))) ||
        ((field_is_variable(config->key_type) == 0) && ((config->key_size == 0) || (config->key_storage_size != 0))) ||
        ((field_is_variable(config->value_type) != 0) &&
         ((config->value_size != 0) || (config->value_storage_size == 0))) ||
        ((field_is_variable(config->value_type) == 0) &&
         ((config->value_size == 0) || (config->value_storage_size != 0))))
    {
        return -1;
    }
    if (field_is_variable(config->value_type) != 0)
    {
        /* 可変長値はストレージ内オフセットで位置が決まるため、境界を保証できない。 */
        if (config->value_align != 0)
        {
            return -1;
        }
    }
    else if (config->value_align != 0)
    {
        if ((config->value_align > (size_t)CPLAT_HASHTABLE_VALUE_ALIGN_MAX) ||
            ((config->value_align & (config->value_align - 1u)) != 0))
        {
            return -1;
        }
    }
    /* REC_DELETED が 2 なので、寿命は削除直後に空へ戻す 2 以上が必要。 */
    if (config->lifetime < 2)
    {
        return -1;
    }
    return 0;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_required_size(const cplat_hashtable_config *config, size_t *mgmt_size_out,
                                     size_t *data_size_out)
{
    size_t mgmt_size;
    size_t data_size;

    if ((config == NULL) || ((mgmt_size_out == NULL) && (data_size_out == NULL)))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if (hashtable_validate_config(config) != 0)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if (hashtable_mgmt_layout(config, NULL, NULL, &mgmt_size) != 0)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if (hashtable_data_region_size(config, &data_size) != 0)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if (mgmt_size_out != NULL)
    {
        *mgmt_size_out = mgmt_size;
    }
    if (data_size_out != NULL)
    {
        *data_size_out = data_size;
    }
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_create(const cplat_hashtable_config *config, void *buf_mgmt, size_t buf_mgmt_size,
                              void *buf_data, size_t buf_data_size, cplat_hashtable **ht_out)
{
    size_t mgmt_size;
    size_t data_size;
    struct hashtable_persist_header *hdr;
    unsigned char owns_buffer;
    unsigned char *data;
    cplat_hashtable *ht;

    if (ht_out != NULL)
    {
        *ht_out = NULL;
    }
    if ((ht_out == NULL) || (config == NULL))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if (hashtable_validate_config(config) != 0)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    /* 管理領域とデータ領域は、両方 NULL(内部確保)か両方非 NULL(外部指定)のいずれかに限る。 */
    if ((buf_mgmt == NULL) != (buf_data == NULL))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }

    if (hashtable_mgmt_layout(config, NULL, NULL, &mgmt_size) != 0)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if (hashtable_data_region_size(config, &data_size) != 0)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }

    if (buf_mgmt == NULL)
    {
        size_t data_offset;
        size_t total_size;

        /* 管理領域の直後を、データ領域が必要とする境界へ切り上げる。 */
        if (align_up_checked(mgmt_size, data_region_align(config), &data_offset) != 0)
        {
            return CPLAT_ERR_INVALID_ARGUMENT;
        }
        if (add_checked(data_offset, data_size, &total_size) != 0)
        {
            return CPLAT_ERR_INVALID_ARGUMENT;
        }
        hdr = (struct hashtable_persist_header *)cplat_calloc(1, total_size);
        if (hdr == NULL)
        {
            return CPLAT_ERR_OUT_OF_MEMORY;
        }
        data = (unsigned char *)hdr + data_offset;
        owns_buffer = 1;
    }
    else
    {
        if (buf_mgmt_size < mgmt_size)
        {
            return CPLAT_ERR_BUFFER_TOO_SMALL;
        }
        /* 管理領域の先頭アドレスは、呼び出し側が満たすべき引数条件です。 */
        if (((uintptr_t)buf_mgmt % _Alignof(struct hashtable_persist_header)) != 0)
        {
            return CPLAT_ERR_INVALID_ARGUMENT;
        }
        /* 可変長値の descriptor、および value_align が非 0 の値は境界を要求する。 */
        if (((uintptr_t)buf_data % data_region_align(config)) != 0)
        {
            return CPLAT_ERR_INVALID_ARGUMENT;
        }
        if (buf_data_size < data_size)
        {
            return CPLAT_ERR_BUFFER_TOO_SMALL;
        }
        memset(buf_mgmt, 0, mgmt_size);
        memset(buf_data, 0, data_size);
        hdr = (struct hashtable_persist_header *)buf_mgmt;
        data = (unsigned char *)buf_data;
        owns_buffer = 0;
    }

    /* 内部管理データは、永続化領域・データ領域とは別に確保する。 */
    ht = (cplat_hashtable *)cplat_calloc(1, sizeof(struct cplat_hashtable));
    if (ht == NULL)
    {
        if (owns_buffer != 0)
        {
            cplat_free(hdr);
        }
        return CPLAT_ERR_OUT_OF_MEMORY;
    }

    hdr->magic = CPLAT_HASHTABLE_MAGIC;
    hdr->version = CPLAT_HASHTABLE_VERSION;
    hdr->config.capacity = config->capacity;
    hdr->config.key_type = config->key_type;
    hdr->config.value_type = config->value_type;
    hdr->config.timestamp_scope = config->timestamp_scope;
    hdr->config.key_size = config->key_size;
    hdr->config.value_size = config->value_size;
    hdr->config.key_storage_size = config->key_storage_size;
    hdr->config.value_storage_size = config->value_storage_size;
    hdr->config.value_align = config->value_align;
    hdr->config.lifetime = config->lifetime;
    hdr->config.reuse_deleted = config->reuse_deleted;
    hdr->next_empty = 1; /* 全スロット空きなので、最小空きはレコード 1。 */
    hdr->in_use_count = 0;
    hdr->deleted_count = 0;
    hdr->table_generation = 0;

    ht->hdr = hdr;
    ht->data = data;
    ht->owns_buffer = owns_buffer;
    /* 可変長ストレージ全体を 1 個の空きブロックとして登録する。 */
    hashtable_reset_arenas(ht);

    *ht_out = ht;
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_get_config_ref(const cplat_hashtable *ht, const cplat_hashtable_config **config_out)
{
    if ((ht == NULL) || (config_out == NULL))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    *config_out = &ht->hdr->config;
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_get_config_val(const cplat_hashtable *ht, cplat_hashtable_config *config_out)
{
    const cplat_hashtable_config *src;
    int ret;

    if (config_out == NULL)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    ret = cplat_hashtable_get_config_ref(ht, &src);
    if (ret != CPLAT_OK)
    {
        return ret;
    }
    *config_out = *src;
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_buffer_size(const cplat_hashtable *ht, size_t *mgmt_size_out, size_t *data_size_out)
{
    if ((ht == NULL) || ((mgmt_size_out == NULL) && (data_size_out == NULL)))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if (mgmt_size_out != NULL)
    {
        (void)hashtable_mgmt_layout(&ht->hdr->config, NULL, NULL, mgmt_size_out);
    }
    if (data_size_out != NULL)
    {
        (void)hashtable_data_region_size(&ht->hdr->config, data_size_out);
    }
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_buffer_ref(const cplat_hashtable *ht, const void **mgmt_out, const void **data_out)
{
    if ((ht == NULL) || ((mgmt_out == NULL) && (data_out == NULL)))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if (mgmt_out != NULL)
    {
        *mgmt_out = hashtable_bytes(ht);
    }
    if (data_out != NULL)
    {
        *data_out = hashtable_data(ht);
    }
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_attach(void *buf_mgmt, size_t buf_mgmt_size, void *buf_data, size_t buf_data_size,
                              cplat_hashtable **ht_out)
{
    struct hashtable_persist_header *hdr;
    cplat_hashtable *ht;
    size_t mgmt_size;
    size_t data_size;

    if (ht_out != NULL)
    {
        *ht_out = NULL;
    }
    if ((ht_out == NULL) || (buf_mgmt == NULL) || (buf_data == NULL))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    /* マジックを読むために、まず永続化ヘッダー分だけを要求する。 */
    if (buf_mgmt_size < sizeof(struct hashtable_persist_header))
    {
        return CPLAT_ERR_BUFFER_TOO_SMALL;
    }
    if (((uintptr_t)buf_mgmt % _Alignof(struct hashtable_persist_header)) != 0)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }

    hdr = (struct hashtable_persist_header *)buf_mgmt;

    if (hdr->magic != CPLAT_HASHTABLE_MAGIC)
    {
        return CPLAT_ERR_CORRUPT_DESCRIPTOR;
    }
    if (hdr->version != CPLAT_HASHTABLE_VERSION)
    {
        return CPLAT_ERR_CORRUPT_DESCRIPTOR;
    }
    if (hashtable_validate_config(&hdr->config) != 0)
    {
        return CPLAT_ERR_CORRUPT_DESCRIPTOR;
    }
    if (((uintptr_t)buf_data % data_region_align(&hdr->config)) != 0)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    /* 0 は満杯の正当値。capacity 超えだけを拒否する。 */
    if (hdr->next_empty > hdr->config.capacity)
    {
        return CPLAT_ERR_CORRUPT_DESCRIPTOR;
    }
    /* 減算方向で判定し、あふれを避ける。 */
    if (hdr->in_use_count > hdr->config.capacity)
    {
        return CPLAT_ERR_CORRUPT_DESCRIPTOR;
    }
    if (hdr->deleted_count > hdr->config.capacity - hdr->in_use_count)
    {
        return CPLAT_ERR_CORRUPT_DESCRIPTOR;
    }
    if ((hdr->key_storage_used > hdr->config.key_storage_size) ||
        (hdr->value_storage_used > hdr->config.value_storage_size))
    {
        return CPLAT_ERR_CORRUPT_DESCRIPTOR;
    }
    /* 空きリストの走査が領域外へ出ないよう、個数の上限だけは先に検査する。 */
    if ((hdr->key_free_count > (uint64_t)hdr->config.capacity + 1u) ||
        (hdr->value_free_count > (uint64_t)hdr->config.capacity + 1u))
    {
        return CPLAT_ERR_CORRUPT_DESCRIPTOR;
    }

    if (hashtable_mgmt_layout(&hdr->config, NULL, NULL, &mgmt_size) != 0)
    {
        return CPLAT_ERR_CORRUPT_DESCRIPTOR;
    }
    if (buf_mgmt_size < mgmt_size)
    {
        return CPLAT_ERR_BUFFER_TOO_SMALL;
    }
    if (hashtable_data_region_size(&hdr->config, &data_size) != 0)
    {
        return CPLAT_ERR_CORRUPT_DESCRIPTOR;
    }
    if (buf_data_size < data_size)
    {
        return CPLAT_ERR_BUFFER_TOO_SMALL;
    }

    /* 内部管理データは、永続化領域・データ領域とは別に確保する。 */
    ht = (cplat_hashtable *)cplat_calloc(1, sizeof(struct cplat_hashtable));
    if (ht == NULL)
    {
        return CPLAT_ERR_OUT_OF_MEMORY;
    }

    ht->hdr = hdr;
    /* data は実行時のみ有効なアドレスのため、呼び出し側が渡した値を必ず使う。 */
    ht->data = (unsigned char *)buf_data;
    /* 再接続後の所有は常に呼び出し側。永続化領域とデータ領域は dispose で解放しない。 */
    ht->owns_buffer = 0;
    *ht_out = ht;
    return CPLAT_OK;
}

/**
 *  @brief          チェーンと空きヒントの整合を検査します。
 *  @param[in]      ht       対象。NULL を渡してはなりません。
 *  @param[in,out]  visited  capacity バイトの作業領域。呼び出し側が 0 埋めします。
 *  @return         整合なら 0、破損なら -1 です。
 */
static int hashtable_validate_impl(const cplat_hashtable *ht, unsigned char *visited)
{
    size_t capacity = ht->hdr->config.capacity;
    uint64_t *bucket_head = hashtable_bucket_head(ht);
    size_t idx;
    size_t min_empty = 0;
    uint64_t counted_in_use = 0;
    uint64_t counted_deleted = 0;
    uint64_t counted_key_storage = 0;
    uint64_t counted_value_storage = 0;
    struct hashtable_arena key_arena;
    struct hashtable_arena value_arena;
    size_t i;

    /* 空きリストは、descriptor の検査より先に構造を確かめる。以降は昇順を前提にできる。 */
    if (field_is_variable(ht->hdr->config.key_type) != 0)
    {
        hashtable_key_arena(ht, &key_arena);
        if (arena_validate(&key_arena, ht->hdr->key_storage_used, capacity) != 0)
        {
            return -1;
        }
    }
    if (field_is_variable(ht->hdr->config.value_type) != 0)
    {
        hashtable_value_arena(ht, &value_arena);
        if (arena_validate(&value_arena, ht->hdr->value_storage_used, capacity) != 0)
        {
            return -1;
        }
    }

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
            if (((field_is_variable(ht->hdr->config.key_type) != 0) && (hashtable_key_ref_at(ht, i)->length != 0)) ||
                ((field_is_variable(ht->hdr->config.value_type) != 0) && (hashtable_value_ref_at(ht, i)->length != 0)))
            {
                return -1;
            }
            /* 空スロットの世代カウンターは 0 に戻っているはず。 */
            if ((hashtable_has_record_timestamp(ht) != 0) && (*hashtable_entry_generation(ht, i) != 0))
            {
                return -1;
            }
            if (min_empty == 0)
            {
                min_empty = i + 1;
            }
        }
        else
        {
            if (field_is_variable(ht->hdr->config.key_type) != 0)
            {
                struct hashtable_string_ref *ref = hashtable_key_ref_at(ht, i);

                if ((ref->length == 0) || (ref->offset > ht->hdr->config.key_storage_size) ||
                    (ref->length > ht->hdr->config.key_storage_size - ref->offset) ||
                    (hashtable_key_storage(ht)[ref->offset + ref->length - 1u] != '\0'))
                {
                    return -1;
                }
                if (arena_validate_used_block(&key_arena, ref->offset, ref->length) != 0)
                {
                    return -1;
                }
                counted_key_storage += ref->length;
            }
            if (field_is_variable(ht->hdr->config.value_type) != 0)
            {
                struct hashtable_string_ref *ref = hashtable_value_ref_at(ht, i);

                if ((ref->length == 0) || (ref->offset > ht->hdr->config.value_storage_size) ||
                    (ref->length > ht->hdr->config.value_storage_size - ref->offset) ||
                    (hashtable_value_storage(ht)[ref->offset + ref->length - 1u] != '\0'))
                {
                    return -1;
                }
                if (arena_validate_used_block(&value_arena, ref->offset, ref->length) != 0)
                {
                    return -1;
                }
                counted_value_storage += ref->length;
            }
            else if ((ht->hdr->config.value_type == CPLAT_HASHTABLE_FIELD_FIXED_STRING) &&
                     (fixed_string_fits(ht->hdr->config.value_type, ht->hdr->config.value_size,
                                        hashtable_data_at(ht, i)) == 0))
            {
                return -1;
            }
            if (visited[i] == 0)
            {
                /* 実装中または削除済みなのに、どのバケットからも辿れない。 */
                return -1;
            }
            /* レコードの世代は、テーブルの世代を追い越しません。 */
            if ((hashtable_has_record_timestamp(ht) != 0) &&
                (*hashtable_entry_generation(ht, i) > ht->hdr->table_generation))
            {
                return -1;
            }
            if (status == REC_IN_USE)
            {
                counted_in_use++;
            }
            else
            {
                /* 2 以上は加齢中の削除済み。255 も含める。 */
                counted_deleted++;
            }
        }
    }

    if (ht->hdr->next_empty != min_empty)
    {
        return -1;
    }
    if (counted_in_use != ht->hdr->in_use_count)
    {
        return -1;
    }
    if (counted_deleted != ht->hdr->deleted_count)
    {
        return -1;
    }
    if ((counted_key_storage != ht->hdr->key_storage_used) || (counted_value_storage != ht->hdr->value_storage_used))
    {
        return -1;
    }
    for (i = 0; i < capacity; i++)
    {
        size_t j;

        if (*hashtable_entry_status(ht, i) == REC_EMPTY)
        {
            continue;
        }
        for (j = i + 1u; j < capacity; j++)
        {
            if (*hashtable_entry_status(ht, j) == REC_EMPTY)
            {
                continue;
            }
            if (field_is_variable(ht->hdr->config.key_type) != 0)
            {
                struct hashtable_string_ref *a = hashtable_key_ref_at(ht, i);
                struct hashtable_string_ref *b = hashtable_key_ref_at(ht, j);

                if ((a->offset < b->offset + b->length) && (b->offset < a->offset + a->length))
                {
                    return -1;
                }
            }
            if (field_is_variable(ht->hdr->config.value_type) != 0)
            {
                struct hashtable_string_ref *a = hashtable_value_ref_at(ht, i);
                struct hashtable_string_ref *b = hashtable_value_ref_at(ht, j);

                if ((a->offset < b->offset + b->length) && (b->offset < a->offset + a->length))
                {
                    return -1;
                }
            }
        }
    }
    return 0;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_validate(const cplat_hashtable *ht)
{
    unsigned char *visited;
    int result;

    if (ht == NULL)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    visited = (unsigned char *)cplat_calloc(ht->hdr->config.capacity, 1);
    if (visited == NULL)
    {
        return CPLAT_ERR_OUT_OF_MEMORY;
    }
    result = hashtable_validate_impl(ht, visited);
    cplat_free(visited);
    return (result == 0) ? CPLAT_OK : CPLAT_ERR_CORRUPT_DESCRIPTOR;
}

/**
 *  @brief          reuse_deleted 用に、再利用する削除中レコードを選びます。
 *
 *  status が最大、同点なら変更時刻が最も古いレコードを選びます。\n
 *  SCOPE_TABLE でレコード世代が無い場合は、走査順(添字が小さい方を優先)により
 *  自然にレコード番号が最も小さいものが残ります。\n
 *  実時刻ではなく世代カウンターで比較します。実時刻は時計の巻き戻しで逆行し、
 *  最も古いレコードを選べなくなるためです。\n
 *  チェーンからの切り離しはしません。呼び出し側が外します。
 *
 *  @param[in]      ht          対象のハンドルです。
 *  @param[in]      keep        capacity 個の候補マスク。NULL なら全レコードを候補にします。
 *                              非 NULL なら、値が 0 以外のレコードだけを候補にします。
 *  @return         見つかった場合は 1 起点のレコード番号、無ければ 0 です。
 */
static uint64_t hashtable_find_best_deleted_record(const cplat_hashtable *ht, const unsigned char *keep)
{
    size_t capacity = ht->hdr->config.capacity;
    int has_record_timestamp = hashtable_has_record_timestamp(ht);
    size_t best_rec = 0;
    int found = 0;
    unsigned char best_status = 0;
    uint64_t best_generation = 0;
    size_t i;

    for (i = 0; i < capacity; i++)
    {
        unsigned char status = *hashtable_entry_status(ht, i);
        int better;

        if (status < REC_DELETED)
        {
            continue;
        }
        if ((keep != NULL) && (keep[i] == 0))
        {
            continue;
        }
        if (!found)
        {
            better = 1;
        }
        else if (status > best_status)
        {
            better = 1;
        }
        else if ((status == best_status) && (has_record_timestamp != 0) &&
                 (*hashtable_entry_generation(ht, i) < best_generation))
        {
            better = 1;
        }
        else
        {
            better = 0;
        }
        if (better != 0)
        {
            found = 1;
            best_rec = i;
            best_status = status;
            if (has_record_timestamp != 0)
            {
                best_generation = *hashtable_entry_generation(ht, i);
            }
        }
    }
    if (found == 0)
    {
        return 0;
    }
    return (uint64_t)(best_rec + 1);
}

/**
 *  @brief          レコードをバケット チェーンから切り離します。
 *
 *  レコードの状態・キー・値は変更しません。
 *
 *  @param[in,out]  ht          対象のハンドルです。
 *  @param[in]      rec         切り離すレコードの 0 起点の番号です。
 */
static void hashtable_unlink_record(cplat_hashtable *ht, size_t rec)
{
    size_t idx = hash_key(ht, hashtable_entry_key(ht, rec));
    uint64_t *link = &hashtable_bucket_head(ht)[idx];
    uint64_t target = (uint64_t)(rec + 1);

    while (*link != 0)
    {
        if (*link == target)
        {
            *link = *hashtable_entry_next(ht, rec);
            *hashtable_entry_next(ht, rec) = 0;
            return;
        }
        link = hashtable_entry_next(ht, (size_t)(*link - 1));
    }
}

/**
 *  @brief          追加した / 更新したの別を、必要なら呼び出し側へ書きます。
 *  @param[out]     inserted_out  格納先。NULL を渡せます。
 *  @param[in]      inserted      新規追加なら 1、既存更新なら 0。
 */
static void store_inserted(int *inserted_out, int inserted)
{
    if (inserted_out != NULL)
    {
        *inserted_out = inserted;
    }
}

/**
 *  @brief          キーを追加、または既存キーの値を書き換えます。
 *  @param[in,out]  ht              対象。NULL を渡してはなりません。
 *  @param[in]      key             キー。NULL を渡してはなりません。
 *  @param[in]      value           設定した形式の値。NULL を渡してはなりません。
 *  @param[in]      deleted_policy  削除済みの同一キーが見つかった場合の振る舞い。
 *  @param[in]      allow_update    0 なら使用中の同一キーを @ref CPLAT_ERR_DUPLICATE_KEY 、
 *                                  0 以外なら値を書き換えます。
 *  @param[out]     inserted_out    新規追加なら 1、既存更新なら 0。NULL を渡せます。
 *  @return         @ref cplat_hashtable_add と同じ結果コードです。
 *
 *  @ref cplat_hashtable_add と @ref cplat_hashtable_upsert の共通実装です。\n
 *  @p inserted_out は @ref CPLAT_OK のときだけ書きます。
 */
static int hashtable_put(cplat_hashtable *ht, const void *key, const void *value,
                         cplat_hashtable_add_deleted_policy deleted_policy, int allow_update, int *inserted_out)
{
    size_t key_storage_offset = 0;
    size_t value_storage_offset = 0;
    size_t idx;
    uint64_t *bucket_head;
    uint64_t cur;
    uint64_t rec_no;
    size_t rec;

    if ((ht == NULL) || (key == NULL) || (value == NULL))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if ((input_points_into_table(ht, key) != 0) || (input_points_into_table(ht, value) != 0))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if ((deleted_policy != CPLAT_HASHTABLE_ADD_DELETED_OVERWRITE) &&
        (deleted_policy != CPLAT_HASHTABLE_ADD_DELETED_REVIVE))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if (!key_fits(ht, key))
    {
        return CPLAT_ERR_OUT_OF_RANGE;
    }
    if (fixed_string_fits(ht->hdr->config.value_type, ht->hdr->config.value_size, value) == 0)
    {
        return CPLAT_ERR_OUT_OF_RANGE;
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
                if (allow_update == 0)
                {
                    return CPLAT_ERR_DUPLICATE_KEY;
                }
                /* upsert は使用中の同一キーの値を書き換える。 */
                if (value_storage_find_free(ht, rec, 1, value, &value_storage_offset) == 0)
                {
                    return CPLAT_ERR_STORAGE_FULL;
                }
                release_value(ht, rec);
                value_store(ht, rec, value, value_storage_offset);
                stamp_record(ht, rec);
                store_inserted(inserted_out, 0);
                return CPLAT_OK;
            }
            /* 削除済みの同一キーは、同じレコードを再利用する。 */
            if (deleted_policy == CPLAT_HASHTABLE_ADD_DELETED_OVERWRITE)
            {
                if (value_storage_find_free(ht, rec, 1, value, &value_storage_offset) == 0)
                {
                    return CPLAT_ERR_STORAGE_FULL;
                }
                release_value(ht, rec);
                value_store(ht, rec, value, value_storage_offset);
            }
            /* REVIVE はスロットに残っている削除前の値をそのまま使う。 */
            *hashtable_entry_status(ht, rec) = REC_IN_USE;
            stamp_record(ht, rec);
            ht->hdr->deleted_count--;
            ht->hdr->in_use_count++;
            store_inserted(inserted_out, 1);
            return CPLAT_OK;
        }
        cur = *hashtable_entry_next(ht, rec);
    }

    rec_no = ht->hdr->next_empty;
    if ((rec_no == 0) && (ht->hdr->config.reuse_deleted != 0))
    {
        rec_no = hashtable_find_best_deleted_record(ht, NULL);
    }
    if (rec_no == 0)
    {
        return CPLAT_ERR_LIMIT_EXCEEDED;
    }

    rec = (size_t)(rec_no - 1);
    if ((key_storage_find_free(ht, rec, *hashtable_entry_status(ht, rec) != REC_EMPTY, key, &key_storage_offset) ==
         0) ||
        (value_storage_find_free(ht, rec, *hashtable_entry_status(ht, rec) != REC_EMPTY, value,
                                 &value_storage_offset) == 0))
    {
        return CPLAT_ERR_STORAGE_FULL;
    }
    if (*hashtable_entry_status(ht, rec) != REC_EMPTY)
    {
        hashtable_unlink_record(ht, rec); /* 削除中の既存キーをチェーンから外す。 */
        ht->hdr->deleted_count--;
        release_key(ht, rec);
        release_value(ht, rec);
    }
    value_store(ht, rec, value, value_storage_offset);
    *hashtable_entry_status(ht, rec) = REC_IN_USE;
    key_store(ht, rec, key, key_storage_offset);
    stamp_record(ht, rec);
    /* 新しいレコードをチェーン先頭へ挿す。 */
    *hashtable_entry_next(ht, rec) = bucket_head[idx];
    bucket_head[idx] = rec_no;
    if (rec_no == ht->hdr->next_empty) /* 空きスロットを使ったときだけ、次の空きを探し直す。 */
    {
        ht->hdr->next_empty = scan_next_empty(ht, rec + 1);
    }
    ht->hdr->in_use_count++;
    store_inserted(inserted_out, 1);
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_add(cplat_hashtable *ht, const void *key, const void *value,
                           cplat_hashtable_add_deleted_policy deleted_policy)
{
    return hashtable_put(ht, key, value, deleted_policy, 0, NULL);
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_upsert(cplat_hashtable *ht, const void *key, const void *value, int *inserted_out)
{
    return hashtable_put(ht, key, value, CPLAT_HASHTABLE_ADD_DELETED_OVERWRITE, 1, inserted_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_insert_direct(cplat_hashtable *ht, uint64_t record, const void *key, int status,
                                     const void *value, const cplat_timespec *timestamp, uint64_t generation)
{
    size_t key_storage_offset = 0;
    size_t value_storage_offset = 0;
    size_t idx;
    uint64_t *bucket_head;
    uint64_t cur;
    size_t rec;

    if ((ht == NULL) || (key == NULL) || (value == NULL))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if ((input_points_into_table(ht, key) != 0) || (input_points_into_table(ht, value) != 0))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if (hashtable_has_record_timestamp(ht) != 0)
    {
        if (timestamp == NULL)
        {
            return CPLAT_ERR_INVALID_ARGUMENT;
        }
    }
    else if ((timestamp != NULL) || (generation != 0))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if ((status < REC_IN_USE) || (status > CPLAT_HASHTABLE_LIFETIME_INFINITE))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if (!key_fits(ht, key))
    {
        return CPLAT_ERR_OUT_OF_RANGE;
    }
    if (fixed_string_fits(ht->hdr->config.value_type, ht->hdr->config.value_size, value) == 0)
    {
        return CPLAT_ERR_OUT_OF_RANGE;
    }
    if ((record == 0) || (record > ht->hdr->config.capacity))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    /* 有限寿命では、加齢後に存在し得ない status は書かずに省略する。 */
    if ((status >= REC_DELETED) && (ht->hdr->config.lifetime != CPLAT_HASHTABLE_LIFETIME_INFINITE) &&
        (status >= (int)ht->hdr->config.lifetime))
    {
        return CPLAT_SKIPPED;
    }

    rec = (size_t)(record - 1);
    if (*hashtable_entry_status(ht, rec) != REC_EMPTY)
    {
        return CPLAT_ERR_DUPLICATE_DEFINITION;
    }

    idx = hash_key(ht, key);
    bucket_head = hashtable_bucket_head(ht);
    cur = bucket_head[idx];
    while (cur != 0)
    {
        size_t existing = (size_t)(cur - 1);

        if (key_equal(ht, hashtable_entry_key(ht, existing), key))
        {
            return CPLAT_ERR_DUPLICATE_KEY;
        }
        cur = *hashtable_entry_next(ht, existing);
    }

    if ((key_storage_find_free(ht, rec, 0, key, &key_storage_offset) == 0) ||
        (value_storage_find_free(ht, rec, 0, value, &value_storage_offset) == 0))
    {
        return CPLAT_ERR_STORAGE_FULL;
    }
    value_store(ht, rec, value, value_storage_offset);
    key_store(ht, rec, key, key_storage_offset);
    *hashtable_entry_status(ht, rec) = (unsigned char)status;
    if (hashtable_has_record_timestamp(ht) != 0)
    {
        *hashtable_entry_timestamp(ht, rec) = *timestamp;
        if (cplat_timespec_cmp(timestamp, &ht->hdr->table_timestamp) > 0)
        {
            ht->hdr->table_timestamp = *timestamp;
        }
        *hashtable_entry_generation(ht, rec) = generation;
        if (generation > ht->hdr->table_generation)
        {
            ht->hdr->table_generation = generation;
        }
    }
    *hashtable_entry_next(ht, rec) = bucket_head[idx];
    bucket_head[idx] = record;
    /* 最小空きを使ったときだけ、次の空きを探し直す。 */
    if (ht->hdr->next_empty == record)
    {
        ht->hdr->next_empty = scan_next_empty(ht, rec + 1);
    }
    if (status == REC_IN_USE)
    {
        ht->hdr->in_use_count++;
    }
    else
    {
        ht->hdr->deleted_count++;
    }
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_update(cplat_hashtable *ht, const void *key, const void *value)
{
    size_t value_storage_offset = 0;
    size_t idx;
    uint64_t *bucket_head;
    uint64_t cur;

    if ((ht == NULL) || (key == NULL) || (value == NULL))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if (input_points_into_table(ht, value) != 0)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if (!key_fits(ht, key))
    {
        return CPLAT_ERR_OUT_OF_RANGE;
    }
    if (fixed_string_fits(ht->hdr->config.value_type, ht->hdr->config.value_size, value) == 0)
    {
        return CPLAT_ERR_OUT_OF_RANGE;
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
                return CPLAT_ERR_NOT_FOUND;
            }
            if (value_storage_find_free(ht, rec, 1, value, &value_storage_offset) == 0)
            {
                return CPLAT_ERR_STORAGE_FULL;
            }
            release_value(ht, rec);
            value_store(ht, rec, value, value_storage_offset);
            stamp_record(ht, rec);
            return CPLAT_OK;
        }
        cur = *hashtable_entry_next(ht, rec);
    }
    return CPLAT_ERR_NOT_FOUND;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_update_rec(cplat_hashtable *ht, uint64_t record, const void *value)
{
    size_t rec;
    size_t value_storage_offset = 0;

    if ((ht == NULL) || (value == NULL))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if (input_points_into_table(ht, value) != 0)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if (fixed_string_fits(ht->hdr->config.value_type, ht->hdr->config.value_size, value) == 0)
    {
        return CPLAT_ERR_OUT_OF_RANGE;
    }
    if ((record == 0) || (record > ht->hdr->config.capacity))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    rec = (size_t)(record - 1);
    if (*hashtable_entry_status(ht, rec) != REC_IN_USE)
    {
        return CPLAT_ERR_NOT_FOUND;
    }
    if (value_storage_find_free(ht, rec, 1, value, &value_storage_offset) == 0)
    {
        return CPLAT_ERR_STORAGE_FULL;
    }
    release_value(ht, rec);
    value_store(ht, rec, value, value_storage_offset);
    stamp_record(ht, rec);
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_find_value_ref(const cplat_hashtable *ht, const void *key, const void **value_out)
{
    size_t idx;
    uint64_t *bucket_head;
    uint64_t cur;

    if ((ht == NULL) || (key == NULL) || (value_out == NULL))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if (!key_fits(ht, key))
    {
        return CPLAT_ERR_OUT_OF_RANGE;
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
                return CPLAT_OK;
            }
            return CPLAT_ERR_NOT_FOUND;
        }
        cur = *hashtable_entry_next(ht, rec);
    }
    return CPLAT_ERR_NOT_FOUND;
}

static size_t stored_value_size(const cplat_hashtable *ht, size_t rec)
{
    if (field_is_variable(ht->hdr->config.value_type) != 0)
    {
        return (size_t)hashtable_value_ref_at(ht, rec)->length;
    }
    if (ht->hdr->config.value_type == CPLAT_HASHTABLE_FIELD_FIXED_STRING)
    {
        return strlen((const char *)hashtable_data_at(ht, rec)) + 1u;
    }
    return ht->hdr->config.value_size;
}

static size_t stored_key_size(const cplat_hashtable *ht, size_t rec)
{
    if (field_is_variable(ht->hdr->config.key_type) != 0)
    {
        return (size_t)hashtable_key_ref_at(ht, rec)->length;
    }
    if (ht->hdr->config.key_type == CPLAT_HASHTABLE_FIELD_FIXED_STRING)
    {
        return strlen(hashtable_entry_key(ht, rec)) + 1u;
    }
    return ht->hdr->config.key_size;
}

static int copy_field(const void *src, size_t required_size, void *dest, size_t dest_size, size_t *required_size_out)
{
    if ((required_size_out == NULL) || ((dest == NULL) && (dest_size != 0)) || ((dest != NULL) && (dest_size == 0)))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    *required_size_out = required_size;
    if (dest == NULL)
    {
        return CPLAT_OK;
    }
    if (dest_size < required_size)
    {
        return CPLAT_ERR_BUFFER_TOO_SMALL;
    }
    memcpy(dest, src, required_size);
    return CPLAT_OK;
}

static int copy_arguments_are_valid(const void *dest, size_t dest_size, const size_t *required_size_out)
{
    return (required_size_out != NULL) &&
           (((dest == NULL) && (dest_size == 0)) || ((dest != NULL) && (dest_size != 0)));
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_find_value_copy(const cplat_hashtable *ht, const void *key, void *dest, size_t dest_size,
                                       size_t *required_size_out)
{
    uint64_t record;
    int ret;

    if (copy_arguments_are_valid(dest, dest_size, required_size_out) == 0)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    ret = cplat_hashtable_find_recno(ht, key, &record);
    if (ret != CPLAT_OK)
    {
        return ret;
    }
    return copy_field(hashtable_data_at(ht, (size_t)(record - 1)), stored_value_size(ht, (size_t)(record - 1)), dest,
                      dest_size, required_size_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_find_recno(const cplat_hashtable *ht, const void *key, uint64_t *record_out)
{
    size_t idx;
    uint64_t *bucket_head;
    uint64_t cur;

    if ((ht == NULL) || (key == NULL) || (record_out == NULL))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if (!key_fits(ht, key))
    {
        return CPLAT_ERR_OUT_OF_RANGE;
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
                return CPLAT_OK;
            }
            return CPLAT_ERR_NOT_FOUND;
        }
        cur = *hashtable_entry_next(ht, rec);
    }
    return CPLAT_ERR_NOT_FOUND;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_find_timestamp_ref(const cplat_hashtable *ht, const void *key,
                                          const cplat_timespec **timestamp_out)
{
    size_t idx;
    uint64_t *bucket_head;
    uint64_t cur;

    if ((ht == NULL) || (key == NULL) || (timestamp_out == NULL))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if (hashtable_has_record_timestamp(ht) == 0)
    {
        return CPLAT_ERR_UNSUPPORTED;
    }
    if (!key_fits(ht, key))
    {
        return CPLAT_ERR_OUT_OF_RANGE;
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
                return CPLAT_OK;
            }
            return CPLAT_ERR_NOT_FOUND;
        }
        cur = *hashtable_entry_next(ht, rec);
    }
    return CPLAT_ERR_NOT_FOUND;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_find_timestamp_val(const cplat_hashtable *ht, const void *key,
                                          cplat_timespec *timestamp_out)
{
    const cplat_timespec *src;
    int ret;

    if (timestamp_out == NULL)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    ret = cplat_hashtable_find_timestamp_ref(ht, key, &src);
    if (ret != CPLAT_OK)
    {
        return ret;
    }
    *timestamp_out = *src;
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_find_generation(const cplat_hashtable *ht, const void *key, uint64_t *generation_out)
{
    size_t idx;
    uint64_t *bucket_head;
    uint64_t cur;

    if ((ht == NULL) || (key == NULL) || (generation_out == NULL))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if (hashtable_has_record_timestamp(ht) == 0)
    {
        return CPLAT_ERR_UNSUPPORTED;
    }
    if (!key_fits(ht, key))
    {
        return CPLAT_ERR_OUT_OF_RANGE;
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
                *generation_out = *hashtable_entry_generation(ht, rec);
                return CPLAT_OK;
            }
            return CPLAT_ERR_NOT_FOUND;
        }
        cur = *hashtable_entry_next(ht, rec);
    }
    return CPLAT_ERR_NOT_FOUND;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_get_key_ref(const cplat_hashtable *ht, uint64_t record, const void **key_out)
{
    size_t rec;

    if ((ht == NULL) || (key_out == NULL))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if ((record == 0) || (record > ht->hdr->config.capacity))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    rec = (size_t)(record - 1);
    /* 空は失敗。削除済みは削除直前のキーを返す。 */
    if (*hashtable_entry_status(ht, rec) == REC_EMPTY)
    {
        return CPLAT_ERR_NOT_FOUND;
    }
    *key_out = hashtable_entry_key(ht, rec);
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_get_key_copy(const cplat_hashtable *ht, uint64_t record, void *dest, size_t dest_size,
                                    size_t *required_size_out)
{
    const void *src;
    int ret;

    if (copy_arguments_are_valid(dest, dest_size, required_size_out) == 0)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    ret = cplat_hashtable_get_key_ref(ht, record, &src);
    if (ret != CPLAT_OK)
    {
        return ret;
    }
    return copy_field(src, stored_key_size(ht, (size_t)(record - 1)), dest, dest_size, required_size_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_get_value_ref(const cplat_hashtable *ht, uint64_t record, const void **value_out)
{
    size_t rec;

    if ((ht == NULL) || (value_out == NULL))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if ((record == 0) || (record > ht->hdr->config.capacity))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    rec = (size_t)(record - 1);
    /* 空は失敗。削除済みは削除直前の値を返す。 */
    if (*hashtable_entry_status(ht, rec) == REC_EMPTY)
    {
        return CPLAT_ERR_NOT_FOUND;
    }
    *value_out = hashtable_data_at(ht, rec);
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_get_value_copy(const cplat_hashtable *ht, uint64_t record, void *dest, size_t dest_size,
                                      size_t *required_size_out)
{
    const void *src;
    int ret;

    if (copy_arguments_are_valid(dest, dest_size, required_size_out) == 0)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    ret = cplat_hashtable_get_value_ref(ht, record, &src);
    if (ret != CPLAT_OK)
    {
        return ret;
    }
    return copy_field(src, stored_value_size(ht, (size_t)(record - 1)), dest, dest_size, required_size_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_get_status(const cplat_hashtable *ht, uint64_t record, int *status_out)
{
    if ((ht == NULL) || (status_out == NULL))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if ((record == 0) || (record > ht->hdr->config.capacity))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    *status_out = (int)*hashtable_entry_status(ht, (size_t)(record - 1));
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_next_record(const cplat_hashtable *ht, uint64_t from, unsigned int status_mask,
                                   uint64_t *record_out, int *has_record_out)
{
    unsigned int known_bits =
        CPLAT_HASHTABLE_SCAN_IN_USE | CPLAT_HASHTABLE_SCAN_DELETED | CPLAT_HASHTABLE_SCAN_EMPTY;
    size_t i;

    if ((ht == NULL) || (record_out == NULL) || (has_record_out == NULL))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if ((status_mask == 0) || ((status_mask & ~known_bits) != 0))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    /* from は直前に返したレコード番号。capacity と等しいときは走査済みで、終端を返す。 */
    if (from > ht->hdr->config.capacity)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }

    *has_record_out = 0;
    for (i = (size_t)from; i < ht->hdr->config.capacity; i++)
    {
        unsigned char status = *hashtable_entry_status(ht, i);
        unsigned int bit;

        if (status == REC_EMPTY)
        {
            bit = CPLAT_HASHTABLE_SCAN_EMPTY;
        }
        else if (status == REC_IN_USE)
        {
            bit = CPLAT_HASHTABLE_SCAN_IN_USE;
        }
        else
        {
            bit = CPLAT_HASHTABLE_SCAN_DELETED;
        }
        if ((status_mask & bit) != 0)
        {
            *record_out = (uint64_t)(i + 1);
            *has_record_out = 1;
            return CPLAT_OK;
        }
    }
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_get_timestamp_ref(const cplat_hashtable *ht, uint64_t record,
                                         const cplat_timespec **timestamp_out)
{
    size_t rec;

    if ((ht == NULL) || (timestamp_out == NULL))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if (hashtable_has_record_timestamp(ht) == 0)
    {
        return CPLAT_ERR_UNSUPPORTED;
    }
    if ((record == 0) || (record > ht->hdr->config.capacity))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    rec = (size_t)(record - 1);
    if (*hashtable_entry_status(ht, rec) == REC_EMPTY)
    {
        return CPLAT_ERR_NOT_FOUND;
    }
    *timestamp_out = hashtable_entry_timestamp(ht, rec);
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_get_timestamp_val(const cplat_hashtable *ht, uint64_t record,
                                         cplat_timespec *timestamp_out)
{
    const cplat_timespec *src;
    int ret;

    if (timestamp_out == NULL)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    ret = cplat_hashtable_get_timestamp_ref(ht, record, &src);
    if (ret != CPLAT_OK)
    {
        return ret;
    }
    *timestamp_out = *src;
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_get_generation(const cplat_hashtable *ht, uint64_t record, uint64_t *generation_out)
{
    size_t rec;

    if ((ht == NULL) || (generation_out == NULL))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if (hashtable_has_record_timestamp(ht) == 0)
    {
        return CPLAT_ERR_UNSUPPORTED;
    }
    if ((record == 0) || (record > ht->hdr->config.capacity))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    rec = (size_t)(record - 1);
    if (*hashtable_entry_status(ht, rec) == REC_EMPTY)
    {
        return CPLAT_ERR_NOT_FOUND;
    }
    *generation_out = *hashtable_entry_generation(ht, rec);
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_get_table_timestamp_ref(const cplat_hashtable *ht, const cplat_timespec **timestamp_out)
{
    if ((ht == NULL) || (timestamp_out == NULL))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    *timestamp_out = &ht->hdr->table_timestamp;
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_get_table_timestamp_val(const cplat_hashtable *ht, cplat_timespec *timestamp_out)
{
    const cplat_timespec *src;
    int ret;

    if (timestamp_out == NULL)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    ret = cplat_hashtable_get_table_timestamp_ref(ht, &src);
    if (ret != CPLAT_OK)
    {
        return ret;
    }
    *timestamp_out = *src;
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_get_table_generation(const cplat_hashtable *ht, uint64_t *generation_out)
{
    if ((ht == NULL) || (generation_out == NULL))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    *generation_out = ht->hdr->table_generation;
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_count_status(const cplat_hashtable *ht, size_t *in_use_out, size_t *deleted_out,
                                    size_t *empty_out)
{
    if (ht == NULL)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }

    /* 永続化領域のカウンタをそのまま返す。capacity に依存しない一定時間。 */
    if (in_use_out != NULL)
    {
        *in_use_out = (size_t)ht->hdr->in_use_count;
    }
    if (deleted_out != NULL)
    {
        *deleted_out = (size_t)ht->hdr->deleted_count;
    }
    if (empty_out != NULL)
    {
        *empty_out = (size_t)(ht->hdr->config.capacity - ht->hdr->in_use_count - ht->hdr->deleted_count);
    }
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_count(const cplat_hashtable *ht, size_t *count_out)
{
    if ((ht == NULL) || (count_out == NULL))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    return cplat_hashtable_count_status(ht, count_out, NULL, NULL);
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_deleted_count(const cplat_hashtable *ht, size_t *count_out)
{
    if ((ht == NULL) || (count_out == NULL))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    return cplat_hashtable_count_status(ht, NULL, count_out, NULL);
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_empty_count(const cplat_hashtable *ht, size_t *count_out)
{
    if ((ht == NULL) || (count_out == NULL))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    return cplat_hashtable_count_status(ht, NULL, NULL, count_out);
}

/**
 *  @brief          スロットを空へ戻し、キーと値と変更時刻を消します。
 *  @param[in]      ht   対象。NULL を渡してはなりません。
 *  @param[in]      rec  0 相対のスロット添字。
 *
 *  チェーンからの切り離しはしません。呼び出し側が外します。\n
 *  SCOPE_RECORD のときは変更時刻も 0 埋めします。
 */
static void hashtable_expire_record(cplat_hashtable *ht, size_t rec)
{
    *hashtable_entry_status(ht, rec) = REC_EMPTY;
    release_value(ht, rec);
    release_key(ht, rec);
    if (hashtable_has_record_timestamp(ht) != 0)
    {
        memset(hashtable_entry_timestamp(ht, rec), 0, sizeof(cplat_timespec));
        *hashtable_entry_generation(ht, rec) = 0;
    }
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_delete(cplat_hashtable *ht, const void *key)
{
    size_t idx;
    uint64_t *link;

    if ((ht == NULL) || (key == NULL))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if (!key_fits(ht, key))
    {
        return CPLAT_ERR_OUT_OF_RANGE;
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
                return CPLAT_ERR_NOT_FOUND;
            }
            *hashtable_entry_status(ht, rec) = REC_DELETED;
            stamp_record(ht, rec);
            ht->hdr->in_use_count--;
            /* lifetime が 2 のときは、削除済みのまま置けないので直ちに空へ戻す。 */
            if (REC_DELETED >= ht->hdr->config.lifetime)
            {
                uint64_t rec_no = (uint64_t)(rec + 1);

                hashtable_expire_record(ht, rec);
                *link = *hashtable_entry_next(ht, rec);
                *hashtable_entry_next(ht, rec) = 0;
                if ((ht->hdr->next_empty == 0) || (rec_no < ht->hdr->next_empty))
                {
                    ht->hdr->next_empty = rec_no;
                }
            }
            else
            {
                ht->hdr->deleted_count++;
            }
            return CPLAT_OK;
        }
        link = hashtable_entry_next(ht, rec);
    }
    return CPLAT_ERR_NOT_FOUND;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_delete_rec(cplat_hashtable *ht, uint64_t record)
{
    size_t rec;

    if (ht == NULL)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if ((record == 0) || (record > ht->hdr->config.capacity))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    rec = (size_t)(record - 1);
    if (*hashtable_entry_status(ht, rec) != REC_IN_USE)
    {
        return CPLAT_ERR_NOT_FOUND;
    }
    return cplat_hashtable_delete(ht, hashtable_entry_key(ht, rec));
}

/**
 *  @brief          空きになったスロットを全バケットのチェーンから外します。
 *  @param[in]      ht  対象。NULL を渡してはなりません。
 *
 *  expire はチェーンを触らないため、加齢や一括回収のあとで呼びます。
 */
static void hashtable_unlink_empty_chains(cplat_hashtable *ht)
{
    uint64_t *bucket_head = hashtable_bucket_head(ht);
    size_t i;

    for (i = 0; i < ht->hdr->config.capacity; i++)
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

int cplat_hashtable_push_deleted(cplat_hashtable *ht)
{
    uint64_t new_next_empty = 0;
    uint64_t expired_count = 0;
    size_t i;

    if (ht == NULL)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }

    for (i = 0; i < ht->hdr->config.capacity; i++)
    {
        unsigned char *status = hashtable_entry_status(ht, i);

        /* 255 は増やさない。無限寿命では空へ戻さない。 */
        if ((*status >= REC_DELETED) && (*status < CPLAT_HASHTABLE_LIFETIME_INFINITE))
        {
            (*status)++;
            if ((*status >= ht->hdr->config.lifetime) &&
                (ht->hdr->config.lifetime != CPLAT_HASHTABLE_LIFETIME_INFINITE))
            {
                hashtable_expire_record(ht, i);
                expired_count++;
            }
        }
        if ((new_next_empty == 0) && (*status == REC_EMPTY))
        {
            new_next_empty = (uint64_t)(i + 1);
        }
    }

    hashtable_unlink_empty_chains(ht);
    ht->hdr->next_empty = new_next_empty;
    ht->hdr->deleted_count -= expired_count;
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_purge_deleted(cplat_hashtable *ht)
{
    uint64_t new_next_empty = 0;
    size_t i;

    if (ht == NULL)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }

    for (i = 0; i < ht->hdr->config.capacity; i++)
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
    ht->hdr->next_empty = new_next_empty;
    ht->hdr->deleted_count = 0; /* 削除済みは無条件で全て空へ戻すため。 */
    return CPLAT_OK;
}

/**
 *  @brief          設定のうち、伸長・縮小で変えてよい項目以外が一致するかを判定します。
 *  @param[in]      current  現在の設定。NULL を渡してはなりません。
 *  @param[in]      next     移行後の設定。NULL を渡してはなりません。
 *  @return         一致すれば 1、異なれば 0 です。
 *
 *  変えてよいのは capacity、key_storage_size、value_storage_size の 3 つだけです。
 *  フィールド形式やレイアウト要件まで変えるのは create と insert_direct の役割です。
 */
static int hashtable_config_is_compatible(const cplat_hashtable_config *current,
                                          const cplat_hashtable_config *next)
{
    if ((current->key_type != next->key_type) || (current->value_type != next->value_type))
    {
        return 0;
    }
    if ((current->key_size != next->key_size) || (current->value_size != next->value_size))
    {
        return 0;
    }
    if ((current->value_align != next->value_align) || (current->timestamp_scope != next->timestamp_scope))
    {
        return 0;
    }
    if ((current->lifetime != next->lifetime) || (current->reuse_deleted != next->reuse_deleted))
    {
        return 0;
    }
    return 1;
}

/**
 *  @brief          移行で残すレコードを決めます。
 *  @param[in]      src         移行元。NULL を渡してはなりません。
 *  @param[in]      new_config  移行後の設定。NULL を渡してはなりません。
 *  @param[out]     keep_out    capacity 個のマスクの格納先。成功時は呼び出し側が解放します。
 *  @return         @ref CPLAT_OK 、@ref CPLAT_ERR_OUT_OF_MEMORY 、
 *                  @ref CPLAT_ERR_LIMIT_EXCEEDED 、@ref CPLAT_ERR_STORAGE_FULL 。
 *
 *  移行元を変更しません。移行を始める前に、収まるかどうかだけを判定します。\n
 *  使用中のレコードは必ず残します。削除済みのレコードは、reuse_deleted が 0 なら
 *  必ず残し、収まらなければ失敗します。reuse_deleted が非 0 なら、add の追い出しと
 *  同じ規則で古い順に落とします。
 */
static int hashtable_plan_migration(const cplat_hashtable *src, const cplat_hashtable_config *new_config,
                                    unsigned char **keep_out)
{
    size_t src_capacity = src->hdr->config.capacity;
    size_t new_capacity = new_config->capacity;
    unsigned char *keep;
    size_t kept = 0;
    size_t key_used = 0;
    size_t value_used = 0;
    size_t i;

    keep = (unsigned char *)cplat_calloc(src_capacity, sizeof(unsigned char));
    if (keep == NULL)
    {
        return CPLAT_ERR_OUT_OF_MEMORY;
    }

    for (i = 0; i < src_capacity; i++)
    {
        if (*hashtable_entry_status(src, i) != REC_EMPTY)
        {
            keep[i] = 1;
            kept++;
        }
    }

    if (src->hdr->in_use_count > (uint64_t)new_capacity)
    {
        cplat_free(keep);
        return CPLAT_ERR_LIMIT_EXCEEDED;
    }
    if (kept > new_capacity)
    {
        /* reuse_deleted が 0 の設定は、削除済みを自動的には手放さないという意思表示。 */
        if (src->hdr->config.reuse_deleted == 0)
        {
            cplat_free(keep);
            return CPLAT_ERR_LIMIT_EXCEEDED;
        }
        while (kept > new_capacity)
        {
            uint64_t victim = hashtable_find_best_deleted_record(src, keep);

            if (victim == 0)
            {
                cplat_free(keep);
                return CPLAT_ERR_LIMIT_EXCEEDED;
            }
            keep[victim - 1u] = 0;
            kept--;
        }
    }

    for (i = 0; i < src_capacity; i++)
    {
        if (keep[i] == 0)
        {
            continue;
        }
        if (field_is_variable(src->hdr->config.key_type) != 0)
        {
            key_used += (size_t)hashtable_key_ref_at(src, i)->length;
        }
        if (field_is_variable(src->hdr->config.value_type) != 0)
        {
            value_used += (size_t)hashtable_value_ref_at(src, i)->length;
        }
    }
    if ((key_used > new_config->key_storage_size) || (value_used > new_config->value_storage_size))
    {
        cplat_free(keep);
        return CPLAT_ERR_STORAGE_FULL;
    }

    *keep_out = keep;
    return CPLAT_OK;
}

/**
 *  @brief          決めたレコードを移行先へ書き込みます。
 *  @param[in]      src   移行元。NULL を渡してはなりません。
 *  @param[in,out]  dst   移行先。構築直後の空テーブルを渡してください。
 *  @param[in]      keep  @ref hashtable_plan_migration が決めたマスク。
 *  @return         @ref CPLAT_OK 、または @ref cplat_hashtable_insert_direct の失敗値。
 *
 *  拡大 (移行先の capacity が移行元以上) ではレコード番号をそのまま写します。\n
 *  縮小では 1 から順に詰めて番号を振り直します。\n
 *  変更時刻と世代カウンターは、レコードもテーブルも移行元の値を引き継ぎます。
 */
static int hashtable_apply_migration(const cplat_hashtable *src, cplat_hashtable *dst, const unsigned char *keep)
{
    size_t src_capacity = src->hdr->config.capacity;
    int preserve_record = 0;
    uint64_t next_record = 1;
    size_t i;

    if (dst->hdr->config.capacity >= src_capacity)
    {
        preserve_record = 1;
    }

    for (i = 0; i < src_capacity; i++)
    {
        const cplat_timespec *timestamp = NULL;
        uint64_t generation = 0;
        uint64_t record;
        int status;
        int ret;

        if (keep[i] == 0)
        {
            continue;
        }
        status = (int)*hashtable_entry_status(src, i);
        if (hashtable_has_record_timestamp(src) != 0)
        {
            timestamp = hashtable_entry_timestamp(src, i);
            generation = *hashtable_entry_generation(src, i);
        }
        if (preserve_record != 0)
        {
            record = (uint64_t)(i + 1);
        }
        else
        {
            record = next_record;
            next_record++;
        }
        ret = cplat_hashtable_insert_direct(dst, record, hashtable_entry_key(src, i), status,
                                               hashtable_data_at(src, i), timestamp, generation);
        if (ret != CPLAT_OK)
        {
            return ret;
        }
    }

    dst->hdr->table_timestamp = src->hdr->table_timestamp;
    dst->hdr->table_generation = src->hdr->table_generation;
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_resize(cplat_hashtable *ht, const cplat_hashtable_config *new_config)
{
    cplat_hashtable *staged = NULL;
    unsigned char *keep = NULL;
    struct hashtable_persist_header *old_hdr;
    int ret;

    if ((ht == NULL) || (new_config == NULL))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if (ht->owns_buffer == 0)
    {
        return CPLAT_ERR_UNSUPPORTED;
    }
    if (hashtable_validate_config(new_config) != 0)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if (hashtable_config_is_compatible(&ht->hdr->config, new_config) == 0)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }

    ret = hashtable_plan_migration(ht, new_config, &keep);
    if (ret != CPLAT_OK)
    {
        return ret;
    }
    ret = cplat_hashtable_create(new_config, NULL, 0, NULL, 0, &staged);
    if (ret != CPLAT_OK)
    {
        cplat_free(keep);
        return ret;
    }
    ret = hashtable_apply_migration(ht, staged, keep);
    cplat_free(keep);
    if (ret != CPLAT_OK)
    {
        cplat_hashtable_dispose(staged);
        return ret;
    }

    /* 移行が終わってから旧領域を解放する。失敗時は元のテーブルを変更しない。 */
    old_hdr = ht->hdr;
    ht->hdr = staged->hdr;
    ht->data = staged->data;
    cplat_free(old_hdr);
    /* 領域は ht が引き継いだため、staged は内部管理データだけを解放する。 */
    cplat_free(staged);
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_rebuild_into(const cplat_hashtable *src, const cplat_hashtable_config *new_config,
                                    void *buf_mgmt, size_t buf_mgmt_size, void *buf_data, size_t buf_data_size,
                                    cplat_hashtable **ht_out)
{
    cplat_hashtable *dst = NULL;
    unsigned char *keep = NULL;
    int ret;

    if (ht_out != NULL)
    {
        *ht_out = NULL;
    }
    if ((src == NULL) || (new_config == NULL) || (ht_out == NULL) || (buf_mgmt == NULL) || (buf_data == NULL))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if (hashtable_validate_config(new_config) != 0)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if (hashtable_config_is_compatible(&src->hdr->config, new_config) == 0)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }

    ret = hashtable_plan_migration(src, new_config, &keep);
    if (ret != CPLAT_OK)
    {
        return ret;
    }
    ret = cplat_hashtable_create(new_config, buf_mgmt, buf_mgmt_size, buf_data, buf_data_size, &dst);
    if (ret != CPLAT_OK)
    {
        cplat_free(keep);
        return ret;
    }
    ret = hashtable_apply_migration(src, dst, keep);
    cplat_free(keep);
    if (ret != CPLAT_OK)
    {
        cplat_hashtable_dispose(dst);
        return ret;
    }

    *ht_out = dst;
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_compact(cplat_hashtable *ht)
{
    if (ht == NULL)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if (field_is_variable(ht->hdr->config.key_type) != 0)
    {
        struct hashtable_arena arena;

        hashtable_key_arena(ht, &arena);
        arena_compact(ht, &arena, hashtable_key_ref_at, ht->hdr->key_storage_used);
    }
    if (field_is_variable(ht->hdr->config.value_type) != 0)
    {
        struct hashtable_arena arena;

        hashtable_value_arena(ht, &arena);
        arena_compact(ht, &arena, hashtable_value_ref_at, ht->hdr->value_storage_used);
    }
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_hashtable_clear(cplat_hashtable *ht)
{
    size_t off_bucket_head;
    size_t mgmt_size = 0;
    size_t data_size = 0;

    if (ht == NULL)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    (void)hashtable_mgmt_layout(&ht->hdr->config, &off_bucket_head, NULL, &mgmt_size);
    (void)hashtable_data_region_size(&ht->hdr->config, &data_size);
    /* 識別子、設定、テーブル時刻は残し、バケットとエントリだけを消す。 */
    memset(hashtable_bytes(ht) + off_bucket_head, 0, mgmt_size - off_bucket_head);
    /* 値配列(データ領域)も 0 埋めする。ポインター自体(ht->data)は変更しない。 */
    memset(ht->data, 0, data_size);
    ht->hdr->next_empty = 1;
    ht->hdr->in_use_count = 0;
    ht->hdr->deleted_count = 0;
    ht->hdr->key_storage_used = 0;
    ht->hdr->value_storage_used = 0;
    hashtable_reset_arenas(ht);
    stamp_table(ht);
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

void cplat_hashtable_dispose(cplat_hashtable *ht)
{
    if (ht == NULL)
    {
        return;
    }
    /* 内部確保時は永続化領域とデータ領域が1ブロックのため、hdr の解放で両方片付く。 */
    if (ht->owns_buffer != 0)
    {
        cplat_free(ht->hdr);
    }
    /* 内部管理データは、生成経路によらず常に解放する。 */
    cplat_free(ht);
}
