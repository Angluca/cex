#include <cex/all.c>
#include <cex/dict.c>
#include <cex/test/fff.h>
#include <cex/test/test.h>
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
    allocator = AllocatorGeneric.destroy();
    return EOK;
}

test$setup()
{
    uassert_enable();
    allocator = AllocatorGeneric.create();
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

    tassert(_dict$hashfunc(rec.key_u64) == _cex_dict_u64_hash);
    tassert(_dict$hashfunc(rec.key) == _cex_dict_str_hash);
    tassert(_dict$hashfunc(rec.cexstr) == _cex_dict_cexstr_hash);
    tassert(_dict$cmpfunc(rec.key_u64) == _cex_dict_u64_cmp);
    tassert(_dict$cmpfunc(rec.key) == _cex_dict_str_cmp);
    tassert(_dict$cmpfunc(rec.cexstr) == _cex_dict_cexstr_cmp);

    // These are intentionally unsupported at this moment
    tassert(_dict$hashfunc(rec.key_ptr) == NULL);
    tassert(_dict$cmpfunc(rec.key_ptr) == NULL);

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

    dict$typedef(dict_ss, struct s, u64, true);
    dict_ss_c hm;
    _Static_assert(sizeof(hm) == sizeof(_cex_dict_c), "custom size mismatch");

    tassert_eqs(EOK, dict$new(&hm, allocator, .capacity = 128));


    tassert_eqs(dict_ss.set(&hm, (&(struct s){ .key = 123, .val = 'z' })), EOK);

    e$except_silent(err, dict_ss.set(&hm, &(struct s){ .key = 123, .val = 'z' }))
    {
        tassert(false && "unexpected");
    }

    e$except_silent(err, dict_ss.set(&hm, &(struct s){ .key = 133, .val = 'z' }))
    {
        tassert(false && "unexpected");
    }

    u64 key = 9890080;

    key = 123;
    const struct s* res = dict_ss.get(&hm, key);
    tassert(res != NULL);
    tassert(res->key == 123);

    res = dict_ss.get(&hm, 133);
    tassert(res != NULL);
    tassert(res->key == 133);

    const struct s* res2 = dict_ss.get(&hm, 222);
    tassert(res2 == NULL);

    var res3 = dict_ss.get(&hm, 133);
    tassert(res3 != NULL);
    tassert_eqi(res3->key, 133);

    tassert_eqi(dict_ss.len(&hm), 2);

    tassert(dict_ss.del(&hm, 133) != NULL);
    tassert(dict_ss.get(&hm, 133) == NULL);

    tassert(dict_ss.del(&hm, 123) != NULL);
    tassert(dict_ss.get(&hm, 123) == NULL);

    tassert(dict_ss.del(&hm, 12029381038) == NULL);

    tassert_eqi(dict_ss.len(&hm), 0);

    dict_ss.destroy(&hm);

    return EOK;
}

