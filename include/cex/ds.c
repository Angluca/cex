#include "ds.h"
#include "cex.h"
#include "mem.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>


#ifdef CEXDS_STATISTICS
#define CEXDS_STATS(x) x
size_t cexds_array_grow;
size_t cexds_hash_grow;
size_t cexds_hash_shrink;
size_t cexds_hash_rebuild;
size_t cexds_hash_probes;
size_t cexds_hash_alloc;
size_t cexds_rehash_probes;
size_t cexds_rehash_items;
#else
#define CEXDS_STATS(x)
#endif

//
// cexds_arr implementation
//

void*
cexds_arrgrowf(void* a, size_t elemsize, size_t addlen, size_t min_cap, const Allocator_i* allc)
{
    uassert(addlen < PTRDIFF_MAX && "negative or overflow");
    uassert(min_cap < PTRDIFF_MAX && "negative or overflow");

    if (a == NULL) {
        if (allc == NULL) {
            uassert(allc != NULL && "using uninitialized arr/hm or out-of-mem error");
            // unconditionally abort even in production
            abort();
        }
    } else {
        uassert(allc == NULL && "you must pass NULL to allc, when array/hm initialized");
        uassert(
            (cexds_header(a)->magic_num == CEXDS_ARR_MAGIC ||
             cexds_header(a)->magic_num == CEXDS_HM_MAGIC) &&
            "bad array pointer"
        );
    }
    cexds_array_header temp = { 0 }; // force debugging
    size_t min_len = (a ? cexds_header(a)->length : 0) + addlen;
    (void)sizeof(temp);

    // compute the minimum capacity needed
    if (min_len > min_cap) {
        min_cap = min_len;
    }

    // increase needed capacity to guarantee O(1) amortized
    if (min_cap < 2 * arr$cap(a)) {
        min_cap = 2 * arr$cap(a);
    } else if (min_cap < 16) {
        min_cap = 16;
    }
    uassert(min_cap < PTRDIFF_MAX && "negative or overflow after processing");
    uassert(addlen > 0 || min_cap > 0);

    if (min_cap <= arr$cap(a)) {
        return a;
    }

    void* b;
    if (a == NULL) {
        b = allc->malloc_aligned(
            CEXDS_HDR_PAD,
            mem$aligned_round(elemsize * min_cap + CEXDS_HDR_PAD, CEXDS_HDR_PAD)
        );
    } else {
        b = cexds_header(a)->allocator->realloc_aligned(
            cexds_base(a),
            CEXDS_HDR_PAD,
            mem$aligned_round(elemsize * min_cap + CEXDS_HDR_PAD, CEXDS_HDR_PAD)
        );
    }

    if (b == NULL) {
        // oops memory error
        return NULL;
    }

    b = (char*)b + CEXDS_HDR_PAD;
    if (a == NULL) {
        cexds_header(b)->length = 0;
        cexds_header(b)->hash_table = NULL;
        cexds_header(b)->temp = 0;
        cexds_header(b)->allocator = allc;
        cexds_header(b)->magic_num = CEXDS_ARR_MAGIC;
    } else {
        CEXDS_STATS(++cexds_array_grow);
    }
    cexds_header(b)->capacity = min_cap;

    return b;
}

void
cexds_arrfreef(void* a)
{
    if (a != NULL) {
        uassert(cexds_header(a)->allocator != NULL);
        cexds_array_header* h = cexds_header(a);
        h->allocator->free(cexds_base(a));
    }
}

//
// cexds_hm hash table implementation
//

#define CEXDS_BUCKET_LENGTH 8
#define CEXDS_BUCKET_SHIFT (CEXDS_BUCKET_LENGTH == 8 ? 3 : 2)
#define CEXDS_BUCKET_MASK (CEXDS_BUCKET_LENGTH - 1)
#define CEXDS_CACHE_LINE_SIZE 64

#define CEXDS_ALIGN_FWD(n, a) (((n) + (a) - 1) & ~((a) - 1))

typedef struct
{
    size_t hash[CEXDS_BUCKET_LENGTH];
    ptrdiff_t index[CEXDS_BUCKET_LENGTH];
} cexds_hash_bucket; // in 32-bit, this is one 64-byte cache line; in 64-bit, each array is one
                     // 64-byte cache line
_Static_assert(sizeof(cexds_hash_bucket) == 128, "cacheline aligned");

typedef struct
{
    char* temp_key; // this MUST be the first field of the hash table
    size_t slot_count;
    size_t used_count;
    size_t used_count_threshold;
    size_t used_count_shrink_threshold;
    size_t tombstone_count;
    size_t tombstone_count_threshold;
    size_t seed;
    size_t slot_count_log2;
    cexds_string_arena string;

    // not a separate allocation, just 64-byte aligned storage after this struct
    cexds_hash_bucket* storage;
} cexds_hash_index;

