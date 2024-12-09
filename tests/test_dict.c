#include <cex/all.c>
#include <cex/dict.c>
#include <cex/fff.h>
#include <cex/test.h>
#include <stdalign.h>
#include <stdio.h>

DEFINE_FFF_GLOBALS

FAKE_VALUE_FUNC(void*, my_alloc, usize)

const Allocator_i* allocator;

/*
 * SUITE INIT / SHUTDOWN
 */
test$teardown()
{
    allocator = allocators.heap.destroy();
    return EOK;
}

test$setup()
{
    uassert_enable();
    allocator = allocators.heap.create();
    return EOK;
}

/*
 *
 *   TEST SUITE
 *
 */
test$case(test_dict_generic_auto_cmp_hash)
{

    struct s
    {
        char key[30];
        u64 key_u64;
        char* key_ptr;
        char val;
        str_c cexstr;
    } rec;
    (void)rec;

    tassert(_dict$hashfunc(typeof(rec), key_u64) == dict.hashfunc.u64_hash);
    tassert(_dict$hashfunc(typeof(rec), key) == dict.hashfunc.str_hash);
    tassert(_dict$cmpfunc(typeof(rec), key_u64) == dict.hashfunc.u64_cmp);
    tassert(_dict$cmpfunc(typeof(rec), key) == dict.hashfunc.str_cmp);

    // NOTE: these are intentionally unsupported (because we store copy of data, and passing
    // but passing pointers may leave them dangling, or use-after-free)
    // tassert(_dict$hashfunc(typeof(rec), cexstr) == NULL);
    // tassert(_dict$hashfunc(typeof(rec), key_ptr) == NULL);
    return EOK;
}

test$case(test_dict_int64)
{
    struct s
    {
        u64 key;
        char val;
    } rec = { 0 };
    (void)rec;

    dict$define(struct s) hm;
    _Static_assert(sizeof(hm) == sizeof(dict_c), "custom size mismatch");

    tassert_eqs(EOK, dict$new(&hm, allocator, .capacity = 128));

    // dict$set(&hm, &rec);

    tassert_eqs(dict$set(&hm, (&(struct s){ .key = 123, .val = 'z' })), EOK);

    e$except_silent(err, dict.set(&hm.base, &(struct s){ .key = 123, .val = 'z' }))
    {
        tassert(false && "unexpected");
    }

    e$except_silent(err, dict.set(&hm.base, &(struct s){ .key = 133, .val = 'z' }))
    {
        tassert(false && "unexpected");
    }


    u64 key = 9890080;

    key = 123;
    const struct s* res = dict.get(&hm.base, &key);
    tassert(res != NULL);
    tassert(res->key == 123);

    res = dict.geti(&hm.base, 133);
    tassert(res != NULL);
    tassert(res->key == 133);

    const struct s* res2 = dict.get(&hm.base, &(struct s){ .key = 222 });
    tassert(res2 == NULL);

    var res3 = (struct s*)dict.get(&hm.base, &(struct s){ .key = 133 });
    tassert(res3 != NULL);
    tassert_eqi(res3->key, 133);

    var res4 = dict$get(&hm, 123);
    tassert(res4 != NULL);
    tassert_eqi(res4->key, 123);

    tassert_eqi(dict.len(&hm.base), 2);

    tassert(dict.deli(&hm.base, 133) != NULL);
    tassert(dict.geti(&hm.base, 133) == NULL);

    tassert(dict.del(&hm.base, &(struct s){ .key = 123 }) != NULL);
    tassert(dict.geti(&hm.base, 123) == NULL);

    tassert(dict.deli(&hm.base, 12029381038) == NULL);

    tassert_eqi(dict.len(&hm.base), 0);

    dict.destroy(&hm.base);

    return EOK;
}

