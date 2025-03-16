#include "cex/cex.h"
#include <cex/all.c>
#include <cex/list.c>
#include <cex/test/fff.h>
#include <cex/test/test.h>
#include <stdio.h>
#include "_generic_defs.h"

// NOTE: this list is defined in _generic_defs.h
list$impl(list_i32);

// We are allowed to define lists in .c global files 
list$typedef(list_u64, u64, true);

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

test$case(testlist_alloc_capacity)
{
    // 4 is minimum
    tassert_eql(4, _cex_list__alloc_capacity(1));
    tassert_eql(4, _cex_list__alloc_capacity(2));
    tassert_eql(4, _cex_list__alloc_capacity(3));
    tassert_eql(4, _cex_list__alloc_capacity(4));

    // up to 1024 grow with factor of 2
    tassert_eql(8, _cex_list__alloc_capacity(5));
    tassert_eql(16, _cex_list__alloc_capacity(12));
    tassert_eql(64, _cex_list__alloc_capacity(45));
    tassert_eql(1024, _cex_list__alloc_capacity(1023));

    // after 1024 increase by 20%
    tassert_eql(1024 * 1.2, _cex_list__alloc_capacity(1024));
    u64 expected_cap = 1000000 * 1.2;
    u64 alloc_cap = _cex_list__alloc_capacity(1000000);
    tassert(expected_cap - alloc_cap <= 1); // x32 - may have precision rounding

    return EOK;
}


test$case(testlist_new)
{
    list$typedef(list_int, int, true);
    list_int_c a = {0};

    e$except(err, list$new(&a, 5, allocator))
    {
        tassert(false && "list$new fail");
    }

    tassert(a.arr != NULL);
    tassert_eqi(a.len, 0);

    _cex_list_head_s* head = (_cex_list_head_s*)((char*)a.arr - _CEX_LIST_BUF);
    tassert_eqi(head->header.magic, 0x1eed);
    tassert_eqi(head->header.elalign, alignof(int));
    tassert_eqi(head->header.elsize, sizeof(int));
    tassert_eqi(head->count, 0);
    tassert_eqi(head->capacity, 8);
    tassert(head->allocator == allocator);

    list_int.destroy(&a);
    return EOK;
}

test$case(testlist_new_as_typedef)
{
    list_i32_c a = {};

    e$except(err, list$new(&a, 5, allocator))
    {
        tassert(false && "list$new fail");
    }

    tassert(a.arr != NULL);
    tassert_eqi(a.len, 0);

    _cex_list_head_s* head = (_cex_list_head_s*)((char*)a.arr - _CEX_LIST_BUF);
    tassert_eqi(head->header.magic, 0x1eed);
    tassert_eqi(head->header.elalign, alignof(int));
    tassert_eqi(head->header.elsize, sizeof(int));
    tassert_eqi(head->count, 0);
    tassert_eqi(head->capacity, 8);
    tassert(head->allocator == allocator);

    list_i32.destroy(&a);

    return EOK;
}
test$case(test_cex_list_new_as_struct_member)
{
    struct
    {
        list_i32_c a;
    } mystruct = { 0 };


    e$except(err, list$new(&mystruct.a, 5, allocator))
    {
        tassert(false && "list$new fail");
    }

    tassert(mystruct.a.arr != NULL);
    tassert_eqi(mystruct.a.len, 0);

    _cex_list_head_s* head = (_cex_list_head_s*)((char*)mystruct.a.arr - _CEX_LIST_BUF);
    tassert_eqi(head->header.magic, 0x1eed);
    tassert_eqi(head->header.elalign, alignof(int));
    tassert_eqi(head->header.elsize, sizeof(int));
    tassert_eqi(head->count, 0);
    tassert_eqi(head->capacity, 8);
    tassert(head->allocator == allocator);

    list_i32.destroy(&mystruct.a);
    return EOK;
}
test$case(testlist_append)
{
    list_i32_c a = {0};

    e$except(err, list$new(&a, 4, allocator))
    {
        tassert(false && "list$new fail");
    }

    // adding new elements into the end
    for (i32 i = 0; i < 4; i++) {
        tassert_eqs(EOK, list_i32.append(&a, &i));
    }
    tassert_eqi(a.len, 4);
    _cex_list_head_s* head = _cex_list__head(&a.base);
    tassert_eqi(head->count, 4);
    tassert_eqi(head->capacity, 4);
    tassert_eqi(list_i32.capacity(&a), 4);

    // validate appended values
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i], i);
    }

    // check if array got resized
    for (i32 i = 4; i < 8; i++) {
        tassert_eqs(EOK, list_i32.append(&a, &i));
    }

    // validate appended values
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i], i);
    }

    tassert_eqi(a.len, 8);
    head = _cex_list__head(&a.base);
    tassert_eqi(head->count, 8);
    tassert_eqi(head->capacity, 8);

    list_i32.destroy(&a);
    return EOK;
}

