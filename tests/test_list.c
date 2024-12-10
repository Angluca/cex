#include "cex/cex.h"
#include <cex/all.c>
#include <cex/test/fff.h>
#include <cex/list.c>
#include <cex/test/test.h>
#include <stdio.h>

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

test$case(testlist_alloc_capacity)
{
    // 4 is minimum
    tassert_eql(4, list__alloc_capacity(1));
    tassert_eql(4, list__alloc_capacity(2));
    tassert_eql(4, list__alloc_capacity(3));
    tassert_eql(4, list__alloc_capacity(4));

    // up to 1024 grow with factor of 2
    tassert_eql(8, list__alloc_capacity(5));
    tassert_eql(16, list__alloc_capacity(12));
    tassert_eql(64, list__alloc_capacity(45));
    tassert_eql(1024, list__alloc_capacity(1023));

    // after 1024 increase by 20%
    tassert_eql(1024 * 1.2, list__alloc_capacity(1024));
    u64 expected_cap = 1000000 * 1.2;
    u64 alloc_cap = list__alloc_capacity(1000000);
    tassert(expected_cap - alloc_cap <= 1); // x32 - may have precision rounding

    return EOK;
}


test$case(testlist_new)
{
    list$define(int) a;

    e$except(err, list$new(&a, 5, allocator))
    {
        tassert(false && "list$new fail");
    }

    tassert(a.arr != NULL);
    tassert_eqi(a.len, 0);
    tassert_eqi(list$len(&a), a.len);

    list_head_s* head = (list_head_s*)((char*)a.arr - _CEX_LIST_BUF);
    tassert_eqi(head->header.magic, 0x1eed);
    tassert_eqi(head->header.elalign, alignof(int));
    tassert_eqi(head->header.elsize, sizeof(int));
    tassert_eqi(head->count, 0);
    tassert_eqi(head->capacity, 8);
    tassert(head->allocator == allocator);

    list.destroy(&a.base);
    return EOK;
}

test$case(testlist_new_as_typedef)
{
    typedef list$define(int) my_int_list;

    my_int_list a;

    e$except(err, list$new(&a, 5, allocator))
    {
        tassert(false && "list$new fail");
    }

    tassert(a.arr != NULL);
    tassert_eqi(a.len, 0);

    list_head_s* head = (list_head_s*)((char*)a.arr - _CEX_LIST_BUF);
    tassert_eqi(head->header.magic, 0x1eed);
    tassert_eqi(head->header.elalign, alignof(int));
    tassert_eqi(head->header.elsize, sizeof(int));
    tassert_eqi(head->count, 0);
    tassert_eqi(head->capacity, 8);
    tassert(head->allocator == allocator);

    list.destroy(&a.base);
    return EOK;
}
test$case(testlist_new_as_struct_member)
{
    struct
    {
        list$define(int) a;
    } mystruct = { 0 };


    e$except(err, list$new(&mystruct.a, 5, allocator))
    {
        tassert(false && "list$new fail");
    }

    tassert(mystruct.a.arr != NULL);
    tassert_eqi(mystruct.a.len, 0);

    list_head_s* head = (list_head_s*)((char*)mystruct.a.arr - _CEX_LIST_BUF);
    tassert_eqi(head->header.magic, 0x1eed);
    tassert_eqi(head->header.elalign, alignof(int));
    tassert_eqi(head->header.elsize, sizeof(int));
    tassert_eqi(head->count, 0);
    tassert_eqi(head->capacity, 8);
    tassert(head->allocator == allocator);

    list$destroy(&mystruct.a);
    return EOK;
}
test$case(testlist_append)
{
    list$define(int) a;

    e$except(err, list$new(&a, 4, allocator))
    {
        tassert(false && "list$new fail");
    }

    // adding new elements into the end
    for (u32 i = 0; i < 4; i++) {
        tassert_eqs(EOK, list.append(&a.base, &i));
    }
    tassert_eqi(a.len, 4);
    tassert_eqi(list$len(&a), a.len);
    list_head_s* head = list__head((list_c*)&a.base);
    tassert_eqi(head->count, 4);
    tassert_eqi(head->capacity, 4);
    tassert_eqi(list$capacity(&a), 4);

    // validate appended values
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i], i);
    }

    // check if array got resized
    for (u32 i = 4; i < 8; i++) {
        tassert_eqs(EOK, list.append(&a.base, &i));
    }

    // validate appended values
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i], i);
    }

    tassert_eqi(a.len, 8);
    head = list__head((list_c*)&a.base);
    tassert_eqi(head->count, 8);
    tassert_eqi(head->capacity, 8);

    list$destroy(&a);
    return EOK;
}