#define CEXDS_INDEX_EMPTY -1
#define CEXDS_INDEX_DELETED -2
#define CEXDS_INDEX_IN_USE(x) ((x) >= 0)

#define CEXDS_HASH_EMPTY 0
#define CEXDS_HASH_DELETED 1

#define CEXDS_SIZE_T_BITS ((sizeof(size_t)) * 8)

static inline size_t
cexds_probe_position(size_t hash, size_t slot_count, size_t slot_log2)
{
    size_t pos;
    (void)(slot_log2);
    pos = hash & (slot_count - 1);
#ifdef CEXDS_INTERNAL_BUCKET_START
    pos &= ~CEXDS_BUCKET_MASK;
#endif
    return pos;
}

static size_t
cexds_log2(size_t slot_count)
{
    size_t n = 0;
    while (slot_count > 1) {
        slot_count >>= 1;
        ++n;
    }
    return n;
}

static cexds_hash_index*
cexds_make_hash_index(
    size_t slot_count,
    cexds_hash_index* old_table,
    const Allocator_i* allc,
    size_t cexds_hash_seed
)
{
    cexds_hash_index* t = allc->realloc(
        NULL,
        (slot_count >> CEXDS_BUCKET_SHIFT) * sizeof(cexds_hash_bucket) + sizeof(cexds_hash_index) +
            CEXDS_CACHE_LINE_SIZE - 1
    );
    t->storage = (cexds_hash_bucket*)CEXDS_ALIGN_FWD((size_t)(t + 1), CEXDS_CACHE_LINE_SIZE);
    t->slot_count = slot_count;
    t->slot_count_log2 = cexds_log2(slot_count);
    t->tombstone_count = 0;
    t->used_count = 0;

    // compute without overflowing
    t->used_count_threshold = slot_count - (slot_count >> 2);
    t->tombstone_count_threshold = (slot_count >> 3) + (slot_count >> 4);
    t->used_count_shrink_threshold = slot_count >> 2;

    if (slot_count <= CEXDS_BUCKET_LENGTH) {
        t->used_count_shrink_threshold = 0;
    }
    // to avoid infinite loop, we need to guarantee that at least one slot is empty and will
    // terminate probes
    uassert(t->used_count_threshold + t->tombstone_count_threshold < t->slot_count);
    CEXDS_STATS(++cexds_hash_alloc);
    if (old_table) {
        t->string = old_table->string;
        // reuse old seed so we can reuse old hashes so below "copy out old data" doesn't do any
        // hashing
        t->seed = old_table->seed;
    } else {
        memset(&t->string, 0, sizeof(t->string));
        t->seed = cexds_hash_seed;
    }

    {
        size_t i, j;
        for (i = 0; i < slot_count >> CEXDS_BUCKET_SHIFT; ++i) {
            cexds_hash_bucket* b = &t->storage[i];
            for (j = 0; j < CEXDS_BUCKET_LENGTH; ++j) {
                b->hash[j] = CEXDS_HASH_EMPTY;
            }
            for (j = 0; j < CEXDS_BUCKET_LENGTH; ++j) {
                b->index[j] = CEXDS_INDEX_EMPTY;
            }
        }
    }

    // copy out the old data, if any
    if (old_table) {
        size_t i, j;
        t->used_count = old_table->used_count;
        for (i = 0; i < old_table->slot_count >> CEXDS_BUCKET_SHIFT; ++i) {
            cexds_hash_bucket* ob = &old_table->storage[i];
            for (j = 0; j < CEXDS_BUCKET_LENGTH; ++j) {
                if (CEXDS_INDEX_IN_USE(ob->index[j])) {
                    size_t hash = ob->hash[j];
                    size_t pos = cexds_probe_position(hash, t->slot_count, t->slot_count_log2);
                    size_t step = CEXDS_BUCKET_LENGTH;
                    CEXDS_STATS(++cexds_rehash_items);
                    for (;;) {
                        size_t limit, z;
                        cexds_hash_bucket* bucket;
                        bucket = &t->storage[pos >> CEXDS_BUCKET_SHIFT];
                        CEXDS_STATS(++cexds_rehash_probes);

                        for (z = pos & CEXDS_BUCKET_MASK; z < CEXDS_BUCKET_LENGTH; ++z) {
                            if (bucket->hash[z] == 0) {
                                bucket->hash[z] = hash;
                                bucket->index[z] = ob->index[j];
                                goto done;
                            }
                        }

                        limit = pos & CEXDS_BUCKET_MASK;
                        for (z = 0; z < limit; ++z) {
                            if (bucket->hash[z] == 0) {
                                bucket->hash[z] = hash;
                                bucket->index[z] = ob->index[j];
                                goto done;
                            }
                        }

                        pos += step; // quadratic probing
                        step += CEXDS_BUCKET_LENGTH;
                        pos &= (t->slot_count - 1);
                    }
                }
            done:;
            }
        }
    }

    return t;
}

