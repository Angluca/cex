#ifndef INCLUDE_STB_DS_H
#define INCLUDE_STB_DS_H

#include "cex.h"
#include <stddef.h>
#include <string.h>

#if defined(CEXDS_REALLOC) && !defined(CEXDS_FREE) || !defined(CEXDS_REALLOC) && defined(CEXDS_FREE)
#error "You must define both CEXDS_REALLOC and CEXDS_FREE, or neither."
#endif
#if !defined(CEXDS_REALLOC) && !defined(CEXDS_FREE)
#include <stdlib.h>
#define CEXDS_REALLOC(c,p,s) realloc(p,s)
#define CEXDS_FREE(c,p)      free(p)
#endif

#ifdef _MSC_VER
#define CEXDS_NOTUSED(v)  (void)(v)
#else
#define CEXDS_NOTUSED(v)  (void)sizeof(v)
#endif

// for security against attackers, seed the library with a random number, at least time() but stronger is better
extern void cexds_rand_seed(size_t seed);

// these are the hash functions used internally if you want to test them or use them for other purposes
extern size_t cexds_hash_bytes(void *p, size_t len, size_t seed);
extern size_t cexds_hash_string(char *str, size_t seed);

// this is a simple string arena allocator, initialize with e.g. 'cexds_string_arena my_arena={0}'.
typedef struct cexds_string_arena cexds_string_arena;
extern char * cexds_stralloc(cexds_string_arena *a, char *str);
extern void   cexds_strreset(cexds_string_arena *a);

///////////////
//
// Everything below here is implementation details
//

extern void * cexds_arrgrowf(void *a, size_t elemsize, size_t addlen, size_t min_cap, const Allocator_i* allc);
extern void   cexds_arrfreef(void *a);
extern void   cexds_hmfree_func(void *p, size_t elemsize);
extern void * cexds_hmget_key(void *a, size_t elemsize, void *key, size_t keysize, int mode);
extern void * cexds_hmget_key_ts(void *a, size_t elemsize, void *key, size_t keysize, ptrdiff_t *temp, int mode);
extern void * cexds_hmput_default(void *a, size_t elemsize);
extern void * cexds_hmput_key(void *a, size_t elemsize, void *key, size_t keysize, int mode);
extern void * cexds_hmdel_key(void *a, size_t elemsize, void *key, size_t keysize, size_t keyoffset, int mode);
extern void * cexds_shmode_func(size_t elemsize, int mode);

#if defined(__GNUC__) || defined(__clang__)
#define CEXDS_HAS_TYPEOF
#endif

#if !defined(__cplusplus)
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define CEXDS_HAS_LITERAL_ARRAY
#endif
#endif

// this macro takes the address of the argument, but on gcc/clang can accept rvalues
#if defined(CEXDS_HAS_LITERAL_ARRAY) && defined(CEXDS_HAS_TYPEOF)
  #if __clang__
  #define CEXDS_ADDRESSOF(typevar, value)     ((__typeof__(typevar)[1]){value}) // literal array decays to pointer to value
  #else
  #define CEXDS_ADDRESSOF(typevar, value)     ((typeof(typevar)[1]){value}) // literal array decays to pointer to value
  #endif
#else
#define CEXDS_ADDRESSOF(typevar, value)     &(value)
#endif

#define CEXDS_OFFSETOF(var,field)           ((char *) &(var)->field - (char *) (var))

typedef struct
{
  size_t      length;
  size_t      capacity;
  void      * hash_table;
  ptrdiff_t   temp;
    const Allocator_i* allocator;
} cexds_array_header;

// _Static_assert(sizeof(cexds_array_header) == 40, "size");

#define cexds_header(t)  ((cexds_array_header *) (t) - 1)
#define cexds_temp(t)    cexds_header(t)->temp
#define cexds_temp_key(t) (*(char **) cexds_header(t)->hash_table)

#define arr$(T) T*
#define arr$new(a, capacity, allocator) \
    (typeof(*a)*)cexds_arrgrowf(NULL, sizeof(*a), capacity, 0, allocator)

#define arr$free(a)       (cexds_arrfreef((a)), (a)=NULL)