test$case(testlist_insert)
{
    list$define(int) a;

    e$except(err, list$new(&a, 4, allocator))
    {
        tassert(false && "list$new fail");
    }

    // adding new elements into the end
    tassert_eqs(EOK, list.append(&a.base, &(int){ 1 }));
    tassert_eqs(EOK, list.append(&a.base, &(int){ 2 }));
    tassert_eqs(EOK, list.append(&a.base, &(int){ 3 }));
    tassert_eqi(a.len, 3);

    tassert_eqs(Error.argument, list.insert(&a.base, &(int){ 4 }, 4));
    tassert_eqs(Error.ok, list.insert(&a.base, &(int){ 4 }, 3)); // same as append!
    //
    tassert_eqi(a.len, 4);
    tassert_eqi(a.arr[0], 1);
    tassert_eqi(a.arr[1], 2);
    tassert_eqi(a.arr[2], 3);
    tassert_eqi(a.arr[3], 4);


    list$clear(&a);
    tassert_eqi(a.len, 0);
    tassert_eqs(EOK, list.append(&a.base, &(int){ 1 }));
    tassert_eqs(EOK, list.append(&a.base, &(int){ 2 }));
    tassert_eqs(EOK, list.append(&a.base, &(int){ 3 }));
    tassert_eqs(Error.ok, list$insert(&a, &(int){ 4 }, 0)); // same as append!
    tassert_eqi(a.len, 4);
    tassert_eqi(a.arr[0], 4);
    tassert_eqi(a.arr[1], 1);
    tassert_eqi(a.arr[2], 2);
    tassert_eqi(a.arr[3], 3);

    list.clear(&a.base);
    tassert_eqi(a.len, 0);
    tassert_eqs(EOK, list.append(&a.base, &(int){ 1 }));
    tassert_eqs(EOK, list.append(&a.base, &(int){ 2 }));
    tassert_eqs(EOK, list.append(&a.base, &(int){ 3 }));
    tassert_eqs(Error.ok, list.insert(&a.base, &(int){ 4 }, 1)); // same as append!
    tassert_eqi(a.len, 4);
    tassert_eqi(a.arr[0], 1);
    tassert_eqi(a.arr[1], 4);
    tassert_eqi(a.arr[2], 2);
    tassert_eqi(a.arr[3], 3);

    list.destroy(&a.base);
    return EOK;
}

