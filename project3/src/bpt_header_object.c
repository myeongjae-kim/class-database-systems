/* File Name : bpt_header_object.c
 * Author    : Kim, Myeong Jae
 * Due Date  : 2017-11-5 */

#include "bpt_header_object.h"
#include "bpt_fd_table_map.h"
#include <string.h>

extern void go_to_page_number(const int32_t table_id, int64_t page_number);

static void __print_header_page(const header_object_t * const header) {

  printf(" ** Printing Header Page ** \n");

  printf("  page.free_page_offset: %ld\n",
      header->page.free_page_offset);
  printf("  page.root_page_offset: %ld\n",
      header->page.root_page_offset);
  printf("  page.number_of_pages : %ld\n",
      header->page.number_of_pages);

  printf(" **         End          ** \n");
}

static void __read_header_from_db(header_object_t * const this){ 
  go_to_page_number(this->table_id, 0);
  int32_t fd = get_fd_of_table(this->table_id);
  if(read(fd, &this->page, sizeof(this->page)) < 0) {
    perror("(header_object_t.read_to_db)");
    exit(1);
  }
}

static void __write_header_to_db(const header_object_t * const this) {
  go_to_page_number(this->table_id, 0);
  int32_t fd = get_fd_of_table(this->table_id);
  if(write(fd, &this->page, sizeof(this->page)) < 0) {
    perror("(header_object_t.write_to_db)");
    exit(1);
  }
  fsync(fd);
}

static void __set__header_page(struct __header_object * const this,
    const int64_t free_page_offset,
    const int64_t root_page_offset,
    const int64_t number_of_pages) {

  this->page.free_page_offset = free_page_offset;
  this->page.root_page_offset = root_page_offset;
  this->page.number_of_pages = number_of_pages;

  /** this->write(this); */
}

static int64_t __get_free_page_offset(const struct __header_object * const this){
  return this->page.free_page_offset;
}

static void __set_free_page_offset(struct __header_object * const this,
    int64_t free_page_offset){
  this->page.free_page_offset = free_page_offset;
  /** this->write(this); */
}

static int64_t __get_root_page_offset(const struct __header_object * const this){
  return this->page.root_page_offset;
}
static void __set_root_page_offset(struct __header_object * const this,
    int64_t root_page_offset){
  this->page.root_page_offset = root_page_offset;
  /** this->write(this); */
}

static int64_t __get_number_of_pages(const struct __header_object * const this){
  return this->page.number_of_pages;
}

static void __set_number_of_pages(struct __header_object * const this,
    int64_t number_of_pages){
  this->page.number_of_pages = number_of_pages;
  /** this->write(this); */
}



void header_object_constructor(header_object_t * const header,
    const int32_t table_id) {
  header->this = header;
  header->table_id = table_id;

  header->read = __read_header_from_db;
  header->write = __write_header_to_db;

  header->set = __set__header_page;
  header->print = __print_header_page;

  header->get_free_page_offset = __get_free_page_offset;
  header->set_free_page_offset = __set_free_page_offset;

  header->get_root_page_offset = __get_root_page_offset;
  header->set_root_page_offset = __set_root_page_offset;

  header->get_number_of_pages = __get_number_of_pages;
  header->set_number_of_pages = __set_number_of_pages;

}


void header_object_destructor(header_object_t * const header) {
  memset(header, 0, sizeof(*header));
}
