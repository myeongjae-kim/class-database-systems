/* File Name : bpt_header_object.h
 * Author    : Kim, Myeong Jae
 * Due Date  : 2017-11-5 */

#ifndef __BPT_HEADER_OBJECT_H__
#define __BPT_HEADER_OBJECT_H__

#include "bpt.h"

// It is a structure of header page.
typedef struct __header_page {
  int64_t free_page_offset;
  int64_t root_page_offset;
  int64_t number_of_pages;
} header_page_t;


// Realtime synchronize with table.
typedef struct __header_object {
  struct __header_object *this;

  header_page_t page;
  int32_t table_id;

  void (*read)(struct __header_object * const) ;
  void (*write)(const struct __header_object * const);
  void (*print)(const struct __header_object * const);

  void (*set)(struct __header_object * const,
      const int64_t free_page_offset,
      const int64_t root_page_offset,
      const int64_t number_of_pages);


  int64_t (*get_free_page_offset)(const struct __header_object * const);
  void (*set_free_page_offset)(struct __header_object * const,
      int64_t free_page_offset);

  int64_t (*get_root_page_offset)(const struct __header_object * const);
  void (*set_root_page_offset)(struct __header_object * const,
      int64_t root_page_offset);

  int64_t (*get_number_of_pages)(const struct __header_object * const);
  void (*set_number_of_pages)(struct __header_object * const,
      int64_t number_of_pages);

} header_object_t;

void header_object_constructor(header_object_t * const header,
    const int32_t table_id);

void header_object_destructor(header_object_t * const header);


#endif