#define CEXDS_ROTATE_LEFT(val, n) (((val) << (n)) | ((val) >> (CEXDS_SIZE_T_BITS - (n))))
#define CEXDS_ROTATE_RIGHT(val, n) (((val) >> (n)) | ((val) << (CEXDS_SIZE_T_BITS - (n))))


#ifdef CEXDS_SIPHASH_2_4
#define CEXDS_SIPHASH_C_ROUNDS 2
#define CEXDS_SIPHASH_D_ROUNDS 4
typedef int CEXDS_SIPHASH_2_4_can_only_be_used_in_64_bit_builds[sizeof(size_t) == 8 ? 1 : -1];
#endif

#ifndef CEXDS_SIPHASH_C_ROUNDS
#define CEXDS_SIPHASH_C_ROUNDS 1
#endif
#ifndef CEXDS_SIPHASH_D_ROUNDS
#define CEXDS_SIPHASH_D_ROUNDS 1
#endif

size_t
cexds_hash_string(char* str, size_t str_cap, size_t seed)
{
    (void)str_cap;
    size_t hash = seed;
    while (*str) {
        hash = CEXDS_ROTATE_LEFT(hash, 9) + (unsigned char)*str++;
    }

    // Thomas Wang 64-to-32 bit mix function, hopefully also works in 32 bits
    hash ^= seed;
    hash = (~hash) + (hash << 18);
    hash ^= hash ^ CEXDS_ROTATE_RIGHT(hash, 31);
    hash = hash * 21;
    hash ^= hash ^ CEXDS_ROTATE_RIGHT(hash, 11);
    hash += (hash << 6);
    hash ^= CEXDS_ROTATE_RIGHT(hash, 22);
    return hash + seed;
}

static size_t
cexds_siphash_bytes(void* p, size_t len, size_t seed)
{
    unsigned char* d = (unsigned char*)p;
    size_t i, j;
    size_t v0, v1, v2, v3, data;

    // hash that works on 32- or 64-bit registers without knowing which we have
    // (computes different results on 32-bit and 64-bit platform)
    // derived from siphash, but on 32-bit platforms very different as it uses 4 32-bit state not 4
    // 64-bit
    v0 = ((((size_t)0x736f6d65 << 16) << 16) + 0x70736575) ^ seed;
    v1 = ((((size_t)0x646f7261 << 16) << 16) + 0x6e646f6d) ^ ~seed;
    v2 = ((((size_t)0x6c796765 << 16) << 16) + 0x6e657261) ^ seed;
    v3 = ((((size_t)0x74656462 << 16) << 16) + 0x79746573) ^ ~seed;

#ifdef CEXDS_TEST_SIPHASH_2_4
    // hardcoded with key material in the siphash test vectors
    v0 ^= 0x0706050403020100ull ^ seed;
    v1 ^= 0x0f0e0d0c0b0a0908ull ^ ~seed;
    v2 ^= 0x0706050403020100ull ^ seed;
    v3 ^= 0x0f0e0d0c0b0a0908ull ^ ~seed;
#endif

#define CEXDS_SIPROUND()                                                                           \
    do {                                                                                           \
        v0 += v1;                                                                                  \
        v1 = CEXDS_ROTATE_LEFT(v1, 13);                                                            \
        v1 ^= v0;                                                                                  \
        v0 = CEXDS_ROTATE_LEFT(v0, CEXDS_SIZE_T_BITS / 2);                                         \
        v2 += v3;                                                                                  \
        v3 = CEXDS_ROTATE_LEFT(v3, 16);                                                            \
        v3 ^= v2;                                                                                  \
        v2 += v1;                                                                                  \
        v1 = CEXDS_ROTATE_LEFT(v1, 17);                                                            \
        v1 ^= v2;                                                                                  \
        v2 = CEXDS_ROTATE_LEFT(v2, CEXDS_SIZE_T_BITS / 2);                                         \
        v0 += v3;                                                                                  \
        v3 = CEXDS_ROTATE_LEFT(v3, 21);                                                            \
        v3 ^= v0;                                                                                  \
    } while (0)

    for (i = 0; i + sizeof(size_t) <= len; i += sizeof(size_t), d += sizeof(size_t)) {
        data = d[0] | (d[1] << 8) | (d[2] << 16) | (d[3] << 24);
        data |= (size_t)(d[4] | (d[5] << 8) | (d[6] << 16) | (d[7] << 24))
             << 16 << 16; // discarded if size_t == 4

        v3 ^= data;
        for (j = 0; j < CEXDS_SIPHASH_C_ROUNDS; ++j) {
            CEXDS_SIPROUND();
        }
        v0 ^= data;
    }
    data = len << (CEXDS_SIZE_T_BITS - 8);
    switch (len - i) {
        case 7:
            data |= ((size_t)d[6] << 24) << 24; // fall through
        case 6:
            data |= ((size_t)d[5] << 20) << 20; // fall through
        case 5:
            data |= ((size_t)d[4] << 16) << 16; // fall through
        case 4:
            data |= (d[3] << 24); // fall through
        case 3:
            data |= (d[2] << 16); // fall through
        case 2:
            data |= (d[1] << 8); // fall through
        case 1:
            data |= d[0]; // fall through
        case 0:
            break;
    }
    v3 ^= data;
    for (j = 0; j < CEXDS_SIPHASH_C_ROUNDS; ++j) {
        CEXDS_SIPROUND();
    }
    v0 ^= data;
    v2 ^= 0xff;
    for (j = 0; j < CEXDS_SIPHASH_D_ROUNDS; ++j) {
        CEXDS_SIPROUND();
    }

#ifdef CEXDS_SIPHASH_2_4
    return v0 ^ v1 ^ v2 ^ v3;
#else
    return v1 ^ v2 ^
           v3; // slightly stronger since v0^v3 in above cancels out final round operation? I
               // tweeted at the authors of SipHash about this but they didn't reply
#endif
}

