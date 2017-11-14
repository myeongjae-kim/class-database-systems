/* File Name : bpt.h
 * Author    : Kim, Myeong Jae
 * Due Date  : 2017-11-5
 *
 * This is a header file of disk-based b+tree.
 * The interface this data structure provide is below. */


#ifndef __BPT_H__
#define __BPT_H__

// Allow system to handle a file which is bigger than 4GB
#define _LARGEFILE64_SOURCE

// If fsync(x); is defined, DB will work as buffered I/O
// #define fsync(x);
// #define assert(x);

#define DBG
// #define TESTING


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <unistd.h>
#include <fcntl.h>

#include "bpt_header_object.h"


#define PAGE_SIZE 4096

// Generally, 'offset' means 'page number * PAGE_SIZE'

#define RECORD_PER_PAGE ((PAGE_SIZE - sizeof(page_header_t))\
    / sizeof(record_t))
#define RECORD_ORDER (RECORD_PER_PAGE + 1)

#define OFFSETS_PER_PAGE ((PAGE_SIZE - sizeof(page_header_t))\
    / sizeof(internal_page_element_t))
#define OFFSET_ORDER (OFFSETS_PER_PAGE + 1)


/* page header and record */
#define HEADER_DUMMY_SIZE (120 - (8 + 4 + 4))
// The size of page_header_t is 128 byte.
typedef struct __page_header {
  // This can be ued as:
  //  1. Next free page offset (in free page)
  //  2. Parent page offset (in leaf or internal page)
  int64_t linked_page_offset;
  
  int32_t is_leaf;
  int32_t number_of_keys;
  char dummy[HEADER_DUMMY_SIZE];

  // This can be used as:
  //   1. Right Sibling Page Offset (Leaf Page)
  //   2. One More Page Offset (Internal Page)
  int64_t one_more_page_offset;
} page_header_t;


#define VALUE_SIZE (128 - 8)
typedef struct __record {
  int64_t key;
  char value[VALUE_SIZE];
} record_t;


// This type is used in internal page
typedef struct __internal_page_element {
  int64_t key;
  int64_t page_offset;
} internal_page_element_t;



/*** Interface ***/

// Open database
// If there is no file, create a db and initialize.
int open_table (char *pathname);


int insert (int table_id, int64_t key, char *value);


char * find (int table_id, int64_t key);


int delete (int table_id, int64_t key);

/*** Interface Ends ***/


/** Internal Functions
 * These functions are only used in bpt.c
 * Below declarations will be deleted in the end.
 * **/

/** Utility functions **/

/* It is registered as exit handler. It closes a database file. */
void close_db(void);

/* This function returns the number of current page  */
// int64_t get_current_page_number(void);

/* This function moves file offset to target page */
void go_to_page_number(const int32_t table_id, const int64_t page_number);

/* This function prints haeder page */
void print_header_page(const int32_t table_id);

/* This function prints a content of page including header */
// void print_page(const int32_t table_id, const int64_t page_number);

/* This function writes a header.
 * It checks whether current position is header or not. */
void write_page_header(const page_header_t * const header);

/* Intilizing table */
void init_table(int table_id);

/* close table */
int close_table(int table_id);

void print_all(int32_t table_id);

#endif
