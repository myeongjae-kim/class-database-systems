#include "bpt.h"
#include "bpt_buffer_manager.h"
#include "bpt_fd_table_map.h"

#include <string.h>
#include <assert.h>

buf_mgr_t buf_mgr;

static bool __set(struct __frame_object* const this,
    const int32_t table_id,
    const int64_t page_number) {
  // initialize member variables
  memset(&this->frame, 0, sizeof(this->frame));
  this->is_dirty = false;
  this->pin_count = 0;
  this->LRU_next = NULL;
  this->page = (struct __page*)&this->frame;

  // table_id and page_offset
  this->table_id = table_id;
  this->page_offset = page_number * PAGE_SIZE;
  return true;
}

// Reinitialize member variables except struct __page *page.
static bool __reset(struct __frame_object* const this){
  memset(&this->frame, 0, sizeof(this->frame));
  this->is_dirty = false;
  this->pin_count = 0;
  this->LRU_next = NULL;
  this->page = (struct __page*)&this->frame;

  // table_id and page_offset
  this->table_id = 0;
  this->page_offset = 0;
  return true;
}


static bool __read(struct __frame_object* const this) {
  assert(this->table_id != 0);
  assert(this->page_offset != 0);
  int fd = get_fd_of_table(this->table_id);
  assert(fd != 0);
  go_to_page_number(fd, this->page_offset / PAGE_SIZE);

  memset(this->page, 0, PAGE_SIZE);
  if (read(fd, this->page, PAGE_SIZE) < 0) {
    perror("(frame_object_t->read)");
    assert(false);
    exit(1);
  }

  return true;
}
static bool __write(struct __frame_object* const this) {
  assert(this->table_id != 0);
  assert(this->page_offset != 0);
  int fd = get_fd_of_table(this->table_id);
  assert(fd != 0);

  go_to_page_number(fd, this->page_offset / PAGE_SIZE);
  if (write(fd, this->page, PAGE_SIZE) < 0) {
    perror("(frame_object_t->read)");
    assert(false);
    exit(1);
  }

  this->is_dirty = false;

  return true;
}

void frame_object_constructor(frame_object_t * const this){
  memset(this, 0, sizeof(*this));
  this->set = __set;
  this->reset = __reset;
  this->read = __read;
  this->write = __write;
}

//////////////// Below is buf_mgr_t //////////////


// Return frame's address.
// If target frame is not exists, return false.
frame_object_t * __find_frame(struct __buf_mgr * const this,
    const int32_t table_id, const int64_t page_number) {
  int i;
  for (i = 0; i < this->num_of_frame; ++i) {
    if (this->frame_iter->table_id == table_id
        && this->frame_iter->page_offset == page_number * PAGE_SIZE) {
      return(frame_object_t*)this->frame_iter;
    }
    this->frame_iter++;
    if (this->frame_iter == this->frame_iter_end) {
      this->frame_iter = this->frame_iter_begin;
    }
  }

  return NULL;
}


// Check whether current page is in buffer
// If it is in
//   remove the frame from LRU list
//   increase pin_count
// If not,
//   Get the head of LRU list and remove it from list
//   Preemption
//   Set information
//   Read the page and return.
static struct __page * __request_page(struct __buf_mgr * const this,
      const int32_t table_id, const int64_t page_number) {
  // Check whether current page is in buffer
  frame_object_t * frame = __find_frame(this, table_id, page_number);

  if (frame != NULL) {
    if (frame->pin_count == 0) {
      // It means that this frame is in LRU_queue
      frame_object_t* LRU_iter;
      LRU_iter = this->LRU_queue_head;
      assert(LRU_iter != NULL);
      // If a frame's pin count is zero but not in the LRU queue,
      // segment fault!
      while (LRU_iter->LRU_next != frame) {
        LRU_iter = LRU_iter->LRU_next;
      }

      // LRU_next is found frame
      // remove found frame from list
      LRU_iter->LRU_next = LRU_iter->LRU_next->LRU_next;
    }
    frame->pin_count++;

    // return found frame's page
    return frame->page;
  } else {
    // get the first element of the list
    frame = this->LRU_queue_head->LRU_next;
    this->LRU_queue_head->LRU_next = this->LRU_queue_head->LRU_next->LRU_next;

    // Below case is occurred when all of the frames is pinned.
    // This is not occurred in single core environment
    assert(frame != NULL);

    // Eviction
    if (frame->is_dirty) {
      frame->write(frame);
      frame->is_dirty = false;
    }

    // reinitialize
    frame->reset(frame);
    frame->set(frame, table_id, page_number);
    frame->read(frame);


    frame->pin_count++;
    return frame->page;
  }
}

// find target frame
// Decrease pin_count
// If the pin_count is zero, add frame to LRU list.
bool __release_page(struct __buf_mgr * const this,
    const int32_t table_id, const int64_t page_number){
  // Check whether current page is in buffer
  frame_object_t * frame = __find_frame(this, table_id, page_number);
  // frame must be in the buffer.
  assert(frame != NULL);

  frame->pin_count--;
  if (frame->pin_count == 0) {
    assert(this->LRU_queue_tail->LRU_next == NULL);
    this->LRU_queue_tail->LRU_next = frame;
  }

  return true;
}


// TODO
// free page mananger will call buf_mgr->page_alloc