static size_t
cexds_hash_bytes(void* p, size_t len, size_t seed)
{
#ifdef CEXDS_SIPHASH_2_4
    return cexds_siphash_bytes(p, len, seed);
#else
    unsigned char* d = (unsigned char*)p;

    if (len == 4) {
        u32 hash = (u32)d[0] | ((u32)d[1] << 8) | ((u32)d[2] << 16) | ((u32)d[3] << 24);
        // HASH32-BB  Bob Jenkin's presumably-accidental version of Thomas Wang hash with rotates
        // turned into shifts. Note that converting these back to rotates makes it run a lot slower,
        // presumably due to collisions, so I'm not really sure what's going on.
        hash ^= seed;
        hash = (hash ^ 61) ^ (hash >> 16);
        hash = hash + (hash << 3);
        hash = hash ^ (hash >> 4);
        hash = hash * 0x27d4eb2d;
        hash ^= seed;
        hash = hash ^ (hash >> 15);
        return (((size_t)hash << 16 << 16) | hash) ^ seed;
    } else if (len == 8 && sizeof(size_t) == 8) {
        size_t hash = (size_t)d[0] | ((size_t)d[1] << 8) | ((size_t)d[2] << 16) |
                      ((size_t)d[3] << 24);

        hash |= (size_t)((size_t)d[4] | ((size_t)d[5] << 8) | ((size_t)d[6] << 16) |
                         ((size_t)d[7] << 24))
             << 16 << 16;
        hash ^= seed;
        hash = (~hash) + (hash << 21);
        hash ^= CEXDS_ROTATE_RIGHT(hash, 24);
        hash *= 265;
        hash ^= CEXDS_ROTATE_RIGHT(hash, 14);
        hash ^= seed;
        hash *= 21;
        hash ^= CEXDS_ROTATE_RIGHT(hash, 28);
        hash += (hash << 31);
        hash = (~hash) + (hash << 18);
        return hash;
    } else {
        return cexds_siphash_bytes(p, len, seed);
    }
#endif
}

static inline size_t
cexds_hash(enum _CexDsKeyType_e key_type, void* key, size_t key_size, size_t seed) {
    switch(key_type) {
        case _CexDsKeyType__generic: 
            return cexds_hash_bytes(key, key_size, seed);

        case _CexDsKeyType__charptr: 
            // NOTE: we can't know char* length without touching it, 
            // 65536 is a max key length in case of char was not null term
            return cexds_hash_string(key, 65536, seed);

        case _CexDsKeyType__charbuf: 
            return cexds_hash_string(key, key_size, seed);
        
        case _CexDsKeyType__cexstr: {
            str_c s = *(str_c*)key;
            uassert(s.buf != NULL && "NULL str_c not allowed");
            return cexds_hash_string(s.buf, s.len, seed);
        }
    }
    abort();
}

