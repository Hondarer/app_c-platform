/**
 *******************************************************************************
 *  @file           filter_source.c
 *  @brief          フィルター オブジェクトを受け渡すソース領域の公開と読み取りを実装します。
 *  @author         Tetsuo Honda
 *  @date           2026/10/03
 *  @version        0.1.0
 *
 *  公開と読み取りは seqlock と同じ手順です。書き込み側は公開時刻を奇数にしてから内容を書き、
 *  最後に新しい偶数の値を書き込みます。読み取り側はロックを取らず、読み取りの前後で公開時刻を比べます。\n
 *  see: https://www.hpl.hp.com/techreports/2012/HPL-2012-68.pdf
 *
 *  内容の読み取りは書き込みと重なり得ます。重なった内容は公開時刻の比較で捨て、
 *  比較をすり抜けた破損もフィルター オブジェクトのハッシュ値の検証で拒否します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *******************************************************************************
 */

#include "filter.h"

#include <cplat/base/result.h>
#include <cplat/clock/clock.h>
#include <cplat/runtime/process.h>

#include <stdint.h>
#include <string.h>

_Static_assert(sizeof(string_catalog_filter_source_header) == CPLAT_STRING_CATALOG_FILTER_SOURCE_HEADER_SIZE,
               "cplat: source header size");

/** 1 秒あたりのナノ秒数です。 */
#define NANOSECONDS_PER_SECOND 1000000000ULL

/** 公開時刻のうち、書き込み中を表すビットです。 */
#define TIMESTAMP_WRITING_BIT 1ULL

/** 公開時刻が上限を超えた場合に戻る値です。0 は未公開を表すため、0 でない最小の偶数とします。 */
#define TIMESTAMP_WRAPPED 2ULL

/**
 *  @brief          単調増加クロックの現在値をナノ秒で返します。
 *
 *  Linux の CLOCK_MONOTONIC と Windows の GetTickCount64() は、どちらもシステム起動からの経過時間で、
 *  プロセス間で共通の時間軸です。\n
 *  see: https://man7.org/linux/man-pages/man2/clock_gettime.2.html \n
 *  see: https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-gettickcount64
 */
static uint64_t monotonic_nanoseconds(void)
{
    cplat_timespec now;

    cplat_clock_get_monotonic(&now);
    return ((uint64_t)now.tv_sec * NANOSECONDS_PER_SECOND) + (uint64_t)now.tv_nsec;
}

/** ヘッダーが、0 で埋まった未公開の領域か、本ライブラリの形式の領域であるかを返します。 */
static bool is_known_header(const string_catalog_filter_source_header *header)
{
    return (header->signature == 0U) || string_catalog_filter_source_is_header_valid(header);
}

/**
 *  @brief          前回の公開時刻と単調増加クロックの現在値から、新しい公開時刻を求めます。
 *  @param[in]      base 前回の公開時刻 (偶数)。未公開の場合は 0。
 *  @param[in]      now  単調増加クロックの現在値 (偶数)。
 *  @return         0 でない偶数で、@p base と異なる値。
 *
 *  通常は、前回の値に 2 を加えた値と現在の値のうち、大きいほうを返します。\n
 *  ファイルをマップした領域は OS の再起動を越えて残るため、前回の値が現在の単調増加クロックより大きいことがあります。
 *  この場合も前回の値から増やし、値の重複を避けます。
 *
 *  前回の値に 2 を加えると 64 ビットを超える場合は、@ref TIMESTAMP_WRAPPED と現在の値のうち大きいほうへ戻します。\n
 *  読み取り側は大小ではなく不一致で変化を判定するため、戻った値でも取り込みます。
 *  ナノ秒の単調増加クロックで上限に届くには約 584 年かかるため、通常は、壊れた値や意図的に大きくした値でだけ起こります。
 */
