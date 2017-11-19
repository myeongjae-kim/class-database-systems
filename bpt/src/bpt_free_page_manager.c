#include "bpt_page_object.h"
#include "bpt_fd_table_map.h"
#include "bpt_buffer_manager.h"

#include <string.h>
#include <assert.h>

// A size of free page list
static int64_t __current_capacity[MAX_TABLE_NUM + 1];

// tail of free page list
static int64_t __tail_page_number[MAX_TABLE_NUM + 1];


// These variable are in bpt.c
extern const int8_t empty_page_dummy[PAGE_SIZE];
extern header_object_t header_page[MAX_TABLE_NUM + 1];

static int64_t __head_free_page_number[MAX_TABLE_NUM + 1];


// This function check whether the page is free page of not
static bool __is_free_page(const int table_id, const int64_t page_number) {
  if (page_number < 2) {
#ifdef DBG
    fprintf(stderr, "(__is_free_page) reserved pages.");
    fprintf(stderr, "Header or Dummy page.\n");
#endif
    return false;
  } else if (page_number == __tail_page_number[table_id]) {
    // Tail is a free page.
    // Its content is empty.
    return true;
  }

  // free page will be considered as internal page
  page_object_t page;
  page_object_constructor(&page, table_id, page_number);


  // linked_page_offset is an next free page offset
  if (page.page->header.linked_page_offset == 0) {
#ifdef DBG
    fprintf(stderr, "(__is_free_page)");
    fprintf(stderr, " Page #%ld is not a free page.\n", page_number);
#endif
    page_object_destructor(&page);
    return false;
  } else if (memcmp(&page.page->header.is_leaf,
        &empty_page_dummy[sizeof(int64_t)],
        PAGE_SIZE - sizeof(int64_t)) == 0) {
    // It means that this page only have next free page offset
    // and other bytes are zero
#ifdef DBG
    printf("(__is_free_page) Page #%ld is a free page.\n",
        page_number);
#endif
    page_object_destructor(&page);
    return true;
  }

  page_object_destructor(&page);
  return false;
}



// Add new free page to the tail.
static bool __add(const int table_id, const int64_t number_of_new_pages) {
  int64_t i, next_free_page_number;

  page_object_t page;
  page_object_constructor(&page,
      table_id, __tail_page_number[table_id]);


  // tail page should be empty.
  assert(memcmp(&page.page->header.linked_page_offset,
        empty_page_dummy, PAGE_SIZE) == 0);

  // Add free pages.
  
  for (i = 0; i < number_of_new_pages; ++i) {
    page.current_page_number = __tail_page_number[table_id];
    page.read(&page);

    next_free_page_number = header_page[table_id].
      get_number_of_pages(&header_page[table_id]);

    // next free page number should be the number of tail + 1
    assert(next_free_page_number == __tail_page_number[table_id] + 1);

    // Wrtie next page's offset to current page
    page.page->header.linked_page_offset = next_free_page_number * PAGE_SIZE;
    page.write(&page);
    
    // Increase number of pages
    header_page[table_id].set_number_of_pages(&header_page[table_id], next_free_page_number + 1);

    // Increase free page capacity
    __current_capacity[table_id]++;
    __tail_page_number[table_id]++;

#ifdef DBG
    printf("(__add) free page is added. The whole number of pages: %ld\n",
        header_page[table_id].get_number_of_pages(&header_page[table_id]));
#endif
  }

  header_page[table_id].write(&header_page[table_id]);

#ifdef DBG
  printf("(__add) %ld free pages are added.\n", number_of_new_pages);
#endif

  page_object_destructor(&page);
  return true;
}


// find previous of target free page
static int64_t __find_prev_free_page_number_of(int table_id,
    const int64_t free_page_number) {

  int64_t current_free_page_number;
  int64_t next_free_page_offset;

  // To read head page first, set next_free_page_offset as head free page.
  next_free_page_offset = __head_free_page_number[table_id] * PAGE_SIZE;

  // Read head free page
  current_free_page_number = next_free_page_offset / PAGE_SIZE;
  page_object_t page;
  page_object_constructor(&page, table_id, current_free_page_number);
  do{
    current_free_page_number = next_free_page_offset / PAGE_SIZE;
    page.table_id = table_id;
    page.current_page_number = current_free_page_number;
    page.read(&page);

    next_free_page_offset = page.page->header.linked_page_offset;
    if (next_free_page_offset == 0) {
      // This case cannot be happen.
      fprintf(stderr, "(__find_prev_free_page_number_of)");
      fprintf(stderr, "Page #%ld is not a page.\n", free_page_number);
      page_object_destructor(&page);
      return 0;
    }

  } while(next_free_page_offset != free_page_number * PAGE_SIZE);


  // previous page number is found
  page_object_destructor(&page);
  return current_free_page_number;
}