static int
cexds_is_key_equal(
    void* a,
    size_t elemsize,
    void* key,
    size_t keysize,
    size_t keyoffset,
    int mode,
    size_t i
)
{
    if (mode >= CEXDS_HM_STRING) {
        return 0 == strcmp((char*)key, *(char**)((char*)a + elemsize * i + keyoffset));
    } else {
        return 0 == memcmp(key, (char*)a + elemsize * i + keyoffset, keysize);
    }
}

void
cexds_hmfree_func(void* a, size_t elemsize)
{
    if (a == NULL) {
        return;
    }
    cexds_array_header* h = cexds_header(a);
    uassert(h->allocator != NULL);

    if (cexds_hash_table(a) != NULL) {
        if (cexds_hash_table(a)->string.mode == CEXDS_SH_STRDUP) {
            size_t i;
            // skip 0th element, which is default
            for (i = 1; i < h->length; ++i) {
                h->allocator->free(*(char**)((char*)a + elemsize * i));
            }
        }
        cexds_strreset(&cexds_hash_table(a)->string);
    }
    h->allocator->free(h->hash_table);
    h->allocator->free(cexds_base(a));
}

static ptrdiff_t
cexds_hm_find_slot(void* a, size_t elemsize, void* key, size_t keysize, size_t keyoffset, int mode)
{
    uassert(a != NULL);
    void* raw_a = CEXDS_HASH_TO_ARR(a, elemsize);
    cexds_hash_index* table = cexds_hash_table(raw_a);
    size_t hash = cexds_hash(cexds_header(raw_a)->hm_key_type,key, keysize, table->seed);
    size_t step = CEXDS_BUCKET_LENGTH;

    if (hash < 2) {
        hash += 2; // stored hash values are forbidden from being 0, so we can detect empty slots
    }

    size_t pos = cexds_probe_position(hash, table->slot_count, table->slot_count_log2);

    // TODO: check when this could be infinite loop (due to overflow or something)?
    for (;;) {
        CEXDS_STATS(++cexds_hash_probes);
        cexds_hash_bucket* bucket = &table->storage[pos >> CEXDS_BUCKET_SHIFT];

        // start searching from pos to end of bucket, this should help performance on small hash
        // tables that fit in cache
        for (size_t i = pos & CEXDS_BUCKET_MASK; i < CEXDS_BUCKET_LENGTH; ++i) {
            if (bucket->hash[i] == hash) {
                if (cexds_is_key_equal(a, elemsize, key, keysize, keyoffset, mode, bucket->index[i])) {
                    return (pos & ~CEXDS_BUCKET_MASK) + i;
                }
            } else if (bucket->hash[i] == CEXDS_HASH_EMPTY) {
                return -1;
            }
        }

        // search from beginning of bucket to pos
        size_t limit = pos & CEXDS_BUCKET_MASK;
        for (size_t i = 0; i < limit; ++i) {
            if (bucket->hash[i] == hash) {
                if (cexds_is_key_equal(a, elemsize, key, keysize, keyoffset, mode, bucket->index[i])) {
                    return (pos & ~CEXDS_BUCKET_MASK) + i;
                }
            } else if (bucket->hash[i] == CEXDS_HASH_EMPTY) {
                return -1;
            }
        }

        // quadratic probing
        pos += step;
        step += CEXDS_BUCKET_LENGTH;
        pos &= (table->slot_count - 1);
    }
}

void*
cexds_hmget_key_ts(void* a, size_t elemsize, void* key, size_t keysize, ptrdiff_t* temp, int mode)
{
    uassert(a != NULL);
    size_t keyoffset = 0;
    void* raw_a = CEXDS_HASH_TO_ARR(a, elemsize);
    cexds_hash_index* table = (cexds_hash_index*)cexds_header(raw_a)->hash_table;
    if (table == NULL) {
        *temp = -1;
    } else {
        ptrdiff_t slot = cexds_hm_find_slot(a, elemsize, key, keysize, keyoffset, mode);
        if (slot < 0) {
            *temp = CEXDS_INDEX_EMPTY;
        } else {
            cexds_hash_bucket* b = &table->storage[slot >> CEXDS_BUCKET_SHIFT];
            *temp = b->index[slot & CEXDS_BUCKET_MASK];
        }
    }
    return a;
}

void*
cexds_hmget_key(void* a, size_t elemsize, void* key, size_t keysize, int mode)
{
    uassert(a != NULL);
    ptrdiff_t temp;
    void* p = cexds_hmget_key_ts(a, elemsize, key, keysize, &temp, mode);
    cexds_temp(CEXDS_HASH_TO_ARR(p, elemsize)) = temp;
    return p;
}