static uint64_t next_timestamp(const uint64_t base, const uint64_t now)
{
    uint64_t next;

    if (base <= (UINT64_MAX - 3U))
    {
        next = base + 2U;
    }
    else
    {
        next = TIMESTAMP_WRAPPED;
    }
    if (next < now)
    {
        next = now;
    }
    /* 上限から戻した値が前回と同じになることは現実にはないが、不一致の判定を確実にするため避ける */
    if (next == base)
    {
        next = (base == TIMESTAMP_WRAPPED) ? (TIMESTAMP_WRAPPED + 2U) : TIMESTAMP_WRAPPED;
    }
    return next;
}

/* Doxygen コメントは、ヘッダーに記載 */

bool string_catalog_filter_source_is_header_valid(const string_catalog_filter_source_header *header)
{
    return (header->signature == STRING_CATALOG_FILTER_SOURCE_SIGNATURE) &&
           (header->format_version == STRING_CATALOG_FILTER_SOURCE_FORMAT_VERSION) &&
           (header->header_size == CPLAT_STRING_CATALOG_FILTER_SOURCE_HEADER_SIZE);
}

/* Doxygen コメントは、ヘッダーに記載 */

bool string_catalog_filter_source_is_region_valid(const void *source, const size_t source_size, const size_t image_size)
{
    if ((source == NULL) || (((uintptr_t)source % CPLAT_STRING_CATALOG_FILTER_SOURCE_ALIGNMENT) != 0U))
    {
        return false;
    }
    return (source_size >= CPLAT_STRING_CATALOG_FILTER_SOURCE_HEADER_SIZE) &&
           ((source_size - CPLAT_STRING_CATALOG_FILTER_SOURCE_HEADER_SIZE) >= image_size);
}

/* Doxygen コメントは、ヘッダーに記載 */

uint64_t string_catalog_filter_source_begin_read(const void *source)
{
    const string_catalog_filter_source_header *header = (const string_catalog_filter_source_header *)source;

    return cplat_atomic_load_u64(&header->published_timestamp, CPLAT_MEMORY_ORDER_ACQUIRE);
}

/* Doxygen コメントは、ヘッダーに記載 */

