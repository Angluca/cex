#include <cex/all.c>
#include <lib/deque/deque.h>
#include <lib/deque/deque.c>
#include <lib/test/fff.h>
#include <cex/test.h>
#include <stdalign.h>
#include <stdio.h>


deque$typedef(deque_int, int, false);
deque$impl(deque_int);

/*
 * SUITE INIT / SHUTDOWN
 */
test$teardown()
{
    return EOK;
}

test$setup()
{
    uassert_enable();
    return EOK;
}

/*
 *
 *   TEST SUITE
 *
 */

test$case(testlist_alloc_capacity)
{
    // 16 is minimum
    tassert_eql(16, _cex_deque__alloc_capacity(0));
    tassert_eql(16, _cex_deque__alloc_capacity(1));
    tassert_eql(16, _cex_deque__alloc_capacity(15));
    tassert_eql(16, _cex_deque__alloc_capacity(16));
    tassert_eql(32, _cex_deque__alloc_capacity(17));
    tassert_eql(32, _cex_deque__alloc_capacity(32));
    tassert_eql(128, _cex_deque__alloc_capacity(100));


    return EOK;
}

test$case(test_deque_new)
{
    deque_int_c a;
    e$except(err, deque$new(&a, mem$))
    {
        tassert(false && "deque$new fail");
    }

    tassert_eqs(EOK, _cex_deque_validate(&a.base));

    _cex_deque_head_s* head = _cex_deque__head(a.base);
    // _cex_deque_head_s head = (*a)._head;
    tassert_eqi(head->header.magic, 0xdef0);
    tassert_eqi(head->header.elalign, alignof(int));
    tassert_eqi(head->header.elsize, sizeof(int));
    tassert_eqi(head->header.eloffset, sizeof(_cex_deque_head_s));
    tassert_eqi(head->capacity, 16);
    tassert_eqi(head->max_capacity, 0);
    tassert_eqi(head->idx_head, 0);
    tassert_eqi(head->idx_tail, 0);
    tassert_eqi(head->header.elsize, sizeof(int));
    tassert(head->allocator == mem$);

    deque_int.destroy(&a);
    return EOK;
}

test$case(test_element_alignment_16)
{
    struct foo16
    {
        alignas(16) usize foo;
    };
    _Static_assert(sizeof(struct foo16) == 16, "size");
    _Static_assert(alignof(struct foo16) == 16, "align");

    deque$typedef(deque_foo, struct foo16, true);
    deque_foo_c a;

    e$except(err, deque$new(&a, mem$))
    {
        tassert(false && "deque$new fail");
    }
    tassert_eqs(EOK, _cex_deque_validate(&a.base));

    _cex_deque_head_s* head = &a.base->_head;
    tassert_eqi(head->header.magic, 0xdef0);
    tassert_eqi(head->header.elalign, 16);
    tassert_eqi(head->header.elsize, 16);
    tassert_eqi(head->header.eloffset, 64);
    tassert_eqi(head->capacity, 16);
    tassert(head->allocator == mem$);
    tassert_eqi(deque_foo.len(&a), 0);

    for (u32 i = 0; i < 16; i++) {
        struct foo16 f = { .foo = i };
        tassert_eqs(EOK, deque_foo.push(&a, &f));
    }
    tassert_eqi(deque_foo.len(&a), 16);
    tassert_eqi(head->idx_tail, 16);

    u32 nit = 0;
    for (u32 i = 0; i < 16; i++) {
        struct foo16* f = deque_foo.dequeue(&a);
        tassertf(f != NULL, "%d\n: NULL", i);
        tassertf(f->foo == i, "%zu: i=%d\n", f->foo, i);
        nit++;
    }
    tassert_eqi(nit, 16);
    tassert_eqi(deque_foo.len(&a), 0);

    deque_foo.destroy(&a);
    return EOK;

}