test$case(testlist_insert)
{
    list_i32_c a;

    e$except(err, list$new(&a, 4, allocator))
    {
        tassert(false && "list$new fail");
    }

    // adding new elements into the end
    tassert_eqs(EOK, list_i32.append(&a, &(int){ 1 }));
    tassert_eqs(EOK, list_i32.append(&a, &(int){ 2 }));
    tassert_eqs(EOK, list_i32.append(&a, &(int){ 3 }));
    tassert_eqi(a.len, 3);

    tassert_eqs(Error.argument, list_i32.insert(&a, &(int){ 4 }, 4));
    tassert_eqs(Error.ok, list_i32.insert(&a, &(int){ 4 }, 3)); // same as append!
    //
    tassert_eqi(a.len, 4);
    tassert_eqi(a.arr[0], 1);
    tassert_eqi(a.arr[1], 2);
    tassert_eqi(a.arr[2], 3);
    tassert_eqi(a.arr[3], 4);


    list_i32.clear(&a);
    tassert_eqi(a.len, 0);
    tassert_eqs(EOK, list_i32.append(&a, &(int){ 1 }));
    tassert_eqs(EOK, list_i32.append(&a, &(int){ 2 }));
    tassert_eqs(EOK, list_i32.append(&a, &(int){ 3 }));
    tassert_eqs(Error.ok, list_i32.insert(&a, &(int){ 4 }, 0)); // same as append!
    tassert_eqi(a.len, 4);
    tassert_eqi(a.arr[0], 4);
    tassert_eqi(a.arr[1], 1);
    tassert_eqi(a.arr[2], 2);
    tassert_eqi(a.arr[3], 3);

    list_i32.clear(&a);
    tassert_eqi(a.len, 0);
    tassert_eqs(EOK, list_i32.append(&a, &(int){ 1 }));
    tassert_eqs(EOK, list_i32.append(&a, &(int){ 2 }));
    tassert_eqs(EOK, list_i32.append(&a, &(int){ 3 }));
    tassert_eqs(Error.ok, list_i32.insert(&a, &(int){ 4 }, 1)); // same as append!
    tassert_eqi(a.len, 4);
    tassert_eqi(a.arr[0], 1);
    tassert_eqi(a.arr[1], 4);
    tassert_eqi(a.arr[2], 2);
    tassert_eqi(a.arr[3], 3);

    list_i32.destroy(&a);
    return EOK;
}

