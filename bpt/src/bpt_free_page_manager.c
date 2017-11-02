#include "bpt_free_page_manager.h"

#include <string.h>
#include <assert.h>

// A size of free page list
static int64_t __current_capacity = 0;

// This is just a buffer
static int8_t __page_buffer[PAGE_SIZE];

// Head. This value will be always 1.
// Page #1 is a dummy free header. It will always be free page.
static const int8_t __head_page_number = 1;
static int64_t __tail_page_number;


// These variable are in bpt.c
extern int32_t db;
extern const int8_t empty_page_dummy[PAGE_SIZE];
extern header_object_t *header_page;

// This function check whether the page is free page of not
static bool __is_free_page(const int64_t page_number) {
  if (page_number < 2) {
#ifdef DBG
    fprintf(stderr, "(__is_free_page) reserved pages.");
    fprintf(stderr, "Header or Dummy page.\n");
#endif
    return false;
  } else if (page_number == __tail_page_number) {
    // Tail is a free page.
    // Its content is empty.
    return true;
  }

  // In a normal situation, leaf or internal page must get a content.
  // If a content is whole zero, it is leaf page.

  // clear page_buffer
  memset(__page_buffer, 0, PAGE_SIZE);


  go_to_page_number(page_number);
  memcpy(__page_buffer, empty_page_dummy, PAGE_SIZE);
  if (read(db, __page_buffer, PAGE_SIZE) < 0) {
    perror("(is_free_page_except_last_one)");
    assert(false);
    exit(1);
  }

  // Read first eight byte.
  int64_t *next_free_page_offset = (int64_t*)__page_buffer;


  if (*next_free_page_offset == 0) {
#ifdef DBG
    fprintf(stderr, "(__is_free_page)");
    fprintf(stderr, " Page #%ld is not a free page.\n", page_number);
#endif
    return false;
  } else if (memcmp(&__page_buffer[sizeof(int64_t)],
        &empty_page_dummy[sizeof(int64_t)],
        PAGE_SIZE - sizeof(int64_t)) == 0) {
    // It is free page.
#ifdef DBG
    printf("(__is_free_page) Page #%ld is a free page.\n",
        page_number);
#endif
    return true;
  }

  // Page exists but not a free page.
  return false;
}




// Add new free page to the tail.
static bool __add(const int64_t number_of_new_pages) {
  int64_t i, next_free_page_number;
  go_to_page_number(__tail_page_number);
  memcpy(__page_buffer, empty_page_dummy, PAGE_SIZE);
  if (read(db, __page_buffer, PAGE_SIZE) < 0) {
    perror("(__add)");
    assert(false);
    exit(1);
  }

  // tail page should be empty.
  assert(memcmp(__page_buffer, empty_page_dummy, PAGE_SIZE) == 0);

  // Add free pages.
  for (i = 0; i < number_of_new_pages; ++i) {
    next_free_page_number = header_page->get_number_of_pages(header_page);

    // next free page number should be the number of tail + 1
    assert(next_free_page_number == __tail_page_number + 1);

    // Wrtie next page's offset to current page
    *(int64_t*)__page_buffer = next_free_page_number * PAGE_SIZE;
    go_to_page_number(__tail_page_number);
    if (write(db, __page_buffer, PAGE_SIZE) < 0) {
      perror("(__add)");
      assert(false);
      exit(1);
    }
    fsync(db);

    // Increase number of pages
    header_page->set_number_of_pages(header_page, next_free_page_number + 1);

    // Increase free page capacity
    __current_capacity++;
    __tail_page_number++;

#ifdef DBG
    printf("(__add) free page is added. The whole number of pages: %ld\n",
        header_page->get_number_of_pages(header_page));
#endif
  }

  header_page->write(header_page);

#ifdef DBG
  printf("(__add) %ld free pages are added.\n", number_of_new_pages);
#endif

  return true;
}