test$case(test_element_alignment_64)
{


    struct foo64
    {
        alignas(64) usize foo;
    };
    _Static_assert(sizeof(struct foo64) == 64, "size");
    _Static_assert(alignof(struct foo64) == 64, "align");

    struct foo128
    {
        alignas(128) usize foo;
    };

    deque$typedef(deque_foo, struct foo64, true);
    deque_foo_c a;

    e$except(err, deque$new(&a, mem$))
    {
        tassert(false && "deque$new fail");
    }
    tassert_eqs(EOK, _cex_deque_validate(&a.base));

    _cex_deque_head_s* head = &a.base->_head;
    tassert_eqi(head->header.magic, 0xdef0);
    tassert_eqi(head->header.elalign, 64);
    tassert_eqi(head->header.elsize, 64);
    tassert_eqi(head->header.eloffset, 64);
    tassert_eqi(head->capacity, 16);
    tassert(head->allocator == mem$);
    tassert_eqi(deque_foo.len(&a), 0);

    for (u32 i = 0; i < 16; i++) {
        struct foo64 f = { .foo = i };
        tassert_eqs(EOK, deque_foo.push(&a, &f));
    }
    tassert_eqi(deque_foo.len(&a), 16);
    tassert_eqi(head->idx_tail, 16);

    u32 nit = 0;
    for (u32 i = 0; i < 16; i++) {
        struct foo64* f = deque_foo.dequeue(&a);
        tassertf(f != NULL, "%d\n: NULL", i);
        tassertf(f->foo == i, "%zu: i=%d\n", f->foo, i);
        nit++;
    }
    tassert_eqi(nit, 16);

    tassert_eqs(EOK, _cex_deque_validate(&a.base));
    deque_foo.destroy(&a);

    uassert_disable();

    // NOTE: Alignment is too high, static assert failure
    // deque$typedef(deque_foo128, struct foo128, true);
    // deque_foo128_c b;
    // tassert_eqs(Error.argument, deque$new(&b, allocator));
    // (void)deque_foo128;

    return EOK;

}

test$case(test_deque_new_append_pop)
{
    deque_int_c a;
    e$except(err, deque$new(&a, mem$))
    {
        tassert(false && "deque$new fail");
    }
    tassert_eqs(EOK, _cex_deque_validate(&a.base));

    _cex_deque_head_s* head = &a.base->_head;
    tassert_eqi(head->capacity, 16);
    tassert_eqi(deque_int.len(&a), 0);

    for (i32 i = 0; i < 16; i++) {
        tassert_eqs(EOK, deque_int.push(&a, &i));
    }
    tassert_eqi(deque_int.len(&a), 16);
    tassert_eqi(head->idx_tail, 16);

    u32 nit = 0;
    for (i32 i = 16; i > 0; i--) {
        i32* p = deque_int.pop(&a);
        tassertf(p != NULL, "%d\n: NULL", i);
        tassertf(*p == (i - 1), "%d: i=%d\n", *p, i);
        nit++;
    }
    tassert_eqi(nit, 16);
    tassert_eqi(deque_int.len(&a), 0);

    for (i32 i = 0; i < 16; i++) {
        tassert_eqs(EOK, deque_int.enqueue(&a, &i));
    }
    tassert_eqi(deque_int.len(&a), 16);
    tassert_eqi(head->idx_head, 0);
    tassert_eqi(head->idx_tail, 16);

    for (i32 i = 0; i < 16; i++) {
        i32* p = deque_int.dequeue(&a);
        tassertf(p != NULL, "%d\n: NULL", i);
        tassertf(*p == i, "%d: i=%d\n", *p, i);
    }
    tassert_eqi(deque_int.len(&a), 0);
    tassert_eqi(head->idx_head, 16);
    tassert_eqi(head->idx_tail, 16);

    tassert_eqs(EOK, _cex_deque_validate(&a.base));
    deque_int.destroy(&a);
    return EOK;
}


test$case(test_deque_new_append_roll_over)
{
    deque_int_c a;
    e$except(err, deque$new(&a, mem$))
    {
        tassert(false && "deque$new fail");
    }
    tassert_eqs(EOK, _cex_deque_validate(&a.base));

    _cex_deque_head_s* head = &a.base->_head;
    tassert_eqi(head->capacity, 16);
    tassert_eqi(deque_int.len(&a), 0);

    for (i32 i = 0; i < 16; i++) {
        tassert_eqs(EOK, deque_int.push(&a, &i));
    }
    tassert_eqi(deque_int.len(&a), 16);
    tassert_eqi(head->idx_head, 0);
    tassert_eqi(head->idx_tail, 16);

    i32 nit = 0;
    for (i32 i = 0; i < 16; i++) {
        i32* p = deque_int.dequeue(&a);
        tassertf(p != NULL, "%d\n: NULL", i);
        nit++;
    }
    tassert_eqi(deque_int.len(&a), 0);
    tassert_eqi(head->idx_head, 16);
    tassert_eqi(head->idx_tail, 16);

    // Que is emty, next push/append/enque - resets all to zero
    tassert_eqs(EOK, deque_int.push(&a, &nit));
    tassert_eqs(EOK, deque_int.push(&a, &nit));
    tassert_eqi(deque_int.len(&a), 2);
    tassert_eqi(head->idx_head, 0);
    tassert_eqi(head->idx_tail, 2);

    deque_int.dequeue(&a);
    deque_int.dequeue(&a);
    tassert_eqi(deque_int.len(&a), 0);
    tassert_eqi(head->idx_head, 2);
    tassert_eqi(head->idx_tail, 2);

    // Que is emty, next push/append/enque - resets all to zero
    tassert_eqs(EOK, deque_int.enqueue(&a, &nit));
    tassert_eqi(deque_int.len(&a), 1);
    tassert_eqi(head->idx_head, 0);
    tassert_eqi(head->idx_tail, 1);

    deque_int.clear(&a);
    tassert_eqi(deque_int.len(&a), 0);
    tassert_eqi(head->idx_head, 0);
    tassert_eqi(head->idx_tail, 0);

    tassert_eqs(EOK, _cex_deque_validate(&a.base));
    deque_int.destroy(&a);
    return EOK;
}

