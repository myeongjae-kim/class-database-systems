/* File Name : bpt.c
 * Author    : Kim, Myeong Jae
 * Due Date  : 2017-11-5
 *
 * This is a implementation of disk-based b+tree */

#include "bpt.h"
#include <sys/stat.h>
#include <assert.h>

// File descriptor.
int32_t db;

// Current page offset
// Page moving must be occurred via go_to_page function.
off64_t current_page_start_offset = 0;

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

void print_header_page(void) {
  go_to_page_number(0);
  header_page_t page;

  if (read(db, &page, sizeof(page)) < 0) {
    perror("(print_header_page)");
    exit(1);
  }

  printf(" ** Printing Header Page ** \n");

  printf("  page.free_page_offset: %ld\n", page.free_page_offset);
  printf("  page.root_page_offset: %ld\n", page.root_page_offset);
  printf("  page.number_of_pages : %ld\n", page.number_of_pages);

  printf(" **         End          ** \n");
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


  header_page_t header_page;
  header_page.root_page_offset = 1 * PAGE_SIZE;
  header_page.free_page_offset = 2 * PAGE_SIZE;

  // The number of pages includes all kinds of pages.
  // '3' means header page, root page, first free page.
  header_page.number_of_pages  = 3;

  // Header page
  if(write(db, &header_page, sizeof(header_page_t)) < 0) {
    perror("(initialize_db)");
    exit(1);
  }

  // Go to root page
  go_to_page_number(1);

  // Write header of root
  // Parent of root is root.

  page_header_t header = {1 * PAGE_SIZE, 1, 0, {0}, 0};
  write_page_header(&header);

  // Go to free page
  go_to_page_number(2);
  // Do nothing. Here is the end of free page.

}

int open_db (char *pathname){
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
#ifdef DBG
      perror("(open_db) DB opening is failed.");
      fprintf(stderr, "(open_db) Terminate the program\n");
      return -1;
#endif
    }

    // Change its permission.
    chmod(pathname, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    // DB is created. Initialize header page and root.
    initialize_db();
  }

  // Register file closing function to prevent memory leakage.
  atexit(close_db);

  // TODO: Read database


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