// This function removes an element of free page list.
// It only can be called in page_clean()
// ** Warning: If a page which is not in the last of db is deleted,
//  unexpected situation will be occurred! **
static bool __delete_free_page_in_last_of_db(int table_id,
    const int64_t free_page_number) {
  int64_t prev_page_number;

  if (__is_free_page(table_id, free_page_number) == false) {
#ifdef DBG
    printf("(__delete_free_page_in_last_of_db)");
    printf("Page #%ld is not a free page.\n", free_page_number);
#endif
    return false;
  }

  prev_page_number = __find_prev_free_page_number_of(
      table_id, free_page_number);

  if (prev_page_number == 0) {
    fprintf(stderr, "(__delete_free_page_in_last_of_db)");
    fprintf(stderr,"Free page deletion is failed.\n");
    return false;
  }

  // read next page number and write the content to prev_page
  page_object_t found_free_page;
  page_object_t prev_page;

  page_object_constructor(&found_free_page, table_id,
      free_page_number);
  page_object_constructor(&prev_page, table_id,
      prev_page_number);

  memcpy(prev_page.page, found_free_page.page, PAGE_SIZE);
  prev_page.write(&prev_page);
  prev_page.frame->write(prev_page.frame);

  // clear deleted page
  memset(found_free_page.page, 0, PAGE_SIZE);
  found_free_page.write(&found_free_page);
  found_free_page.frame->write(found_free_page.frame);


  // Decrease the number of pages.
  header_page[table_id].set_number_of_pages(&header_page[table_id],
      header_page[table_id].get_number_of_pages(&header_page[table_id]) - 1);

  // Decrease capacity
  __current_capacity[table_id]--;
  assert(__current_capacity[table_id] >= 0);

  header_page[table_id].write(&header_page[table_id]);

#ifdef DBG
  printf("(__delete_free_page_in_last_of_db) Free page #%ld is deleted.\n",
      free_page_number);
#endif

  page_object_destructor(&found_free_page);
  page_object_destructor(&prev_page);

  return true;
}

/** below functions will be called in other source files **/

// Find the last free page
void free_page_manager_init(int table_id) {
  __head_free_page_number[table_id]
    = header_page[table_id].page.free_page_offset / PAGE_SIZE;

  int64_t current_free_page_offset = __head_free_page_number[table_id] * PAGE_SIZE;

  // Read dummy page
  page_object_t page;
  page_object_constructor(&page,
      table_id, __head_free_page_number[table_id]);

  assert(page.page->header.linked_page_offset % PAGE_SIZE == 0);

  //  If there is only free page,
  // generate one page.

  if (page.page->header.linked_page_offset == 0) {
    page.page->header.linked_page_offset =
      header_page[table_id].get_number_of_pages(&header_page[table_id]) * PAGE_SIZE;

    page.write(&page);

    header_page[table_id].set_number_of_pages(&header_page[table_id],
        header_page[table_id].get_number_of_pages(&header_page[table_id]) + 1);
    header_page[table_id].write(&header_page[table_id]);
  }

  __current_capacity[table_id] = 0;
  __tail_page_number[table_id] = 0;

  // Interate list to find last free page
  while (page.page->header.linked_page_offset != 0) {
#ifdef DBG
    // Check whether current free page is a real free page.
    if (current_free_page_offset / PAGE_SIZE != __head_free_page_number[table_id]
        && __is_free_page(table_id, current_free_page_offset / PAGE_SIZE) == false) {
      fprintf(stderr, "(free_page_manager_init)");
      fprintf(stderr, "current page is not a free page");
      assert(false);
    }
#endif

    // Next free page is found.
    // Increase capacity.
    __current_capacity[table_id]++;

    // save next free page offset to current free page offset
    current_free_page_offset = page.page->header.linked_page_offset;

    page.table_id = table_id;
    page.current_page_number = current_free_page_offset / PAGE_SIZE;
    page.read(&page);
    assert(page.page->header.linked_page_offset % PAGE_SIZE == 0);
  }

  // Set tail page.
  __tail_page_number[table_id] = current_free_page_offset / PAGE_SIZE;

  page_object_destructor(&page);
}