test$case(test_deque_new_append_grow)
{
    deque_int_c a;
    e$except(err, deque$new(&a, mem$))
    {
        tassert(false && "deque$new fail");
    }
    tassert_eqs(EOK, _cex_deque_validate(&a.base));

    _cex_deque_head_s* head = &a.base->_head;
    tassert_eqi(head->capacity, 16);
    tassert_eqi(deque_int.len(&a), 0);

    for (i32 i = 0; i < 16; i++) {
        tassert_eqs(EOK, deque_int.push(&a, &i));
    }
    tassert_eqi(deque_int.len(&a), 16);
    tassert_eqi(head->idx_head, 0);
    tassert_eqi(head->idx_tail, 16);

    // dequeue a couple elements, in this case we expect
    // realloc + growing the deque
    deque_int.dequeue(&a);
    deque_int.dequeue(&a);
    tassert_eqi(deque_int.len(&a), 14);

    i32 nit = 123;
    tassert_eqs(EOK, deque_int.push(&a, &nit));
    head = &a.base->_head; // head may change after resize!
    tassert_eqi(deque_int.len(&a), 15);
    tassert_eqi(head->header.magic, 0xdef0);
    tassert_eqi(head->header.elsize, sizeof(int));
    tassert_eqi(head->header.eloffset, 64);
    tassert_eqi(head->header.elalign, 4);
    tassert_eqi(head->max_capacity, 0);
    tassert_eqi(head->capacity, 32);
    tassert_eqi(head->idx_head, 2);
    tassert_eqi(head->idx_tail, 17);

    tassert_eqs(EOK, _cex_deque_validate(&a.base));
    deque_int.destroy(&a);
    return EOK;
}

test$case(test_deque_new_append_max_cap_wrap)
{
    deque_int_c a;
    e$except(err, deque$new(&a, mem$, .max_capacity = 16))
    {
        tassert(false && "deque$new fail");
    }
    tassert_eqs(EOK, _cex_deque_validate(&a.base));

    _cex_deque_head_s* head = &a.base->_head;
    tassert_eqi(head->capacity, 16);
    tassert_eqi(head->max_capacity, 16);
    tassert_eqi(deque_int.len(&a), 0);

    i32 nit = 123;
    for (i32 i = 0; i < 16; i++) {
        tassert_eqs(EOK, deque_int.push(&a, &i));
    }
    tassert_eqi(deque_int.len(&a), 16);
    tassert_eqi(head->idx_head, 0);
    tassert_eqi(head->idx_tail, 16);

    // Max capacity set, and no more room for extra
    tassert_eqs(Error.overflow, deque_int.push(&a, &nit));

    // dequeue a couple elements, in this case we expect
    // realloc + growing the deque
    deque_int.dequeue(&a);
    deque_int.dequeue(&a);
    tassert_eqi(deque_int.len(&a), 14);

    // These two pushes will wrap the ring buffer and add to the beginning
    nit = 123;
    tassert_eqs(EOK, deque_int.push(&a, &nit));
    nit = 124;
    tassert_eqs(EOK, deque_int.push(&a, &nit));
    tassert_eqi(deque_int.len(&a), 16);
    tassert_eqs(Error.overflow, deque_int.push(&a, &nit));

    head = &a.base->_head; // head may change after resize!
    tassert_eqi(deque_int.len(&a), 16);
    tassert_eqi(head->max_capacity, 16);
    tassert_eqi(head->capacity, 16);
    tassert_eqi(head->idx_head, 2);
    tassert_eqi(head->idx_tail, 18);

    // Wrapped pop first in reversed order (i.e. as stack)
    tassert_eqi(124, *((int*)deque_int.pop(&a)));
    tassert_eqi(123, *((int*)deque_int.pop(&a)));
    // Wraps back to the end of que array and gets last on
    tassert_eqi(15, *((int*)deque_int.pop(&a)));

    tassert_eqi(head->capacity, 16);
    tassert_eqi(head->idx_head, 2);
    tassert_eqi(head->idx_tail, 15);

    // dequeue returns from the head of the queue
    tassert_eqi(2, *((int*)deque_int.dequeue(&a)));
    tassert_eqi(3, *((int*)deque_int.dequeue(&a)));

    tassert_eqs(EOK, _cex_deque_validate(&a.base));
    deque_int.destroy(&a);
    return EOK;
}

