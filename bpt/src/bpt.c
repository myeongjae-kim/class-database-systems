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
#include "bpt_page_object.h"
#include "bpt_free_page_manager.h"
#include "bpt_fd_table_map.h"
#include "bpt_buffer_manager.h"

// Current page offset
// Page moving must be occurred via go_to_page function.
static off64_t current_page_start_offset = 0;

// header page array always exists.
extern header_object_t header_page[MAX_TABLE_NUM + 1];

// Clearing page.
const int8_t empty_page_dummy[PAGE_SIZE] = {0};

static bool clear_resource_is_registered = false;

/** int64_t get_current_page_number(void) {
  *   off64_t current_offset = lseek64(db, 0, SEEK_CUR);
  *   if (current_offset < 0) {
  *     perror("(get_current_page_offset)");
  *     exit(1);
  *   }
  *   return current_offset / PAGE_SIZE;
  * } */


// This function change current_page_head of required page.
void go_to_page_number(const int32_t table_id, const int64_t page_number) {
  int32_t fd = get_fd_of_table(table_id);
  current_page_start_offset = lseek64(fd, page_number * PAGE_SIZE, SEEK_SET);
  if (current_page_start_offset < 0) {
    perror("(go_to_page) moving file offset failed.");
    assert(false);
    exit(1);
  }
}

void print_all(int32_t table_id) {
  page_object_t page_buffer;
  page_object_constructor(&page_buffer, table_id,
      header_page[table_id].get_root_page_offset(&header_page[table_id]) / PAGE_SIZE);
  /** page_buffer.set_current_page_number(&page_buffer,
    *     header_page[table_id].get_root_page_offset(&header_page[table_id]) / PAGE_SIZE);
    * page_buffer.read(&page_buffer); */

  while (page_buffer.get_type(&page_buffer) != LEAF_PAGE) {
    assert(page_buffer.get_type(&page_buffer) != INVALID_PAGE);

    page_buffer.set_current_page_number(&page_buffer,
        page_buffer.page.header.one_more_page_offset / PAGE_SIZE);
    page_buffer.read(&page_buffer);
  }

  // Leaf is found.
  int64_t i;
  int64_t correct = 1;

  while(1){
    assert(page_buffer.get_type(&page_buffer) == LEAF_PAGE);
    for (i = 0; i < page_buffer.page.header.number_of_keys; ++i) {

      printf("[table_id: %d, page #%ld] key:%ld, value:%s\n",
          page_buffer.table_id,
          page_buffer.get_current_page_number(&page_buffer),
          page_buffer.page.content.records[i].key,
          page_buffer.page.content.records[i].value);

      if (page_buffer.page.content.records[i].key
          != correct++)  {
        assert(false);
      }
    }
    if (page_buffer.page.header.one_more_page_offset != 0) {
      page_buffer.set_current_page_number(&page_buffer,
          page_buffer.page.header.one_more_page_offset / PAGE_SIZE);
      page_buffer.read(&page_buffer);
    } else {
      break;
    }
  }


  page_object_destructor(&page_buffer);
}

void print_header_page(int32_t table_id) {
  header_page[table_id].print(&header_page[table_id]);
}


/** void print_page(const int32_t table_id, const int64_t page_number) {
  *   page_header_t header;
  *
  *   go_to_page_number(table_id, page_number);
  *
  *   memset(&header, 0 , sizeof(header));
  *   if (read(db, &header, sizeof(header)) < 0) {
  *     perror("(print_page)");
  *     exit(1);
  *   }
  *
  *   printf(" ** Printing Page Header ** \n");
  *   printf("   linked_page_offset: %ld\n",
  *       header.linked_page_offset);
  *   printf("   is_leaf: %d\n", header.is_leaf);
  *   printf("   number_of_keys: %d\n",
  *       header.number_of_keys);
  *   printf("   one_more_page_offset: %ld\n",
  *       header.one_more_page_offset);
  *   printf(" **         End          ** \n");
  *
  *
  *   //TODO: print other informations
  * } */

// This function is called when a DB file is first generated.
// Initialize header page and root.
void init_table(const int32_t table_id) {
  // DB exists
#ifdef DBG
  printf("(initialize_db) DB Initializing start!\n");
#endif

  // Page 0 is header page.
  // Page 1 is free page dummy.
  // Page 2 is Root page.

  // The number of pages includes all kinds of pages.
  // '3' means header page, free page dummy, root page

  // Free page offset, Root page offset, number of pages.
  header_page[table_id].set(&header_page[table_id], 1 * PAGE_SIZE, 2 * PAGE_SIZE, 3);
  header_page[table_id].write(&header_page[table_id]);

  // Go to root page
  page_object_t page;
  memset(&page, 0, sizeof(page));
  page_object_constructor(&page, table_id, 2);
  /** page.current_page_number = 2; */
  /** page.read(&page); */

  // Write header of root
  // Parent of root is root.


  // DB has at least three pages.
  // 1. Header page
  // 2. Free page dummy
  // 3. Root page

  // Root's parent offset should be zero.
  page.page.header.is_leaf = true;
  page.write(&page);


  page_object_destructor(&page);
}


