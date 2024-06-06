//
// Created by easy on 27.10.23.
//

/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef AXVECTOR_AXVECTOR_H
#define AXVECTOR_AXVECTOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/*
    axvector is a dynamic vector/array library that has some functional programming concepts and some useful utility
    functions built in.

    Some functions use a comparator. The default comparator compares the addresses of items, but a custom
    comparator can be given and shall conform to the C standard library's comparison function specifications.

    A destructor function may also be supplied. There is no default destructor. The destructor will be called on items
    that are irrevocably removed from the vector. Its prototype is void (*)(void *), like the free() function.

    To provide built-in bookkeeping, the vector also has storage for a context. This is simply a void *. The context is
    not used by the vector itself and is exclusively controlled by the user.

    axvector supports negative indexing: -1 is the last item, -2 the penultimate one etc. All functions that take
    two indices to signify a section treat the first index inclusively and the second index exclusively.

    The struct definition of axvector is given in its header for optimisation purposes only. To use axvector, you must
    rely solely on the functions of the library.
*/
typedef struct axvector {
    void **items;
    uint64_t len;
    uint64_t cap;
    int (*cmp)(const void *, const void *);
    void (*destroy)(void *);
    void *context;
    bool locked;
    bool overlay;
} axvector;


/**
 * Create axvector with starting capacity.
 * @param size Capacity.
 * @return New axvector or NULL if OOM.
 */
axvector *axv_sizedNew(uint64_t size);
/**
 * Create axvector with default capacity.
 * @return New axvector or NULL if OOM.
 */
axvector *axv_new(void);
/**
 * Create axvector as an overlay. An overlay takes an array of items, how many items are currently stored in
 * that array and how many could potentially be stored there. A new axvector is created using that item array as
 * its internal array. No heap allocations are made. Overlays are locked by default and do not free their internal
 * array upon destruction. Thus, it is not strictly necessary to call axv_destroy() on an overlay, as this will
 * only call the destructor on all items, if there is one.
 * @param items An array of items of type void *.
 * @param length Number of items currently stored in the items array. This must not exceed the capacity.
 * @param capacity Number of items that could potentially be stored in the items array. Upper limit of length.
 * @return New axvector in stack memory, hence no axv_destroy() explicitly necessary. If length was greater than
 * capacity, length has been set equal to capacity instead.
 */
axvector axv_newOverlay(void **items, uint64_t length, uint64_t capacity);
/**
 * If destructor is set, call destructor on all items. Destroy axvector and free memory.
 * @return Context.
 */
void *axv_destroy(axvector *v);
/**
 * Set capacity of vector to some value. If the new capacity is less than the length of the vector and a destructor
 * is set, the destructor will be called on all excess items. This is done even if the resize operation itself fails.
 * @param size New capacity.
 * @return True iff OOM.
 */
bool axv_resize(axvector *v, uint64_t size);
/**
 * Push an item at the end of the vector. Vector is automatically resized if need be.
 * @param val Item.
 * @return True iff OOM during resize operation. Item is not pushed in this case.
 */
static inline bool axv_push(axvector *v, void *val) {
    if (v->len >= v->cap && axv_resize(v, (v->cap << 1) | 1))
        return true;
    v->items[v->len++] = val;
    return false;
}
/**
 * Pop off (remove) the last item.
 * @return The last item.
 */
static inline void *axv_pop(axvector *v) {
    return v->len ? v->items[--v->len] : NULL;
}
/**
 * Get last item without removing it.
 * @return The last item.
 */
static inline void *axv_top(axvector *v) {
    return v->len ? v->items[v->len - 1] : NULL;
}
/**
 * Signed number of items in this vector.
 * @return Signed length of vector.
 */
static inline int64_t axv_len(axvector *v) {
    return (int64_t) v->len;
}
/**
 * Unsigned number of items in this vector.
 * @return Unsigned length of vector.
 */
static inline uint64_t axv_ulen(axvector *v) {
    return v->len;
}
/**
 * Index vector and return item.
 * @param index May be negative.
 * @return Item at index or NULL if index out of range.
 */
static inline void *axv_at(axvector *v, int64_t index) {
    uint64_t uindex = index;
    uindex += (index < 0) * v->len;
    return uindex < v->len ? v->items[uindex] : NULL;
}
/**
 * Index vector directly and return item.
 * @param index Must be positive. No negative indexing allowed.
 * @return Item at index or NULL if index out of range.
 */