// find previous of target free page
static int64_t __find_prev_free_page_number_of(
    const int64_t free_page_number) {

  int64_t * const next_free_page_offset = (int64_t*)__page_buffer;
  int64_t current_free_page_number;

  // To read head page first, set next_free_page_offset as head free page.
  *next_free_page_offset = __head_page_number * PAGE_SIZE;

  // Read head free page
  do{
    current_free_page_number = *next_free_page_offset / PAGE_SIZE;

    go_to_page_number(current_free_page_number);
    memcpy(__page_buffer, empty_page_dummy, PAGE_SIZE);
    if (read(db, __page_buffer, PAGE_SIZE) < 0) {
      perror("(free_page_manager_init)");
      assert(false);
      exit(1);
    }

    if (*next_free_page_offset == 0) {
      // This case cannot be happen.
      fprintf(stderr, "(__find_prev_free_page_number_of)");
      fprintf(stderr, "Page #%ld is not a page.\n", free_page_number);
      return 0;
    }

  } while(*next_free_page_offset != free_page_number * PAGE_SIZE);

  // previous page number is found
  return current_free_page_number;
}


// This function removes an element of free page list.
// It only can be called in page_clean()
// ** Warning: If a page which is not in the last of db is deleted,
//  unexpected situation will be occurred! **
static bool __delete_free_page_in_last_of_db(
    const int64_t free_page_number) {
  int64_t prev_page_number;

  if (__is_free_page(free_page_number) == false) {
#ifdef DBG
    printf("(__delete_free_page_in_last_of_db)");
    printf("Page #%ld is not a free page.\n", free_page_number);
#endif
    return false;
  }

  prev_page_number = __find_prev_free_page_number_of(free_page_number);
  if (prev_page_number == 0) {
    fprintf(stderr, "(__delete_free_page_in_last_of_db)");
    fprintf(stderr,"Free page deletion is failed.\n");
    return false;
  }

  // read next page number and write the content to prev_page
  go_to_page_number(free_page_number);
  memcpy(__page_buffer, empty_page_dummy, PAGE_SIZE);
  if (read(db, __page_buffer, PAGE_SIZE) < 0) {
    perror("(__delete_free_page_in_last_of_db)");
    assert(false);
    exit(1);
  }

  go_to_page_number(prev_page_number);
  if (write(db, __page_buffer, PAGE_SIZE) < 0) {
    perror("(__delete_free_page_in_last_of_db)");
    assert(false);
    exit(1);
  }
  fsync(db);

  // clear deleted page
  go_to_page_number(free_page_number);
  if (write(db, empty_page_dummy, PAGE_SIZE) < 0) {
    perror("(__delete_free_page_in_last_of_db)");
    assert(false);
    exit(1);
  }
  fsync(db);

  // Decrease the number of pages.
  header_page->set_number_of_pages(header_page,
      header_page->get_number_of_pages(header_page) - 1);

  // Decrease capacity
  __current_capacity--;
  assert(__current_capacity >= 0);

  header_page->write(header_page);
#ifdef DBG
  printf("(__delete_free_page_in_last_of_db) Free page #%ld is deleted.\n",
      free_page_number);
#endif

  return true;
}



/** below functions will be called in other source files **/