test$case(testlist_del)
{
    list$define(int) a;

    e$except(err, list$new(&a, 4, allocator))
    {
        tassert(false && "list$new fail");
    }
    tassert_eqs(Error.argument, list.del(&a.base, 0));

    // adding new elements into the end
    tassert_eqs(EOK, list.append(&a.base, &(int){ 1 }));
    tassert_eqs(EOK, list.append(&a.base, &(int){ 2 }));
    tassert_eqs(EOK, list.append(&a.base, &(int){ 3 }));
    tassert_eqi(a.len, 3);
    tassert_eqs(Error.argument, list$del(&a, 3));
    tassert_eqs(Error.ok, list.del(&a.base, 2));

    tassert_eqi(a.len, 2);
    tassert_eqi(a.arr[0], 1);
    tassert_eqi(a.arr[1], 2);

    list.clear(&a.base);
    tassert_eqi(a.len, 0);
    tassert_eqs(EOK, list.append(&a.base, &(int){ 1 }));
    tassert_eqs(EOK, list.append(&a.base, &(int){ 2 }));
    tassert_eqs(EOK, list.append(&a.base, &(int){ 3 }));
    tassert_eqs(EOK, list.append(&a.base, &(int){ 4 }));

    tassert_eqi(a.arr[0], 1);
    tassert_eqi(a.arr[1], 2);
    tassert_eqi(a.arr[2], 3);
    tassert_eqi(a.arr[3], 4);

    tassert_eqs(Error.ok, list.del(&a.base, 2));

    tassert_eqi(a.len, 3);
    tassert_eqi(a.arr[0], 1);
    tassert_eqi(a.arr[1], 2);
    tassert_eqi(a.arr[2], 4);

    list.clear(&a.base);
    tassert_eqi(a.len, 0);
    tassert_eqs(EOK, list.append(&a.base, &(int){ 1 }));
    tassert_eqs(EOK, list.append(&a.base, &(int){ 2 }));
    tassert_eqs(EOK, list.append(&a.base, &(int){ 3 }));
    tassert_eqs(EOK, list.append(&a.base, &(int){ 4 }));

    tassert_eqi(a.arr[0], 1);
    tassert_eqi(a.arr[1], 2);
    tassert_eqi(a.arr[2], 3);
    tassert_eqi(a.arr[3], 4);

    tassert_eqs(Error.ok, list.del(&a.base, 0));

    tassert_eqi(a.len, 3);
    tassert_eqi(a.arr[0], 2);
    tassert_eqi(a.arr[1], 3);
    tassert_eqi(a.arr[2], 4);

    list.destroy(&a.base);
    return EOK;
}

int
test_int_cmp(const void* a, const void* b)
{
    return *(int*)a - *(int*)b;
}

test$case(testlist_sort)
{
    list$define(int) a;

    e$except(err, list$new(&a, 4, allocator))
    {
        tassert(false && "list$new fail");
    }

    // adding new elements into the end
    tassert_eqs(EOK, list.append(&a.base, &(int){ 5 }));
    tassert_eqs(EOK, list.append(&a.base, &(int){ 1 }));
    tassert_eqs(EOK, list.append(&a.base, &(int){ 3 }));
    tassert_eqs(EOK, list.append(&a.base, &(int){ 2 }));
    tassert_eqs(EOK, list.append(&a.base, &(int){ 4 }));
    tassert_eqi(a.len, 5);


    list.sort(&a.base, test_int_cmp);
    tassert_eqi(a.len, 5);

    tassert_eqi(a.arr[0], 1);
    tassert_eqi(a.arr[1], 2);
    tassert_eqi(a.arr[2], 3);
    tassert_eqi(a.arr[3], 4);
    tassert_eqi(a.arr[4], 5);


    list.destroy(&a.base);
    return EOK;
}

test$case(testlist_sort_macro)
{
    list$define(int) a;

    e$except(err, list$new(&a, 4, allocator))
    {
        tassert(false && "list$new fail");
    }

    // adding new elements into the end
    tassert_eqs(EOK, list.append(&a.base, &(int){ 5 }));
    tassert_eqs(EOK, list.append(&a.base, &(int){ 1 }));
    tassert_eqs(EOK, list.append(&a.base, &(int){ 3 }));
    tassert_eqs(EOK, list.append(&a.base, &(int){ 2 }));
    tassert_eqs(EOK, list.append(&a.base, &(int){ 4 }));
    tassert_eqi(a.len, 5);


    list$sort(&a, test_int_cmp);
    tassert_eqi(a.len, 5);

    tassert_eqi(a.arr[0], 1);
    tassert_eqi(a.arr[1], 2);
    tassert_eqi(a.arr[2], 3);
    tassert_eqi(a.arr[3], 4);
    tassert_eqi(a.arr[4], 5);


    list.destroy(&a.base);
    return EOK;
}