static inline void *axv_get(struct axvector *v, uint64_t index) {
    return index < v->len ? v->items[index] : NULL;
}
/**
 * Replace item at index with a new item.
 * @param index May be negative.
 * @param val The new item.
 * @return True iff index out of range.
 */
static inline bool axv_set(axvector *v, int64_t index, void *val) {
    uint64_t i = index + (index < 0) * v->len;
    if (i >= v->len)
        return true;
    v->items[i] = val;
    return false;
}
/**
 * Swap two items.
 * @param index1 May be negative.
 * @param index2 May be negative.
 * @return True iff any index out of range.
 */
bool axv_swap(axvector *v, int64_t index1, int64_t index2);
/**
 * Reverse all items in-place.
 * @return Self.
 */
axvector *axv_reverse(axvector *v);
/**
 * Reverse items in some section in-place.
 * @param index1 Beginning of section. May be negative. Inclusive.
 * @param index2 End of section. May be negative. Exclusive.
 * @return True iff index out of range.
 */
bool axv_reverseSection(axvector *v, int64_t index1, int64_t index2);
/**
 * Rotate items by k places to the right. Negative k rotates to the left. In-place. O(n).
 * @param k Rotation value.
 * @return Self.
 */
axvector *axv_rotate(axvector *v, int64_t k);
/**
 * Shift all items toward or away from some anchor point. If n is positive, all items starting at the anchor point
 * are shifted n places to the right and the vector is resized as needed. The resulting gap in the vector is
 * filled with zeroes. If n is negative, the first n items starting at the anchor point are removed from the vector.
 * Subsequent items are then shifted toward the anchor point. If a destructor is set, it is called upon all removed
 * items.
 * @param index The anchor point. May be negative. Inclusive.
 * @param n Positive to reserve space amidst the items. Negative to remove items and collapse the vector.
 * @return True iff OOM during resize operation. Vector is unmodified in this case.
 */
bool axv_shift(axvector *v, int64_t index, int64_t n);
/**
 * Remove the last n items. If a destructor is set, it is called upon all removed items.
 * @param n Number of items to remove.
 * @return Self.
 */
axvector *axv_discard(axvector *v, uint64_t n);
/**
 * Removes all items. If a destructor is set, it is called upon every item.
 * @return Self.
 */
axvector *axv_clear(axvector *v);
/**
 * Create a shallow copy of a vector. The copy contains all items of the original and is created with the same
 * capacity. The comparator and context are copied, the destructor is not.
 * @return New axvector or NULL if OOM.
 */
axvector *axv_copy(axvector *v);
/**
 * All items of the second vector are moved to the end of the first vector, thereby clearing the second vector.
 * If both vectors are the same, nothing is done. The first vector is resized as needed.
 * @param v1 First vector.
 * @param v2 Second vector.
 * @return True iff OOM during resize operation.
 */
bool axv_extend(axvector *v1, axvector *v2);
/**
 * All items of the second vector are copied to the end of the first vector. No changes are done to the second
 * vector. The vectors may be the same. The first vector is resized as needed.
 * @param v1 First vector.
 * @param v2 Second vector.
 * @return True iff OOM during resize operation.
 */
bool axv_concat(axvector *v1, axvector *v2);
/**
 * Create a shallow copy of a vector. The copy contains all items of the original which are in the specified slice.
 * The copy is created with capacity equal to the number of items copied or 1 if the slice is empty. The comparator
 * and context are copied, the destructor is not.
 * @param index1 Beginning of slice. May be negative. Inclusive.
 * @param index2 End of slice. May be negative. Exclusive.
 * @return New axvector or NULL if OOM.
 */
axvector *axv_slice(axvector *v, int64_t index1, int64_t index2);
/**
 * Create a shallow copy of a vector. The copy contains all items of the original which are in the specified slice
 * in reverse order. The copy is created with capacity equal to the number of items copied or 1 if the slice is empty.
 * The comparator and context are copied, the destructor is not.
 * @param index1 Beginning of slice. May be negative. Inclusive.
 * @param index2 End of slice. May be negative. Exclusive.
 * @return New axvector or NULL if OOM.
 */
axvector *axv_rslice(axvector *v, int64_t index1, int64_t index2);
/**
 * Search the greatest item according to the comparator using forward linear search.
 * @return The greatest item or NULL if the vector is empty.
 */
