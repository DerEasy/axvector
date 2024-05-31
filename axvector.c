//
// Created by easy on 27.10.23.
//

#ifdef __cplusplus
extern "C" {
#endif

#include "axvector.h"
#include <string.h>

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))


static void *(*malloc_)(size_t size) = malloc;
static void *(*realloc_)(void *ptr, size_t size) = realloc;
static void (*free_)(void *ptr) = free;


static uint64_t normaliseIndex(uint64_t len, int64_t index) {
    uint64_t uindex = index;
    uindex += (index < 0) * len;
    return uindex;
}


static uint64_t toItemSize(uint64_t n) {
    return n * sizeof(void *);
}


static int defaultComparator(const void *a, const void *b) {
    uintptr_t x = (uintptr_t) *(const void **) a;
    uintptr_t y = (uintptr_t) *(const void **) b;
    return (x > y) - (x < y);
}


axvector *axv_sizedNew(uint64_t size) {
    size = MAX(1, size);
    axvector *v = (axvector *) malloc_(sizeof *v);
    if (v)
        v->items = (void **) malloc_(toItemSize(size));
    if (!v || !v->items) {
        free_(v);
        return NULL;
    }
    v->len = 0;
    v->cap = size;
    v->cmp = defaultComparator;
    v->context = NULL;
    v->destroy = NULL;
    v->locked = false;
    v->overlay = false;
    return v;
}


axvector *axv_new(void) {
    return axv_sizedNew(7);
}


axvector axv_newOverlay(void **items, uint64_t length, uint64_t capacity) {
    axvector v;
    length = MIN(length, capacity);
    v.items = items;
    v.len = length;
    v.cap = capacity;
    v.cmp = NULL;
    v.destroy = NULL;
    v.context = NULL;
    v.locked = true;
    v.overlay = true;
    return v;
}


void *axv_destroy(axvector *v) {
    if (v->destroy) {
        while (v->len)
            v->destroy(v->items[--v->len]);
    }
    void *context = v->context;
    if (!v->overlay) {
        free_(v->items);
        free_(v);
    }
    return context;
}


bool axv_set(axvector *v, int64_t index, void *val) {
    uint64_t i = normaliseIndex(v->len, index);
    if (i >= v->len)
        return true;
    v->items[i] = val;
    return false;
}


bool axv_swap(axvector *v, int64_t index1, int64_t index2) {
    uint64_t i1 = normaliseIndex(v->len, index1);
    uint64_t i2 = normaliseIndex(v->len, index2);
    if (i1 >= v->len || i2 >= v->len)
        return true;
    void *tmp = v->items[i1];
    v->items[i1] = v->items[i2];
    v->items[i2] = tmp;
    return false;
}


axvector *axv_reverse(axvector *v) {
    void **l = v->items;
    void **r = v->items + v->len - 1;
    while (l < r) {
        void *tmp = *l;
        *l = *r;
        *r = tmp;
        ++l; --r;
    }
    return v;
}


bool axv_reverseSection(axvector *v, int64_t index1, int64_t index2) {
    uint64_t i1 = normaliseIndex(v->len, index1);
    uint64_t i2 = normaliseIndex(v->len, index2);
    if (i1 >= v->len || i2 > v->len)
        return true;
    void **l = v->items + i1;
    void **r = v->items + i2 - 1;
    while (l < r) {
        void *tmp = *l;
        *l = *r;
        *r = tmp;
        ++l; --r;
    }
    return false;
}


axvector *axv_rotate(axvector *v, int64_t k) {
    k %= axv_len(v);
    if (k == 0)
        return v;
    axv_reverse(v);
    axv_reverseSection(v, 0, k);
    axv_reverseSection(v, k, axv_len(v));
    return v;
}


bool axv_shift(axvector *v, int64_t index, int64_t n) {
    if (n == 0)
        return false;
    if (n > 0) {
        const uint64_t oldlen = v->len;
        if (v->len + n > v->cap && axv_resize(v, v->len + n))
            return true;
        memmove(v->items + index + n, v->items + index, toItemSize(oldlen - n - 1));
        memset(v->items + index, 0, toItemSize(n));
        if (oldlen == v->len)
            v->len += n;
    } else {
        n = MIN(-n, axv_len(v) - index);
        if (v->destroy) {
            for (int64_t i = index; i < index + n; ++i)
                v->destroy(v->items[i]);
        }
        memmove(v->items + index, v->items + index + n, toItemSize(v->len - index - n));
        v->len -= n;
    }
    return false;
}