test$case(test_deque_new_append_max_cap__rewrite_overflow)
{
    deque_int_c a;
    e$except(err, deque$new(&a, mem$, .max_capacity = 16, .rewrite_overflowed = true))
    {
        tassert(false && "deque$new fail");
    }
    tassert_eqs(EOK, _cex_deque_validate(&a.base));

    _cex_deque_head_s* head = &a.base->_head;
    tassert_eqi(head->capacity, 16);
    tassert_eqi(head->max_capacity, 16);
    tassert_eqi(deque_int.len(&a), 0);

    i32 nit = 123;
    for (i32 i = 0; i < 16; i++) {
        tassert_eqs(EOK, deque_int.push(&a, &i));
    }
    tassert_eqi(deque_int.len(&a), 16);
    tassert_eqi(head->idx_head, 0);
    tassert_eqi(head->idx_tail, 16);

    // Max capacity set, and no more room for extra
    tassert_eqs(EOK, deque_int.push(&a, &nit));
    tassert_eqi(deque_int.len(&a), 16);
    tassert_eqi(head->idx_head, 1);
    tassert_eqi(head->idx_tail, 17);

    tassert_eqi(123, *((int*)deque_int.pop(&a)));
    tassert_eqi(15, *((int*)deque_int.pop(&a)));

    tassert_eqs(EOK, _cex_deque_validate(&a.base));
    deque_int.destroy(&a);
    return EOK;
}


test$case(test_deque_new_append_max_cap__rewrite_overflow_with_rollover)
{
    deque_int_c a;
    e$except(err, deque$new(&a, mem$, .max_capacity = 16, .rewrite_overflowed = true))
    {
        tassert(false && "deque$new fail");
    }
    tassert_eqs(EOK, _cex_deque_validate(&a.base));

    _cex_deque_head_s* head = &a.base->_head;
    tassert_eqi(head->capacity, 16);
    tassert_eqi(head->max_capacity, 16);
    tassert_eqi(deque_int.len(&a), 0);

    i32 nit = 123;
    for (i32 i = 0; i < 16; i++) {
        tassert_eqs(EOK, deque_int.push(&a, &i));
    }
    tassert_eqi(deque_int.len(&a), 16);
    tassert_eqi(head->idx_head, 0);
    tassert_eqi(head->idx_tail, 16);

    tassert_eqi(0, *((int*)deque_int.dequeue(&a)));
    tassert_eqi(1, *((int*)deque_int.dequeue(&a)));

    // Max capacity set, and no more room for extra

    nit = 123;
    tassert_eqs(EOK, deque_int.push(&a, &nit));
    nit = 124;
    tassert_eqs(EOK, deque_int.push(&a, &nit));
    nit = 125;
    tassert_eqs(EOK, deque_int.push(&a, &nit));

    tassert_eqi(deque_int.len(&a), 16);
    tassert_eqi(head->idx_head, 3);
    tassert_eqi(head->idx_tail, 19);

    // at the beginning
    tassert_eqi(3, *((int*)deque_int.get(&a, 0)));
    tassert_eqi(125, *((int*)deque_int.get(&a, deque_int.len(&a) - 1)));
    tassert(NULL == deque_int.get(&a, deque_int.len(&a)));

    tassert_eqi(125, *((int*)deque_int.pop(&a)));
    tassert_eqi(124, *((int*)deque_int.pop(&a)));
    tassert_eqi(123, *((int*)deque_int.pop(&a)));
    // wraps to the end
    tassert_eqi(15, *((int*)deque_int.pop(&a)));

    tassert_eqs(EOK, _cex_deque_validate(&a.base));
    deque_int.destroy(&a);
    return EOK;
}