test$case(testlist_del)
{
    list_i32_c a;

    e$except(err, list$new(&a, 4, allocator))
    {
        tassert(false && "list$new fail");
    }
    tassert_eqs(Error.argument, list_i32.del(&a, 0));

    // adding new elements into the end
    tassert_eqs(EOK, list_i32.append(&a, &(int){ 1 }));
    tassert_eqs(EOK, list_i32.append(&a, &(int){ 2 }));
    tassert_eqs(EOK, list_i32.append(&a, &(int){ 3 }));
    tassert_eqi(a.len, 3);
    tassert_eqs(Error.argument, list_i32.del(&a, 3));
    tassert_eqs(Error.ok, list_i32.del(&a, 2));

    tassert_eqi(a.len, 2);
    tassert_eqi(a.arr[0], 1);
    tassert_eqi(a.arr[1], 2);

    list_i32.clear(&a);
    tassert_eqi(a.len, 0);
    tassert_eqs(EOK, list_i32.append(&a, &(int){ 1 }));
    tassert_eqs(EOK, list_i32.append(&a, &(int){ 2 }));
    tassert_eqs(EOK, list_i32.append(&a, &(int){ 3 }));
    tassert_eqs(EOK, list_i32.append(&a, &(int){ 4 }));

    tassert_eqi(a.arr[0], 1);
    tassert_eqi(a.arr[1], 2);
    tassert_eqi(a.arr[2], 3);
    tassert_eqi(a.arr[3], 4);

    tassert_eqs(Error.ok, list_i32.del(&a, 2));

    tassert_eqi(a.len, 3);
    tassert_eqi(a.arr[0], 1);
    tassert_eqi(a.arr[1], 2);
    tassert_eqi(a.arr[2], 4);

    list_i32.clear(&a);
    tassert_eqi(a.len, 0);
    tassert_eqs(EOK, list_i32.append(&a, &(int){ 1 }));
    tassert_eqs(EOK, list_i32.append(&a, &(int){ 2 }));
    tassert_eqs(EOK, list_i32.append(&a, &(int){ 3 }));
    tassert_eqs(EOK, list_i32.append(&a, &(int){ 4 }));

    tassert_eqi(a.arr[0], 1);
    tassert_eqi(a.arr[1], 2);
    tassert_eqi(a.arr[2], 3);
    tassert_eqi(a.arr[3], 4);

    tassert_eqs(Error.ok, list_i32.del(&a, 0));

    tassert_eqi(a.len, 3);
    tassert_eqi(a.arr[0], 2);
    tassert_eqi(a.arr[1], 3);
    tassert_eqi(a.arr[2], 4);

    list_i32.destroy(&a);
    return EOK;
}

int
test_int_cmp(const i32* a, const i32* b)
{
    return *a - *b;
}

test$case(testlist_sort)
{
    list_i32_c a;
    e$except(err, list$new(&a, 4, allocator))
    {
        tassert(false && "list$new fail");
    }

    // adding new elements into the end
    tassert_eqs(EOK, list_i32.append(&a, &(int){ 5 }));
    tassert_eqs(EOK, list_i32.append(&a, &(int){ 1 }));
    tassert_eqs(EOK, list_i32.append(&a, &(int){ 3 }));
    tassert_eqs(EOK, list_i32.append(&a, &(int){ 2 }));
    tassert_eqs(EOK, list_i32.append(&a, &(int){ 4 }));
    tassert_eqi(a.len, 5);


    list_i32.sort(&a, test_int_cmp);
    tassert_eqi(a.len, 5);

    tassert_eqi(a.arr[0], 1);
    tassert_eqi(a.arr[1], 2);
    tassert_eqi(a.arr[2], 3);
    tassert_eqi(a.arr[3], 4);
    tassert_eqi(a.arr[4], 5);


    list_i32.destroy(&a);
    return EOK;
}

test$case(testlist_extend)
{
    list_i32_c a;
    e$except(err, list$new(&a, 4, allocator))
    {
        tassert(false && "list$new fail");
    }

    int arr[4] = { 0, 1, 2, 3 };
    // a.arr = arr;
    // a.len = 1;

    tassert_eqs(EOK, list_i32.extend(&a, arr, arr$len(arr)));
    tassert_eqi(a.len, 4);
    _cex_list_head_s* head = _cex_list__head(&a.base);
    tassert_eqi(head->count, 4);
    tassert_eqi(head->capacity, 4);

    // validate appended values
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i], i);
    }

    int arr2[4] = { 4, 5, 6, 7 };
    // triggers resize
    tassert_eqs(EOK, list_i32.extend(&a, arr2, arr$len(arr2)));

    // validate appended values
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i], i);
    }

    tassert_eqi(a.len, 8);
    head = _cex_list__head(&a.base);
    tassert_eqi(head->count, 8);
    tassert_eqi(head->capacity, 8);

    list_i32.destroy(&a);
    return EOK;
}