void *axv_max(axvector *v);
/**
 * Search the least item according to the comparator using forward linear search.
 * @return The least item or NULL if the vector is empty.
 */
void *axv_min(axvector *v);
/**
 * Let f be a predicate taking (item in vector, optional argument).
 * Check if any item x contained in the vector satisfies f(x, arg) and stop at the first occurrence
 * of such satisfaction. Items are checked linearly from first to last.
 * @param f Some predicate to apply to the vector.
 * @param arg An optional argument passed to the predicate.
 * @return True iff any item satisfies the predicate.
 */
bool axv_any(axvector *v, bool (*f)(const void *, void *), void *arg);
/**
 * Let f be a predicate taking (item in vector, optional argument).
 * Check if all items x contained in the vector satisfy f(x, arg) and stop at the first occurrence
 * of no such satisfaction. Items are checked linearly from first to last.
 * @param f Some predicate to apply to the vector.
 * @param arg An optional argument passed to the predicate.
 * @return True iff all items satisfy the predicate.
 */
bool axv_all(axvector *v, bool (*f)(const void *, void *), void *arg);
/**
 * Number of items in this vector comparing equal to the given argument according to the comparator.
 * @param val The value all items are to be compared against.
 * @return The resulting count.
 */
uint64_t axv_count(axvector *v, void *val);
/**
 * Using the first vector's comparator, compare all items of the first and second vector. Comparison stops
 * prematurely if any unequal item is found. Comparisons are done linearly from first to last item.
 * @param v1 First vector.
 * @param v2 Second vector.
 * @return True iff the vectors have the same length and all items compare equal.
 */
bool axv_compare(axvector *v1, axvector *v2);
/**
 * Apply some function f on every item. Maps are done linearly from first to last item.
 * @param f Function taking an item and returning whatever to overwrite its spot in the vector with.
 * @return Self.
 */
axvector *axv_map(axvector *v, void *(*f)(void *));
/**
 * Let f be a predicate taking (item in vector, optional argument).
 * Keep all items x in the vector that satisfy f(x, arg), remove all those that don't, and close the
 * resulting gaps by contracting the space between all remaining items, thus preserving the relative order
 * of the remaining items. If a destructor is set, it is called upon all removed items. The filter is applied
 * linearly from first to last item. O(n).
 * @param f Some predicate to filter the vector.
 * @param arg An optional argument passed to the predicate.
 * @return Self.
 */
axvector *axv_filter(axvector *v, bool (*f)(const void *, void *), void *arg);
/**
 * Let f be a predicate taking (item in vector, optional argument).
 * Keep all items x in the vector that satisfy f(x, arg), reject all those that don't, and close the
 * resulting gaps by contracting the space between all remaining items, thus preserving the relative order
 * of the remaining items. All rejected items are moved to a new vector in the same relative order in which
 * they appeared in the original vector. The partitioning is applied linearly from first to last item. O(n).
 * @param f Some predicate to filter the vector.
 * @param arg An optional argument passed to the predicate.
 * @return The new axvector containing all rejected items or NULL if OOM, in which case no filtering is done.
 */
axvector *axv_partition(axvector *v, bool (*f)(const void *, void *), void *arg);
/**
 * Let f be a function taking (item in vector, optional argument).
 * Call f(x, arg) on every item x until f returns false or all items of the vector have been exhausted.
 * Items are iterated linearly from first to last.
 * @param f Function to call on items.
 * @param arg An optional argument passed to the function.
 * @return Self.
 */
axvector *axv_foreach(axvector *v, bool (*f)(void *, void *), void *arg);
/**
 * Let f be a function taking (item in vector, optional argument).
 * Call f(x, arg) on every item x until f returns false or all items of the vector have been exhausted.
 * Items are iterated linearly from last to first.
 * @param f Function to call on items.
 * @param arg An optional argument passed to the function.
 * @return Self.
 */
axvector *axv_rforeach(axvector *v, bool (*f)(void *, void *), void *arg);
/**
 * Check if vector is sorted according to comparator. Items are checked linearly from first to last.
 * @return True if sorted, false if not.
 */
bool axv_isSorted(axvector *v);
/**
 * Sort vector using its comparator.
 * @return Self.
 */