test$case(test_deque_new_append_max_cap__rewrite_overflow__multiloop)
{
    deque_int_c a;
    e$except(err, deque$new(&a, mem$, .max_capacity = 16, .rewrite_overflowed = true))
    {
        tassert(false && "deque$new fail");
    }
    tassert_eqs(EOK, _cex_deque_validate(&a.base));

    _cex_deque_head_s* head = &a.base->_head;
    tassert_eqi(head->capacity, 16);
    tassert_eqi(head->max_capacity, 16);
    tassert_eqi(deque_int.len(&a), 0);

    for (i32 i = 0; i < 256; i++) {
        tassert_eqs(EOK, deque_int.push(&a, &i));
    }

    tassert_eqi(head->capacity, 16);
    tassert_eqi(head->max_capacity, 16);
    tassert_eqi(deque_int.len(&a), 16);

    for (u32 i = 256 - 16; i < 256; i++) {
        tassert_eqi(i, *((int*)deque_int.dequeue(&a)));
    }

    tassert_eqs(EOK, _cex_deque_validate(&a.base));
    deque_int.destroy(&a);
    return EOK;
}

test$case(test_deque_new_append_multi_resize)
{
    deque_int_c a;
    e$except(err, deque$new(&a, mem$))
    {
        tassert(false && "deque$new fail");
    }
    tassert_eqs(EOK, _cex_deque_validate(&a.base));

    _cex_deque_head_s* head = &a.base->_head;
    tassert_eqi(head->capacity, 16);
    tassert_eqi(head->max_capacity, 0);
    tassert_eqi(deque_int.len(&a), 0);

    for (i32 i = 0; i < 256; i++) {
        tassert_eqs(EOK, deque_int.push(&a, &i));
    }

    head = &a.base->_head; // refresh dangling head pointer
    tassert_eqi(head->capacity, 256);
    tassert_eqi(head->max_capacity, 0);
    tassert_eqi(deque_int.len(&a), 256);
    for (u32 i = 0; i < 256; i++) {
        tassert_eqi(i, *((int*)deque_int.get(&a, i)));
    }

    for (u32 i = 0; i < 256; i++) {
        tassert_eqi(i, *((int*)deque_int.get(&a, 0)));
        tassert_eqi(i, *((int*)deque_int.dequeue(&a)));
    }

    tassert_eqs(EOK, _cex_deque_validate(&a.base));
    deque_int.destroy(&a);
    return EOK;
}

test$case(test_deque_iter_get)
{
    deque_int_c a;
    e$except(err, deque$new(&a, mem$, .max_capacity = 16, .rewrite_overflowed = true))
    {
        tassert(false && "deque$new fail");
    }
    tassert_eqs(EOK, _cex_deque_validate(&a.base));

    _cex_deque_head_s* head = &a.base->_head;
    tassert_eqi(head->capacity, 16);
    tassert_eqi(head->max_capacity, 16);
    tassert_eqi(deque_int.len(&a), 0);

    u32 nit = 123;

    nit = 0;
    for$iter(int, it, deque_int.iter_get(&a, 1, &it.iterator))
    {
        tassert(false && "should never happen!");
        nit++;
    }
    tassert_eqi(nit, 0);

    for (i32 i = 0; i < 16; i++) {
        tassert_eqs(EOK, deque_int.push(&a, &i));
    }
    tassert_eqi(deque_int.len(&a), 16);
    tassert_eqi(head->idx_head, 0);
    tassert_eqi(head->idx_tail, 16);

    // Getting que item (keeping element in que)
    nit = 0;
    for$iter(int, it, deque_int.iter_get(&a, 1, &it.iterator))
    {
        tassert_eqi(it.idx.i, nit);
        tassert_eqi(*it.val, nit);
        nit++;
    }

    // Getting que in reverse order
    nit = 0;
    for$iter(int, it, deque_int.iter_get(&a, -1, &it.iterator))
    {
        tassert_eqi(it.idx.i, 15 - nit);
        tassert_eqi(*it.val, 15 - nit);
        nit++;
    }

    tassert_eqs(EOK, _cex_deque_validate(&a.base));

    deque_int.destroy(&a);
    return EOK;
}