test$case(testlist_iterator)
{

    list_i32_c a;
    e$except(err, list$new(&a, 4, allocator))
    {
        tassert(false && "list$new fail");
    }

    u32 nit = 0;

    int arr[4] = { 0, 1, 2, 300 };
    nit = 0;
    for$array(it, arr, arr$len(arr))
    {
        tassert_eqi(it.idx, nit);
        tassert_eqi(*it.val, arr[nit]);
        nit++;
    }
    tassert_eqi(nit, 4);

    struct tstruct
    {
        u32 foo;
        char* bar;
    } tarr[2] = {
        { .foo = 1 },
        { .foo = 2 },
    };
    nit = 0;
    for$array(it, tarr, arr$len(tarr))
    {
        tassert_eqi(it.idx, nit);
        tassert_eqi(it.val->foo, nit + 1);
        nit++;
    }
    tassert_eqi(nit, 2);

    nit = 0;
    for$array(it, tarr, arr$len(tarr))
    {
        tassert_eqi(it.idx, nit);
        tassert_eqi(it.val->foo, nit + 1);
        nit++;
    }
    tassert_eqi(nit, 2);
    tassert_eqs(EOK, list_i32.extend(&a, arr, arr$len(arr)));
    tassert_eqi(a.len, 4);

    nit = 0;
    for$array(it, a.arr, a.len)
    {
        tassert_eqi(it.idx, nit);
        tassert_eqi(*it.val, arr[nit]);
        nit++;
    }
    tassert_eqi(nit, 4);
    nit = 0;

    nit = 0;
    for$array(it, a.arr, a.len)
    {
        tassert_eqi(it.idx, nit);
        tassert_eqi(*it.val, arr[nit]);
        nit++;
    }
    tassert_eqi(nit, 4);

    list_i32.destroy(&a);

    return EOK;
}

test$case(testlist_align256)
{

    struct foo64
    {
        alignas(256) usize foo;
    };
    _Static_assert(sizeof(struct foo64) == 256, "size");
    _Static_assert(alignof(struct foo64) == 256, "align");

    list$typedef(list_foo64, struct foo64, true);
    list_foo64_c a;

    e$except(err, list$new(&a, 4, allocator))
    {
        tassert(false && "list$new fail");
    }

    // adding new elements into the end
    for (u32 i = 0; i < 4; i++) {
        struct foo64 f = { .foo = i };
        tassert_eqs(EOK, list_foo64.append(&a, &f));
    }
    tassert_eqi(a.len, 4);
    _cex_list_head_s* head = _cex_list__head(&a.base);
    tassert_eqi(head->count, 4);
    tassert_eqi(head->capacity, 4);
    tassert_eqi(head->header.elalign, 256);

    // pointer is aligned!
    tassert_eqi(0, (usize)&a.arr[0] % head->header.elalign);

    // validate appended values
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i].foo, i);
    }
    u32 nit = 0;
    for$array(it, a.arr, a.len)
    {
        tassert_eqi(it.idx, nit);
        tassert_eqi(it.val->foo, a.arr[it.idx].foo);
        nit++;
    }
    tassert_eqi(nit, a.len);

    // check if array got resized
    for (u32 i = 4; i < 8; i++) {
        struct foo64 f = { .foo = i };
        tassert_eqs(EOK, list_foo64.append(&a, &f));
    }

    // validate appended values
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i].foo, i);
    }

    tassert_eqi(a.len, 8);
    head = _cex_list__head(&a.base);
    tassert_eqi(head->count, 8);
    tassert_eqi(head->capacity, 8);

    list_foo64.destroy(&a);
    return EOK;
}

test$case(testlist_align64)
{

    struct foo64
    {
        alignas(64) usize foo;
    };
    _Static_assert(sizeof(struct foo64) == 64, "size");
    _Static_assert(alignof(struct foo64) == 64, "align");

    list$typedef(list_foo64, struct foo64, true);
    list_foo64_c a;

    e$except(err, list$new(&a, 4, allocator))
    {
        tassert(false && "list$new fail");
    }

    // adding new elements into the end
    for (u32 i = 0; i < 4; i++) {
        struct foo64 f = { .foo = i };
        tassert_eqs(EOK, list_foo64.append(&a, &f));
    }
    tassert_eqi(a.len, 4);
    _cex_list_head_s* head = _cex_list__head(&a.base);
    tassert_eqi(head->count, 4);
    tassert_eqi(head->capacity, 4);
    tassert_eqi(head->header.elalign, 64);

    // pointer is aligned!
    tassert_eqi(0, (usize)&a.arr[0] % head->header.elalign);

    // validate appended values
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i].foo, i);
    }

    // check if array got resized
    for (u32 i = 4; i < 8; i++) {
        struct foo64 f = { .foo = i };
        tassert_eqs(EOK, list_foo64.append(&a, &f));
    }

    // validate appended values
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i].foo, i);
    }

    tassert_eqi(a.len, 8);
    head = _cex_list__head(&a.base);
    tassert_eqi(head->count, 8);
    tassert_eqi(head->capacity, 8);

    list_foo64.destroy(&a);
    return EOK;
}