// Find the last free page
void free_page_manager_init() {
  int64_t current_free_page_offset = __head_page_number * PAGE_SIZE;
  int64_t * const next_free_page_offset = (int64_t*)__page_buffer;
  assert(db >= 2);

  // Read dummy page
  go_to_page_number(__head_page_number);
  memcpy(__page_buffer, empty_page_dummy, PAGE_SIZE);
  if (read(db, __page_buffer, PAGE_SIZE) < 0) {
    perror("(free_page_manager_init)");
    assert(false);
    exit(1);
  }
  assert(*next_free_page_offset % PAGE_SIZE == 0);

  //  If there is only free page,
  // generate one page.
  if (*next_free_page_offset == 0) {
    *next_free_page_offset =
      header_page->get_number_of_pages(header_page) * PAGE_SIZE;

    // Write head page
    go_to_page_number(__head_page_number);
    if (write(db, __page_buffer, PAGE_SIZE) < 0) {
      perror("(free_page_manager_init)");
      assert(false);
      exit(1);
    }
    fsync(db);

    header_page->set_number_of_pages(header_page,
        header_page->get_number_of_pages(header_page) + 1);
    header_page->write(header_page);
  }

  // Interate list to find last free page
  while (*next_free_page_offset != 0) {

#ifdef DBG
    // Check whether current free page is a real free page.
    if (current_free_page_offset / PAGE_SIZE != __head_page_number
        && __is_free_page(current_free_page_offset / PAGE_SIZE) == false) {
      fprintf(stderr, "(free_page_manager_init)");
      fprintf(stderr, "current page is not a free page");
      assert(false);
    }
#endif

    // Next free page is found.
    // Increase capacity.
    __current_capacity++;

    // save next free page offset to current free page offset
    current_free_page_offset = *next_free_page_offset;

    go_to_page_number(current_free_page_offset / PAGE_SIZE);

    memcpy(__page_buffer, empty_page_dummy, PAGE_SIZE);
    if (read(db, __page_buffer, PAGE_SIZE) < 0) {
      perror("(free_page_manager_init)");
      assert(false);
      exit(1);
    }
    assert(*next_free_page_offset % PAGE_SIZE == 0);
  }

  // Set tail page.
  __tail_page_number = current_free_page_offset / PAGE_SIZE;

}


// Return value is a first page number of free page list.
int64_t page_alloc(){
  int64_t first_page_offset;
  page_header_t *page_header;
  // Invariant
  // All the time, at least one free page should exist.
  assert(__current_capacity > 0);

  if (__current_capacity == 1) {
    // add new free page
    /** __add(10); */
    __add(header_page->get_number_of_pages(header_page));
  }

  go_to_page_number(__head_page_number);
  memcpy(__page_buffer, empty_page_dummy, PAGE_SIZE);
  if (read(db, __page_buffer, PAGE_SIZE) < 0) {
    perror("(page_alloc)\n");
    assert(false);
    exit(1);
  }

  // at least one page should exist!
  assert(*(int64_t*)__page_buffer != 0);

  first_page_offset = *(int64_t*)__page_buffer;

  // Reinitialize __page_buffer
  memcpy(__page_buffer, empty_page_dummy, PAGE_SIZE);

  // Read the content of first free page and write it to head.
  go_to_page_number(first_page_offset / PAGE_SIZE);
  memcpy(__page_buffer, empty_page_dummy, PAGE_SIZE);
  if (read(db, __page_buffer, PAGE_SIZE) < 0) {
    perror("(page_alloc)\n");
    assert(false);
    exit(1);
  }

  // Write it to head
  go_to_page_number(__head_page_number);
  if (write(db, __page_buffer, PAGE_SIZE) < 0) {
    perror("(page_alloc)\n");
    assert(false);
    exit(1);
  }
  fsync(db);

  __current_capacity--;
  assert(__current_capacity > 0);

  // Turn on is_leaf bit to show that this page is not a free page.
  memcpy(__page_buffer, empty_page_dummy, PAGE_SIZE);
  page_header = (page_header_t*)__page_buffer;
  page_header->is_leaf = 1;

  go_to_page_number(first_page_offset / PAGE_SIZE);
  if (write(db, __page_buffer, PAGE_SIZE) < 0) {
    perror("(page_alloc)\n");
    assert(false);
    exit(1);
  }
  fsync(db);

  return first_page_offset / PAGE_SIZE;
}

bool page_free(const int64_t page_number){
  // Argument page should not be a free page or empty page.
  assert(__is_free_page(page_number) == false);

  go_to_page_number(page_number);
  memcpy(__page_buffer, empty_page_dummy, PAGE_SIZE);
  if (read(db, __page_buffer, PAGE_SIZE) < 0) {
    perror("(page_alloc)\n");
    assert(false);
    exit(1);
  }

  // It cannot be empty page
  assert(memcmp(__page_buffer, empty_page_dummy, PAGE_SIZE) != 0);


  // Write head's content to this page and make head pointing this page.
  go_to_page_number(__head_page_number);
  memcpy(__page_buffer, empty_page_dummy, PAGE_SIZE);
  if (read(db, __page_buffer, PAGE_SIZE) < 0) {
    perror("(page_alloc)\n");
    assert(false);
    exit(1);
  }

  go_to_page_number(page_number);
  if (write(db, __page_buffer, PAGE_SIZE) < 0) {
    perror("(page_alloc)\n");
    assert(false);
    exit(1);
  }
  fsync(db);

  // buffer clear
  memcpy(__page_buffer, empty_page_dummy, PAGE_SIZE);
  // pointing deleted page
  *(int64_t*)__page_buffer = page_number * PAGE_SIZE;

  // Make head pointing this page

  go_to_page_number(__head_page_number);
  if (write(db, __page_buffer, PAGE_SIZE) < 0) {
    perror("(page_alloc)\n");
    assert(false);
    exit(1);
  }
  fsync(db);
  __current_capacity++;

  return true;
}