test$case(test_deque_iter_fetch)
{
    deque_int_c a;
    e$except(err, deque$new(&a, mem$, .max_capacity = 16, .rewrite_overflowed = true))
    {
        tassert(false && "deque$new fail");
    }
    tassert_eqs(EOK, _cex_deque_validate(&a.base));

    _cex_deque_head_s* head = &a.base->_head;
    tassert_eqi(head->capacity, 16);
    tassert_eqi(head->max_capacity, 16);
    tassert_eqi(deque_int.len(&a), 0);

    u32 nit = 123;

    nit = 0;
    for$iter(int, it, deque_int.iter_fetch(&a, 1, &it.iterator))
    {
        tassert(false && "should never happen!");
        nit++;
    }
    tassert_eqi(nit, 0);

    for (i32 i = 0; i < 16; i++) {
        tassert_eqs(EOK, deque_int.push(&a, &i));
    }
    tassert_eqi(deque_int.len(&a), 16);
    tassert_eqi(head->idx_head, 0);
    tassert_eqi(head->idx_tail, 16);

    // Fetching que item (iter with removing from que)
    nit = 0;
    for$iter(int, it, deque_int.iter_fetch(&a, 1, &it.iterator))
    {
        tassert_eqi(it.idx.i, 0);
        tassert_eqi(*it.val, nit);
        nit++;
    }

    tassert_eqi(deque_int.len(&a), 0);
    tassert_eqi(head->idx_head, 16);
    tassert_eqi(head->idx_tail, 16);

    tassert_eqs(EOK, _cex_deque_validate(&a.base));
    deque_int.destroy(&a);
    return EOK;
}

test$case(test_deque_iter_fetch_reversed)
{
    deque_int_c a;
    e$except(err, deque$new(&a, mem$, .rewrite_overflowed = true, .max_capacity = 16))
    {
        tassert(false && "deque$new fail");
    }
    tassert_eqs(EOK, _cex_deque_validate(&a.base));

    _cex_deque_head_s* head = &a.base->_head;
    tassert_eqi(head->capacity, 16);
    tassert_eqi(head->max_capacity, 16);
    tassert_eqi(deque_int.len(&a), 0);

    u32 nit = 123;

    nit = 0;
    for$iter(int, it, deque_int.iter_fetch(&a, -1, &it.iterator))
    {
        tassert(false && "should never happen!");
        nit++;
    }
    tassert_eqi(nit, 0);

    for (i32 i = 0; i < 16; i++) {
        tassert_eqs(EOK, deque_int.push(&a, &i));
    }
    tassert_eqi(deque_int.len(&a), 16);
    tassert_eqi(head->idx_head, 0);
    tassert_eqi(head->idx_tail, 16);

    // Fetching que item (iter with removing from que)
    nit = 0;
    for$iter(int, it, deque_int.iter_fetch(&a, -1, &it.iterator))
    {
        tassert_eqi(*it.val, 15 - nit);
        tassert_eqi(it.idx.i, head->capacity);
        nit++;
    }

    tassert_eqi(deque_int.len(&a), 0);
    tassert_eqi(head->idx_head, 0);
    tassert_eqi(head->idx_tail, 0);

    tassert_eqs(EOK, _cex_deque_validate(&a.base));
    deque_int.destroy(&a);
    tassert(a.base == NULL);
    return EOK;
}
//
test$case(test_deque_static)
{
    // NOTE: allowed to prepend static! to make function static vars
    static deque$define_static_buf(buf, deque_int, 16);

    deque_int_c a;
    e$except(err, deque$new_static(&a, buf, sizeof(buf), .rewrite_overflowed = true))
    {
        tassert(false && "deque$new fail");


    }
    tassert_eqs(EOK, _cex_deque_validate(&a.base));

    _cex_deque_head_s* head = &a.base->_head;
    tassert_eqi(head->header.magic, 0xdef0);
    tassert_eqi(head->header.elsize, sizeof(int));
    tassert_eqi(head->header.elalign, alignof(int));
    tassert_eqi(head->header.rewrite_overflowed, true);
    tassert_eqi(head->capacity, 16);
    tassert_eqi(head->max_capacity, 16);
    tassert_eqi(head->idx_tail, 0);
    tassert_eqi(head->idx_head, 0);
    tassert(head->allocator == NULL);
    tassert_eqi(deque_int.len(&a), 0);
    return EOK;
}

test$case(test_deque_static_append_grow)
{
    alignas(64) char buf[sizeof(_cex_deque_head_s) + sizeof(int) * 16];

    deque_int_c a;
    e$except(err, deque$new_static(&a, buf, arr$len(buf)))
    {
        tassert(false && "deque$new fail");
    }

    _cex_deque_head_s* head = &a.base->_head;
    tassert_eqi(deque_int.len(&a), 0);

    for (i32 i = 0; i < 16; i++) {
        tassert_eqs(EOK, deque_int.push(&a, &i));
    }
    tassert_eqi(head->header.magic, 0xdef0);
    tassert_eqi(head->header.elsize, sizeof(int));
    tassert_eqi(head->header.elalign, alignof(int));
    tassert_eqi(head->header.rewrite_overflowed, false);
    tassert_eqi(head->capacity, 16);
    tassert_eqi(head->max_capacity, 16);
    tassert_eqi(deque_int.len(&a), 16);


    i32 nit = 1;
    tassert_eqs(Error.overflow, deque_int.push(&a, &nit));

    tassert_eqs(EOK, _cex_deque_validate(&a.base));
    deque_int.destroy(&a);
    return EOK;
}