bool string_catalog_filter_source_end_read(const void *source, const uint64_t timestamp)
{
    const string_catalog_filter_source_header *header = (const string_catalog_filter_source_header *)source;

    /* 内容の読み取りが、公開時刻の読み直しより後へ並べ替わらないようにする */
    cplat_atomic_thread_fence(CPLAT_MEMORY_ORDER_ACQUIRE);
    return cplat_atomic_load_u64(&header->published_timestamp, CPLAT_MEMORY_ORDER_RELAXED) == timestamp;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_string_catalog_filter_source_publish(void *source, const size_t source_size, const void *image,
                                               const size_t image_size,
                                               const cplat_string_catalog_filter_source_lock *lock,
                                               uint64_t *timestamp_out)
{
    string_catalog_filter_source_header *header = (string_catalog_filter_source_header *)source;
    cplat_string_catalog_filter_info info;
    cplat_timespec realtime;
    uint64_t base;
    uint64_t next;
    int ret;

    if ((source == NULL) || (image == NULL) || !string_catalog_filter_source_is_region_valid(source, source_size, 0U) ||
        ((lock != NULL) && ((lock->lock == NULL) || (lock->unlock == NULL))))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    ret = cplat_string_catalog_filter_validate(image, image_size);
    if (ret != CPLAT_OK)
    {
        return ret;
    }
    (void)cplat_string_catalog_filter_get_info(image, image_size, &info);
    if (!string_catalog_filter_source_is_region_valid(source, source_size, (size_t)info.image_size))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    /* ヘッダーの確認と書き込みは排他の下で行う。ほかの書き込み側と競合せずにヘッダーを読めるようにするため */
    if (lock != NULL)
    {
        ret = lock->lock(lock->context);
        if (ret != CPLAT_OK)
        {
            return ret;
        }
    }
    if (!is_known_header(header))
    {
        if (lock != NULL)
        {
            lock->unlock(lock->context);
        }
        return CPLAT_ERR_CORRUPT_DESCRIPTOR;
    }

    /* 書き込みの途中で中断した領域では公開時刻が奇数のまま残るため、偶数へ戻して基準にする */
    base = cplat_atomic_load_u64(&header->published_timestamp, CPLAT_MEMORY_ORDER_RELAXED) & ~TIMESTAMP_WRITING_BIT;

    /* 公開時刻を奇数にしてから書き込む。内容の書き込みが、奇数にするより前へ並べ替わらないようにする */
    cplat_atomic_store_u64(&header->published_timestamp, base | TIMESTAMP_WRITING_BIT, CPLAT_MEMORY_ORDER_RELAXED);
    cplat_atomic_thread_fence(CPLAT_MEMORY_ORDER_RELEASE);

    cplat_clock_get_realtime(&realtime);
    header->signature = STRING_CATALOG_FILTER_SOURCE_SIGNATURE;
    header->format_version = (uint16_t)STRING_CATALOG_FILTER_SOURCE_FORMAT_VERSION;
    header->header_size = (uint16_t)CPLAT_STRING_CATALOG_FILTER_SOURCE_HEADER_SIZE;
    header->line_capacity = info.line_capacity;
    header->line_width = info.line_width;
    header->image_size = info.image_size;
    header->published_realtime_seconds = (int64_t)realtime.tv_sec;
    header->published_realtime_nanoseconds = realtime.tv_nsec;
    header->publisher_process_id = cplat_process_get_pid();
    memset(header->reserved, 0, sizeof(header->reserved));
    memcpy((unsigned char *)source + CPLAT_STRING_CATALOG_FILTER_SOURCE_HEADER_SIZE, image, (size_t)info.image_size);

    /* Windows の単調増加クロックはミリ秒単位のため、同じ値が続き得る。前回と異なる偶数にする */
    next = next_timestamp(base, monotonic_nanoseconds() & ~TIMESTAMP_WRITING_BIT);
    cplat_atomic_store_u64(&header->published_timestamp, next, CPLAT_MEMORY_ORDER_RELEASE);
    if (lock != NULL)
    {
        lock->unlock(lock->context);
    }

    if (timestamp_out != NULL)
    {
        *timestamp_out = next;
    }
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_string_catalog_filter_source_get_info(const void *source, const size_t source_size,
                                                cplat_string_catalog_filter_source_info *info_out)
{
    const string_catalog_filter_source_header *header = (const string_catalog_filter_source_header *)source;
    string_catalog_filter_source_header copy;
    uint64_t timestamp;

    if ((info_out == NULL) || !string_catalog_filter_source_is_region_valid(source, source_size, 0U))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    memset(info_out, 0, sizeof(*info_out));

    timestamp = string_catalog_filter_source_begin_read(source);
    if (timestamp == 0U)
    {
        return CPLAT_OK;
    }
    if ((timestamp & TIMESTAMP_WRITING_BIT) != 0U)
    {
        return CPLAT_ERR_BUSY;
    }
    memcpy(&copy, header, sizeof(copy));
    if (!string_catalog_filter_source_end_read(source, timestamp))
    {
        return CPLAT_ERR_BUSY;
    }
    if (!string_catalog_filter_source_is_header_valid(&copy))
    {
        return CPLAT_ERR_CORRUPT_DESCRIPTOR;
    }

    info_out->published_timestamp = timestamp;
    info_out->published_realtime.tv_sec = (time_t)copy.published_realtime_seconds;
    info_out->published_realtime.tv_nsec = copy.published_realtime_nanoseconds;
    info_out->publisher_process_id = copy.publisher_process_id;
    info_out->line_capacity = copy.line_capacity;
    info_out->line_width = copy.line_width;
    return CPLAT_OK;
}