axvector *axv_discard(axvector *v, uint64_t n) {
    n = v->len - MIN(v->len, n);
    if (v->destroy) {
        while (v->len > n)
            v->destroy(v->items[--v->len]);
    }
    return v;
}


axvector *axv_clear(axvector *v) {
    if (v->destroy) {
        while (v->len)
            v->destroy(v->items[--v->len]);
    }
    v->len = 0;
    return v;
}


axvector *axv_copy(axvector *v) {
    axvector *v2 = axv_sizedNew(v->cap);
    if (!v2)
        return NULL;

    memcpy(v2->items, v->items, toItemSize(v->len));
    v2->len = v->len;
    v2->cmp = v->cmp;
    v2->context = v->context;
    v2->destroy = NULL;
    return v2;
}


bool axv_extend(axvector *v1, axvector *v2) {
    if (v1 == v2)
        return false;
    const uint64_t extlen = v1->len + v2->len;
    if (extlen > v1->cap && axv_resize(v1, extlen))
        return true;
    memcpy(v1->items + v1->len, v2->items, toItemSize(v2->len));
    v1->len = extlen;
    v2->len = 0;
    return false;
}


bool axv_concat(axvector *v1, axvector *v2) {
    const uint64_t extlen = v1->len + v2->len;
    if (extlen > v1->cap && axv_resize(v1, extlen))
        return true;
    memcpy(v1->items + v1->len, v2->items, toItemSize(v2->len));
    v1->len = extlen;
    return false;
}


axvector *axv_slice(axvector *v, int64_t index1, int64_t index2) {
    int64_t i1 = index1 + (index1 < 0) * axv_len(v);
    int64_t i2 = index2 + (index2 < 0) * axv_len(v);
    i1 = MAX(0, i1); i1 = MIN(i1, axv_len(v));
    i2 = MAX(0, i2); i2 = MIN(i2, axv_len(v));

    axvector *v2 = axv_sizedNew(v->len);
    if (!v2)
        return NULL;
    memcpy(v2->items, v->items + i1, toItemSize(i2 - i1));
    v2->len = i2 - i1;
    v2->cmp = v->cmp;
    v2->context = v->context;
    v2->destroy = NULL;
    return v2;
}


axvector *axv_rslice(axvector *v, int64_t index1, int64_t index2) {
    int64_t i1 = index1 + (index1 < 0) * axv_len(v);
    int64_t i2 = index2 + (index2 < 0) * axv_len(v);
    i1 = MAX(0, i1); i1 = MIN(i1, axv_len(v));
    i2 = MAX(0, i2); i2 = MIN(i2, axv_len(v));

    axvector *v2 = axv_sizedNew(v->len);
    if (!v2)
        return NULL;
    void **save = v2->items;
    for (int64_t i = i2 - 1; i >= i1; --i)
        *save++ = v->items[i];
    v2->len = i2 - i1;
    v2->cmp = v->cmp;
    v2->context = v->context;
    v2->destroy = NULL;
    return v2;
}


bool axv_resize(axvector *v, uint64_t size) {
    if (v->locked)
        return true;
    if (size < v->len && v->destroy) {
        while (v->len > size)
            v->destroy(v->items[--v->len]);
    } else {
        v->len = MIN(v->len, size);
    }
    size = MAX(1, size);
    void **items = (void **) realloc_(v->items, toItemSize(size));
    if (!items)
        return true;
    v->items = items;
    v->cap = size;
    return false;
}


void *axv_max(axvector *v) {
    if (v->len == 0)
        return NULL;
    void *max = *v->items;
    for (uint64_t i = 1; i < v->len; ++i) {
        if (v->cmp(v->items + i, &max) > 0)
            max = v->items[i];
    }
    return max;
}


void *axv_min(axvector *v) {
    if (v->len == 0)
        return NULL;
    void *min = *v->items;
    for (uint64_t i = 1; i < v->len; ++i) {
        if (v->cmp(v->items + i, &min) < 0)
            min = v->items[i];
    }
    return min;
}