#define arr$setcap(a,n)   (arr$grow(a,0,n))
#define arr$setlen(a,n)   ((arr$cap(a) < (size_t) (n) ? arr$setcap((a),(size_t)(n)),0 : 0), (a) ? cexds_header(a)->length = (size_t) (n) : 0)
#define arr$cap(a)        ((a) ? cexds_header(a)->capacity : 0)
#define arr$leni(a)        ((a) ? (ptrdiff_t) cexds_header(a)->length : 0)
#define arr$lenu(a)       ((a) ?             cexds_header(a)->length : 0)
#define arr$put(a,v)      (arr$maybegrow(a,1), (a)[cexds_header(a)->length++] = (v))
#define arr$push          arr$put  // synonym
#define arr$pop(a)        (cexds_header(a)->length--, (a)[cexds_header(a)->length])
#define arr$addn(a,n)     ((void)(arr$addnindex(a, n)))    // deprecated, use one of the following instead:
#define arr$addnptr(a,n)  (arr$maybegrow(a,n), (n) ? (cexds_header(a)->length += (n), &(a)[cexds_header(a)->length-(n)]) : (a))
#define arr$addnindex(a,n)(arr$maybegrow(a,n), (n) ? (cexds_header(a)->length += (n), cexds_header(a)->length-(n)) : arr$lenu(a))
#define arr$addnoff       arr$addnindex
#define arr$last(a)       ((a)[cexds_header(a)->length-1])
#define arr$del(a,i)      arr$deln(a,i,1)
#define arr$deln(a,i,n)   (memmove(&(a)[i], &(a)[(i)+(n)], sizeof *(a) * (cexds_header(a)->length-(n)-(i))), cexds_header(a)->length -= (n))
#define arr$delswap(a,i)  ((a)[i] = arr$last(a), cexds_header(a)->length -= 1)
#define arr$insn(a,i,n)   (arr$addn((a),(n)), memmove(&(a)[(i)+(n)], &(a)[i], sizeof(*(a)) * (cexds_header(a)->length-(n)-(i))))
#define arr$ins(a,i,v)    (arr$insn((a),(i),1), (a)[i]=(v))

#define arr$maybegrow(a,n)  ((!(a) || cexds_header(a)->length + (n) > cexds_header(a)->capacity) \
                                  ? (arr$grow(a,n,0),0) : 0)

#define arr$grow(a,add_len,min_cap)   ((a) = cexds_arrgrowf((a), sizeof *(a), (add_len), (min_cap), NULL))

#define hm$(K, V) struct {K key; V value;} *
#define hm$new(t, capacity, allocator) \
    (typeof(*t)*)cexds_arrgrowf(NULL, sizeof(*t), capacity, 0, allocator)

#define hm$put(t, k, v) \
    ((t) = cexds_hmput_key((t), sizeof *(t), (void*) CEXDS_ADDRESSOF((t)->key, (k)), sizeof (t)->key, 0),   \
     (t)[cexds_temp((t)-1)].key = (k),    \
     (t)[cexds_temp((t)-1)].value = (v))

#define hm$puts(t, s) \
    ((t) = cexds_hmput_key((t), sizeof *(t), &(s).key, sizeof (s).key, CEXDS_HM_BINARY), \
     (t)[cexds_temp((t)-1)] = (s))

#define hm$geti(t,k) \
    ((t) = cexds_hmget_key((t), sizeof *(t), (void*) CEXDS_ADDRESSOF((t)->key, (k)), sizeof (t)->key, CEXDS_HM_BINARY), \
      cexds_temp((t)-1))

#define hm$geti_ts(t,k,temp) \
    ((t) = cexds_hmget_key_ts((t), sizeof *(t), (void*) CEXDS_ADDRESSOF((t)->key, (k)), sizeof (t)->key, &(temp), CEXDS_HM_BINARY), \
      (temp))

#define hm$getp(t, k) \
    ((void) hm$geti(t,k), &(t)[cexds_temp((t)-1)])

#define hm$getp_ts(t, k, temp) \
    ((void) hm$geti_ts(t,k,temp), &(t)[temp])

#define hm$del(t,k) \
    (((t) = cexds_hmdel_key((t),sizeof *(t), (void*) CEXDS_ADDRESSOF((t)->key, (k)), sizeof (t)->key, CEXDS_OFFSETOF((t),key), CEXDS_HM_BINARY)),(t)?cexds_temp((t)-1):0)

#define hm$default(t, v) \
    ((t) =  cexds_hmput_default((t), sizeof *(t)), (t)[-1].value = (v))

#define hm$defaults(t, s) \
    ((t) =  cexds_hmput_default((t), sizeof *(t)), (t)[-1] = (s))

#define hm$free(p)        \
    ((void) ((p) != NULL ? cexds_hmfree_func((p)-1,sizeof*(p)),0 : 0),(p)=NULL)