test$case(testlist_extend)
{

    list$define(int) a;
    e$except(err, list$new(&a, 4, allocator))
    {
        tassert(false && "list$new fail");
    }

    int arr[4] = { 0, 1, 2, 3 };
    // a.arr = arr;
    // a.len = 1;

    tassert_eqs(EOK, list.extend(&a.base, arr, arr$len(arr)));
    tassert_eqi(a.len, 4);
    list_head_s* head = list__head((list_c*)&a.base);
    tassert_eqi(head->count, 4);
    tassert_eqi(head->capacity, 4);

    // validate appended values
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i], i);
    }

    int arr2[4] = { 4, 5, 6, 7 };
    // triggers resize
    tassert_eqs(EOK, list$extend(&a, arr2, arr$len(arr2)));

    // validate appended values
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i], i);
    }

    tassert_eqi(a.len, 8);
    head = list__head((list_c*)&a.base);
    tassert_eqi(head->count, 8);
    tassert_eqi(head->capacity, 8);

    list.destroy(&a.base);
    return EOK;
}

test$case(testlist_iterator)
{

    list$define(int) a;
    e$except(err, list$new(&a, 4, allocator))
    {
        tassert(false && "list$new fail");
    }

    u32 nit = 0;
    // type is inferred?
    for$iter(a.arr, it, list.iter(&a.base, &it.iterator))
    {
        tassert(false && "not expected");
        nit++;
    }
    tassert_eqi(nit, 0);

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
    tassert_eqs(EOK, list.extend(&a.base, arr, arr$len(arr)));
    tassert_eqi(a.len, 4);

    nit = 0;
    for$iter(int, it, list$iter(&a, &it.iterator))
    {
        tassert_eqi(it.idx.i, nit);
        tassert_eqi(*it.val, arr[nit]);
        nit++;
    }
    nit = 0;
    for$iter(*a.arr, it, list$iter(&a, &it.iterator))
    {
        tassert_eqi(it.idx.i, nit);
        tassert_eqi(*it.val, arr[nit]);
        nit++;
    }
    tassert_eqi(nit, 4);
    nit = 0;
    for$iter(typeof(*a.arr), it, list$iter(&a, &it.iterator))
    {
        tassert_eqi(it.idx.i, nit);
        tassert_eqi(*it.val, arr[nit]);
        nit++;
    }
    tassert_eqi(nit, 4);

    nit = 0;
    for$array(it, a.arr, a.len)
    {
        tassert_eqi(it.idx, nit);
        tassert_eqi(*it.val, arr[nit]);
        nit++;
    }
    tassert_eqi(nit, 4);

    list.destroy(&a.base);

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

    list$define(struct foo64) a;

    e$except(err, list$new(&a, 4, allocator))
    {
        tassert(false && "list$new fail");
    }

    // adding new elements into the end
    for (u32 i = 0; i < 4; i++) {
        struct foo64 f = { .foo = i };
        tassert_eqs(EOK, list$append(&a, &f));
    }
    tassert_eqi(a.len, 4);
    list_head_s* head = list__head((list_c*)&a.base);
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
    for$iter(struct foo64, it, list$iter(&a, &it.iterator))
    {
        tassert_eqi(it.idx.i, nit);
        tassert_eqi(it.val->foo, a.arr[it.idx.i].foo);
        nit++;
    }
    tassert_eqi(nit, a.len);

    // check if array got resized
    for (u32 i = 4; i < 8; i++) {
        struct foo64 f = { .foo = i };
        tassert_eqs(EOK, list.append(&a.base, &f));
    }

    // validate appended values
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i].foo, i);
    }

    tassert_eqi(a.len, 8);
    head = list__head((list_c*)&a.base);
    tassert_eqi(head->count, 8);
    tassert_eqi(head->capacity, 8);

    list.destroy(&a.base);
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

    list$define(struct foo64) a;

    e$except(err, list$new(&a, 4, allocator))
    {
        tassert(false && "list$new fail");
    }

    // adding new elements into the end
    for (u32 i = 0; i < 4; i++) {
        struct foo64 f = { .foo = i };
        tassert_eqs(EOK, list.append(&a.base, &f));
    }
    tassert_eqi(a.len, 4);
    list_head_s* head = list__head((list_c*)&a.base);
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
        tassert_eqs(EOK, list.append(&a.base, &f));
    }

    // validate appended values
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i].foo, i);
    }

    tassert_eqi(a.len, 8);
    head = list__head((list_c*)&a.base);
    tassert_eqi(head->count, 8);
    tassert_eqi(head->capacity, 8);

    list.destroy(&a.base);
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

    list$define(struct foo64) a;

    e$except(err, list$new(&a, 4, allocator))
    {
        tassert(false && "list$new fail");
    }

    // adding new elements into the end
    for (u32 i = 0; i < 4; i++) {
        struct foo64 f = { .foo = i };
        tassert_eqs(EOK, list.append(&a.base, &f));
    }
    tassert_eqi(a.len, 4);
    list_head_s* head = list__head((list_c*)&a.base);
    tassert_eqi(head->count, 4);
    tassert_eqi(head->capacity, 4);
    tassert_eqi(head->header.elalign, 16);

    // validate appended values
    for$iter(typeof(*a.arr), it, list.iter(&a.base, &it.iterator))
    {
        tassert_eqi(it.val->foo, it.idx.i);
    }

    // check if array got resized
    for (u32 i = 4; i < 8; i++) {
        struct foo64 f = { .foo = i };
        tassert_eqs(EOK, list.append(&a.base, &f));
    }

    // validate appended values
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i].foo, i);
    }

    tassert_eqi(a.len, 8);
    head = list__head((list_c*)&a.base);
    tassert_eqi(head->count, 8);
    tassert_eqi(head->capacity, 8);

    list.destroy(&a.base);
    return EOK;
}