static inline axvector *axv_sort(axvector *v) {
    qsort(v->items, v->len, sizeof *v->items, v->cmp);
    return v;
}
/**
 * Sort section of vector using its comparator.
 * @param index1 Beginning of section. May be negative. Inclusive.
 * @param index2 End of section. May be negative. Exclusive.
 * @return Self.
 */
axvector *axv_sortSection(axvector *v, int64_t index1, int64_t index2);
/**
 * Binary search the argument in the vector. Only applicable if vector is sorted. No check is done for this.
 * @param val Value to search using the vector's comparator.
 * @return Index of any item which matches the argument or -1 if no such item is found.
 */
static inline int64_t axv_binarySearch(axvector *v, void *val) {
    void **found = (void **) bsearch(&val, v->items, v->len, sizeof *v->items, v->cmp);
    return found ? found - v->items : -1;
}
/**
 * Linear search the argument in the vector. Forward search is used.
 * @param val Value to search using the vector's comparator.
 * @return Index of the first item which matches the argument or -1 if no such item is found.
 */
int64_t axv_linearSearch(axvector *v, void *val);
/**
 * Set comparator function. Type must match int (*)(const void *, const void *).
 * Refer to the C standard on the definition of a compliant comparator function.
 * Keep in mind the comparator is passed pointers to the type of item stored in the vector.
 * So if the vector is storing int * then the comparator is given arguments of type int **.
 * @param cmp Comparator or NULL to activate default comparator (compares addresses).
 * @return Self.
 */
axvector *axv_setComparator(axvector *v, int (*cmp)(const void *, const void *));
/**
 * Get comparator function. Type is int (*)(const void *, const void *).
 * @return Comparator.
 */
static inline int (*axv_getComparator(axvector *v))(const void *, const void *) {
    return v->cmp;
}
/**
 * Set destructor function. Type must match void (*)(void *), which matches i.e. the free() function.
 * @param destroy Destructor or NULL to deactivate destructor features.
 * @return Self.
 */
static inline axvector *axv_setDestructor(axvector *v, void (*destroy)(void *)) {
    v->destroy = destroy;
    return v;
}
/**
 * Get destructor function. Type is void (*)(void *).
 * @return Destructor or NULL if not set.
 */
static inline void (*axv_getDestructor(axvector *v))(void *) {
    return v->destroy;
}
/**
 * Store a context in the vector.
 * @param context Context.
 * @return Self.
 */
static inline axvector *axv_setContext(axvector *v, void *context) {
    v->context = context;
    return v;
}
/**
 * Get the stored context of this vector.
 * @return Context.
 */
static inline void *axv_getContext(axvector *v) {
    return v->context;
}
/**
 * Pointer to first item of this vector. This function is useful when you need raw array access.
 * @return The internal array of this vector.
 */
static inline void **axv_data(axvector *v) {
    return v->items;
}
/**
 * Signed capacity of this vector. The capacity is the maximum number of items that fit without the need of resizing
 * the vector.
 * @return Signed capacity.
 */
static inline int64_t axv_cap(axvector *v) {
    return (int64_t) v->cap;
}
/**
 * Unsigned capacity of this vector. The capacity is the maximum number of items that fit without the need of resizing
 * the vector.
 * @return Unsigned capacity.
 */
static inline uint64_t axv_ucap(axvector *v) {
    return v->cap;
}
/**
 * Lock or unlock this vector. A locked vector's capacity cannot be changed. Operations that try to alter
 * the capacity will fail (as OOM).
 * @param lockState True to lock, false to unlock.
 * @return Self.
 */
static inline axvector *axv_lock(axvector *v, bool lockState) {
    v->locked = lockState;
    return v;
}
/**
 * Check if this vector is locked.
 * @return True if locked, false if not.
 */
static inline bool axv_isLocked(axvector *v) {
    return v->locked;
}
/**
 * Check if this vector is an overlay.
 * @return True if this vector is an overlay, false if not.
 */
static inline bool axv_isOverlay(axvector *v) {
    return v->overlay;
}
/**
 * Set custom memory functions. All three of them must be set and be compatible with one another.
 * Passing NULL for any function will activate its standard library counterpart.
 * @param malloc_fn The malloc function.
 * @param realloc_fn The realloc function.
 * @param free_fn The free function.
 */
void axv_memoryfn(void *(*malloc_fn)(size_t), void *(*realloc_fn)(void *, size_t), void (*free_fn)(void *));

#ifdef __cplusplus
}
#endif

#endif //AXVECTOR_AXVECTOR_H