test$case(testlist_align16)
{

    struct foo64
    {
        alignas(16) usize foo;
    };
    _Static_assert(sizeof(struct foo64) == 16, "size");
    _Static_assert(alignof(struct foo64) == 16, "align");

    list$typedef(list_foo64, struct foo64, true);
    list_foo64_c a;

    e$except(err, list$new(&a, 4, allocator))
    {
        tassert(false && "list$new fail");
    }

    // adding new elements into the end
    for (u32 i = 0; i < 4; i++) {
        struct foo64 f = { .foo = i };
        tassert_eqs(EOK, list_foo64.append(&a, &f));
    }
    tassert_eqi(a.len, 4);
    _cex_list_head_s* head = _cex_list__head(&a.base);
    tassert_eqi(head->count, 4);
    tassert_eqi(head->capacity, 4);
    tassert_eqi(head->header.elalign, 16);

    // validate appended values
    for$array(it, a.arr, a.len)
    {
        tassert_eqi(it.val->foo, it.idx);
    }

    // check if array got resized
    for (u32 i = 4; i < 8; i++) {
        struct foo64 f = { .foo = i };
        tassert_eqs(EOK, list_foo64.append(&a, &f));
    }

    // validate appended values
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i].foo, i);
    }

    tassert_eqi(a.len, 8);
    head = _cex_list__head(&a.base);
    tassert_eqi(head->count, 8);
    tassert_eqi(head->capacity, 8);

    list_foo64.destroy(&a);
    return EOK;
}

test$case(testlist_append_static)
{

    static list$define_static_buf(buf, list_i32, 4);

    list_i32_c a;
    tassert_eqs(EOK, list$new_static(&a, buf, arr$len(buf)));
    tassert_eqi(sizeof(buf), _CEX_LIST_BUF + sizeof(i32)*(4));
    tassert_eqi(list_i32.capacity(&a), 4);

    // adding new elements into the end
    for (i32 i = 0; i < 4; i++) {
        tassert_eqs(EOK, list_i32.append(&a, &i));
    }
    tassert_eqi(a.len, 4);
    _cex_list_head_s* head = _cex_list__head(&a.base);
    tassert_eqi(head->count, 4);
    tassert_eqi(head->capacity, 4);
    tassert_eqi(head->header.elalign, alignof(i32));
    tassert_eqi(head->header.elsize, sizeof(i32));

    // validate appended values
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i], i);
    }

    tassert_eqs(Error.overflow, list_i32.append(&a, &(int){ 1 }));
    tassert_eqs(Error.overflow, list_i32.extend(&a, a.arr, a.len));

    tassert_eqi(a.len, 4);
    tassert_eqi(head->count, 4);
    tassert_eqi(head->capacity, 4);

    list_i32.destroy(&a);
    // header reset to 0
    tassert_eqi(head->header.magic, 0);
    tassert_eqi(head->count, 0);
    tassert_eqi(head->capacity, 0);
    return EOK;
}

test$case(testlist_static_buffer_validation)
{
    list_i32_c a;

    alignas(32) char buf[_CEX_LIST_BUF + sizeof(int) * 1];

    tassert_eqs(EOK, list$new_static(&a, buf, arr$len(buf)));
    tassert_eqi(list_i32.capacity(&a), 1);

    // No capacity for 1 element
    tassert_eqs(Error.overflow, list$new_static(&a, buf, arr$len(buf) - 1));

    list_i32.destroy(&a);
    tassert(a.arr == NULL);
    tassert_eqi(a.len, 0);

    return EOK;
}

