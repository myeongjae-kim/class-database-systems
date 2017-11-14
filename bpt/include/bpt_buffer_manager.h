/* File Name : bpt_fd_table_map.h
 * Author    : Kim, Myeong Jae
 * Due Date  : 2017-11-22 */

#ifndef __BPT_BUFFER_MANAGER_H__
#define __BPT_BUFFER_MANAGER_H__

#include <stdint.h>

#include "bpt.h"
#include "bpt_page_object.h"

// API
int init_db(int num_buf); // memory allocation
int shutdown_db(void);    // memory free

// Buffer manager will have a list of struct __frame_object

// Frame object
typedef struct __frame_object {
  // Project specification
  uint8_t frame[PAGE_SIZE];
  int32_t table_id;
  int64_t page_offset;
  bool is_dirty;
  int32_t pin_count;
  struct __frame_object * LRU_next; // go to next element.

  // Custom
  struct __page * page; // This is a pointer pointing a frame.
  // page_object_t will dereference this page pointer to access buffer frame.
  
  // Buffer manager will not concern page type.
  // enum page_type type;

  // Set information of frame
  bool (*set)(struct __frame_object* const this,
      const int32_t table_id,
      const int64_t page_number);
  // Reinitialize member variables except struct __page *page.
  bool (*reset)(struct __frame_object* const this);

  // read, write
  // Every disk I/O is done via this object.
  bool (*read)(struct __frame_object* const this);
  bool (*write)(struct __frame_object* const this);
} frame_object_t;

void frame_object_constructor(frame_object_t * const this);


typedef struct __buf_mgr {
  frame_object_t * frames;
  int32_t num_of_frame;

  const frame_object_t * frame_iter_begin;
  const frame_object_t * frame_iter_end;
  const frame_object_t * frame_iter;

  // Frame is added to LRU_queue when pin_count goes zero.
  // All frames will be in LRU_queue in first.
  frame_object_t * LRU_queue_head; // head is an empty page. It just have pointer
  frame_object_t * LRU_queue_tail;

  struct __page * (*request_page)(struct __buf_mgr * const this,
      const int32_t table_id, const int64_t page_number);
  bool (*release_page)(struct __buf_mgr * const this,
      const int32_t table_id, const int64_t page_number);

  // Every page allocation should be occurred via buffer manager
  // TODO original page_alloc and page_free will be changed to
  // __page_alloc and __page_free
  int64_t (*page_alloc)(struct __buf_mgr * const this,
      const int32_t table_id);
  int64_t (*page_free)(struct __buf_mgr * const this,
      const int32_t table_id, const int64_t page_number);
} buf_mgr_t;

void buf_mgr_constructor(buf_mgr_t * const this);
void buf_mgr_destructor(buf_mgr_t * const this);



#endif