test$case(test_dict_string)
{
    struct s
    {
        char key[30];
        char val;
    };

    dict$define(struct s) hm;
    tassert_eqs(EOK, dict$new(&hm, allocator));

    tassert_eqs(dict$set(&hm, &(struct s){ .key = "abcd", .val = 'z' }), EOK);

    e$except_silent(err, dict.set(&hm.base, &(struct s){ .key = "abcd", .val = 'z' }))
    {
        tassert(false && "unexpected");
    }

    e$except_silent(err, dict.set(&hm.base, &(struct s){ .key = "xyz", .val = 'z' }))
    {
        tassert(false && "unexpected");
    }

    const struct s* res = dict.get(&hm.base, "abcd");
    tassert(res != NULL);
    tassert_eqs(res->key, "abcd");

    res = dict.get(&hm.base, "xyz");
    tassert(res != NULL);
    tassert_eqs(res->key, "xyz");

    const struct s* res2 = dict.get(&hm.base, &(struct s){ .key = "ffff" });
    tassert(res2 == NULL);


    var res3 = (struct s*)dict.get(&hm.base, &(struct s){ .key = "abcd" });
    tassert(res3 != NULL);
    tassert_eqs(res3->key, "abcd");

    tassert_eqi(dict.len(&hm.base), 2);

    var res4 = dict$get(&hm, "xyz");
    // var res4 = dict$get(&hm, 2); // GOOD type check works, compile error
    tassert(res4 != NULL);
    tassert_eqs(res4->key, "xyz");
    res4->val = 'y';

    tassert(dict.del(&hm.base, "xyznotexisting") == NULL);

    tassert(dict.del(&hm.base, &(struct s){ .key = "abcd" }) != NULL);
    tassert(dict.get(&hm.base, "abcd") == NULL);
    tassert(dict.del(&hm.base, "xyz") != NULL);
    tassert(dict.get(&hm.base, "xyz") == NULL);
    tassert_eqi(dict.len(&hm.base), 0);

    dict.destroy(&hm.base);

    return EOK;
}

test$case(test_dict_create_generic)
{
    struct s
    {
        char key[30];
        u64 another_key;
        char val;
    };

    dict$define(struct s) hm;
    // WARNING: default dict_c forces keys to be at the beginning of the struct,
    //  if such key passed -> Error.integrity
    // tassert_eqs(Error.integrity, dict$new(&hm, typeof(rec), another_key, allocator));

    tassert_eqs(EOK, dict$new(&hm, allocator));

    tassert_eqs(dict.set(&hm.base, &(struct s){ .key = "abcd", .val = 'a' }), EOK);
    tassert_eqs(dict.set(&hm.base, &(struct s){ .key = "xyz", .val = 'z' }), EOK);

    tassert_eqi(dict.len(&hm.base), 2);

    const struct s* result = dict.get(&hm.base, "xyz");
    tassert(result != NULL);
    tassert_eqs(result->key, "xyz");
    tassert_eqi(result->val, 'z');

    struct s* res = (struct s*)result;

    // NOTE: it's possible to edit data in dict, as long as you don't touch key!
    res->val = 'f';

    result = dict.get(&hm.base, "xyz");
    tassert(result != NULL);
    tassert_eqs(result->key, "xyz");
    tassert_eqi(result->val, 'f');

    // WARNING: If you try to edit the key it will be lost
    strcpy(res->key, "foo");

    result = dict.get(&hm.base, "foo");
    tassert(result == NULL);
    // WARNING: old key is also lost!
    result = dict.get(&hm.base, "xyz");
    tassert(result == NULL);

    tassert_eqi(dict.len(&hm.base), 2);

    dict.destroy(&hm.base);
    tassert(hm.base.hashmap == NULL);
    return EOK;
}


test$case(test_dict_iter)
{
    struct s
    {
        char key[30];
        u64 another_key;
        char val;
    } rec;

    dict$define(struct s) hm;
    tassert_eqs(EOK, dict$new(&hm, allocator));

    tassert_eqs(dict.set(&hm.base, &(struct s){ .key = "foo", .val = 'a' }), EOK);
    tassert_eqs(dict.set(&hm.base, &(struct s){ .key = "abcd", .val = 'b' }), EOK);
    tassert_eqs(dict.set(&hm.base, &(struct s){ .key = "xyz", .val = 'c' }), EOK);
    tassert_eqs(dict.set(&hm.base, &(struct s){ .key = "bar", .val = 'd' }), EOK);
    tassert_eqi(dict.len(&hm.base), 4);

    u32 nit = 0;
    for$iter(typeof(rec), it, dict.iter(&hm.base, &it.iterator))
    {
        struct s* r = dict.get(&hm.base, it.val->key);
        tassert(r != NULL);
        tassert_eqi(it.idx.i, nit);
        nit++;
    }
    tassert_eqi(nit, 4);

    struct s2
    {
        char key[30];
        u64 another_key;
        char val;
    };

    nit = 0;
    // for$iter(struct s2, it, dict$iter(&hm, &it.iterator)) // GOOD: type check works
    for$iter(struct s, it, dict$iter(&hm, &it.iterator))
    {
        var r = dict$get(&hm, it.val->key);
        tassert(r != NULL);
        tassert_eqi(it.idx.i, nit);
        tassert(it.val->val >= 'a' && it.val->val <= 'd');
        nit++;
    }
    tassert_eqi(nit, 4);

    nit = 0;
    uassert_disable();
    for$iter(typeof(rec), it, dict.iter(&hm.base, &it.iterator))
    {
        // WARNING: changing of dict during iterator is not allowed, you'll get assert
        dict.clear(&hm.base);
        nit++;
    }
    tassert_eqi(nit, 1);

    dict.destroy(&hm.base);
    return EOK;
}

