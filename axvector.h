//
// Created by easy on 27.10.23.
//

#ifndef AXVECTOR_AXVECTOR_H
#define AXVECTOR_AXVECTOR_H

#include <stdbool.h>
#include <stdint.h>

/*
    axvector is a dynamic vector/array library that has some functional programming concepts and some useful utility
    functions built in. It's designed in an OO-esque style. All functions are accessed through the global variable axv.
    This variable acts as a pseudo-namespace and may be renamed by defining a global macro AXVECTOR_NAMESPACE.

    Some functions use a comparator. The default comparator compares the addresses of items, but a custom
    comparator can be given and shall conform to the C standard library's comparison function specifications.

    A destructor function may also be supplied. There is no default destructor. The destructor will be called on items
    that are _irrevocably_ removed from the vector. Its prototype is void axv_(void *), like the free() function.

    To provide built-in bookkeeping, the vector also has storage for a context. This is simply a void *. The context is
    not used by the vector itself and is exclusively controlled by the user.

    All functions on axvector return something. If the meaning of a function doesn't allow for any sensible return
    value, that function will simply return the passed axvector instance instead of void.

    axvector supports negative indexing: -1 is the last item, -2 the penultimate one etc. All functions that take
    two indices to signify a section treat the first index inclusively and the second index exclusively.
*/
typedef struct axvector axvector;

/*
    Snapshots capture the current state of an axvector for iteration purposes. A snapshot consists of an index
    initialised to 0, the current length and a pointer to the first item.

    Snapshots are the quickest way of iterating a vector, but they do not update their state, so the programmer must
    be wary of not invalidating the axv_snapshot.

    Consider using the provided higher-order functions instead if this is not a tight loop.
*/
typedef struct axvsnap {
    int64_t i;         // index (initialised to 0), increment this in your loop
    int64_t len;       // length of vector at time of taking axv_snapshot, best not to change manually
    void **vec;        // pointer to first element at time of taking axv_snapshot, best not to change manually
} axvsnap;

