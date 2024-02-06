//
// Created by easy on 27.10.23.
//

#ifndef AXVECTOR_AXVECTOR_H
#define AXVECTOR_AXVECTOR_H

#include <stdbool.h>

/*
    axvector is a dynamic vector/array library that has some functional programming concepts and some useful utility
    functions built in. It's designed in an OO-esque style. All functions are accessed through the global variable axv.
    This variable acts as a pseudo-namespace and may be renamed by defining a global macro AXVECTOR_NAMESPACE.

    Some functions use a comparator. The default comparator compares the addresses of items, but a custom
    comparator can be given and shall conform to the C standard library's comparison function specifications.

    A destructor function may also be supplied. There is no default destructor. The destructor will be called on items
    that are _irrevocably_ removed from the vector. Its prototype is void (*)(void *), like the free() function.

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
    be wary of not invalidating the snapshot.

    Consider using the provided higher-order functions instead if this is not a tight loop.
*/
typedef struct axvsnap {
    long i;         // index (initialised to 0), increment this in your loop
    long len;       // length of vector at time of taking snapshot, best not to change manually
    void **vec;     // pointer to first element at time of taking snapshot, best not to change manually
} axvsnap;

// this struct contains all user-visible functions of the axvector library
struct axvectorFn {
    // create axvector with given size, returns NULL iff OOM
    axvector *(*sizedNew)(unsigned long size);
    // create axvector with default size, returns NULL iff OOM
    axvector *(*new)(void);
    // call destructor on all items if available, then destroy axvector and return context
    void *(*destroy)(axvector *v);
    // increment reference counter. This function is not thread-safe
    axvector *(*iref)(axvector *v);
    // decrement reference counter. Returns whether the axvector has been destroyed. This function is not thread-safe
    bool (*dref)(axvector *v);
    // reference counter state
    long (*refs)(axvector *v);
    // returns a snapshot of this vector
    axvsnap (*snapshot)(axvector *v);
    // push item at end of vector, true iff OOM
    bool (*push)(axvector *v, void *val);
    // pop item at end of vector; destructor not called
    void *(*pop)(axvector *v);
    // get topmost item without removing
    void *(*top)(axvector *v);
    // length (number of items) in this axvector. It's advised not to use this function in a tight loop and
    // consider the use of snapshots or higher-order functions such as foreach() instead
    long (*len)(axvector *v);
    // index and return item
    void *(*at)(axvector *v, long index);
    // set item at index, true iff out of range; destructor not called
    bool (*set)(axvector *v, long index, void *val);
    // swap two items by index, true iff index out of range
    bool (*swap)(axvector *v, long index1, long index2);
    // reverse order of items, return this vector
    axvector *(*reverse)(axvector *v);
    // reverse a section of items, true iff out of range
    bool (*reverseSection)(axvector *v, long index1, long index2);
    // rotate vector items by n places to the right; negative n rotates left; returns this vector
    axvector *(*rotate)(axvector *v, long n);
    // if n > 0, shift items starting at index right by n places, zero out memory in-between, true iff OOM.
    // if n < 0, let m = -n: remove (and possibly destruct) m items starting at index and shift subsequent elements
    // by m places to the left.
    // This function is useful to reserve indexable space (n > 0) or to remove items in the middle (n < 0)
    // shift([0 1 2 3 4 5 6], 2, +3) = [0 1 0 0 0 2 3 4 5 6]
    // shift([0 1 2 3 4 5 6], 2, -3) = [0 1 5 6]
    bool (*shift)(axvector *v, long index, long n);
    // call destructor on the top n items if available, then remove those items
    long (*discard)(axvector *v, long n);
    // call destructor on all items if available, then remove all items; returns this vector
    axvector *(*clear)(axvector *v);
    // return a copy of this axvector (destructor not copied); NULL iff OOM
    axvector *(*copy)(axvector *v);
    // append all items of second vector to first vector, thereby clearing the second vector.
    // No destructors are called. Does nothing when vectors are the same. True iff OOM
    bool (*extend)(axvector *v1, axvector *v2);
    // concatenate all items of second vector to first vector, with no changes to second vector.
    // True iff OOM
    bool (*concat)(axvector *v1, axvector *v2);
    // return a copy of a section of this axvector (destructor not copied); NULL iff OOM
    axvector *(*slice)(axvector *v, long index1, long index2);
    // return a copy of a section of this axvector in reverse order (destructor not copied); NULL iff OOM
    axvector *(*rslice)(axvector *v, long index1, long index2);
    // set capacity to some value thereby calling the destructor on excess items when shrinking. True iff OOM,
    // changing length of vector and calling of destructors is done regardless of fail or not
    bool (*resize)(axvector *v, unsigned long size);
    // call this vector's destructor if available on item, return this vector
    axvector *(*destroyItem)(axvector *v, void *val);
    // return max item through linear search; NULL if empty
    void *(*max)(axvector *v);
    // return min item through linear search; NULL if empty
    void *(*min)(axvector *v);
    // false iff f(x) for no item x or axvector empty. Stops at first true return value
    bool (*any)(axvector *v, bool (*f)(const void *));
    // true iff f(x) for all items x or axvector empty. Stops at first false return value
    bool (*all)(axvector *v, bool (*f)(const void *));
    // number of items that compare equal to passed value according to comparator
    long (*count)(axvector *v, void *val);
    // compares two axvectors' contents with the first vector's comparator;
    // true iff vectors have same length and all items compare equal
    bool (*compare)(axvector *v1, axvector *v2);
    // map function f to all items; f returns a value that will overwrite the item at the current index;
    // returns this vector
    axvector *(*map)(axvector *v, void *(*f)(void *));
    // only keep items that satisfy condition f and call destructor on all other items; returns this vector
    axvector *(*filter)(axvector *v, bool (*f)(const void *, void *), void *arg);
    // only keep items that satisfy condition f and return a new axvector containing all other items;
    // returns the new vector or NULL iff OOM
    axvector *(*filterSplit)(axvector *v, bool (*f)(const void *, void *), void *arg);
    // call f(x, arg) for every item x  with user-supplied argument arg
    // until f returns false or all items have been exhausted. Returns arg
    axvector *(*foreach)(axvector *v, bool (*f)(void *, void *), void *arg);
    // call f(x, arg) for every item x with user-supplied argument arg in reverse order
    // until f returns false or all items have been exhausted. Returns arg
    axvector *(*rforeach)(axvector *v, bool (*f)(void *, void *), void *arg);
    // call f(x, arg) for every item x with user-supplied argument arg for some given section
    // until f returns false or all items have been exhausted. Returns arg
    axvector *(*forSection)(axvector *v, bool (*f)(void *, void *), void *arg,
                        long index1, long index2);
    // true iff all items in order according to comparator
    bool (*isSorted)(axvector *v);
    // sort vector using comparator, return vector
    axvector *(*sort)(axvector *v);
    // sort some section using comparator, return this vector
    axvector *(*sortSection)(axvector *v, long index1, long index2);
    // return index of some item that compares equal to the passed value using comparator and binary search
    // or -1 if item not found. Items must be sorted!
    long (*binarySearch)(axvector *v, void *val);
    // return index of first item that compares equal to the passed value using comparator and linear search
    // or -1 if item not found
    long (*linearSearch)(axvector *v, void *val);
    // return index of first item that compares equal to the passed value using comparator and linear search
    // in some section or -1 if item not found in that section
    long (*linearSearchSection)(axvector *v, void *val, long index1, long index2);
    // set comparator function (passing NULL will activate default comparator); returns this vector
    axvector *(*setComparator)(axvector *v, int (*comp)(const void *, const void *));
    // get comparator function [type: int (*)(const void *, const void *)]
    int (*(*getComparator)(axvector *v))(const void *, const void *);
    // set destructor function (passing NULL will disable destructor);
    axvector *(*setDestructor)(axvector *v, void (*destroy)(void *));
    // get destructor function [type: void (*)(void *)]
    void (*(*getDestructor)(axvector *v))(void *);
    // set context
    axvector *(*setContext)(axvector *v, void *context);
    // get context
    void *(*getContext)(axvector *v);
    // pointer to first item for direct access
    void **(*data)(axvector *v);
    // capacity (number of items that can fit without resizing) in this axvector
    long (*cap)(axvector *v);
};

#ifdef AXVECTOR_NAMESPACE
#define axv AXVECTOR_NAMESPACE
#endif

// access all axvector functions through this struct as a simulated namespace
extern const struct axvectorFn axv;

#undef axv

#endif //AXVECTOR_AXVECTOR_H
