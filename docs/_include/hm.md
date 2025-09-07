
Generic type-safe hashmap

Principles:

1. Data is backed by engine similar to arr$
2. `arr$len()` works with hashmap too
3. Array indexing works with hashmap
4. `for$each`/`for$eachp` is applicable
5. `hm$` generic type is essentially a struct with `key` and `value` fields
6. `hm$` supports following keys: numeric (by default it's just binary representation), char*,
char[N], str_s (CEX sting slice).
7. `hm$` with string keys are stored without copy, use  `hm$new(hm, mem$, .copy_keys = true)` for
copy-mode.
8. `hm$` can store string keys inside an Arena allocator when  `hm$new(hm, mem$, .copy_keys = true,
.copy_keys_arena_pgsize = NNN)`


```c

test$case(test_simple_hashmap)
{
    hm$(int, int) intmap = hm$new(intmap, mem$);

    // Setting items
    hm$set(intmap, 15, 7);
    hm$set(intmap, 11, 3);
    hm$set(intmap, 9, 5);

    // Length
    tassert_eq(hm$len(intmap), 3);
    tassert_eq(arr$len(intmap), 3);

    // Getting items **by value**
    tassert(hm$get(intmap, 9) == 5);
    tassert(hm$get(intmap, 11) == 3);
    tassert(hm$get(intmap, 15) == 7);

    // Getting items **pointer** - NULL on missing
    tassert(hm$getp(intmap, 1) == NULL);

    // Getting with default if not found
    tassert_eq(hm$get(intmap64, -1, 999), 999);

    // Accessing hashmap as array by i-th index
    // NOTE: hashmap elements are ordered until first deletion
    tassert_eq(intmap[0].key, 1);
    tassert_eq(intmap[0].value, 3);

    // removing items
    hm$del(intmap, 100);

    // cleanup
    hm$clear(intmap);

    // basic iteration **by value**
    for$each (it, intmap) {
        io.printf("key=%d, value=%d\n", it.key, it.value);
    }

    // basic iteration **by pointer**
    for$each (it, intmap) {
        io.printf("key=%d, value=%d\n", it->key, it->value);
    }

    hm$free(intmap);
}

```

- Using hashmap as field of other struct
```c

typedef hm$(char* , int) MyHashmap;

struct my_hm_struct {
    MyHashmap hm;
};


test$case(test_hashmap_string_copy_clear_cleanup)
{
    struct my_hm_struct hs = {0};
    // NOTE: .copy_keys - makes sure that key string was copied
    hm$new(hs.hm, mem$, .copy_keys = true);
    hm$set(hs.hm, "foo", 3);
}
```

- Storing string values in the arena
```c

test$case(test_hashmap_string_copy_arena)
{
    hm$(char*, int) smap = hm$new(smap, mem$, .copy_keys = true, .copy_keys_arena_pgsize = 1024);

    char key2[10] = "foo";

    hm$set(smap, key2, 3);
    tassert_eq(hm$len(smap), 1);
    tassert_eq(hm$get(smap, "foo"), 3);
    tassert_eq(hm$get(smap, key2), 3);
    tassert_eq(smap[0].key, "foo");

    memset(key2, 0, sizeof(key2));
    tassert_eq(smap[0].key, "foo");
    tassert_eq(hm$get(smap, "foo"), 3);

    hm$free(smap);
    return EOK;
}

```

- Checking errors + custom struct backing
```c

test$case(test_hashmap_basic)
{
    hm$(int, int) intmap;
    if(hm$new(intmap, mem$) == NULL) {
        // initialization error
    }

    // struct as a value
    struct test64_s
    {
        usize foo;
        usize bar;
    };
    hm$(int, struct test64_s) intmap = hm$new(intmap, mem$);

    // custom struct as hashmap backend
    struct test64_s
    {
        usize fooa;
        usize key; // this field `key` is mandatory
    };

    hm$s(struct test64_s) smap = hm$new(smap, mem$);
    tassert(smap != NULL);

    // Setting hashmap as a whole struct key/value record
    tassert(hm$sets(smap, (struct test64_s){ .key = 1, .fooa = 10 }));
    tassert_eq(hm$len(smap), 1);
    tassert_eq(smap[0].key, 1);
    tassert_eq(smap[0].fooa, 10);

    // Getting full struct by .key value
    struct test64_s* r = hm$gets(smap, 1);
    tassert(r != NULL);
    tassert(r == &smap[0]);
    tassert_eq(r->key, 1);
    tassert_eq(r->fooa, 10);

}

```



```c
/// Defines hashmap generic type
#define hm$(_KeyType, _ValType)

/// Clears hashmap contents
#define hm$clear(t)

/// Deletes items, IMPORTANT hashmap array may be reordered after this call
#define hm$del(t, k)

/// Frees hashmap resources
#define hm$free(t)

/// Get item by value, def - default value (zeroed by default), can be any type
#define hm$get(t, k, def...)

/// Get item by pointer (no copy, direct pointer inside hashmap)
#define hm$getp(t, k)

/// Get a pointer to full hashmap record, NULL if not found
#define hm$gets(t, k)

/// Returns hashmap length, also you can use arr$len()
#define hm$len(t)

/// Creates new hashmap of hm$(KType, VType) using allocator, kwargs: .capacity, .seed,
/// .copy_keys_arena_pgsize, .copy_keys
#define hm$new(t, allocator, kwargs...)

/// Defines hashmap type based on _StructType, must have `key` field
#define hm$s(_StructType)

/// Set hashmap key/value, replaces if exists
#define hm$set(t, k, v...)

/// Add new item and returns pointer of hashmap record for `k`, for further editing
#define hm$setp(t, k)

/// Set full record, must be initialized by user
#define hm$sets(t, v...)




```
