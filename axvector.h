//
// Created by easy on 27.10.23.
//

#ifndef AXVECTOR_AXVECTOR_H
#define AXVECTOR_AXVECTOR_H

#include <stdbool.h>

/*
    AXvector is a dynamic vector/array library that has some functional programming concepts and some useful utility
    functions built in. It is designed in an OOP style. All functions are accessed through the global variable axv.

    For this, a comparator function is used. The default comparator compares the addresses of items, but a custom
    comparator can be given and shall conform to the C standard library's comparison function specifications.

    A destructor function may also be supplied. There is no default destructor. The destructor will be called on items
    that are *irrevocably* removed from the vector. It's prototype is void (*)(void *), like the free() function.

    To provide built-in flexibility, the vector also has storage for a context. This is simply a void *. The context is
    not used by the vector itself and is exclusively controlled by the user.

    All functions on AXvector return something. If the meaning of a function doesn't allow for any sensible return
    value, that function will simply return the passed AXvector instance instead of void.

    AXvector supports negative indexing: -1 is the last item, -2 the penultimate one etc. All functions that take
    two indices to signify a section treat the first index inclusively and the second index exclusively.
*/
typedef struct AXvector AXvector;

typedef struct AXvectorFuncs {
    // create AXvector with given size
    AXvector *(*sizedNew)(unsigned long size);
    // create AXvector with default size
    AXvector *(*new)(void);
    // call destructor on all items if available, then destroy AXvector and return context
    void *(*destroy)(AXvector *v);
    // push item at end of vector, true iff OOM
    bool (*push)(AXvector *v, void *val);
    // pop item at end of vector
    void *(*pop)(AXvector *v);
    // get topmost item without removing
    void *(*top)(AXvector *v);
    // length (number of items) in this AXvector
    long (*len)(AXvector *v);
    // index and return item
    void *(*at)(AXvector *v, long index);
    // set item at index, true iff out of range
    bool (*set)(AXvector *v, long index, void *val);
    // swap two items by index
    bool (*swap)(AXvector *v, long index1, long index2);
    // reverse order of items
    AXvector *(*reverse)(AXvector *v);
    // reverse a section of items, true iff out of range
    bool (*reverseSection)(AXvector *v, long index1, long index2);
    // rotate vector items by n places to the right; negative n rotates left
    AXvector *(*rotate)(AXvector *v, long n);
    // shift all items starting at index to the right by n places, true iff OOM
    bool (*shift)(AXvector *v, long index, unsigned long n);
    // call destructor on all items if available. then remove all items
    AXvector *(*clear)(AXvector *v);
    // return a copy of this AXvector (destructor not copied); NULL iff OOM
    AXvector *(*copy)(AXvector *v);
    // return a copy of a section of this AXvector (destructor not copied); NULL iff OOM
    AXvector *(*slice)(AXvector *v, long index1, long index2);
    // return a copy of a section of this AXvector in reverse order (destructor not copied); NULL iff OOM
    AXvector *(*rslice)(AXvector *v, long index1, long index2);
    // set capacity to some value thereby calling the destructor on excess items when shrinking. True iff OOM,
    // changing length of vector and calling of destructors is done regardless of fail or not
    bool (*resize)(AXvector *v, unsigned long size);
    // call destructor if available on item
    AXvector *(*destroyItem)(AXvector *v, void *val);
    // return max item through linear search; NULL if empty
    void *(*max)(AXvector *v);
    // return min item through linear search; NULL if empty
    void *(*min)(AXvector *v);
    // true iff f(x) for any item x, false if AXvector empty. Stops at first true return value
    bool (*any)(AXvector *v, bool (*f)(const void *));
    // true iff f(x) for all items x or AXvector empty. Stops at first false return value
    bool (*all)(AXvector *v, bool (*f)(const void *));
    // number of items that compare equal to passed value according to comparator
    long (*count)(AXvector *v, void *val);
    // compares two AXvectors' contents with the first vector's comparator;
    // true iff vectors have same length and all items compare equal
    bool (*compare)(AXvector *v1, AXvector *v2);
    // map function f to all items; f returns a value that will overwrite the item at the current index
    AXvector *(*map)(AXvector *v, void *(*f)(void *));
    // only keep items that satisfy condition f and call destructor on all other items
    AXvector *(*filter)(AXvector *v, bool (*f)(const void *));
    // only keep items that satisfy condition f and return a new AXvector containing all other items
    AXvector *(*filterSplit)(AXvector *v, bool (*f)(const void *));
    // call f(x, i, arg) for every item x at index i with user-supplied argument arg
    // until f returns false or all items have been exhausted
    void *(*foreach)(AXvector *v, bool (*f)(void *, long, void *), void *arg);
    // call f(x, i, arg) for every item x at index i with user-supplied argument arg in reverse order
    // until f returns false or all items have been exhausted
    void *(*rforeach)(AXvector *v, bool (*f)(void *, long, void *), void *arg);
    // call f(x, i, arg) for every item x at index i with user-supplied argument arg for some given section
    // until f returns false or all items have been exhausted
    AXvector *(*forSection)(AXvector *v, bool (*f)(void *, long, void *), void *arg,
                            long index1, long index2);
    // true iff all items in order according to comparator
    bool (*isSorted)(AXvector *v);
    // sort vector using comparator
    AXvector *(*sort)(AXvector *v);
    // sort some section using comparator
    AXvector *(*sortSection)(AXvector *v, long index1, long index2);
    // return index of some item that compares equal to the passed value using comparator and binary search
    // or -1 if item not found. Items must be sorted!
    long (*binarySearch)(AXvector *v, void *val);
    // return index of first item that compares equal to the passed value using comparator and linear search
    // or -1 if item not found
    long (*linearSearch)(AXvector *v, void *val);
    // return index of first item that compares equal to the passed value using comparator and linear search
    // in some section or -1 if item not found in that section
    long (*linearSearchSection)(AXvector *v, void *val, long index1, long index2);
    // true iff some item compares equal to passed value according to comparator. Passing true for the sorted
    // parameter will use binary instead of linear search
    bool (*element)(AXvector *v, void *val, bool sorted);
    // set comparator function (passing NULL will activate default comparator)
    AXvector *(*setComparator)(AXvector *v, int (*comp)(const void *, const void *));
    // get comparator function
    int (*(*getComparator)(AXvector *v))(const void *, const void *);
    // set destructor function (passing NULL will disable destructor)
    AXvector *(*setDestructor)(AXvector *v, void (*destroy)(void *));
    // get destructor function
    void (*(*getDestructor)(AXvector *v))(void *);
    // set context
    AXvector *(*setContext)(AXvector *v, void *userdata);
    // get context
    void *(*getContext)(AXvector *v);
    // pointer to first item for direct access
    void **(*data)(AXvector *v);
    // capacity (number of items that can fit without resizing) in this AXvector
    long (*cap)(AXvector *v);
} AXvectorFuncs;

extern const AXvectorFuncs axv;

#endif //AXVECTOR_AXVECTOR_H