void*
cexds_hminit(
    size_t elemsize,
    const Allocator_i* allc,
    enum _CexDsKeyType_e key_type,
    struct cexds_hm_new_kwargs_s* kwargs
)
{
    size_t capacity = 0;
    size_t hm_seed = 0xBadB0dee;

    if (kwargs) {
        capacity = kwargs->capacity;
        hm_seed = kwargs->seed ? kwargs->seed : hm_seed;
    }
    void* a = cexds_arrgrowf(NULL, elemsize, 0, capacity, allc);
    if (a == NULL) {
        return NULL; // memory error
    }

    cexds_header(a)->length += 1;
    // element-0 is a default hashmap element!
    memset(a, 0, elemsize);

    uassert(cexds_header(a)->magic_num == CEXDS_ARR_MAGIC);
    cexds_header(a)->magic_num = CEXDS_HM_MAGIC;
    cexds_header(a)->hm_seed = hm_seed;
    cexds_header(a)->hm_key_type = key_type;

    // a points to element-1
    a = CEXDS_ARR_TO_HASH(a, elemsize);
    return a;
}

static char* cexds_strdup(char* str);

void*
cexds_hmput_key(void* a, size_t elemsize, void* key, size_t keysize, int mode)
{
    uassert(a != NULL);

    size_t keyoffset = 0;
    void* raw_a = a;

    // adjust a to point to the default element
    a = CEXDS_HASH_TO_ARR(a, elemsize);

    cexds_hash_index* table = (cexds_hash_index*)cexds_header(a)->hash_table;

    if (table == NULL || table->used_count >= table->used_count_threshold) {

        size_t slot_count = (table == NULL) ? CEXDS_BUCKET_LENGTH : table->slot_count * 2;
        cexds_hash_index* nt = cexds_make_hash_index(
            slot_count,
            table,
            cexds_header(a)->allocator,
            cexds_header(a)->hm_seed
        );

        if (table) {
            cexds_header(a)->allocator->free(table);
        } else {
            nt->string.mode = mode >= CEXDS_HM_STRING ? CEXDS_SH_DEFAULT : 0;
        }
        cexds_header(a)->hash_table = table = nt;
        CEXDS_STATS(++cexds_hash_grow);
    }

    // we iterate hash table explicitly because we want to track if we saw a tombstone
    {
        size_t hash = cexds_hash(cexds_header(a)->hm_key_type, key, keysize, table->seed);
        size_t step = CEXDS_BUCKET_LENGTH;
        size_t pos;
        ptrdiff_t tombstone = -1;
        cexds_hash_bucket* bucket;

        // stored hash values are forbidden from being 0, so we can detect empty slots to early out
        // quickly
        if (hash < 2) {
            hash += 2;
        }

        pos = cexds_probe_position(hash, table->slot_count, table->slot_count_log2);

        for (;;) {
            size_t limit, i;
            CEXDS_STATS(++cexds_hash_probes);
            bucket = &table->storage[pos >> CEXDS_BUCKET_SHIFT];

            // start searching from pos to end of bucket
            for (i = pos & CEXDS_BUCKET_MASK; i < CEXDS_BUCKET_LENGTH; ++i) {
                if (bucket->hash[i] == hash) {
                    if (cexds_is_key_equal(
                            raw_a,
                            elemsize,
                            key,
                            keysize,
                            keyoffset,
                            mode,
                            bucket->index[i]
                        )) {
                        cexds_temp(a) = bucket->index[i];
                        if (mode >= CEXDS_HM_STRING) {
                            cexds_temp_key(a
                            ) = *(char**)((char*)raw_a + elemsize * bucket->index[i] + keyoffset);
                        }
                        return CEXDS_ARR_TO_HASH(a, elemsize);
                    }
                } else if (bucket->hash[i] == 0) {
                    pos = (pos & ~CEXDS_BUCKET_MASK) + i;
                    goto found_empty_slot;
                } else if (tombstone < 0) {
                    if (bucket->index[i] == CEXDS_INDEX_DELETED) {
                        tombstone = (ptrdiff_t)((pos & ~CEXDS_BUCKET_MASK) + i);
                    }
                }
            }

            // search from beginning of bucket to pos
            limit = pos & CEXDS_BUCKET_MASK;
            for (i = 0; i < limit; ++i) {
                if (bucket->hash[i] == hash) {
                    if (cexds_is_key_equal(
                            raw_a,
                            elemsize,
                            key,
                            keysize,
                            keyoffset,
                            mode,
                            bucket->index[i]
                        )) {
                        cexds_temp(a) = bucket->index[i];
                        return CEXDS_ARR_TO_HASH(a, elemsize);
                    }
                } else if (bucket->hash[i] == 0) {
                    pos = (pos & ~CEXDS_BUCKET_MASK) + i;
                    goto found_empty_slot;
                } else if (tombstone < 0) {
                    if (bucket->index[i] == CEXDS_INDEX_DELETED) {
                        tombstone = (ptrdiff_t)((pos & ~CEXDS_BUCKET_MASK) + i);
                    }
                }
            }

            // quadratic probing
            pos += step;
            step += CEXDS_BUCKET_LENGTH;
            pos &= (table->slot_count - 1);
        }
    found_empty_slot:
        if (tombstone >= 0) {
            pos = tombstone;
            --table->tombstone_count;
        }
        ++table->used_count;

        {
            ptrdiff_t i = (ptrdiff_t)cexds_header(a)->length;
            // we want to do cexds_arraddn(1), but we can't use the macros since we don't have
            // something of the right type
            if ((size_t)i + 1 > arr$cap(a)) {
                *(void**)&a = cexds_arrgrowf(a, elemsize, 1, 0, NULL);
            }
            raw_a = CEXDS_ARR_TO_HASH(a, elemsize);

            uassert((size_t)i + 1 <= arr$cap(a));
            cexds_header(a)->length = i + 1;
            bucket = &table->storage[pos >> CEXDS_BUCKET_SHIFT];
            bucket->hash[pos & CEXDS_BUCKET_MASK] = hash;
            bucket->index[pos & CEXDS_BUCKET_MASK] = i - 1;
            cexds_temp(a) = i - 1;

            switch (table->string.mode) {
                case CEXDS_SH_STRDUP:
                    cexds_temp_key(a) = *(char**)((char*)a + elemsize * i) = cexds_strdup((char*)key
                    );
                    break;
                case CEXDS_SH_ARENA:
                    cexds_temp_key(a) = *(char**)((char*)a + elemsize * i
                    ) = cexds_stralloc(&table->string, (char*)key);
                    break;
                case CEXDS_SH_DEFAULT:
                    cexds_temp_key(a) = *(char**)((char*)a + elemsize * i) = (char*)key;
                    break;
                default:
                    memcpy((char*)a + elemsize * i, key, keysize);
                    break;
            }
        }
        return CEXDS_ARR_TO_HASH(a, elemsize);
    }
}