#define hm$gets(t, k)    (*hm$getp(t,k))
#define hm$get(t, k)     (hm$getp(t,k)->value)
#define hm$get_ts(t, k, temp)  (hm$getp_ts(t,k,temp)->value)
#define hm$len(t)        ((t) ? (ptrdiff_t) cexds_header((t)-1)->length-1 : 0)
#define hm$lenu(t)       ((t) ?             cexds_header((t)-1)->length-1 : 0)
#define hm$getp_null(t,k)  (hm$geti(t,k) == -1 ? NULL : &(t)[cexds_temp((t)-1)])

#define cexds_shput(t, k, v) \
    ((t) = cexds_hmput_key((t), sizeof *(t), (void*) (k), sizeof (t)->key, CEXDS_HM_STRING),   \
     (t)[cexds_temp((t)-1)].value = (v))

#define cexds_shputi(t, k, v) \
    ((t) = cexds_hmput_key((t), sizeof *(t), (void*) (k), sizeof (t)->key, CEXDS_HM_STRING),   \
     (t)[cexds_temp((t)-1)].value = (v), cexds_temp((t)-1))

#define cexds_shputs(t, s) \
    ((t) = cexds_hmput_key((t), sizeof *(t), (void*) (s).key, sizeof (s).key, CEXDS_HM_STRING), \
     (t)[cexds_temp((t)-1)] = (s), \
     (t)[cexds_temp((t)-1)].key = cexds_temp_key((t)-1)) // above line overwrites whole structure, so must rewrite key here if it was allocated internally

#define cexds_pshput(t, p) \
    ((t) = cexds_hmput_key((t), sizeof *(t), (void*) (p)->key, sizeof (p)->key, CEXDS_HM_PTR_TO_STRING), \
     (t)[cexds_temp((t)-1)] = (p))

#define cexds_shgeti(t,k) \
     ((t) = cexds_hmget_key((t), sizeof *(t), (void*) (k), sizeof (t)->key, CEXDS_HM_STRING), \
      cexds_temp((t)-1))

#define cexds_pshgeti(t,k) \
     ((t) = cexds_hmget_key((t), sizeof *(t), (void*) (k), sizeof (*(t))->key, CEXDS_HM_PTR_TO_STRING), \
      cexds_temp((t)-1))

#define cexds_shgetp(t, k) \
    ((void) cexds_shgeti(t,k), &(t)[cexds_temp((t)-1)])

#define cexds_pshget(t, k) \
    ((void) cexds_pshgeti(t,k), (t)[cexds_temp((t)-1)])

#define cexds_shdel(t,k) \
    (((t) = cexds_hmdel_key((t),sizeof *(t), (void*) (k), sizeof (t)->key, CEXDS_OFFSETOF((t),key), CEXDS_HM_STRING)),(t)?cexds_temp((t)-1):0)
#define cexds_pshdel(t,k) \
    (((t) = cexds_hmdel_key((t),sizeof *(t), (void*) (k), sizeof (*(t))->key, CEXDS_OFFSETOF(*(t),key), CEXDS_HM_PTR_TO_STRING)),(t)?cexds_temp((t)-1):0)

#define cexds_sh_new_arena(t)  \
    ((t) = cexds_shmode_func_wrapper(t, sizeof *(t), CEXDS_SH_ARENA))
#define cexds_sh_new_strdup(t) \
    ((t) = cexds_shmode_func_wrapper(t, sizeof *(t), CEXDS_SH_STRDUP))

#define cexds_shdefault(t, v)  hm$default(t,v)
#define cexds_shdefaults(t, s) hm$defaults(t,s)

#define cexds_shfree       hm$free
#define cexds_shlenu       hm$lenu

#define cexds_shgets(t, k) (*cexds_shgetp(t,k))
#define cexds_shget(t, k)  (cexds_shgetp(t,k)->value)
#define cexds_shgetp_null(t,k)  (cexds_shgeti(t,k) == -1 ? NULL : &(t)[cexds_temp((t)-1)])
#define cexds_shlen        hm$len


typedef struct cexds_string_block
{
  struct cexds_string_block *next;
  char storage[8];
} cexds_string_block;

struct cexds_string_arena
{
  cexds_string_block *storage;
  size_t remaining;
  unsigned char block;
  unsigned char mode;  // this isn't used by the string arena itself
};

#define CEXDS_HM_BINARY         0
#define CEXDS_HM_STRING         1

enum
{
   CEXDS_SH_NONE,
   CEXDS_SH_DEFAULT,
   CEXDS_SH_STRDUP,
   CEXDS_SH_ARENA
};

#define cexds_arrgrowf            cexds_arrgrowf
#define cexds_shmode_func_wrapper(t,e,m)  cexds_shmode_func(e,m)

#endif // INCLUDE_STB_DS_H
