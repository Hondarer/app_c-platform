#ifndef _IN_TEST_SRC
    #include "hashtable.c"
#endif /* _IN_TEST_SRC */

#include "hashtable.inject.h"

uint64_t *test_hashtable_bucket_head(const com_util_hashtable *ht)
{
    return hashtable_bucket_head(ht);
}

unsigned char *test_hashtable_entry_status(const com_util_hashtable *ht, size_t rec)
{
    return hashtable_entry_status(ht, rec);
}

uint64_t *test_hashtable_entry_next(const com_util_hashtable *ht, size_t rec)
{
    return hashtable_entry_next(ht, rec);
}

char *test_hashtable_entry_key(const com_util_hashtable *ht, size_t rec)
{
    return hashtable_entry_key(ht, rec);
}

void *test_hashtable_key_ref_at(const com_util_hashtable *ht, size_t rec)
{
    return hashtable_key_ref_at(ht, rec);
}

void *test_hashtable_value_ref_at(const com_util_hashtable *ht, size_t rec)
{
    return hashtable_value_ref_at(ht, rec);
}

uint64_t *test_hashtable_entry_generation(const com_util_hashtable *ht, size_t rec)
{
    return hashtable_entry_generation(ht, rec);
}

void test_hashtable_set_next_empty(com_util_hashtable *ht, uint64_t value)
{
    ht->hdr->next_empty = value;
}

void test_hashtable_set_config(com_util_hashtable *ht, const com_util_hashtable_config *config)
{
    ht->hdr->config = *config;
}

void test_hashtable_set_counts(com_util_hashtable *ht, uint64_t in_use_count, uint64_t deleted_count)
{
    ht->hdr->in_use_count = in_use_count;
    ht->hdr->deleted_count = deleted_count;
}

void *test_hashtable_value_free_list(const com_util_hashtable *ht)
{
    return hashtable_value_free_list(ht);
}

uint64_t test_hashtable_value_free_count(const com_util_hashtable *ht)
{
    return ht->hdr->value_free_count;
}

void test_hashtable_set_value_free_count(com_util_hashtable *ht, uint64_t count)
{
    ht->hdr->value_free_count = count;
}