test$case(test_dict_tolist)
{
    struct s
    {
        char key[30];
        u64 another_key;
        char val;
    };

    dict$define(struct s) hm;
    tassert_eqs(EOK, dict$new(&hm, allocator));

    tassert_eqs(dict.set(&hm.base, &(struct s){ .key = "foo", .val = 'a' }), EOK);
    tassert_eqs(dict.set(&hm.base, &(struct s){ .key = "abcd", .val = 'b' }), EOK);
    tassert_eqs(dict.set(&hm.base, &(struct s){ .key = "xyz", .val = 'c' }), EOK);
    tassert_eqs(dict.set(&hm.base, &(struct s){ .key = "bar", .val = 'd' }), EOK);
    tassert_eqi(dict.len(&hm.base), 4);

    list$define(struct s) a;
    tassert_eqs(EOK, dict.tolist(&hm.base, &a, allocator));
    tassert_eqi(list.len(&a.base), 4);

    tassert_eqs(Error.argument, dict.tolist(NULL, &a, allocator));
    tassert_eqs(Error.argument, dict.tolist(&hm.base, NULL, allocator));
    tassert_eqs(Error.argument, dict.tolist(&hm.base, &a, NULL));

    for$array(it, a.arr, a.len)
    {
        tassert(dict.get(&hm.base, it.val) != NULL);
        tassert(dict.get(&hm.base, it.val->key) != NULL);
        // same elements buf different pointer - means copy!
        tassert(dict.get(&hm.base, it.val->key) != it.val);
    }

    dict.destroy(&hm.base);
    tassert_eqs(Error.integrity, dict.tolist(&hm.base, &a, allocator));

    list.destroy(&a.base);

    return EOK;
}

test$case(test_dict_int64_get_multitype_macro)
{
    struct s
    {
        u64 key;
        char val;
    } rec = { 0 };
    (void)rec;

    dict$define(struct s) hm;
    _Static_assert(sizeof(hm) == sizeof(dict_c), "custom size mismatch");

    tassert_eqs(EOK, dict$new(&hm, allocator, .capacity = 128));


    tassert_eqs(dict$set(&hm, (&(struct s){ .key = 123, .val = 'z' })), EOK);

    rec.key = 999;
    rec.val = 'a';
    tassert_eqs(dict$set(&hm, &rec), EOK);

    _Static_assert(_Generic((&rec), struct s *: 1, default: 0), "f");

    var item = dict$get(&hm, &rec);
    tassert(item != NULL);
    tassert_eqi(item->key, 999);
    tassert_eqi(item->val, 'a');

    item = dict$get(&hm, 123);
    tassert(item != NULL);
    tassert_eqi(item->val, 'z');

    item = dict$get(&hm, 123UL);
    tassert(item != NULL);
    tassert_eqi(item->val, 'z');
    tassert_eqi(item->key, 123);

    item = dict$get(&hm, 123L);
    tassert(item != NULL);
    tassert_eqi(item->val, 'z');
    tassert_eqi(item->key, 123);

    // item = dict$get(&hm, 123LL); // HM... incompatible

    i32 key = -123;
    item = dict$get(&hm, key * -1);
    tassert(item != NULL);
    tassert_eqi(item->val, 'z');
    tassert_eqi(item->key, 123);

    i16 key16 = -123;
    item = dict$get(&hm, key16 * -1);
    tassert(item != NULL);
    tassert_eqi(item->val, 'z');
    tassert_eqi(item->key, 123);

    i8 key8 = -123;
    item = dict$get(&hm, key8 * -1);
    tassert(item != NULL);
    tassert_eqi(item->val, 'z');
    tassert_eqi(item->key, 123);

    u32 keyu32 = 123;
    item = dict$get(&hm, keyu32);
    tassert(item != NULL);
    tassert_eqi(item->val, 'z');
    tassert_eqi(item->key, 123);

    u16 keyu16 = 123;
    item = dict$get(&hm, keyu16);
    tassert(item != NULL);
    tassert_eqi(item->val, 'z');
    tassert_eqi(item->key, 123);

    u8 keyu8 = 123;
    item = dict$get(&hm, keyu8);
    tassert(item != NULL);
    tassert_eqi(item->val, 'z');
    tassert_eqi(item->key, 123);


    dict.destroy(&hm.base);

    return EOK;
}