test$case(testlist_static_with_alignment)
{
    struct foo64
    {
        alignas(64) usize foo;
    };
    _Static_assert(sizeof(struct foo64) == 64, "size");
    _Static_assert(alignof(struct foo64) == 64, "align");

    list$typedef(list_foo64, struct foo64, true);
    list_foo64_c a;

    list$define_static_buf(buf, list_foo64, 2);
    // _Static_assert(alignof(buf) == 64, "align");
    tassert_eqi(sizeof(buf), sizeof(struct foo64) * 2 + _CEX_LIST_BUF);

    usize unalign = 1;
    char* unaligned = buf + unalign;

    tassert_eqs(EOK, list$new_static(&a, unaligned, arr$len(buf) - unalign));
    tassert_eqi(list_foo64.capacity(&a), 1);

    // adding new elements into the end
    for (u32 i = 0; i < list_foo64.capacity(&a); i++) {
        tassert_eqs(EOK, list_foo64.append(&a, &(struct foo64){ .foo = i + 1 }));
    }

    // Address is aligned to 64
    tassert_eqi(0, (usize)(a.arr) % _Alignof(struct foo64));

    tassert_eqi(a.len, 1);
    _cex_list_head_s* head = _cex_list__head(&a.base);
    // Data is aligned accordingly to item alignment
    tassert_eqi(((char*)a.arr - (char*)head), _CEX_LIST_BUF);
    tassert_eqi(((char*)a.arr - (char*)buf), 64);
    tassert_eqi(((char*)head - (char*)unaligned), 31);

    tassert_eqi(head->count, 1);
    tassert_eqi(head->capacity, 1);
    tassert_eqi(head->header.elsize, sizeof(struct foo64));
    tassert_eqi(head->header.elalign, alignof(struct foo64));

    // validate appended values
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i].foo, i + 1);
    }

    tassert_eqs(Error.overflow, list_foo64.append(&a, &(struct foo64){ .foo = 1 }));
    tassert_eqs(Error.overflow, list_foo64.extend(&a, &(struct foo64){ .foo = 1 }, 1));

    tassert_eqi(a.len, 1);
    tassert_eqi(head->count, 1);
    tassert_eqi(head->capacity, 1);

    // wreck head to make sure we didn't touch the aligned data
    memset(head, 0xff, sizeof(*head));
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i].foo, i + 1);
    }

    // HACK: don't destroy, no issues, and also *head is already INTENTIONALLY wrecked above
    // list_foo64.destroy(&a);

    return EOK;
}


test$case(testlist_slice)
{
    list_i32_c a;

    e$ret(list$new(&a, 4, allocator));

    // adding new elements into the end
    for (i32 i = 0; i < 4; i++) {
        tassert_eqs(EOK, list_i32.append(&a, &i));
    }
    tassert_eqi(a.len, 4);

    var b = list$slice(&a, 1, 0);
    tassert_eqi(b.arr[0], 1);
    tassert_eqi(b.arr[1], 2);
    tassert_eqi(b.arr[2], 3);
    tassert_eqi(b.len, 3);

    var c = list$slice(&a, 1, 0);
    tassert_eqi(c.arr[0], 1);
    tassert_eqi(c.arr[1], 2);
    tassert_eqi(c.arr[2], 3);
    tassert_eqi(c.len, 3);

    list_i32.destroy(&a);

    return EOK;
}


struct s_my
{
    i32 foo;
    char bar;
};

int cmp_func(const struct s_my* a, const struct s_my* b) {
    return b->foo - a->foo;
}

