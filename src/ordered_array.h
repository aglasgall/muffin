#ifndef _ORDERED_ARRAY_H_
#define _ORDERED_ARRAY_H_

#include "common.h"

// we're storing pointers and u32ints (and only care about 32bit :))

typedef void* type_t;

// type for predicates for comparing of objects

typedef s8int (*lessthan_predicate_t)(type_t,type_t);

typedef struct ordered_array_struct {
  type_t *array;
  u32int size;
  u32int max_size;
  lessthan_predicate_t less_than;
} ordered_array_t;


// standard predicate
s8int standard_lessthan_predicate(type_t a, type_t b);

// create an ordered array (optionally at a specific address)
ordered_array_t create_ordered_array(u32int max_size, lessthan_predicate_t less_than);
ordered_array_t place_ordered_array(void *address, u32int max_size, lessthan_predicate_t less_than);


// destroy an ordered array
void destroy_ordered_array(ordered_array_t *array);

// insert an item
void insert_ordered_array(type_t item, ordered_array_t *array);

// pull the item at index i

type_t lookup_ordered_array(u32int i, ordered_array_t *array);


// remove the item at index i from the array
void remove_ordered_array(u32int i, ordered_array_t *array);

#endif