test$case(test_dict_string_get_multitype_macro)
{
    struct s
    {
        char key[10];
        char val;
    } rec = { .key = "999", .val = 'a' };
    (void)rec;

    dict$define(struct s) hm;
    _Static_assert(sizeof(hm) == sizeof(dict_c), "custom size mismatch");

    tassert_eqs(EOK, dict$new(&hm, allocator, .capacity = 128));


    tassert_eqs(dict$set(&hm, (&(struct s){ .key = "123", .val = 'z' })), EOK);

    tassert_eqs(dict$set(&hm, &rec), EOK);

    // _Static_assert(_Generic((&rec), struct s *: 1, default: 0), "f");
    _Static_assert(_Generic((&rec), typeof(rec)*: 1, typeof( ((typeof(rec)){0}).val )*: 1, default: 0), "f");


    var item = dict$get(&hm, "123");
    tassert(item != NULL);
    tassert_eqi(item->val, 'z');
    tassert_eqs(item->key, "123");

    item = dict$get(&hm, &rec);
    tassert(item != NULL);
    tassert_eqs(item->key, "999");
    tassert_eqi(item->val, 'a');

    char* k1 = rec.key;
    item = dict$get(&hm, k1);
    tassert(item != NULL);
    tassert_eqs(item->key, "999");
    tassert_eqi(item->val, 'a');


    char const* k2 = rec.key;
    item = dict$get(&hm, k2);
    tassert(item != NULL);
    tassert_eqs(item->key, "999");
    tassert_eqi(item->val, 'a');

    char const* const k3 = rec.key;
    item = dict$get(&hm, k3);
    tassert(item != NULL);
    tassert_eqs(item->key, "999");
    tassert_eqi(item->val, 'a');

    char karr[10] = { "999" };
    item = dict$get(&hm, karr);
    tassert(item != NULL);
    tassert_eqs(item->key, "999");
    tassert_eqi(item->val, 'a');

    str_c s = s$(rec.key);
    item = dict$get(&hm, s.buf);
    tassert(item != NULL);
    tassert_eqs(item->key, "999");
    tassert_eqi(item->val, 'a');

    dict.destroy(&hm.base);
    return EOK;
}

test$case(test_custom_type_key)
{
    struct sk
    {
        u64 _k;
    };

    struct s
    {
        struct sk key;
        char val;
    } rec = { .key = { ._k = 999 }, .val = 'a' };
    (void)rec;

    dict$define(struct s) hm;
    _Static_assert(sizeof(hm) == sizeof(dict_c), "custom size mismatch");

    tassert_eqs(
        EOK,
        dict$new(
            &hm,
            allocator,
            .capacity = 128,
            // NOTE: when using non-standard keys we must set hash_func/cmp_func
            .cmp_func = dict.hashfunc.u64_cmp,
            .hash_func = dict.hashfunc.u64_hash
        )
    );

    tassert_eqs(dict$set(&hm, (&(struct s){ .key = { ._k = 123 }, .val = 'z' })), EOK);

    tassert_eqs(dict$set(&hm, &rec), EOK);

    var item = dict$get(&hm, &rec);
    tassert(item != NULL);
    tassert_eqi(item->key._k, 999);
    tassert_eqi(item->val, 'a');

    item = dict$get(&hm, &(struct sk){ ._k = 999 });
    tassert(item != NULL);
    tassert_eqi(item->key._k, 999);
    tassert_eqi(item->val, 'a');

    item = dict$get(&hm, &rec.key);
    tassert(item != NULL);
    tassert_eqi(item->key._k, 999);
    tassert_eqi(item->val, 'a');

    item = dict$get(&hm, item);
    tassert(item != NULL);
    tassert_eqi(item->key._k, 999);
    tassert_eqi(item->val, 'a');

    dict.destroy(&hm.base);
    return EOK;
}
/*
 *
 * MAIN (AUTO GENERATED)
 *
 */
int
main(int argc, char* argv[])
{
    test$args_parse(argc, argv);
    test$print_header();  // >>> all tests below
    
    test$run(test_dict_generic_auto_cmp_hash);
    test$run(test_dict_int64);
    test$run(test_dict_string);
    test$run(test_dict_create_generic);
    test$run(test_dict_iter);
    test$run(test_dict_tolist);
    test$run(test_dict_int64_get_multitype_macro);
    test$run(test_dict_string_get_multitype_macro);
    test$run(test_custom_type_key);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
