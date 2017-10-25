/* File Name : bpt.c
 * Author    : Kim, Myeong Jae
 * Due Date  : 2017-11-5
 *
 * This is a implementation of disk-based b+tree */

#include "bpt.h"

#include <assert.h>
#include <string.h>
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

// Clearing page.
const uint8_t empty_page_dummy[PAGE_SIZE] = {0};

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

void clear_page(const uint64_t page_number) {
  go_to_page_number(page_number);
  if (write(db, empty_page_dummy, PAGE_SIZE) < 0) {
    perror("(clear_page)");
    assert(false);
    exit(1);
  }
}

// Delete page and return page to free page list.
// Clear the page (fill whole page zero) and insert next free page offset.
// Deleted page will be the head of free page list.
bool delete_page(const uint64_t page_number){

  //TODO: Return the deleted page to free list.
  // Reinitialize first 8 byte zero.
  // Deleted page will be head of free list.
  if (page_number <= 1) {
    fprintf(stderr, "(delete_page) Warning: Header page or Root page \
cannot be deleted.\n");
    fprintf(stderr, "(delete_page) Deletion failed.\n");
    return false;
  } else if(page_number
      >= header_page->get_number_of_pages(header_page)) {
    fprintf(stderr, "(delete_page) Warning: Page number is \
bigger than last page number.\n");
    fprintf(stderr, "(delete_page) Deletion failed.\n");
    return false;
  }
  // If the argument is last free page number, free list will be cycle
  // It is very dangerous.

  // Clear the page
  clear_page(page_number);

  // get current free page head offset
  off64_t current_free_page_head =
    header_page->get_free_page_offset(header_page);

  // Write current head offset to deleted page.
  // The free page offset of header page will pointing this deleted page.
  go_to_page_number(page_number);
  if (write(db, &current_free_page_head, sizeof(current_free_page_head)) < 0) {
    perror("(delete_page)");
    assert(false);
    exit(1);
  }

  
  // The free page offset of header page will pointing this deleted page.
  header_page->set_free_page_offset(header_page, page_number * PAGE_SIZE);

  return true;
}
  

// It is called when a free page list is empty.
// ** At least one free page must exist always **
void add_free_page(){
  const int8_t N_OF_ADDED_PAGE = 10;
  int8_t i;

  // Ten free page will be added
  off64_t current_free_page = -1;
  off64_t next_free_page = header_page->get_free_page_offset(header_page);
  int32_t read_bytes;

  // At least one free page must exist.
  assert(next_free_page != 0);
  assert(next_free_page % PAGE_SIZE == 0);

  // Find last page
  do {
    current_free_page = next_free_page;
    go_to_page_number(current_free_page / PAGE_SIZE);

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

  // add free pages
  for (i = 0; i < N_OF_ADDED_PAGE; ++i) {
    go_to_page_number(current_free_page / PAGE_SIZE);
    if (write(db, &next_free_page, sizeof(next_free_page)) < 0) {
      perror("(add_free_page)");
      assert(false);
      exit(1);
    }

    current_free_page = next_free_page;
    next_free_page += PAGE_SIZE;
  }


  // increase number of pages
  header_page->set_number_of_pages(header_page, 
      header_page->get_number_of_pages(header_page) + N_OF_ADDED_PAGE);


#ifdef DBG
  printf("(add_free_page) Ten free page is added: Page Number %ld\n",
      header_page->get_number_of_pages(header_page));
#endif

}


// This function returns a page number
// Get a page from free list
uint64_t page_alloc(){
  off64_t current_free_page_offset;
  off64_t next_free_page_offset = 0;

  //  Make sure at least one free page is left.
  //  Below while loop is executed maximum two.
  //  If a head of free page list is not the last free page,
  // we will meet the 'break;' 
  //  If not, ten free page will be added,
  // and we will meet the 'break;' in second loop
  uint8_t i = 0;
  while(1) {
    // Error check
    // While loop cannot be iterated more than 2
    assert(i < 2);

    current_free_page_offset = header_page->get_free_page_offset(header_page);

    // Check whether it is correct offset
    assert(current_free_page_offset % PAGE_SIZE == 0);
    go_to_page_number(current_free_page_offset / PAGE_SIZE);

    // If next_free_page_offset is zero, it is last page.
    // Add free page.
    if (read(db, &next_free_page_offset, sizeof(next_free_page_offset)) < 0) {
      perror("(page_alloc)");
      assert(false);
      exit(1);
    }

    if (next_free_page_offset == 0) {
#ifdef DBG
      printf("(page_alloc) Current free page is last one.\n");
      printf("(page_alloc) Add new free page and use it.\n");
#endif
      add_free_page();
    } else {
      // Current free page is not a last one. use this page.
      break;
    }
    i++;
  }

  // Change free page offset of header_page
  // Clear current free page and fill it.
  //
  // Next free page offset will be the free page offset of header_page
  header_page->set_free_page_offset(header_page, next_free_page_offset);

  // Go to current free page and fill it.
  clear_page(current_free_page_offset / PAGE_SIZE);
  
  // move file offset to allocated page
  go_to_page_number(current_free_page_offset / PAGE_SIZE);

  return current_free_page_offset / PAGE_SIZE;
}


// This function returns a page number
// Get a page from free list
// Leaf and internal page has same structure.
uint64_t leaf_or_internal_page_alloc(const uint64_t parent_page_number,
    const uint32_t is_leaf, const uint64_t one_more_page_number) {
  uint64_t new_page_number = page_alloc();
  page_header_t page_header;
  memset(&page_header, 0, sizeof(page_header));

  // parent page number should be smaller than # of pages.
  assert(parent_page_number < header_page->get_number_of_pages(header_page));
  page_header.linked_page_offset = parent_page_number * PAGE_SIZE;
  page_header.is_leaf = is_leaf;
  page_header.one_more_page_offset = one_more_page_number * PAGE_SIZE;

  // write header to disk
  go_to_page_number(new_page_number);
  write_page_header(&page_header);

  return new_page_number;
}


// This is a wrapper of allocation function
uint64_t leaf_page_alloc(const uint64_t parent_page_number,
    const uint64_t right_sibling_page_number) {
  return leaf_or_internal_page_alloc(parent_page_number,
      1, right_sibling_page_number);
}

// This is a wrapper of allocation function
uint64_t internal_page_alloc(const uint64_t parent_page_number,
    const uint64_t one_more_page_number) {
  return leaf_or_internal_page_alloc(parent_page_number,
      0, one_more_page_number);
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