// void*
// cexds_shmode_func(size_t elemsize, int mode)
// {
//    // TODO: allocator support
//
//     void* a = cexds_arrgrowf(0, elemsize, 0, 1, NULL);
//     cexds_hash_index* h;
//     memset(a, 0, elemsize);
//     cexds_header(a)->length = 1;
//     cexds_header(a)->hash_table = h = cexds_make_hash_index(CEXDS_BUCKET_LENGTH, NULL, );
//     h->string.mode = (unsigned char)mode;
//     return CEXDS_ARR_TO_HASH(a, elemsize);
// }

void*
cexds_hmdel_key(void* a, size_t elemsize, void* key, size_t keysize, size_t keyoffset, int mode)
{
    uassert(a != NULL);
    void* raw_a = CEXDS_HASH_TO_ARR(a, elemsize);
    cexds_hash_index* table = (cexds_hash_index*)cexds_header(raw_a)->hash_table;
    uassert(cexds_header(raw_a)->allocator != NULL);
    cexds_temp(raw_a) = 0;
    if (table == 0) {
        return a;
    }

    ptrdiff_t slot = cexds_hm_find_slot(a, elemsize, key, keysize, keyoffset, mode);
    if (slot < 0) {
        return a;
    }

    cexds_hash_bucket* b = &table->storage[slot >> CEXDS_BUCKET_SHIFT];
    int i = slot & CEXDS_BUCKET_MASK;
    ptrdiff_t old_index = b->index[i];
    // minus one for the raw_a vs a, and minus one for 'last'
    ptrdiff_t final_index = (ptrdiff_t)cexds_header(raw_a)->length - 1 - 1;
    uassert(slot < (ptrdiff_t)table->slot_count);
    uassert(table->used_count > 0);
    --table->used_count;
    ++table->tombstone_count;
    cexds_temp(raw_a) = 1;
    // uassert(table->tombstone_count < table->slot_count/4);
    b->hash[i] = CEXDS_HASH_DELETED;
    b->index[i] = CEXDS_INDEX_DELETED;

    if (mode == CEXDS_HM_STRING && table->string.mode == CEXDS_SH_STRDUP) {
        // FIX: this may conflict with static alloc
        cexds_header(raw_a)->allocator->free(*(char**)((char*)a + elemsize * old_index));
    }

    // if indices are the same, memcpy is a no-op, but back-pointer-fixup will fail, so
    // skip
    if (old_index != final_index) {
        // swap delete
        memmove((char*)a + elemsize * old_index, (char*)a + elemsize * final_index, elemsize);

        // now find the slot for the last element
        if (mode == CEXDS_HM_STRING) {
            slot = cexds_hm_find_slot(
                a,
                elemsize,
                *(char**)((char*)a + elemsize * old_index + keyoffset),
                keysize,
                keyoffset,
                mode
            );
        } else {
            slot = cexds_hm_find_slot(
                a,
                elemsize,
                (char*)a + elemsize * old_index + keyoffset,
                keysize,
                keyoffset,
                mode
            );
        }
        uassert(slot >= 0);
        b = &table->storage[slot >> CEXDS_BUCKET_SHIFT];
        i = slot & CEXDS_BUCKET_MASK;
        uassert(b->index[i] == final_index);
        b->index[i] = old_index;
    }
    cexds_header(raw_a)->length -= 1;

    if (table->used_count < table->used_count_shrink_threshold &&
        table->slot_count > CEXDS_BUCKET_LENGTH) {
        cexds_header(raw_a)->hash_table = cexds_make_hash_index(
            table->slot_count >> 1,
            table,
            cexds_header(raw_a)->allocator,
            cexds_header(raw_a)->hm_seed
        );
        cexds_header(raw_a)->allocator->free(table);
        CEXDS_STATS(++cexds_hash_shrink);
    } else if (table->tombstone_count > table->tombstone_count_threshold) {
        cexds_header(raw_a)->hash_table = cexds_make_hash_index(
            table->slot_count,
            table,
            cexds_header(raw_a)->allocator,
            cexds_header(raw_a)->hm_seed
        );
        cexds_header(raw_a)->allocator->free(table);
        CEXDS_STATS(++cexds_hash_rebuild);
    }

    return a;
}

