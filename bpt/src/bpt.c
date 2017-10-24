/* File Name : bpt.c
 * Author    : Kim, Myeong Jae
 * Due Date  : 2017-11-5
 *
 * This is a implementation of disk-based b+tree */

#include "bpt.h"

#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "bpt_header_object.h"

// File descriptor.
int32_t db;

// Current page offset
// Page moving must be occurred via go_to_page function.
static off64_t current_page_start_offset = 0;

// header page always exists.
header_object_t __header_object;
header_object_t *header_page = &__header_object;

void close_db(void) {
  close(db);
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
  printf("   linked_page_offset: %ld\n",
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
  // Page 1 is Root page.
  // Page 2 is the start of free page.

  // The number of pages includes all kinds of pages.
  // '3' means header page, root page, first free page.
  
  // Free page offset, Root page offset, number of pages.
  header_page->set(header_page, 2 * PAGE_SIZE, 1 * PAGE_SIZE, 3);

  // Go to root page
  go_to_page_number(1);

  // Write header of root
  // Parent of root is root.

  // DB has at least three pages.
  // 1. Header page
  // 2. Leaf page
  // 3. Free page

  page_header_t page_header = {1 * PAGE_SIZE, 3, 0, {0}, 0};
  write_page_header(&page_header);

  // Go to free page
  go_to_page_number(2);
  // Do nothing. Here is the end of free page.
}


// delete page and return page to free page list.
bool delete_page(uint64_t page_number){

  //TODO: Return the deleted page to free list.
  // Reinitialize first 8 byte zero.
  return false;
}
  

// This function returns an offset of new free page.
// It is called when a free page list is empty.
// ** At least one free page must exist always **
void add_free_page(){
  uint64_t current_free_page = -1;
  uint64_t next_free_page = header_page->get_free_page_offset(header_page);
  int32_t read_bytes;

  // At least one free page must exist.
  assert(next_free_page != 0);

  // Find last page
  do {
    current_free_page = next_free_page;
    if (lseek64(db, current_free_page, SEEK_SET) < 0) {
      perror("(add_free_page)");
      assert(false);
      exit(1);
    }
    // File offset is on free page
    // Read next page offset

    read_bytes = read(db, &next_free_page, sizeof(next_free_page));
    if (read_bytes < 0) {
      perror("(add_free_page)");
      assert(false);
      exit(1);
    }
  } while(next_free_page != 0 && read_bytes != 0);

  // current_free_page is last page offset because its content is zero.
  next_free_page = header_page->get_number_of_pages(header_page) * PAGE_SIZE;

  // add next free page
  if (write(db, &next_free_page, sizeof(next_free_page)) < 0) {
    perror("(add_free_page)");
    assert(false);
    exit(1);
  }

  // increase number of pages
  header_page->set_number_of_pages(header_page, 
      header_page->get_number_of_pages(header_page) + 1);
}


// This function returns a page number
// Get a page from free list
uint64_t leaf_page_alloc(){
  return 0;
}

// This function returns a page number
// Get a page from free list
uint64_t internal_page_alloc(){
  return 0;
}






















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
  atexit(close_db);

  // TODO: Read database

  // Read header
  header_page->read(header_page);


  return 0;
}


int insert (int64_t key, char *value){

  return 0;
}


char * find (int64_t key){

  return NULL;
}


int delete (int64_t key){

  return 0;
}