test$case(testlist_append_static)
{
    union
    {
        list_c base;

        struct
        {
            int* const arr;
            const usize len;
        };

    } a;
    _Static_assert(sizeof(a) == sizeof(list_c), "size");

    // list$define(int) a;
    list$define_static_buf(buf, int, 4);

    tassert_eqs(EOK, list$new_static(&a, buf, arr$len(buf)));
    tassert_eqi(list.capacity(&a.base), 4);

    // adding new elements into the end
    for (u32 i = 0; i < 4; i++) {
        tassert_eqs(EOK, list.append(&a.base, &i));
    }
    tassert_eqi(a.len, 4);
    list_head_s* head = list__head((list_c*)&a.base);
    tassert_eqi(head->count, 4);
    tassert_eqi(head->capacity, 4);

    // validate appended values
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i], i);
    }

    tassert_eqs(Error.overflow, list.append(&a.base, &(int){ 1 }));
    tassert_eqs(Error.overflow, list.extend(&a.base, a.arr, a.len));

    tassert_eqi(a.len, 4);
    tassert_eqi(head->count, 4);
    tassert_eqi(head->capacity, 4);

    list.destroy(&a.base);
    // header reset to 0
    tassert_eqi(head->header.magic, 0);
    tassert_eqi(head->count, 0);
    tassert_eqi(head->capacity, 0);
    return EOK;
}