test$case(test_deque_static_append_grow_overwrite)
{
    alignas(64) char buf[sizeof(_cex_deque_head_s) + sizeof(int) * 16];

    deque_int_c a;
    e$except(err, deque$new_static(&a, buf, arr$len(buf), .rewrite_overflowed = true))
    {
        tassert(false && "deque$new fail");
    }
    tassert_eqs(EOK, _cex_deque_validate(&a.base));

    _cex_deque_head_s* head = &a.base->_head;
    tassert_eqi(deque_int.len(&a), 0);

    for (i32 i = 0; i < 16; i++) {
        tassert_eqs(EOK, deque_int.push(&a, &i));
    }
    tassert_eqi(head->header.rewrite_overflowed, true);
    tassert_eqi(head->capacity, 16);
    tassert_eqi(head->max_capacity, 16);
    tassert_eqi(deque_int.len(&a), 16);


    i32 nit = 123;
    tassert_eqi(head->idx_head, 0);
    tassert_eqi(head->idx_tail, 16);

    tassert_eqs(EOK, deque_int.push(&a, &nit));
    tassert_eqi(123, *(int*)deque_int.get(&a, deque_int.len(&a) - 1));
    tassert_eqi(head->idx_head, 1);
    tassert_eqi(head->idx_tail, 17);

    tassert_eqs(EOK, _cex_deque_validate(&a.base));
    deque_int.destroy(&a);
    return EOK;
}

test$case(test_deque_validate)
{
    alignas(64) char buf[sizeof(_cex_deque_head_s) + sizeof(int) * 16];

    deque_int_c a;
    tassert_eqs(EOK, deque$new_static(&a, buf, arr$len(buf), .rewrite_overflowed = true));
    tassert((void*)a.base == (void*)buf);
    tassert_eqs(Error.argument, _cex_deque_validate(NULL));
    tassert_eqs(EOK, _cex_deque_validate(&a.base));
    return EOK;
}

test$case(test_deque_validate__head_gt_tail)
{
    alignas(64) char buf[sizeof(_cex_deque_head_s) + sizeof(int) * 16];

    deque_int_c a;
    tassert_eqs(EOK, deque$new_static(&a, buf, arr$len(buf), .rewrite_overflowed = true));
    _cex_deque_head_s* head = &a.base->_head;
    head->idx_head = 1;
    tassert_eqs(Error.integrity, _cex_deque_validate(&a.base));
    return EOK;
}

test$case(test_deque_validate__eloffset_weird)
{
    alignas(64) char buf[sizeof(_cex_deque_head_s) + sizeof(int) * 16];

    deque_int_c a;
    tassert_eqs(EOK, deque$new_static(&a, buf, arr$len(buf), .rewrite_overflowed = true));
    _cex_deque_head_s* head = &a.base->_head;
    head->header.eloffset = 1;
    tassert_eqs(Error.integrity, _cex_deque_validate(&a.base));
    return EOK;

}

test$case(test_deque_validate__eloffset_elalign)
{
    alignas(64) char buf[sizeof(_cex_deque_head_s) + sizeof(int) * 16];

    deque_int_c a;
    tassert_eqs(EOK, deque$new_static(&a, buf, arr$len(buf), .rewrite_overflowed = true));
    _cex_deque_head_s* head = &a.base->_head;
    head->header.elalign = 0;
    tassert_eqs(Error.integrity, _cex_deque_validate(&a.base));
    head->header.elalign = 65;
    tassert_eqs(Error.integrity, _cex_deque_validate(&a.base));
    return EOK;

}

test$case(test_deque_validate__capacity_gt_max_capacity)
{
    alignas(64) char buf[sizeof(_cex_deque_head_s) + sizeof(int) * 16];

    deque_int_c a;
    tassert_eqs(EOK, deque$new_static(&a, buf, arr$len(buf), .rewrite_overflowed = true));
    _cex_deque_head_s* head = &a.base->_head;
    head->max_capacity = 16;
    head->capacity = 17;
    tassert_eqs(Error.integrity, _cex_deque_validate(&a.base));
    return EOK;

}


