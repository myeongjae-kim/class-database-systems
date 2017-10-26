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

  page_header_t page_header = {1 * PAGE_SIZE, 1, 0, {0}, 0};
  write_page_header(&page_header);

  // Go to free page
  go_to_page_number(2);
  // Do nothing. Here is the head and tail of free page list.
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

  // Return the deleted page to free list.
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
// Pages are added to tail of the list.
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
  printf("(add_free_page) Ten free page is added: Last page Number %ld\n",
      header_page->get_number_of_pages(header_page) - 1);
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


// This function check whether the page is free page of not
bool is_free_page_except_last_one(const uint64_t page_number) {
  if (page_number < 2) {
#ifdef DBG
    fprintf(stderr, "(is_free_page_except_last_one) reserved pages. \
Header or Root page.\n");
#endif
    return false;
  }

  // In a normal situation, leaf or internal page must get a content.
  // If a content is whole zero, it is leaf page.

  go_to_page_number(page_number);

  // clear page_buffer
  uint8_t page_buffer[PAGE_SIZE];
  memset(page_buffer, 0, PAGE_SIZE);

  if (read(db, page_buffer, PAGE_SIZE) < 0) {
    perror("(is_free_page_except_last_one)");
    assert(false);
    exit(1);
  }

  const uint8_t next_free_page_offset_size = sizeof(uint64_t);
  uint64_t next_free_page_offset;

  // Read first eight byte.
  memcpy(&next_free_page_offset,
      page_buffer,
      next_free_page_offset_size);

  if (next_free_page_offset == 0) {
#ifdef DBG
    printf("(is_free_page_except_last_one) Page #%ld is \
the last free page or not allocated page.\n", page_number);
#endif
    return false;
  } else if (memcmp(&page_buffer[next_free_page_offset_size],
        &empty_page_dummy[next_free_page_offset_size],
        PAGE_SIZE - next_free_page_offset_size) == 0) {
    // It is free page.
#ifdef DBG
    printf("(is_free_page_except_last_one) Page #%ld is \
a free page.\n", page_number);
#endif
    return true;
  }

  // Page exists but not a free page.
  return false;
}


// This function removes free pages which are in the end of file.
// By removing free pages, DB size will be shrinked.
void make_free_page_list_compact() {
#ifdef DBG
  printf("(make_free_page_list_compact) I am called!\n");
#endif

  // Find the start of last free pages
  // Generally, free pages are in the end of file.
  // Below page number indicates where the start of last free pages.

  uint64_t number_of_pages = header_page->get_number_of_pages(header_page);

  // db at least three pages.
  // Header page, Root page, Free page.
  assert(number_of_pages >= 3);

  // i is the last page
  uint64_t reverse_free_page_iterator = number_of_pages - 1;

  // last free page had no value
#ifdef DBG
  uint8_t page_buffer[PAGE_SIZE];
  memset(page_buffer, 0, PAGE_SIZE);
  go_to_page_number(reverse_free_page_iterator);
  if (read(db, page_buffer, PAGE_SIZE) < 0) {
    perror("make_free_page_list_compact()");
    assert(false);
    exit(1);
  }
  assert(memcmp(page_buffer, empty_page_dummy, PAGE_SIZE) == 0);
#endif

  // Below '2' means the minimum page number of free page
  while (reverse_free_page_iterator > 2) {
    // The reason for decreasing iterator first is that
    // we already know current reverse_free_page_iterator is pointing
    // a free page which is last.
    //
    // If a last free page number is inserted to is_free_page_except_last_one,
    // it will return false.
    reverse_free_page_iterator--;
    if ( ! is_free_page_except_last_one(reverse_free_page_iterator)) {
      break;
    }
  }

  uint64_t start_of_last_free_pages = reverse_free_page_iterator;

  // Fill zero to the start page of last free pages
  // It will be last page of free pages.
  go_to_page_number(start_of_last_free_pages);
  if (write(db, empty_page_dummy, PAGE_SIZE) < 0) {
    perror("make_free_page_list_compact()");
    assert(false);
    exit(1);
  }

  //  Truncate file
  //  Below function removes last free page from the DB,
  // yet it is okay because last free page does not have any value.
  if (ftruncate64(db, PAGE_SIZE * (start_of_last_free_pages)) < 0) {
    perror("make_free_page_list_compact()");
    assert(false);
    exit(1);
  }

  // Make number_of_pages correct.
  header_page->set_number_of_pages(header_page, start_of_last_free_pages + 1);
}


/*       Functions related with Insert()         */

// TODO: Not yet debugged.
// This function finds the leaf page that a key will be stored.
uint64_t find_leaf_page(const int key) {
  uint32_t i = 0;
  uint8_t page_buffer[PAGE_SIZE];
  memset(page_buffer, 0, PAGE_SIZE);

  // Get root page
  go_to_page_number(
      header_page->get_root_page_offset(header_page) / PAGE_SIZE);

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
    i = 0;
    while(i < page.get_number_of_keys(&page)) {
      if (key >= page.get_key_and_offset(&page, i)->key) {
        i++;
      } else {
       break;
      }
    }
    // Next page is found. Go to next page
    page.set_current_page_number(&page, 
        page.get_key_and_offset(&page, i)->page_offset / PAGE_SIZE);
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
  atexit(close_db);
  atexit(make_free_page_list_compact);

  // Read header
  header_page->read(header_page);

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


  /** Create a new rcord */
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
  // TODO
  fprintf(stderr, "(insert) Not yet implemented\n");
  assert(false);

  return 0;
}


char * find (int64_t key){

  return NULL;
}


int delete (int64_t key){

  return 0;
}