test$case(testlist_static_buffer_validation)
{
    list$define(int) a;

    alignas(32) char buf[_CEX_LIST_BUF + sizeof(int) * 1];

    tassert_eqs(EOK, list$new_static(&a, buf, arr$len(buf)));
    tassert_eqi(list.capacity(&a.base), 1);

    // No capacity for 1 element
    tassert_eqs(Error.overflow, list$new_static(&a, buf, arr$len(buf) - 1));

    list.destroy(&a.base);
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

    list$define(struct foo64) a;

    alignas(64) char buf[sizeof(list_head_s) + 64 * 2];
    usize unalign = 1;
    char* unaligned = buf + unalign;

    tassert_eqs(EOK, list$new_static(&a, unaligned, arr$len(buf) - unalign));
    tassert_eqi(list.capacity(&a.base), 1);

    // adding new elements into the end
    for (u32 i = 0; i < list.capacity(&a.base); i++) {
        tassert_eqs(EOK, list.append(&a.base, &(struct foo64){ .foo = i + 1 }));
    }

    // Address is aligned to 64
    tassert_eqi(0, (usize)(a.arr) % _Alignof(struct foo64));

    tassert_eqi(a.len, 1);
    list_head_s* head = list__head((list_c*)&a.base);
    tassert_eqi(((char*)a.arr - (char*)head), _CEX_LIST_BUF);
    tassert_eqi(((char*)a.arr - (char*)buf), 64);
    tassert_eqi(((char*)head - (char*)unaligned), 31);

    tassert_eqi(head->count, 1);
    tassert_eqi(head->capacity, 1);

    // validate appended values
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i].foo, i + 1);
    }

    tassert_eqs(Error.overflow, list.append(&a.base, &(struct foo64){ .foo = 1 }));
    tassert_eqs(Error.overflow, list.extend(&a.base, &(struct foo64){ .foo = 1 }, 1));

    tassert_eqi(a.len, 1);
    tassert_eqi(head->count, 1);
    tassert_eqi(head->capacity, 1);

    // wreck head to make sure we didn't touch the aligned data
    memset(head, 0xff, sizeof(*head));
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i].foo, i + 1);
    }

    // list.destroy(&a.base);
    return EOK;
}

test$case(testlist_cast)
{

    struct foo64
    {
        alignas(16) usize foo;
    };
    _Static_assert(sizeof(struct foo64) == 16, "size");
    _Static_assert(alignof(struct foo64) == 16, "align");

    list$define(struct foo64) a = { 0 };
    e$ret(list$new(&a, 4, allocator));
    tassert((void*)&a.base == &a.base.arr);
    tassert((void*)&a.base == &a.base);
    tassert((void*)&a.base.arr == &a.base);
    tassert((void*)&a.base.arr == &((&a.base)->arr));

    list$define(struct foo64)* b = list$cast(&a.base, b);

    tassert((void*)&a.base == (void*)b);
    tassert_eqi(list.capacity(&a.base), 4);

    // adding new elements into the end
    for (u32 i = 0; i < list.capacity(&a.base); i++) {
        tassert_eqs(EOK, list.append(&a.base, &(struct foo64){ .foo = i + 1 }));
    }

    // Address is aligned to 64
    tassert_eqi(0, (usize)(a.arr) % _Alignof(struct foo64));

    tassert_eqi(a.len, 4);

    // validate appended values
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i].foo, i + 1);
    }

    tassert_eqi(list.capacity(&b->base), 4);
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(b->arr[i].foo, i + 1);
    }

    list.destroy(&a.base);
    return EOK;
}

struct foo64a
{
    alignas(16) usize foo;
};

typedef list$define(struct foo64a) FooList;
typedef list$define(i32) Int32List;

static Exception
list_add_el_typedef(FooList* out_list)
{

    tassert_eqi(list.capacity(&out_list->base), 4);

    for (u32 i = 0; i < list.capacity(&out_list->base); i++) {
        e$ret(list.append(&out_list->base, &(struct foo64a){ .foo = i + 1 }));
    }

    return EOK;
}

test$case(testlist_typedef_list)
{

    _Static_assert(sizeof(struct foo64a) == 16, "size");
    _Static_assert(alignof(struct foo64a) == 16, "align");

    FooList a = { 0 };
    e$ret(list$new(&a, 4, allocator));

    tassert_eqe(EOK, list_add_el_typedef(&a));

    tassert_eqi(list.capacity(&a.base), 4);
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i].foo, i + 1);
    }

    list.destroy(&a.base);
    return EOK;
}