// Return value is a first page number of free page list.
int64_t page_alloc(int table_id){
  // Invariant
  // All the time, at least one free page should exist.
  assert(__current_capacity[table_id] > 0);

  if (__current_capacity[table_id] == 1) {
    // add new free page
    __add(table_id, header_page[table_id].get_number_of_pages(&header_page[table_id]));
  }

  page_object_t head_free_page;
  page_object_constructor(&head_free_page,
      table_id, __head_free_page_number[table_id]);

  // at least one page should exist!
  assert(head_free_page.page->header.linked_page_offset != 0);

  int64_t first_page_offset = head_free_page.page->header.linked_page_offset;

  // Read the content of first free page and write it to head.
  page_object_t first_free_page;
  page_object_constructor(&first_free_page,
      table_id, first_page_offset / PAGE_SIZE);

  head_free_page.page->header.linked_page_offset = 
    first_free_page.page->header.linked_page_offset;

  head_free_page.write(&head_free_page);

  __current_capacity[table_id]--;
  assert(__current_capacity[table_id] > 0);

  // Turn on is_leaf bit to show that this page is not a free page.
  memset(first_free_page.page, 0, PAGE_SIZE);
  first_free_page.page->header.is_leaf = 1;
  first_free_page.write(&first_free_page);




  page_object_destructor(&first_free_page);
  page_object_destructor(&head_free_page);

  return first_page_offset / PAGE_SIZE;
}


bool page_free(int table_id, const int64_t page_number){
  // Argument page should not be a free page or empty page.
  assert(__is_free_page(table_id, page_number) == false);

  page_object_t target_page;
  page_object_constructor(&target_page, table_id, page_number);

  // It cannot be empty page
  assert(memcmp(target_page.page, empty_page_dummy, PAGE_SIZE) != 0);


  // Write head's content to this page and make head pointing this page.
  page_object_t head_free_page;
  page_object_constructor(&head_free_page,
      table_id, __head_free_page_number[table_id]);

  memcpy(target_page.page, head_free_page.page, PAGE_SIZE);
  target_page.type = INTERNAL_PAGE; // TODO Can it be removed?
  target_page.write(&target_page);

  head_free_page.page->header.linked_page_offset =
    target_page.current_page_number * PAGE_SIZE;
  head_free_page.write(&head_free_page);

  // page is added to free page list
  __current_capacity[table_id]++;

  page_object_destructor(&head_free_page);

  // Ref. target page is already pinned before page_free() is called.
  page_object_destructor(&target_page);

  return true;
}



void free_page_clean(int table_id){
#ifdef DBG
  printf("(free_page_clean) Free page cleaning start. Table #%d!\n", table_id);
#endif

  // Find the start of last free pages
  // Generally, free pages are in the end of file.
  // Below page number indicates where the start of last free pages.

  int64_t number_of_pages = header_page[table_id].get_number_of_pages(&header_page[table_id]);

  // db at least three pages.
  // Header page, Root page, Free page.
  assert(number_of_pages >= 3);

  // the last page
  int64_t reverse_free_page_iterator = number_of_pages - 1;

  if (__is_free_page(table_id, reverse_free_page_iterator) == false) {
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
    if ( __is_free_page(table_id, reverse_free_page_iterator) == false) {
      break;
    }
  }

  const int64_t last_content_page_number = reverse_free_page_iterator;

  // Iterate whole free page list and remove free page
  // which is following last content page.

  int64_t current_free_page = __head_free_page_number[table_id];

  page_object_t iter_page;
  page_object_constructor(&iter_page, table_id, current_free_page);

  while (1) {
    iter_page.current_page_number = current_free_page;
    iter_page.read(&iter_page);

    if (memcmp(iter_page.page, empty_page_dummy, PAGE_SIZE) == 0) {
      // We are in the end of free page list
      break;
    }

    // delete free page
    if (iter_page.page->header.linked_page_offset > 
        last_content_page_number * PAGE_SIZE) {
      __delete_free_page_in_last_of_db(table_id, iter_page.page->header.linked_page_offset / PAGE_SIZE);
    } else {
      // or go to next free page
      current_free_page = iter_page.page->header.linked_page_offset;
    }
  }

  // All pages free pages are deleted.
  // Add one free page
  __tail_page_number[table_id] = last_content_page_number + 1;
  /** __add(table_id, 1); */
  

  page_object_destructor(&iter_page);


  //  Truncate file
  // Below function removes last free page from the DB,
  // yet it is okay because last free page does not have any value.
  if (ftruncate64(get_fd_of_table(table_id),
        PAGE_SIZE * (__tail_page_number[table_id])) < 0) {
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