// this struct contains all user-visible functions of the axvector library
//struct axvectorFn {
    // create axvector with given size, returns NULL iff OOM
    axvector *axv_sizedNew(uint64_t size);
    // create axvector with default size, returns NULL iff OOM
    axvector *axv_new(void);
    // call destructor on all items if available, then axv_destroy axvector and return context
    void *axv_destroy(axvector *v);
    // increment reference counter. This function is not thread-safe
    axvector *axv_iref(axvector *v);
    // decrement reference counter. Returns whether the axvector has been destroyed. This function is not thread-safe
    bool axv_dref(axvector *v);
    // reference counter state
    int64_t axv_refs(axvector *v);
    // returns a axv_snapshot of this vector
    axvsnap axv_snapshot(axvector *v);
    // push item at end of vector, true iff OOM
    bool axv_push(axvector *v, void *val);
    // pop item at end of vector; destructor not called
    void *axv_pop(axvector *v);
    // get topmost item without removing
    void *axv_top(axvector *v);
    // length (number of items) in this axvector. It's advised not to use this function in a tight loop and
    // consider the use of snapshots or higher-order functions such as foreach() instead
    int64_t axv_len(axvector *v);
    // index and return item
    void *axv_at(axvector *v, int64_t index);
    // set item at index, true iff out of range; destructor not called
    bool axv_set(axvector *v, int64_t index, void *val);
    // swap two items by index, true iff index out of range
    bool axv_swap(axvector *v, int64_t index1, int64_t index2);
    // axv_reverse order of items, return this vector
    axvector *axv_reverse(axvector *v);
    // axv_reverse a section of items, true iff out of range
    bool axv_reverseSection(axvector *v, int64_t index1, int64_t index2);
    // rotate vector items by n places to the right; negative n rotates left; returns this vector
    axvector *axv_rotate(axvector *v, int64_t n);
    // if n > 0, shift items starting at index right by n places, zero out memory in-between, true iff OOM.
    // if n < 0, let m = -n: remove (and possibly destruct) m items starting at index and shift subsequent elements
    // by m places to the left.
    // This function is useful to reserve indexable space (n > 0) or to remove items in the middle (n < 0)
    // shift([0 1 2 3 4 5 6], 2, +3) = [0 1 0 0 0 2 3 4 5 6]
    // shift([0 1 2 3 4 5 6], 2, -3) = [0 1 5 6]
    bool axv_shift(axvector *v, int64_t index, int64_t n);
    // call destructor on the top n items if available, then remove those items
    int64_t axv_discard(axvector *v, int64_t n);
    // call destructor on all items if available, then remove all items; returns this vector
    axvector *axv_clear(axvector *v);
    // return a copy of this axvector (destructor not copied); NULL iff OOM
    axvector *axv_copy(axvector *v);
    // append all items of second vector to first vector, thereby clearing the second vector.
    // No destructors are called. Does nothing when vectors are the same. True iff OOM
    bool axv_extend(axvector *v1, axvector *v2);
    // concatenate all items of second vector to first vector, with no changes to second vector.
    // True iff OOM
    bool axv_concat(axvector *v1, axvector *v2);
    // return a copy of a section of this axvector (destructor not copied); NULL iff OOM
    axvector *axv_slice(axvector *v, int64_t index1, int64_t index2);
    // return a copy of a section of this axvector in axv_reverse order (destructor not copied); NULL iff OOM
    axvector *axv_rslice(axvector *v, int64_t index1, int64_t index2);
    // set capacity to some value thereby calling the destructor on excess items when shrinking. True iff OOM,
    // changing length of vector and calling of destructors is done regardless of fail or not
    bool axv_resize(axvector *v, uint64_t size);
    // call this vector's destructor if available on item, return this vector
    axvector *axv_destroyItem(axvector *v, void *val);
    // return max item through linear search; NULL if empty
    void *axv_max(axvector *v);
    // return min item through linear search; NULL if empty
    void *axv_min(axvector *v);
    // false iff f(x, arg) for no item x or axvector empty. Stops at first true return value
    bool axv_any(axvector *v, bool axv_f(const void *, void *), void *arg);
    // true iff f(x, arg) for all items x or axvector empty. Stops at first false return value
    bool axv_all(axvector *v, bool axv_f(const void *, void *), void *arg);
    // number of items that compare equal to passed value according to comparator
    int64_t axv_count(axvector *v, void *val);
    // compares two axvectors' contents with the first vector's comparator;
    // true iff vectors have same length and all items compare equal
    bool axv_compare(axvector *v1, axvector *v2);
    // map function f to all items; f returns a value that will overwrite the item at the current index;
    // returns this vector
    axvector *axv_map(axvector *v, void *axv_f(void *));
    // only keep items that satisfy condition f and call destructor on all other items; returns this vector
    axvector *axv_filter(axvector *v, bool axv_f(const void *, void *), void *arg);
    // only keep items that satisfy condition f and return a new axvector containing all other items;
    // returns the new vector or NULL iff OOM
    axvector *axv_filterSplit(axvector *v, bool axv_f(const void *, void *), void *arg);
    // call f(x, arg) for every item x  with user-supplied argument arg
    // until f returns false or all items have been exhausted. Returns arg
    axvector *axv_foreach(axvector *v, bool axv_f(void *, void *), void *arg);
    // call f(x, arg) for every item x with user-supplied argument arg in axv_reverse order
    // until f returns false or all items have been exhausted. Returns arg
    axvector *axv_rforeach(axvector *v, bool axv_f(void *, void *), void *arg);
    // call f(x, arg) for every item x with user-supplied argument arg for some given section
    // until f returns false or all items have been exhausted. Returns arg
    axvector *axv_forSection(axvector *v, bool axv_f(void *, void *), void *arg,
                            int64_t index1, int64_t index2);
    // true iff all items in order according to comparator
    bool axv_isSorted(axvector *v);
    // sort vector using comparator, return vector
    axvector *axv_sort(axvector *v);
    // sort some section using comparator, return this vector
    axvector *axv_sortSection(axvector *v, int64_t index1, int64_t index2);
    // return index of some item that compares equal to the passed value using comparator and binary search
    // or -1 if item not found. Items must be sorted!
    int64_t axv_binarySearch(axvector *v, void *val);
    // return index of first item that compares equal to the passed value using comparator and linear search
    // or -1 if item not found
    int64_t axv_linearSearch(axvector *v, void *val);
    // return index of first item that compares equal to the passed value using comparator and linear search
    // in some section or -1 if item not found in that section
    int64_t axv_linearSearchSection(axvector *v, void *val, int64_t index1, int64_t index2);
    // set comparator function (passing NULL will activate default comparator); returns this vector
    axvector *axv_setComparator(axvector *v, int axv_comp(const void *, const void *));
    // get comparator function [type: int (*)(const void *, const void *)]
    int (*axv_getComparator(axvector *v))(const void *, const void *);
    // set destructor function (passing NULL will disable destructor);
    axvector *axv_setDestructor(axvector *v, void axv_destroy(void *));
    // get destructor function [type: void (*)(void *)]
    void (*axv_getDestructor(axvector *v))(void *);
    // set context
    axvector *axv_setContext(axvector *v, void *context);
    // get context
    void *axv_getContext(axvector *v);
    // pointer to first item for direct access
    void **axv_data(axvector *v);
    // capacity (number of items that can fit without resizing) in this axvector
    int64_t axv_cap(axvector *v);
//};

// access all axvector functions through this struct as a simulated namespace
//extern const struct axvectorFn axv;

#endif //AXVECTOR_AXVECTOR_H
