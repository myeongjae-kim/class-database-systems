#ifndef __BPT_HEADER_OBJECT_H__
#define __BPT_HEADER_OBJECT_H__

#include "bpt.h"
// Realtime synchronize with db.
typedef struct __header_object {
  struct __header_object *this;

  header_page_t page;

  void (*read)(struct __header_object * const) ;
  void (*write)(const struct __header_object * const);
  void (*print)(const struct __header_object * const);

  void (*set)(struct __header_object * const,
      const uint64_t free_page_offset,
      const uint64_t root_page_offset,
      const uint64_t number_of_pages);


  uint64_t (*get_free_page_offset)(const struct __header_object * const);
  void (*set_free_page_offset)(struct __header_object * const,
      uint64_t free_page_offset);

  uint64_t (*get_root_page_offset)(const struct __header_object * const);
  void (*set_root_page_offset)(struct __header_object * const,
      uint64_t root_page_offset);

  uint64_t (*get_number_of_pages)(const struct __header_object * const);
  void (*set_number_of_pages)(struct __header_object * const,
      uint64_t number_of_pages);



} header_object_t;

void header_object_constructor(header_object_t * const header);

#endif