void free_page_clean(){
#ifdef DBG
  printf("(free_page_clean) I am called!\n");
#endif

  // Find the start of last free pages
  // Generally, free pages are in the end of file.
  // Below page number indicates where the start of last free pages.

  int64_t number_of_pages = header_page->get_number_of_pages(header_page);

  // db at least three pages.
  // Header page, Root page, Free page.
  assert(number_of_pages >= 3);

  // the last page
  int64_t reverse_free_page_iterator = number_of_pages - 1;

  if (__is_free_page(reverse_free_page_iterator) == false) {
    //  If last page is not a free page,
    // there is no free page to delete.
    return;
  }

  // Below '2' means the minimum page number of free page
  while (reverse_free_page_iterator > 2) {
    // The reason for decreasing iterator first is that
    // we already know current reverse_free_page_iterator is pointing
    // a free page which is last.
    //
    // If a last free page number is inserted to is_free_page_except_last_one,
    // it will return false.
    reverse_free_page_iterator--;
    if ( __is_free_page(reverse_free_page_iterator) == false) {
      break;
    }
  }

  const int64_t last_content_page_number = reverse_free_page_iterator;

  // Iterate whole free page list and remove free page
  // which is following last content page.

  int64_t current_free_page = __head_page_number;
  int64_t *next_free_page_offset = (int64_t*)__page_buffer;
  while (1) {
    go_to_page_number(current_free_page);
    memcpy(__page_buffer, empty_page_dummy, PAGE_SIZE);
    if (read(db, __page_buffer, PAGE_SIZE) < 0) {
      perror("(page_clean)");
      assert(false);
      exit(1);
    }

    if (memcmp(__page_buffer, empty_page_dummy, PAGE_SIZE) == 0) {
      // We are in the end of free page list
      break;
    }

    // delete free page
    if (*next_free_page_offset > last_content_page_number * PAGE_SIZE) {
      __delete_free_page_in_last_of_db(*next_free_page_offset / PAGE_SIZE);
    } else {
      // or go to next free page
      current_free_page = *next_free_page_offset / PAGE_SIZE;
    }
  }

  // All pages free pages are deleted.
  // Add one free page
  memcpy(__page_buffer, empty_page_dummy, PAGE_SIZE);
  __tail_page_number = last_content_page_number + 1;
  *(int64_t*)__page_buffer = __tail_page_number * PAGE_SIZE;

  go_to_page_number(header_page->get_free_page_offset(header_page) / PAGE_SIZE);
  if (write(db, __page_buffer, PAGE_SIZE) < 0) {
    perror("(free_page_clean)");
    assert(false);
    exit(1);
  }
  fsync(db);
  __current_capacity++;
  header_page->set_number_of_pages(header_page, __tail_page_number + 1);
  header_page->write(header_page);

  //  Truncate file
  // Below function removes last free page from the DB,
  // yet it is okay because last free page does not have any value.
  if (ftruncate64(db, PAGE_SIZE * (__tail_page_number)) < 0) {
    perror("make_free_page_list_compact()");
    assert(false);
    exit(1);
  }

  // Make number_of_pages correct.
  // Plus 2
  // (last_content_page_number + 1 == last free page number)
  // (last free page number + 1 == number of pages)
  /** header_page->set_number_of_pages(header_page, last_content_page_number + 2); */
  return;
}
