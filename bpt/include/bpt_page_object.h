/* File Name : bpt_page_object.h
 * Author    : Kim, Myeong Jae
 * Due Date  : 2017-11-5 */

#ifndef __BPT_PAGE_OBJECT_H__
#define __BPT_PAGE_OBJECT_H__

#include "bpt.h"

// the size of page_content_t is 'PAGE_SIZE - HEADER_SIZE'
  // page content can be interpreted as leaf page of internal page.
union __page_content {
  record_t records[RECORD_PER_PAGE];
  internal_page_element_t key_and_offsets[OFFSETS_PER_PAGE];
};

struct __page {
  page_header_t header;
  union __page_content content;
};


enum page_type {INVALID_PAGE, INTERNAL_PAGE, LEAF_PAGE};

// Object start

typedef struct __page_object {
  struct __page_object *this;

  // page content can be interpreted as leaf page of internal page.
  struct __page page;

  enum page_type type;

  int64_t current_page_number;

  // methods


  page_header_t (*get_header)(const struct __page_object * const);
  void (*set_header)(struct __page_object * const,
      page_header_t *header);

  enum page_type (*get_type)(const struct __page_object * const);
  void (*set_type)(struct __page_object * const,
      enum page_type type);

  int64_t (*get_current_page_number)(const struct __page_object * const);
  void (*set_current_page_number)(struct __page_object * const,
      int64_t page_number);

  int32_t (*get_number_of_keys)(const struct __page_object * const);
  void (*set_number_of_keys)(struct __page_object * const,
      const int32_t number_of_keys);


  internal_page_element_t* (*get_key_and_offset)(
      const struct __page_object * const,
      const int32_t idx);
  void (*set_key_and_offset)(
      struct __page_object * const,
      const int32_t idx,
      const internal_page_element_t * const key_and_offset);


  record_t* (*get_record)(
      const struct __page_object * const,
      const int32_t idx);
  void (*set_record)(
      struct __page_object * const,
      const int32_t idx,
      const record_t * const key_and_offset);

  bool (*has_room)(const struct __page_object * const);

  bool (*insert_record)(struct __page_object * const,
      const record_t * const record);
  /* bool (*insert_key_and_offset)(struct __page_object * const,
   *     const internal_page_element_t * const key_and_offset); */

  bool (*insert_record_after_splitting)(struct __page_object * const this,
      const record_t * const record);

  bool (*delete_record_of_key)(struct __page_object * const this,
      const int64_t key);

  bool (*read)(struct __page_object * const) ;
  bool (*write)(const struct __page_object * const);
  void (*print)(const struct __page_object * const);

  // write record
  // write offset


} page_object_t;

void page_object_constructor(page_object_t * const this);


#endif
