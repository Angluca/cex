#pragma once
#include "dict.h"
#include "_hashmap.c"
#include <stdarg.h>
#include <time.h>

static inline u64
hm_int_hash_simple(u64 x)
{
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    x = x ^ (x >> 31);
    return x;
}

/**
 * @brief Hashmap: int compare function
 */
int
_cex_dict_u64_cmp(const void* a, const void* b, void* udata)
{
    (void)udata;
    uassert(a != NULL && "must be set");
    uassert(b != NULL && "must be set");

    uassert((usize)a % alignof(u64) == 0 && "a operand not aligned to u64");
    uassert((usize)b % alignof(u64) == 0 && "b operand not aligned to u64");

    return (*(u64*)a) - (*(u64*)b);
}


/**
 * @brief Hashmap: int hash function, for making ticker_id -> hash (uniformly distrib)
 */
u64
_cex_dict_u64_hash(const void* item, u64 seed0, u64 seed1)
{
    (void)seed0;
    (void)seed1;
    return hm_int_hash_simple(*(u64*)item);
}


/**
 * @brief Compares static char[] buffer keys **must be null terminated**
 */
int
_cex_dict_str_cmp(const void* a, const void* b, void* udata)
{
    (void)udata;
    return strcmp(a, b);
}

/**
 * @brief Compares str_c item keys (str.buf must not be NULL)
 */
int
_cex_dict_cexstr_cmp(const void* a, const void* b, void* udata)
{
    (void)udata;
    str_c* self = (str_c*)a;
    str_c* other = (str_c*)b;

    if (unlikely(self->len == 0)) {
        if (other->len == 0) {
            return 0;
        } else {
            return -other->buf[0];
        }
    }

    usize min_len = self->len < other->len ? self->len : other->len;
    int cmp = memcmp(self->buf, other->buf, min_len);

    if (unlikely(cmp == 0 && self->len != other->len)) {
        if (self->len > other->len) {
            cmp = self->buf[min_len] - '\0';
        } else {
            cmp = '\0' - other->buf[min_len];
        }
    }
    return cmp;
}

/**
 * @brief Hash function for char[] buffer keys **must be null terminated**
 */
u64
_cex_dict_str_hash(const void* item, u64 seed0, u64 seed1)
{
    return hashmap_sip(item, strlen((char*)item), seed0, seed1);
}

/**
 * @brief Hash function for str_c keys (str.buf must be not NULL)
 *
 * @return 
 */
u64
_cex_dict_cexstr_hash(const void* item, u64 seed0, u64 seed1)
{
    str_c* s = (str_c*)item;
    return hashmap_sip(s->buf, s->len, seed0, seed1);
}


Exception
_cex_dict_create(
    _cex_dict_c* self,
    usize item_size,
    const Allocator_i* allocator,
    dict_new_kwargs_s* kwargs
)
{
    if (item_size < sizeof(u64)) {
        uassert(item_size >= sizeof(u64) && "item_size is too small");
        return Error.argument;
    }
    if (kwargs->cmp_func == NULL) {
        uassert(
            kwargs->cmp_func != NULL &&
            "cmp_func is not set or dict$define key type is not supported"
        );
        return Error.argument;
    }
    if (kwargs->hash_func == NULL) {
        uassert(
            kwargs->hash_func != NULL &&
            "hash_func is not set or dict$define key type is not supported"
        );
        return Error.argument;
    }

    self->hashmap = hashmap_new_with_allocator(
        allocator->malloc,
        allocator->realloc,
        allocator->free,
        item_size,
        kwargs->capacity,
        kwargs->seed0,
        kwargs->seed1,
        kwargs->hash_func,
        kwargs->cmp_func,
        kwargs->elfree_func,
        kwargs->cmp_func_udata
    );
    if (self->hashmap == NULL) {
        return Error.memory;
    }

    self->allc = allocator;

    return Error.ok;
}

