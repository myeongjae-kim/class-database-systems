/* File Name : bpt.c Author    : Kim, Myeong Jae
 * Due Date  : 2017-11-5
 *
 * This is a implementation of disk-based b+tree */

#include "bpt.h"

#ifdef DBG
#include <assert.h>
#endif

#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "bpt_header_object.h"
#include "bpt_page_object.h"
#include "bpt_free_page_manager.h"

// File descriptor.
int32_t db;

// Current page offset
// Page moving must be occurred via go_to_page function.
static off64_t current_page_start_offset = 0;

// header page always exists.
header_object_t __header_object;
header_object_t *header_page = &__header_object;

// Clearing page.
const uint8_t empty_page_dummy[PAGE_SIZE] = {0};

void clear_resource(void) {
  close(db);
  /** make_free_page_list_compact(); */
}

uint64_t get_current_page_number(void) {
  off64_t current_offset = lseek64(db, 0, SEEK_CUR);
  if (current_offset < 0) {
    perror("(get_current_page_offset)");
    exit(1);
  }
  return current_offset / PAGE_SIZE;
}


// This function change current_page_head of required page.
void go_to_page_number(const uint64_t page_number) {
  current_page_start_offset = lseek64(db, page_number * PAGE_SIZE, SEEK_SET);
  if (current_page_start_offset < 0) {
    perror("(go_to_page) moving file offset failed.");
    exit(1);
  }
}


void print_header_page() {
  header_page->print(header_page);
}


void print_page(const uint64_t page_number) {
  page_header_t header;

  go_to_page_number(page_number);

  memset(&header, 0 , sizeof(header));
  if (read(db, &header, sizeof(header)) < 0) {
    perror("(print_page)");
    exit(1);
  }

  printf(" ** Printing Page Header ** \n");
  printf("   linked_page_offset: %ld\n",
      header.linked_page_offset);
  printf("   is_leaf: %d\n", header.is_leaf);
  printf("   number_of_keys: %d\n",
      header.number_of_keys);
  printf("   one_more_page_offset: %ld\n",
      header.one_more_page_offset);
  printf(" **         End          ** \n");


  //TODO: print other informations
}

// This function writes page header to db.
// linked_page_offset can be a offset of parent or next free page.
void write_page_header(const page_header_t * const header) {
  // File pointer must be at header position
  assert(lseek64(db, 0, SEEK_CUR) % PAGE_SIZE == 0);

  if(write(db, header, sizeof(*header)) < 0) {
    perror("(write_page_header)");
    exit(1);
  }
}


// This function is called when a DB file is first generated.
// Initialize header page and root.
void initialize_db(void) {
  // DB exists
  assert(db >= 0);
  assert(lseek64(db, 0, SEEK_CUR) == 0);
#ifdef DBG
  printf("(initialize_db) DB Initializing start!\n");
#endif

  // Page 0 is header page.
  // Page 1 is free page dummy.
  // Page 2 is Root page.

  // The number of pages includes all kinds of pages.
  // '3' means header page, free page dummy, root page
  
  // Free page offset, Root page offset, number of pages.
  header_page->set(header_page, 1 * PAGE_SIZE, 2 * PAGE_SIZE, 3);

  // Go to root page
  go_to_page_number(2);

  // Write header of root
  // Parent of root is root.

  // DB has at least three pages.
  // 1. Header page
  // 2. Free page dummy
  // 3. Root page

  // Root's parent offset should be zero.
  page_header_t page_header = {0, 1, 0, {0}, 0};
  write_page_header(&page_header);
}

// This function returns a page number
// Get a page from free list
// Leaf and internal page has same structure.
/** uint64_t leaf_or_internal_page_alloc(const uint64_t parent_page_number,
  *     const uint32_t is_leaf, const uint64_t one_more_page_number) {
  *   uint64_t new_page_number = page_alloc();
  *   page_header_t page_header;
  *   memset(&page_header, 0, sizeof(page_header));
  *
  *   // parent page number should be smaller than # of pages.
  *   assert(parent_page_number < header_page->get_number_of_pages(header_page));
  *   page_header.linked_page_offset = parent_page_number * PAGE_SIZE;
  *   page_header.is_leaf = is_leaf;
  *   page_header.one_more_page_offset = one_more_page_number * PAGE_SIZE;
  *
  *   // write header to disk
  *   go_to_page_number(new_page_number);
  *   write_page_header(&page_header);
  *
  *   return new_page_number;
  * }
  *
  *
  * // This is a wrapper of allocation function
  * uint64_t leaf_page_alloc(const uint64_t parent_page_number,
  *     const uint64_t right_sibling_page_number) {
  *   return leaf_or_internal_page_alloc(parent_page_number,
  *       1, right_sibling_page_number);
  * }
  *
  * // This is a wrapper of allocation function
  * uint64_t internal_page_alloc(const uint64_t parent_page_number,
  *     const uint64_t one_more_page_number) {
  *   return leaf_or_internal_page_alloc(parent_page_number,
  *       0, one_more_page_number);
  * } */




