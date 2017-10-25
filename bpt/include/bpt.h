/* File Name : bpt.h
 * Author    : Kim, Myeong Jae
 * Due Date  : 2017-11-5
 *
 * This is a header file of disk-based b+tree
 * The interface this data structure provide is below. */


#ifndef __BPT_H__
#define __BPT_H__

// Allow system to handle a file which is bigger than 4GB
#define _LARGEFILE64_SOURCE


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <unistd.h>
#include <fcntl.h>

#define DBG

#define PAGE_SIZE 4096

// Generally, 'offset' means 'page number * PAGE_SIZE'

/* page header and record */
#define HEADER_DUMMY_SIZE (120 - (8 + 4 + 4))
// The size of page_header_t is 128 byte.
typedef struct __page_header {
  // This can be ued as:
  //  1. Next free page offset (in free page)
  //  2. Parent page offset (in leaf or internal page)
  uint64_t linked_page_offset;
  
  uint32_t is_leaf;
  uint32_t number_of_keys;
  const char dummy[HEADER_DUMMY_SIZE];

  // This can be used as:
  //   1. Right Sibling Page Offset (Leaf Page)
  //   2. One More Page Offset (Internal Page)
  uint64_t one_more_page_offset;
} page_header_t;


#define VALUE_SIZE (128 - 8)
typedef struct __record {
  uint64_t key;
  char value[VALUE_SIZE];
} record_t;


// This type is used in internal page
typedef struct __internal_page_element {
  uint64_t key;
  uint64_t page_offset;
} internal_page_element_t;


/****** Pages ******/

// It is a structure of header page.
typedef struct __header_page {
  uint64_t free_page_offset;
  uint64_t root_page_offset;
  uint64_t number_of_pages;
} header_page_t;

#define RECORD_PER_PAGE ((PAGE_SIZE - sizeof(page_header_t))\
    / sizeof(record_t))
typedef struct __leaf_page {
  page_header_t header;
  record_t records[RECORD_PER_PAGE];
} leaf_page_t;

#define OFFSETS_PER_PAGE ((PAGE_SIZE - sizeof(page_header_t))\
    / sizeof(internal_page_element_t))
typedef struct __internal_page {
  page_header_t header;
  internal_page_element_t offsets[OFFSETS_PER_PAGE];
} internal_page_t;




// Below type will not be used.
/* typedef struct __free_page {
 *   uint64_t next_free_page_offset;
 * } free_page_t; */


/*** Interface ***/

// Open database
// If there is no file, create a db and initialize.
int open_db (char *pathname);


int insert (int64_t key, char *value);


char * find (int64_t key);


int delete (int64_t key);

/*** Interface Ends ***/


/** Internal Functions
 * These functions are only used in bpt.c
 * Below declarations will be commenterized.
 * **/

/** Utility functions **/

/* It is registered as exit handler. It closes a database file. */
void close_db(void);

/* This function returns the number of current page  */
uint64_t get_current_page_number(void);

/* This function moves file offset to target page */
void go_to_page(const uint64_t page_number);

/* This function prints haeder page */
void print_header_page(void);

/* This function prints a content of page including header */
void print_page(const uint64_t page_number);

/* This function writes a header.
 * It checks whether current position is header or not. */
void write_page_header(const page_header_t * const header);

/* Intilizing database */
void initialize_db(void);



//TODO: Implement

// delete page and return page to free page list.
bool delete_page(const uint64_t page_number);
  

// This function returns an offset of new free page.
// It is called when a free page list is empty.
// At least one free page must exist always.
void add_free_page();


// This function returns a page number
// Get a page from free list
// Returned page will be clean.
uint64_t page_alloc();


// This function returns a page number
// Get a page from free list
uint64_t leaf_or_internal_page_alloc(const uint64_t parent_page_number,
    const uint32_t is_leaf, const uint64_t one_more_page_number);


// This function returns a page number
// Get a page from free list
uint64_t leaf_page_alloc(const uint64_t parent_page_number,
    const uint64_t right_sibling_page_number);

// This function returns a page number
// Get a page from free list
uint64_t internal_page_alloc(const uint64_t parent_page_number,
    const uint64_t one_more_page_number);




#endif