Exception
_cex_dict_set(_cex_dict_c* self, const void* item)
{
    uassert(self != NULL);
    uassert(self->hashmap != NULL);

    const void* set_result = hashmap_set(self->hashmap, item);
    if (set_result == NULL && hashmap_oom(self->hashmap)) {
        return Error.memory;
    }

    return EOK;
}

/**
 * @brief Get key by i64 value, returns NULL if not found
 */
void*
_cex_dict_geti(_cex_dict_c* self, u64 key)
{
    uassert(self != NULL);
    uassert(self->hashmap != NULL);
    return (void*)hashmap_get(self->hashmap, &key);
}

void*
_cex_dict_gets(_cex_dict_c* self, str_c key)
{
    uassert(self != NULL);
    uassert(self->hashmap != NULL);
    uassert(key.buf != NULL);
    return (void*)hashmap_get(self->hashmap, &key);
}
/**
 * @brief Get key by pointer, returns NULL if not found
 */
void*
_cex_dict_get(_cex_dict_c* self, const void* key)
{
    uassert(self != NULL);
    uassert(self->hashmap != NULL);
    return (void*)hashmap_get(self->hashmap, key);
}

usize
_cex_dict_len(_cex_dict_c* self)
{
    uassert(self != NULL);
    uassert(self->hashmap != NULL);
    return hashmap_count(self->hashmap);
}

void
_cex_dict_destroy(_cex_dict_c* self)
{
    if (self != NULL) {
        if (self->hashmap != NULL) {
            hashmap_free(self->hashmap);
            self->hashmap = NULL;
        }
        memset(self, 0, sizeof(*self));
    }
}

void
_cex_dict_clear(_cex_dict_c* self)
{
    uassert(self != NULL);
    uassert(self->hashmap != NULL);
    // clear all elements, but keeps old capacity unchanged
    hashmap_clear(self->hashmap, false);
}

/**
 * @brief Delete key by u64 value, returns NULL if not found
 */
void*
_cex_dict_deli(_cex_dict_c* self, u64 key)
{
    uassert(self != NULL);
    uassert(self->hashmap != NULL);
    return (void*)hashmap_delete(self->hashmap, &key);
}

void*
_cex_dict_dels(_cex_dict_c* self, str_c key)
{
    uassert(self != NULL);
    uassert(self->hashmap != NULL);
    uassert(key.buf != NULL);
    return (void*)hashmap_delete(self->hashmap, &key);
}

/**
 * @brief Delete key by pointer, returns NULL if not found
 */
void*
_cex_dict_del(_cex_dict_c* self, const void* key)
{
    uassert(self != NULL);
    uassert(self->hashmap != NULL);
    return (void*)hashmap_delete(self->hashmap, key);
}


void*
_cex_dict_iter(_cex_dict_c* self, cex_iterator_s* iterator)
{
    uassert(self != NULL);
    uassert(self->hashmap != NULL);
    uassert(iterator != NULL);

    struct hashmap* hm = self->hashmap;

    // temporary struct based on _ctxbuffer
    struct iter_ctx
    {
        usize nbuckets;
        usize count;
        usize cursor;
        usize counter;
    }* ctx = (struct iter_ctx*)iterator->_ctx;
    _Static_assert(sizeof(*ctx) <= sizeof(iterator->_ctx), "ctx size overflow");
    _Static_assert(alignof(struct iter_ctx) == alignof(usize), "ctx alignment mismatch");

    if (unlikely(iterator->val == NULL)) {
        if (hm->count == 0) {
            return NULL;
        }
        *ctx = (struct iter_ctx){
            .count = hm->count,
            .nbuckets = hm->nbuckets,
        };
    } else {
        ctx->counter++;
    }

    if (unlikely(ctx->count != hm->count || ctx->nbuckets != hm->nbuckets)) {
        uassert(ctx->count == hm->count && "hashmap changed during iteration");
        uassert(ctx->nbuckets == hm->nbuckets && "hashmap changed during iteration");
        return NULL;
    }

    if (hashmap_iter(self->hashmap, &ctx->cursor, &iterator->val)) {
        iterator->idx.i = ctx->counter;
        return iterator->val;
    } else {
        return NULL;
    }
}