static Exception
list_add_el(list_c* out_list)
{

    list$define(struct foo64a)* b = list$cast(out_list, b);
    tassert_eqi(list.capacity(&b->base), 4);

    for (u32 i = 0; i < list.capacity(&b->base); i++) {
        e$ret(list.append(&b->base, &(struct foo64a){ .foo = i + 1 }));
    }

    // WARNING: Sanity checks: raises assert - elsize mismatch!
    uassert_disable();

    log$warn("list$casting - runtime check of size/alignment compatibility");
    list$define(int)* c = list$cast(out_list, c);

    struct foo64b
    {
        usize foo;
        usize foo2;
    };
    list$define(struct foo64b)* d = list$cast(out_list, d);

    return EOK;
}

test$case(testlist_cast_pass_tofunc)
{

    _Static_assert(sizeof(struct foo64a) == 16, "size");
    _Static_assert(alignof(struct foo64a) == 16, "align");

    list$define(struct foo64a) a = { 0 };
    e$ret(list$new(&a, 4, allocator));

    tassert_eqe(EOK, list_add_el(&a.base));

    tassert_eqi(list.capacity(&a.base), 4);
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i].foo, i + 1);
    }

    list.destroy(&a.base);
    return EOK;
}

test$case(testlist_append_macro)
{
    list$define(int) a;

    e$except(err, list$new(&a, 4, allocator))
    {
        tassert(false && "list$new fail");
    }

    // adding new elements into the end
    for (int i = 0; i < 4; i++) {
        tassert_eqe(EOK, list$append(&a, &i));
        // int* p = &i;
        // tassert_eqe(EOK, list$append(&a, &p));
    }
    tassert_eqi(a.len, 4);
    list_head_s* head = list__head((list_c*)&a.base);
    tassert_eqi(head->count, 4);
    tassert_eqi(head->capacity, 4);

    // validate appended values
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i], i);
    }

    // check if array got resized
    for (i32 i = 4; i < 8; i++) {
        tassert_eqs(EOK, list$append(&a, &i));
    }

    // validate appended values
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i], i);
    }

    tassert_eqi(a.len, 8);
    head = list__head((list_c*)&a.base);
    tassert_eqi(head->count, 8);
    tassert_eqi(head->capacity, 8);

    list.destroy(&a.base);
    return EOK;
}

test$case(testlist_slice)
{
    list$define(int) a;
    Int32List il;

    e$ret(list$new(&a, 4, allocator));
    e$ret(list$new(&il, 4, allocator));

    // adding new elements into the end
    for (u32 i = 0; i < 4; i++) {
        tassert_eqs(EOK, list.append(&a.base, &i));
        tassert_eqs(EOK, list.append(&il.base, &i));
    }
    tassert_eqi(list$len(&a), 4);

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

    // possible to access slice in line (but it will call a function each time)
    tassert_eqi(list$slice(&il, 2, 0).arr[0], 2);

    list$destroy(&a);
    list$destroy(&il);
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
    test$run(testlist_new_as_struct_member);
    test$run(testlist_append);
    test$run(testlist_insert);
    test$run(testlist_del);
    test$run(testlist_sort);
    test$run(testlist_sort_macro);
    test$run(testlist_extend);
    test$run(testlist_iterator);
    test$run(testlist_align256);
    test$run(testlist_align64);
    test$run(testlist_align16);
    test$run(testlist_append_static);
    test$run(testlist_static_buffer_validation);
    test$run(testlist_static_with_alignment);
    test$run(testlist_cast);
    test$run(testlist_typedef_list);
    test$run(testlist_cast_pass_tofunc);
    test$run(testlist_append_macro);
    test$run(testlist_slice);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