test$case(test_dict_string)
{
    struct s
    {
        char key[30];
        char val;
    };

    dict$typedef(dict_ss, struct s, char*, true);
    dict_ss_c hm;

    tassert_eqs(EOK, dict$new(&hm, allocator));

    tassert_eqs(dict_ss.set(&hm, &(struct s){ .key = "abcd", .val = 'z' }), EOK);

    e$except_silent(err, dict_ss.set(&hm, &(struct s){ .key = "abcd", .val = 'z' }))
    {
        tassert(false && "unexpected");
    }

    e$except_silent(err, dict_ss.set(&hm, &(struct s){ .key = "xyz", .val = 'z' }))
    {
        tassert(false && "unexpected");
    }

    const struct s* res = dict_ss.get(&hm, "abcd");
    tassert(res != NULL);
    tassert_eqs(res->key, "abcd");

    res = dict_ss.get(&hm, "xyz");
    tassert(res != NULL);
    tassert_eqs(res->key, "xyz");

    const struct s* res2 = dict_ss.get(&hm, ((struct s){ .key = "ffff" }.key));
    tassert(res2 == NULL);


    var res3 = dict_ss.get(&hm, (struct s){ .key = "abcd" }.key);
    tassert(res3 != NULL);
    tassert_eqs(res3->key, "abcd");

    tassert_eqi(dict_ss.len(&hm), 2);

    var res4 = dict_ss.get(&hm, "xyz");
    tassert(res4 != NULL);
    tassert_eqs(res4->key, "xyz");
    res4->val = 'y';

    tassert(dict_ss.del(&hm, "xyznotexisting") == NULL);

    tassert(dict_ss.del(&hm, (struct s){ .key = "abcd" }.key) != NULL);
    tassert(dict_ss.get(&hm, "abcd") == NULL);
    tassert(dict_ss.del(&hm, "xyz") != NULL);
    tassert(dict_ss.get(&hm, "xyz") == NULL);
    tassert_eqi(dict_ss.len(&hm), 0);

    dict_ss.destroy(&hm);

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

    dict$typedef(dict_ss, struct s, char*, true);
    dict_ss_c hm;

    tassert_eqs(EOK, dict$new(&hm, allocator));

    tassert_eqs(dict_ss.set(&hm, &(struct s){ .key = "abcd", .val = 'a' }), EOK);
    tassert_eqs(dict_ss.set(&hm, &(struct s){ .key = "xyz", .val = 'z' }), EOK);

    tassert_eqi(dict_ss.len(&hm), 2);

    const struct s* result = dict_ss.get(&hm, "xyz");
    tassert(result != NULL);
    tassert_eqs(result->key, "xyz");
    tassert_eqi(result->val, 'z');

    struct s* res = (struct s*)result;

    // NOTE: it's possible to edit data in dict, as long as you don't touch key!
    res->val = 'f';

    result = dict_ss.get(&hm, "xyz");
    tassert(result != NULL);
    tassert_eqs(result->key, "xyz");
    tassert_eqi(result->val, 'f');

    // WARNING: If you try to edit the key it will be lost
    strcpy(res->key, "foo");

    result = dict_ss.get(&hm, "foo");
    tassert(result == NULL);
    // WARNING: old key is also lost!
    result = dict_ss.get(&hm, "xyz");
    tassert(result == NULL);

    tassert_eqi(dict_ss.len(&hm), 2);

    dict_ss.destroy(&hm);
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

    dict$typedef(dict_custom, struct s, char*, true);
    dict_custom_c hm;

    tassert_eqs(EOK, dict$new(&hm, allocator));

    tassert_eqs(dict_custom.set(&hm, &(struct s){ .key = "foo", .val = 'a' }), EOK);
    tassert_eqs(dict_custom.set(&hm, &(struct s){ .key = "abcd", .val = 'b' }), EOK);
    tassert_eqs(dict_custom.set(&hm, &(struct s){ .key = "xyz", .val = 'c' }), EOK);
    tassert_eqs(dict_custom.set(&hm, &(struct s){ .key = "bar", .val = 'd' }), EOK);
    tassert_eqi(dict_custom.len(&hm), 4);

    u32 nit = 0;
    for$iter(typeof(rec), it, dict_custom.iter(&hm, &it.iterator))
    {
        struct s* r = dict_custom.get(&hm, it.val->key);
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
    for$iter(struct s, it, dict_custom.iter(&hm, &it.iterator))
    {
        var r = dict_custom.get(&hm, it.val->key);
        tassert(r != NULL);
        tassert_eqi(it.idx.i, nit);
        tassert(it.val->val >= 'a' && it.val->val <= 'd');
        nit++;
    }
    tassert_eqi(nit, 4);

    nit = 0;
    uassert_disable();
    for$iter(typeof(rec), it, dict_custom.iter(&hm, &it.iterator))
    {
        // WARNING: changing of dict during iterator is not allowed, you'll get assert
        dict_custom.clear(&hm);
        nit++;
    }
    tassert_eqi(nit, 1);

    dict_custom.destroy(&hm);
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

    dict$typedef(dict_custom, struct s, struct sk*, true);

    // WARNING: always expect pointer of key, i.e. `struct sk*`
    // dict$typedef(dict_custom, struct s, struct sk, true);  // <--- this raises static assert

    dict_custom_c hm;

    tassert_eqs(
        EOK,
        dict$new(
            &hm,
            allocator,
            .capacity = 128,
            // NOTE: when using non-standard keys we must set hash_func/cmp_func
            .cmp_func = _cex_dict_u64_cmp,
            .hash_func = _cex_dict_u64_hash
        )
    );

    tassert_eqs(dict_custom.set(&hm, (&(struct s){ .key = { ._k = 123 }, .val = 'z' })), EOK);
    tassert_eqs(dict_custom.set(&hm, &rec), EOK);
    tassert_eqi(dict_custom.len(&hm), 2);

    struct s* item = dict_custom.get(&hm, &rec.key);
    tassert(item != NULL);
    tassert_eqi(item->key._k, 999);
    tassert_eqi(item->val, 'a');

    tassert(dict_custom.del(&hm, &rec.key) != NULL);
    tassert_eqi(dict_custom.len(&hm), 1);

    dict_custom.destroy(&hm);
    return EOK;
}


test$case(test_dict_typedef_char)
{
    struct s
    {
        char key[10];
        char val;
    } rec = { .key = "999", .val = 'a' };
    (void)rec;

    dict$typedef(dict_ss, struct s, char*, true);

    dict_ss_c hm;
    tassert_eqs(EOK, dict$new(&hm, allocator, .capacity = 128));

    tassert_eqs(dict_ss.set(&hm, &rec), EOK);

    var item = dict_ss.get(&hm, "999");

    tassert(item != NULL);
    tassert_eqs(item->key, "999");
    tassert_eqi(item->val, 'a');

    item = dict_ss.get(&hm, rec.key);

    tassert(item != NULL);
    tassert_eqs(item->key, "999");
    tassert_eqi(item->val, 'a');

    var ditem = dict_ss.del(&hm, "999");
    tassert(ditem != NULL);
    tassert_eqs(ditem->key, "999");
    tassert_eqi(dict_ss.len(&hm), 0);


    dict_ss.destroy(&hm);
    return EOK;
}

test$case(test_dict_typedef_int)
{
    struct s
    {
        u64 key;
        char val;
    } rec = { .key = 999, .val = 'a' };
    (void)rec;

    dict$typedef(dict_ss, struct s, u64, true);

    dict_ss_c hm;
    tassert_eqs(EOK, dict$new(&hm, allocator, .capacity = 128));

    tassert_eqs(dict_ss.set(&hm, &rec), EOK);

    var item = dict_ss.get(&hm, 999);

    tassert(item != NULL);
    tassert_eqi(item->key, 999);
    tassert_eqi(item->val, 'a');

    tassert_eqi(dict_ss.len(&hm), 1);
    var ditem = dict_ss.del(&hm, 888);
    tassert(ditem == NULL);

    ditem = dict_ss.del(&hm, 999);
    tassert(ditem != NULL);
    tassert_eqi(ditem->key, 999);
    tassert_eqi(dict_ss.len(&hm), 0);

    ditem = dict_ss.del(&hm, 999);
    tassert(ditem == NULL);

    tassert_eqs(dict_ss.set(&hm, &rec), EOK);
    tassert_eqi(dict_ss.len(&hm), 1);
    dict_ss.clear(&hm);
    tassert_eqi(dict_ss.len(&hm), 0);

    dict_ss.destroy(&hm);

    _Static_assert(_Generic((dict_ss_c){ 0 }._dtype->key, u64: 1, default: 0), "ddd");

    return EOK;
}

test$case(test_dict_cex_str_key)
{
    struct s
    {
        str_c key;
        char val;
    } rec = { .key = str$("999"), .val = 'a' };
    (void)rec;

    dict$typedef(dict_ss, struct s, str_c, true);

    dict_ss_c hm;
    tassert_eqs(EOK, dict$new(&hm, allocator, .capacity = 128));

    tassert_eqs(dict_ss.set(&hm, &rec), EOK);

    var item = dict_ss.get(&hm, str$("999"));

    tassert(item != NULL);
    tassert_eqs(item->key.buf, "999");
    tassert_eqi(item->val, 'a');

    item = dict_ss.get(&hm, rec.key);

    tassert(item != NULL);
    tassert_eqs(item->key.buf, "999");
    tassert_eqi(item->val, 'a');

    var ditem = dict_ss.del(&hm, str$("999"));
    tassert(ditem != NULL);
    tassert_eqs(ditem->key.buf, "999");
    tassert_eqi(dict_ss.len(&hm), 0);


    dict_ss.destroy(&hm);
    return EOK;
}

test$case(test_dict_cex_str_hash_cmp_func_test)
{
    var s = str$("");
    var s2 = str$("");

    // No segfault, or something
    tassert(_cex_dict_cexstr_hash(&s, 0, 0) > 0);
    tassert(_cex_dict_cexstr_hash(&str$("foofoobar"), 0, 0) > 0);
    tassert(
        _cex_dict_cexstr_hash(&str$("foofoobar"), 0, 0) !=
        _cex_dict_cexstr_hash(&str$("booboob"), 0, 0)
    );

    tassert_eqi(0, _cex_dict_cexstr_cmp(&s, &s2, NULL));

    tassert_eqi(_cex_dict_cexstr_cmp(&str$("123456"), &str$("123456"), NULL), 0);
    tassert_eqi(_cex_dict_cexstr_cmp(&str$(""), &str$(""), NULL), 0);
    tassert_eqi(_cex_dict_cexstr_cmp(&str$("ABC"), &str$("AB"), NULL), 67);
#ifdef CEX_ENV32BIT
    tassert(_cex_dict_cexstr_cmp(&str$("ABA"), &str$("ABZ"), NULL) < 0);
#else
    tassert_eqi(_cex_dict_cexstr_cmp(&str$("ABA"), &str$("ABZ"), NULL), -25);
#endif
    tassert_eqi(_cex_dict_cexstr_cmp(&str$("AB"), &str$("ABC"), NULL), -67);
    tassert_eqi(_cex_dict_cexstr_cmp(&str$("A"), &str$(""), NULL), (int)'A');
    tassert_eqi(_cex_dict_cexstr_cmp(&str$(""), &str$("A"), NULL), -1 * ((int)'A'));
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
    test$run(test_custom_type_key);
    test$run(test_dict_typedef_char);
    test$run(test_dict_typedef_int);
    test$run(test_dict_cex_str_key);
    test$run(test_dict_cex_str_hash_cmp_func_test);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
