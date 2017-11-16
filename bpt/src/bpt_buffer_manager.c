#include "bpt.h"
#include "bpt_buffer_manager.h"
#include "bpt_fd_table_map.h"
#include "bpt_free_page_manager.h"

#include <string.h>
#include <assert.h>

buf_mgr_t buf_mgr;

// LRU doubly linked list function
frame_object_t * __LRU_find_from_head(buf_mgr_t * const this,
    frame_object_t * frame);
frame_object_t * __LRU_find_from_tail(buf_mgr_t * const this,
    frame_object_t * frame);

void __LRU_head_insert(buf_mgr_t * const this,
    frame_object_t * frame);
void __LRU_tail_insert(buf_mgr_t * const this,
    frame_object_t * frame);

void __LRU_remove_from_head(buf_mgr_t * const this,
    frame_object_t * frame);
void __LRU_remove_from_tail(buf_mgr_t * const this,
    frame_object_t * frame);

frame_object_t * __LRU_head_pop(buf_mgr_t * const this);
frame_object_t * __LRU_tail_pop(buf_mgr_t * const this);
// LRU doubly linked list function end


static bool __set(struct __frame_object* const this,
    const int32_t table_id,
    const int64_t page_number) {
  // initialize member variables
  memset(&this->frame, 0, sizeof(this->frame));
  this->is_dirty = false;
  this->pin_count = 0;
  this->LRU_next = NULL;
  this->LRU_prev = NULL;
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
  this->LRU_prev = NULL;
  this->page = (struct __page*)&this->frame;

  // table_id and page_offset
  this->table_id = 0;
  this->page_offset = 0;
  return true;
}


static bool __read(struct __frame_object* const this) {
  assert(this->table_id != 0);
  assert(this->page_offset != 0);

  go_to_page_number(this->table_id, this->page_offset / PAGE_SIZE);

  int fd = get_fd_of_table(this->table_id);
  assert(fd != 0);

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

  go_to_page_number(this->table_id, this->page_offset / PAGE_SIZE);


  int fd = get_fd_of_table(this->table_id);
  assert(fd != 0);

  if (write(fd, this->page, PAGE_SIZE) < 0) {
    perror("(frame_object_t->read)");
    assert(false);
    exit(1);
  }

  this->is_dirty = false;

  return true;
}

