#ifndef HASHTABLE_INJECT_H
#define HASHTABLE_INJECT_H

#include <com_util/hashtable/hashtable.h>

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    extern uint64_t *test_hashtable_bucket_head(const com_util_hashtable *ht);
    extern unsigned char *test_hashtable_entry_status(const com_util_hashtable *ht, size_t rec);
    extern uint64_t *test_hashtable_entry_next(const com_util_hashtable *ht, size_t rec);
    extern char *test_hashtable_entry_key(const com_util_hashtable *ht, size_t rec);
    extern void *test_hashtable_key_ref_at(const com_util_hashtable *ht, size_t rec);
    extern void *test_hashtable_value_ref_at(const com_util_hashtable *ht, size_t rec);
    extern uint64_t *test_hashtable_entry_generation(const com_util_hashtable *ht, size_t rec);
    extern void test_hashtable_set_next_empty(com_util_hashtable *ht, uint64_t value);
    extern void test_hashtable_set_config(com_util_hashtable *ht, const com_util_hashtable_config *config);
    extern void test_hashtable_set_counts(com_util_hashtable *ht, uint64_t in_use_count, uint64_t deleted_count);
    extern void *test_hashtable_value_holes(const com_util_hashtable *ht);
    extern uint64_t test_hashtable_value_hole_count(const com_util_hashtable *ht);
    extern void test_hashtable_set_value_hole_count(com_util_hashtable *ht, uint64_t count);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* HASHTABLE_INJECT_H */