bool axv_any(axvector *v, bool (*f)(const void *, void *), void *arg) {
    void **val = v->items;
    void **bound = v->items + v->len;
    while (val < bound) {
        if (f(*val++, arg))
            return true;
    }
    return false;
}


bool axv_all(axvector *v, bool (*f)(const void *, void *), void *arg) {
    void **val = v->items;
    void **bound = v->items + v->len;
    while (val < bound) {
        if (!f(*val++, arg))
            return false;
    }
    return true;
}


int64_t axv_count(axvector *v, void *val) {
    int64_t n = 0;
    void **curr = v->items;
    void **bound = v->items + v->len;
    while (curr < bound)
        n += v->cmp(&val, curr++) == 0;
    return n;
}


bool axv_compare(axvector *v1, axvector *v2) {
    if (v1->len != v2->len)
        return false;
    for (uint64_t i = 0; i < v1->len; ++i) {
        if (v1->cmp(v1->items + i, v2->items + i) != 0)
            return false;
    }
    return true;
}


axvector *axv_map(axvector *v, void *(*f)(void *)) {
    void **val = v->items;
    void **bound = v->items + v->len;
    while (val < bound) {
        *val = f(*val);
        ++val;
    }
    return v;
}


axvector *axv_filter(axvector *v, bool (*f)(const void *, void *), void *arg) {
    uint64_t len = 0;
    const bool shouldFree = v->destroy;
    for (uint64_t i = 0; i < v->len; ++i) {
        if (f(v->items[i], arg)) {
            v->items[len++] = v->items[i];
        } else if (shouldFree) {
            v->destroy(v->items[i]);
        }
    }
    v->len = len;
    return v;
}


axvector *axv_partition(axvector *v, bool (*f)(const void *, void *), void *arg) {
    axvector *v2 = axv_sizedNew(v->len);
    if (!v2) return NULL;

    uint64_t len1 = 0, len2 = 0;
    for (uint64_t i = 0; i < v->len; ++i) {
        if (f(v->items[i], arg)) {
            v->items[len1++] = v->items[i];
        } else {
            v2->items[len2++] = v->items[i];
        }
    }

    v->len = len1;
    v2->len = len2;
    v2->cmp = v->cmp;
    v2->context = v->context;
    v2->destroy = v->destroy;
    return v2;
}


axvector *axv_foreach(axvector *v, bool (*f)(void *, void *), void *arg) {
    const int64_t length = axv_len(v);
    for (int64_t i = 0; i < length; ++i) {
        if (!f(v->items[i], arg)) {
            return v;
        }
    }
    return v;
}


axvector *axv_rforeach(axvector *v, bool (*f)(void *, void *), void *arg) {
    for (int64_t i = axv_len(v) - 1; i >= 0; --i) {
        if (!f(v->items[i], arg)) {
            return v;
        }
    }
    return v;
}


bool axv_isSorted(axvector *v) {
    for (uint64_t i = 1; i < v->len; ++i) {
        if (v->cmp(v->items + i - 1, v->items + i) != 0)
            return false;
    }
    return true;
}


axvector *axv_sortSection(axvector *v, int64_t index1, int64_t index2) {
    uint64_t i1 = normaliseIndex(v->len, index1);
    uint64_t i2 = normaliseIndex(v->len, index2);
    qsort(v->items + i1, i2 - i1, sizeof *v->items, v->cmp);
    return v;
}


int64_t axv_linearSearch(axvector *v, void *val) {
    const int64_t length = axv_len(v);
    for (int64_t i = 0; i < length; ++i) {
        if (v->cmp(&val, v->items + i) == 0)
            return i;
    }
    return -1;
}


axvector *axv_setComparator(axvector *v, int (*cmp)(const void *, const void *)) {
    v->cmp = cmp ? cmp : defaultComparator;
    return v;
}


void axv_memoryfn(void *(*malloc_fn)(size_t), void *(*realloc_fn)(void *, size_t), void (*free_fn)(void *)) {
    malloc_ = malloc_fn ? malloc_fn : malloc;
    realloc_ = realloc_fn ? realloc_fn : realloc;
    free_ = free_fn ? free_fn : free;
}

#ifdef __cplusplus
}
#endif