static char*
cexds_strdup(char* str)
{
    // to keep replaceable allocator simple, we don't want to use strdup.
    // rolling our own also avoids problem of strdup vs _strdup
    size_t len = strlen(str) + 1;
    // char* p = (char*)CEXDS_REALLOC(NULL, 0, len);
    // TODO
    char* p = (char*)realloc(NULL, len);
    memmove(p, str, len);
    return p;
}

#ifndef CEXDS_STRING_ARENA_BLOCKSIZE_MIN
#define CEXDS_STRING_ARENA_BLOCKSIZE_MIN 512u
#endif
#ifndef CEXDS_STRING_ARENA_BLOCKSIZE_MAX
#define CEXDS_STRING_ARENA_BLOCKSIZE_MAX (1u << 20)
#endif

char*
cexds_stralloc(cexds_string_arena* a, char* str)
{
    char* p;
    size_t len = strlen(str) + 1;
    if (len > a->remaining) {
        // compute the next blocksize
        size_t blocksize = a->block;

        // size is 512, 512, 1024, 1024, 2048, 2048, 4096, 4096, etc., so that
        // there are log(SIZE) allocations to free when we destroy the table
        blocksize = (size_t)(CEXDS_STRING_ARENA_BLOCKSIZE_MIN) << (blocksize >> 1);

        // if size is under 1M, advance to next blocktype
        if (blocksize < (size_t)(CEXDS_STRING_ARENA_BLOCKSIZE_MAX)) {
            ++a->block;
        }

        if (len > blocksize) {
            // if string is larger than blocksize, then just allocate the full size.
            // note that we still advance string_block so block size will continue
            // increasing, so e.g. if somebody only calls this with 1000-long strings,
            // eventually the arena will start doubling and handling those as well
            cexds_string_block* sb = realloc(NULL, sizeof(*sb) - 8 + len);
            memmove(sb->storage, str, len);
            if (a->storage) {
                // insert it after the first element, so that we don't waste the space there
                sb->next = a->storage->next;
                a->storage->next = sb;
            } else {
                sb->next = 0;
                a->storage = sb;
                a->remaining = 0; // this is redundant, but good for clarity
            }
            return sb->storage;
        } else {
            cexds_string_block* sb = realloc(NULL, sizeof(*sb) - 8 + blocksize);
            sb->next = a->storage;
            a->storage = sb;
            a->remaining = blocksize;
        }
    }

    uassert(len <= a->remaining);
    p = a->storage->storage + a->remaining - len;
    a->remaining -= len;
    memmove(p, str, len);
    return p;
}

void
cexds_strreset(cexds_string_arena* a)
{
    cexds_string_block *x, *y;
    x = a->storage;
    while (x) {
        y = x->next;
        free(x);
        x = y;
    }
    memset(a, 0, sizeof(*a));
}