// This function finds the leaf page that a key will be stored.
int64_t find_leaf_page(int32_t table_id, const int key) {
  int32_t i = 0;

  // page object is needed.
  page_object_t page;
  page_object_constructor(&page, table_id,
      header_page[table_id].get_root_page_offset(&header_page[table_id]) / PAGE_SIZE);
  /** page.set_current_page_number(&page, 
    *     header_page[table_id].get_root_page_offset(&header_page[table_id]) / PAGE_SIZE);
    * page.read(&page); */

  // Leaf is not found. Go to found leaf
  /** int64_t parent_offset; */
  while(page.get_type(&page) != LEAF_PAGE) {
    assert(page.get_type(&page) != INVALID_PAGE);

    /** parent_offset = page.current_page_number * PAGE_SIZE; */
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


    ///////////////////////////////////////////////
    /** page.page.header.linked_page_offset = parent_offset; */
    /** page.write(&page); */
    ///////////////////////////////////////////////

  }


  int64_t rt_value = page.get_current_page_number(&page);
  page_object_destructor(&page);

  return rt_value;
}

/*     Functions related with Insert() End       */


int open_table (char *pathname){

  // setting header page
  int32_t fd = open(pathname, O_RDWR | O_LARGEFILE);
  int32_t table_id;

  if (fd < 0) {
    // Opening is failed.
#ifdef DBG
    printf("(open_table) DB not exists. Create new one.\n");
#endif
    // Create file and change its permission
    fd = open(pathname, O_RDWR | O_CREAT | O_EXCL | O_LARGEFILE);
    if (fd < 0) {
      // Creating is failed.
      perror("(open_table) DB opening is failed.");
      fprintf(stderr, "(open_table) Terminate the program\n");
      return -1;
    }

    // Change its permission.
    chmod(pathname, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    // DB is created. Initialize header page and root.
    table_id = set_new_table_of_fd(fd);
    init_table(table_id);
  } else{
    table_id = set_new_table_of_fd(fd);
    // Table is already initialized.
  }

  // Read header
  header_page[table_id].read(&header_page[table_id]);

  // Initialize free_page_manger.
  free_page_manager_init(table_id);
  return table_id;
}


int insert(int32_t table_id, int64_t key, char *value){
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
  int64_t leaf_page = find_leaf_page(table_id, key);


  /** If leaf has room for record, insert */

  page_object_t page;
  page_object_constructor(&page,table_id, leaf_page);
  /** page.set_current_page_number(&page, leaf_page);
    * page.read(&page); */
  assert(page.get_type(&page) == LEAF_PAGE);
  if (page.insert_record(&page, &record)) {
    // insertion is successful.
    page_object_destructor(&page);
    return 0;
  }

  /** If leaf has no room for record, split and insert */
  int rt_value = page.insert_record_after_splitting(&page, &record);
  page_object_destructor(&page);
  return rt_value;
}


char find_result_buffer[VALUE_SIZE];
char * find(int32_t table_id, int64_t key){
  int64_t i = 0;
  page_object_t page_buffer;

  int64_t leaf_page = find_leaf_page(table_id, key);

  if (leaf_page == 0) {
    return NULL;
  }

  page_object_constructor(&page_buffer, table_id, leaf_page);
  /** page_buffer.set_current_page_number(&page_buffer, leaf_page); */
  /** page_buffer.read(&page_buffer); */

  char* rt_value;

  for (i = 0; i < page_buffer.get_number_of_keys(&page_buffer); ++i) {
    if (page_buffer.page.content.records[i].key == (int64_t)key) {
      break;
    }
  }
  if (i == page_buffer.get_number_of_keys(&page_buffer)) {
    rt_value = NULL;
  } else {
    memset(find_result_buffer, 0, sizeof(find_result_buffer));
    memcpy(find_result_buffer, page_buffer.page.content.records[i].value,
        sizeof(find_result_buffer));
    rt_value = find_result_buffer;
  }

  page_object_destructor(&page_buffer);
  return rt_value;
}


int delete(int32_t table_id, int64_t key){
  page_object_t page_buffer;
  int64_t leaf_page = find_leaf_page(table_id, key);

  if (leaf_page == 0) {
    // Key is not found
    return 1;
  }

  page_object_constructor(&page_buffer, table_id, leaf_page);
  /** page_buffer.set_current_page_number(&page_buffer, leaf_page);
    * page_buffer.read(&page_buffer); */
  assert(page_buffer.get_type(&page_buffer) == LEAF_PAGE);

  int rt_value;
  if (page_buffer.delete_record_of_key(&page_buffer, key) == false) {
    // Deletion is failed.
    rt_value = 1;
  } else {
    rt_value = 0;
  }

  page_object_destructor(&page_buffer);
  return rt_value;
}


int close_table(int table_id){
  // clear free pages.
  free_page_clean(table_id);

  int fd = get_fd_of_table(table_id);
  if(close(fd) < 0) {
    perror("(close_table)");
    exit(1);
  }


  if(remove_table_id_mapping(table_id) == true) {
    return 0;
  } else {
    return -1;
  }
}

extern buf_mgr_t buf_mgr;

int init_db (int buf_num){
  buf_mgr_constructor(&buf_mgr, buf_num);
}

int shutdown_db(void) {
  buf_mgr_destructor(&buf_mgr);
}