// TODO: Not yet debugged.
// This function finds the leaf page that a key will be stored.
uint64_t find_leaf_page(const int key) {
  uint32_t i = 0;
  uint8_t page_buffer[PAGE_SIZE];

  // Get root page
  go_to_page_number(
      header_page->get_root_page_offset(header_page) / PAGE_SIZE);

  memset(page_buffer, 0, PAGE_SIZE);
  if (read(db, page_buffer, PAGE_SIZE) < 0) {
    perror("(find_leaf_page)");
    assert(false);
    exit(1);
  }

  // page object is needed.
  page_object_t page;
  page_object_constructor(&page);
  page.set_current_page_number(&page, 
      header_page->get_root_page_offset(header_page) / PAGE_SIZE);
  page.read(&page);

  // Leaf is not found. Go to found leaf
  while(page.get_type(&page) != LEAF_PAGE) {
    assert(page.get_type(&page) != INVALID_PAGE);

    // Internal page is found
    
    // Check one_more_page_offset
    i = 0;
    while(i < page.get_number_of_keys(&page)) {
      if (key >= page.get_key_and_offset(&page, i)->key) {
        i++;
      } else {
       break;
      }
    }


    // Next page is found. Go to next page
    if (i == 0) {
      page.set_current_page_number(&page, 
          page.page.header.one_more_page_offset / PAGE_SIZE);
    } else {
    // i-1 because of the internal page structure
      page.set_current_page_number(&page, 
          page.get_key_and_offset(&page, i-1)->page_offset / PAGE_SIZE);
    }
    page.read(&page);

  }


  return page.get_current_page_number(&page);
}

/*     Functions related with Insert() End       */


int open_db (char *pathname){
  // setting header page
  header_object_constructor(header_page);


  db = open(pathname, O_RDWR | O_LARGEFILE);
  if (db < 0) {
    // Opening is failed.
#ifdef DBG
    printf("(open_db) DB not exists. Create new one.\n");
#endif
    // Create file and change its permission
    db = open(pathname, O_RDWR | O_CREAT | O_EXCL | O_LARGEFILE);
    if (db < 0) {
      // Creating is failed.
      perror("(open_db) DB opening is failed.");
      fprintf(stderr, "(open_db) Terminate the program\n");
      return -1;
    }

    // Change its permission.
    chmod(pathname, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    // DB is created. Initialize header page and root.
    initialize_db();
  }

  // Register file closing function to prevent memory leakage.
  atexit(clear_resource);

  // Read header
  header_page->read(header_page);

  // Initialize free_page_manger.
  free_page_manager_init();
  return 0;
}


int insert (int64_t key, char *value){
  uint64_t leaf_page;
  page_object_t page;
  page_object_constructor(&page);
#ifdef DBG
  printf("(insert) Start insert. key: %ld, value: %s\n", key, value);
#endif

  /** If same value is exist, do nothing */
  // TODO: Implement find


  /** Create a new record */
  record_t record;
  record.key = key;
  strncpy(record.value, value, sizeof(record.value));


  /** Find a leaf page to insert the value */
  leaf_page = find_leaf_page(key);


  /** If leaf has room for record, insert */
  page.set_current_page_number(&page, leaf_page);
  page.read(&page);
  assert(page.get_type(&page) == LEAF_PAGE);
  if (page.insert_record(&page, &record)) {
    // insertion is successful.
    return 0;
  }

  /** If leaf has no room for record, split and insert */
  return page.insert_record_after_splitting(&page, &record);
}


/** char * find (int64_t key){
  *
  *   return NULL;
  * }
  *
  *
  * int delete (int64_t key){
  *
  *   return 0;
  * } */