static void frame_object_constructor(frame_object_t * const this){
  memset(this, 0, sizeof(*this));

  this->page = (struct __page * )&this->frame;

  this->this = this;
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
  for (i = 0; i < this->num_of_frames; ++i) {
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
static frame_object_t * __request_frame(struct __buf_mgr * const this,
      const int32_t table_id, const int64_t page_number) {
  // Check whether current page is in buffer
  frame_object_t * frame = __find_frame(this, table_id, page_number);

  if (frame != NULL) {
    if (frame->pin_count == 0) {
      // revmoe from LRU
      __LRU_remove_from_tail(this, frame);
    }
    frame->pin_count++;

    // return found frame's page
    return frame;
  } else {
    // get the first element of the list

    frame = __LRU_head_pop(this);

    // Below case is occurred when all of the frames is pinned.
    // This is not occurred in single core environment
    if(frame == NULL) {
      fprintf(stderr, "(__request_frame) buffer is full!\n");
      assert(false);
      exit(1);
    }

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
    return frame;
  }
}

// find target frame
// Decrease pin_count
// If the pin_count is zero, add frame to LRU list.
bool __release_frame(struct __buf_mgr * const this,
    frame_object_t * frame){
  // Check whether current page is in buffer
  /** frame_object_t * frame = __find_frame(this, table_id, page_number); */
  // frame must be in the buffer.
  assert(frame != NULL);
  assert(frame->LRU_next == NULL);

  frame->pin_count--;
  if (frame->pin_count == 0) {
    __LRU_tail_insert(this, frame);
  }

  return true;
}


//If no frame is found, return null
frame_object_t * __LRU_find_from_head(buf_mgr_t * const this,
    frame_object_t * frame) {
  assert(this->LRU_queue_head != NULL);
  assert(this->LRU_queue_tail != NULL);

  assert(this->LRU_queue_head->LRU_prev == NULL);
  assert(this->LRU_queue_tail->LRU_next == NULL);
  int i;

  if (this->LRU_queue_count == 0) {
    return NULL;
  }

  frame_object_t * iter = this->LRU_queue_head->LRU_next;
  for (i = 0; i < this->LRU_queue_count; ++i) {
    if (iter == frame) {
      return frame;
    }
    iter = iter->LRU_next;
  }

  // Not found
  assert(iter == this->LRU_queue_tail);
  return NULL;
}

frame_object_t * __LRU_find_from_tail(buf_mgr_t * const this,
    frame_object_t * frame) {
  assert(this->LRU_queue_head != NULL);
  assert(this->LRU_queue_tail != NULL);

  assert(this->LRU_queue_head->LRU_prev == NULL);
  assert(this->LRU_queue_tail->LRU_next == NULL);
  int i;

  if (this->LRU_queue_count == 0) {
    return NULL;
  }

  frame_object_t * iter = this->LRU_queue_tail->LRU_prev;
  for (i = 0; i < this->LRU_queue_count; ++i) {
    if (iter == frame) {
      return frame;
    }
    iter = iter->LRU_prev;
  }

  // Not found
  assert(iter == this->LRU_queue_head);
  return NULL;

}


void __LRU_head_insert(buf_mgr_t * const this, frame_object_t * frame) {

  // Precondition
  assert(this->LRU_queue_head != NULL);
  assert(this->LRU_queue_tail != NULL);

  assert(this->LRU_queue_head->LRU_prev == NULL);
  assert(this->LRU_queue_tail->LRU_next == NULL);

  // If duplication is occurred, it is an error.
  if (__LRU_find_from_head(this, frame) != NULL) {
    fprintf(stderr, "(__LRU_head_insert) duplicate is found\n");
    assert(false);
    exit(1);
  }


  // frame
  frame->LRU_prev = this->LRU_queue_head;
  frame->LRU_next = this->LRU_queue_head->LRU_next;

  // change head's next
  frame->LRU_next->LRU_prev = frame;

  // change head
  frame->LRU_prev->LRU_next = frame;

  this->LRU_queue_count++;

  // postcondition
  assert(this->LRU_queue_head != NULL);
  assert(this->LRU_queue_tail != NULL);

  assert(this->LRU_queue_head->LRU_prev == NULL);
  assert(this->LRU_queue_tail->LRU_next == NULL);
}

void __LRU_tail_insert(buf_mgr_t * const this,
    frame_object_t * frame){
  // Precondition
  assert(this->LRU_queue_head != NULL);
  assert(this->LRU_queue_tail != NULL);

  assert(this->LRU_queue_head->LRU_prev == NULL);
  assert(this->LRU_queue_tail->LRU_next == NULL);

  // If duplication is occurred, it is an error.
  if (__LRU_find_from_tail(this, frame) != NULL) {
    fprintf(stderr, "(__LRU_tail_insert) duplicate is found\n");
    assert(false);
    exit(1);
  }


  // frame
  frame->LRU_next = this->LRU_queue_tail;
  frame->LRU_prev = this->LRU_queue_tail->LRU_prev;

  // change tail
  frame->LRU_next->LRU_prev = frame;

  // change tail's prev
  frame->LRU_prev->LRU_next = frame;

  this->LRU_queue_count++;

  // postcondition
  assert(this->LRU_queue_head != NULL);
  assert(this->LRU_queue_tail != NULL);

  assert(this->LRU_queue_head->LRU_prev == NULL);
  assert(this->LRU_queue_tail->LRU_next == NULL);

}

void __LRU_remove_from_head(buf_mgr_t * const this, frame_object_t * frame){
  // Precondition
  assert(this->LRU_queue_head != NULL);
  assert(this->LRU_queue_tail != NULL);

  assert(this->LRU_queue_head->LRU_prev == NULL);
  assert(this->LRU_queue_tail->LRU_next == NULL);

  // If target is not found, it is an error.
  if (__LRU_find_from_head(this, frame) == NULL) {
    fprintf(stderr, "(__LRU_remove_from_head) Target not found\n");
    assert(false);
    exit(1);
  }



  // Remove from list
  frame->LRU_next->LRU_prev = frame->LRU_prev;
  frame->LRU_prev->LRU_next = frame->LRU_next;


  // frame
  frame->LRU_next = NULL;
  frame->LRU_prev = NULL;


  this->LRU_queue_count--;
  // When it is empty, below is postcondition
  if (this->LRU_queue_count == 0) {
    assert(this->LRU_queue_head->LRU_next == this->LRU_queue_tail);
    assert(this->LRU_queue_tail->LRU_prev == this->LRU_queue_head);
  }


  // postcondition
  assert(this->LRU_queue_head != NULL);
  assert(this->LRU_queue_tail != NULL);

  assert(this->LRU_queue_head->LRU_prev == NULL);
  assert(this->LRU_queue_tail->LRU_next == NULL);
}

void __LRU_remove_from_tail(buf_mgr_t * const this, frame_object_t * frame){
  // Precondition
  assert(this->LRU_queue_head != NULL);
  assert(this->LRU_queue_tail != NULL);

  assert(this->LRU_queue_head->LRU_prev == NULL);
  assert(this->LRU_queue_tail->LRU_next == NULL);

  // If target is not found, it is an error.
  if (__LRU_find_from_tail(this, frame) == NULL) {
    fprintf(stderr, "(__LRU_remove_from_head) Target not found\n");
    assert(false);
    exit(1);
  }



  // Remove from list
  frame->LRU_next->LRU_prev = frame->LRU_prev;
  frame->LRU_prev->LRU_next = frame->LRU_next;


  // frame
  frame->LRU_next = NULL;
  frame->LRU_prev = NULL;


  this->LRU_queue_count--;
  // When it is empty, below is postcondition
  if (this->LRU_queue_count == 0) {
    assert(this->LRU_queue_head->LRU_next == this->LRU_queue_tail);
    assert(this->LRU_queue_tail->LRU_prev == this->LRU_queue_head);
  }


  // postcondition
  assert(this->LRU_queue_head != NULL);
  assert(this->LRU_queue_tail != NULL);

  assert(this->LRU_queue_head->LRU_prev == NULL);
  assert(this->LRU_queue_tail->LRU_next == NULL);
}


frame_object_t * __LRU_head_pop(buf_mgr_t * const this) {
  if (this->LRU_queue_count == 0) {
    return NULL;
  }

  assert(this->LRU_queue_head != NULL);
  assert(this->LRU_queue_tail != NULL);

  assert(this->LRU_queue_head->LRU_next != this->LRU_queue_tail);

  frame_object_t * rt_value = this->LRU_queue_head->LRU_next;
  __LRU_remove_from_head(this, rt_value);

  return rt_value;
}

frame_object_t * __LRU_tail_pop(buf_mgr_t * const this) {
  if (this->LRU_queue_count == 0) {
    return NULL;
  }

  assert(this->LRU_queue_head != NULL);
  assert(this->LRU_queue_tail != NULL);

  assert(this->LRU_queue_tail->LRU_prev != this->LRU_queue_head);

  frame_object_t * rt_value = this->LRU_queue_tail->LRU_prev;
  __LRU_remove_from_tail(this, rt_value);

  return rt_value;
}


void buf_mgr_constructor(buf_mgr_t * const this, int num_of_frames){
  int i;
  this->this = this;
  this->num_of_frames = num_of_frames;
  this->frames = (frame_object_t*)malloc(num_of_frames * sizeof(*this->frames));
  for (i = 0; i < num_of_frames; ++i) {
    frame_object_constructor(&this->frames[i]);
  }

  this->frame_iter_begin = this->frames;
  this->frame_iter_end = this->frames + num_of_frames;
  this->frame_iter = this->frame_iter_begin;

  // dummy head
  this->LRU_queue_head = (frame_object_t*)calloc(1,
      sizeof(*this->LRU_queue_head));
  // dummy tail
  this->LRU_queue_tail = (frame_object_t*)calloc(1,
      sizeof(*this->LRU_queue_head));
  this->LRU_queue_count = 0;

  // link dummy head and dummy tail
  this->LRU_queue_head->LRU_next = this->LRU_queue_tail;
  this->LRU_queue_tail->LRU_prev = this->LRU_queue_head;

  // add all frame to LRU list
  for (i = 0; i < num_of_frames; ++i) {
    __LRU_tail_insert(this, &this->frames[i]);
  }

  this->request_frame = __request_frame;
  this->release_frame = __release_frame;
}


void buf_mgr_destructor(buf_mgr_t * const this){
  //write every dirty page to disk before free.
  int i;
  for (i = 0; i < this->num_of_frames; ++i) {
    if (this->frames[i].table_id != 0) {
      this->frames[i].write(&this->frames[i]);
    }
  }
  
  free(this->frames);
  free(this->LRU_queue_head);
  free(this->LRU_queue_tail);
  memset(this, 0, sizeof(*this));
}