test$case(test_deque_validate__zero_capacity)
{
    alignas(64) char buf[sizeof(_cex_deque_head_s) + sizeof(int) * 16];

    deque_int_c a;
    tassert_eqs(EOK, deque$new_static(&a, buf, arr$len(buf), .rewrite_overflowed = true));
    _cex_deque_head_s* head = &a.base->_head;
    head->capacity = 0;
    tassert_eqs(Error.integrity, _cex_deque_validate(&a.base));
    return EOK;

}

test$case(test_deque_validate__bad_magic)
{
    alignas(64) char buf[sizeof(_cex_deque_head_s) + sizeof(int) * 16];

    deque_int_c a;
    tassert_eqs(EOK, deque$new_static(&a, buf, arr$len(buf), .rewrite_overflowed = true));
    _cex_deque_head_s* head = &a.base->_head;
    head->header.magic = 0xdef1;
    tassert_eqs(Error.integrity, _cex_deque_validate(&a.base));
    return EOK;

}

test$case(test_deque_validate__bad_pointer_alignment)
{
    alignas(64) char buf[sizeof(_cex_deque_head_s) + sizeof(int) * 16];

    deque_int_c a;
    tassert_eqs(EOK, deque$new_static(&a, buf, arr$len(buf), .rewrite_overflowed = true));
    a.base = (void*)(buf + 1);
    tassert_eqs(Error.memory, _cex_deque_validate(&a.base));
    return EOK;

}

test$case(test_deque_generic_new_append_pop)
{
    deque$typedef(deque_i32, int, true);

    deque_i32_c a;
    tassert_eqe(EOK, deque$new(&a, mem$));
    tassert_eqs(EOK, _cex_deque_validate(&a.base));

    _cex_deque_head_s* head = &a.base->_head;
    tassert_eqi(head->capacity, 16);
    tassert_eqi(deque_i32.len(&a), 0);

    for (i32 i = 0; i < 16; i++) {
        tassert_eqs(EOK, deque_i32.push(&a, &i));
    }
    tassert_eqi(deque_i32.len(&a), 16);
    tassert_eqi(head->idx_tail, 16);

    u32 nit = 0;
    for (i32 i = 16; i > 0; i--) {
        i32* p = deque_i32.pop(&a);
        tassertf(p != NULL, "%d\n: NULL", i);
        tassertf(*p == (i - 1), "%d: i=%d\n", *p, i);
        nit++;
    }
    tassert_eqi(nit, 16);
    tassert_eqi(deque_i32.len(&a), 0);

    for (i32 i = 0; i < 16; i++) {
        tassert_eqs(EOK, deque_i32.enqueue(&a, &i));
    }
    tassert_eqi(deque_i32.len(&a), 16);
    tassert_eqi(head->idx_head, 0);
    tassert_eqi(head->idx_tail, 16);

    for (i32 i = 0; i < 16; i++) {
        i32* p = deque_i32.dequeue(&a);
        tassertf(p != NULL, "%d\n: NULL", i);
        tassertf(*p == i, "%d: i=%d\n", *p, i);
    }
    tassert_eqi(deque_i32.len(&a), 0);
    tassert_eqi(head->idx_head, 16);
    tassert_eqi(head->idx_tail, 16);

    tassert_eqs(EOK, _cex_deque_validate(&a.base));
    deque_i32.destroy(&a);
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
    test$run(test_deque_new);
    test$run(test_element_alignment_16);
    test$run(test_element_alignment_64);
    test$run(test_deque_new_append_pop);
    test$run(test_deque_new_append_roll_over);
    test$run(test_deque_new_append_grow);
    test$run(test_deque_new_append_max_cap_wrap);
    test$run(test_deque_new_append_max_cap__rewrite_overflow);
    test$run(test_deque_new_append_max_cap__rewrite_overflow_with_rollover);
    test$run(test_deque_new_append_max_cap__rewrite_overflow__multiloop);
    test$run(test_deque_new_append_multi_resize);
    test$run(test_deque_iter_get);
    test$run(test_deque_iter_fetch);
    test$run(test_deque_iter_fetch_reversed);
    test$run(test_deque_static);
    test$run(test_deque_static_append_grow);
    test$run(test_deque_static_append_grow_overwrite);
    test$run(test_deque_validate);
    test$run(test_deque_validate__head_gt_tail);
    test$run(test_deque_validate__eloffset_weird);
    test$run(test_deque_validate__eloffset_elalign);
    test$run(test_deque_validate__capacity_gt_max_capacity);
    test$run(test_deque_validate__zero_capacity);
    test$run(test_deque_validate__bad_magic);
    test$run(test_deque_validate__bad_pointer_alignment);
    test$run(test_deque_generic_new_append_pop);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