test$case(testlist_new_macro_def_testing)
{
    // NOTE: it's possible to define temporary list classes in functions!
    list$typedef(MyLst, struct s_my, true);

    MyLst_c a = { 0 };
    tassert_eqe(EOK, list$new(&a, 4, allocator));

    // adding new elements into the end
    for (i32 i = 0; i < 4; i++) {
        tassert_eqs(EOK, MyLst.append(&a, &(struct s_my){ .foo = i, .bar = 'a' + i }));
    }
    tassert_eqi(a.len, 4);

    tassert_eqi(a.len, 4);
    tassert_eqe(EOK, MyLst.insert(&a, &(struct s_my){ .foo = 100, .bar = 'z' }, 0));
    tassert_eqi(a.len, 5);
    tassert_eqi(a.len, 5);

    tassert_eqi(a.arr[0].foo, 100);
    tassert_eqi(a.arr[0].bar, 'z');

    tassert_eqi(a.arr[1].foo, 0);
    tassert_eqi(a.arr[1].bar, 'a');

    tassert_eqe(EOK, MyLst.del(&a, 1));
    tassert_eqi(a.arr[0].foo, 100);
    tassert_eqi(a.arr[0].bar, 'z');

    tassert_eqi(a.arr[1].foo, 1);
    tassert_eqi(a.arr[1].bar, 'b');

    tassert_eqi(MyLst.capacity(&a), 8);

    MyLst.clear(&a);
    tassert_eqi(MyLst.capacity(&a), 8);
    tassert_eqi(a.len, 0);

    struct s_my sarr[3] = {
        (struct s_my){ .foo = 3 },
        (struct s_my){ .foo = 4 },
        (struct s_my){ .foo = 5 },
    };
    tassert_eqe(EOK, MyLst.extend(&a, sarr, arr$len(sarr)));
    tassert_eqi(a.len, 3);
    tassert_eqi(a.arr[0].foo, 3);
    tassert_eqi(a.arr[1].foo, 4);
    tassert_eqi(a.arr[2].foo, 5);

    MyLst.sort(&a, cmp_func);
    tassert_eqi(a.len, 3);
    tassert_eqi(a.arr[0].foo, 5);
    tassert_eqi(a.arr[1].foo, 4);
    tassert_eqi(a.arr[2].foo, 3);
    
    MyLst.destroy(&a);
    return EOK;
}
test$case(testlist_array_iter)
{

    list_i32_c a;
    e$except(err, list$new(&a, 4, allocator))
    {
        tassert(false && "list$new fail");
    }

    u32 nit = 0;

    int arr[4] = { 0, 1, 2, 300 };
    nit = 0;
    for$array(it, arr, arr$len(arr))
    {
        tassert_eqi(it.idx, nit);
        tassert_eqi(*it.val, arr[nit]);
        nit++;
    }
    tassert_eqi(nit, 4);

    struct tstruct
    {
        u32 foo;
        char* bar;
    } tarr[2] = {
        { .foo = 1 },
        { .foo = 2 },
    };
    nit = 0;
    for$array(it, tarr, arr$len(tarr))
    {
        tassert_eqi(it.idx, nit);
        tassert_eqi(it.val->foo, nit + 1);
        nit++;
    }
    tassert_eqi(nit, 2);

    nit = 0;
    for$array(it, tarr, arr$len(tarr))
    {
        tassert_eqi(it.idx, nit);
        tassert_eqi(it.val->foo, nit + 1);
        nit++;
    }
    tassert_eqi(nit, 2);
    tassert_eqs(EOK, list_i32.extend(&a, arr, arr$len(arr)));
    tassert_eqi(a.len, 4);

    nit = 0;
    for$array(it, a.arr, a.len)
    {
        tassert_eqi(it.idx, nit);
        tassert_eqi(*it.val, arr[nit]);
        nit++;
    }
    tassert_eqi(nit, 4);
    nit = 0;

    nit = 0;
    for$array(it, a.arr, a.len)
    {
        tassert_eqi(it.idx, nit);
        tassert_eqi(*it.val, arr[nit]);
        nit++;
    }
    tassert_eqi(nit, 4);

    list_i32.destroy(&a);

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
    
    test$run(testlist_alloc_capacity);
    test$run(testlist_new);
    test$run(testlist_new_as_typedef);
    test$run(test_cex_list_new_as_struct_member);
    test$run(testlist_append);
    test$run(testlist_insert);
    test$run(testlist_del);
    test$run(testlist_sort);
    test$run(testlist_extend);
    test$run(testlist_iterator);
    test$run(testlist_align256);
    test$run(testlist_align64);
    test$run(testlist_align16);
    test$run(testlist_append_static);
    test$run(testlist_static_buffer_validation);
    test$run(testlist_static_with_alignment);
    test$run(testlist_slice);
    test$run(testlist_new_macro_def_testing);
    test$run(testlist_array_iter);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
