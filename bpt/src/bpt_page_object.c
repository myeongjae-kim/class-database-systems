#include "bpt_page_object.h"
#include <string.h>
#include <assert.h>

extern int32_t db;

static bool __page_read(struct __page_object * const this) {
  if (this->current_page_number == 0) {
    printf("(page_object_t->read) You are trying to read header page\
 which is forbidden\n");
    printf("(page_object_t->read) call set_current_page_number() method\
 before calling read()\n");
    return false;
  }

  go_to_page_number(this->current_page_number);
  if (read(db, &this->page, PAGE_SIZE) < 0) {
    perror("(page_object_t->read)");
    assert(false);
    exit(1);
  }

  // Set page type.
  if (this->page.header.is_leaf) {
    this->type = LEAF_PAGE;
  } else {
    this->type = INTERNAL_PAGE;
  }

  return true;
}
static bool __page_write(const struct __page_object * const this){
  if (this->current_page_number == 0) {
    printf("(page_object_t->write) You are trying to write header page\
 which is forbidden\n");
    printf("(page_object_t->write) call set_current_page_number() method\
 before calling write()\n");
    return false;
  }

  go_to_page_number(this->current_page_number);
  if (write(db, &this->page, PAGE_SIZE) < 0) {
    perror("(page_object_t->write)");
    assert(false);
    exit(1);
  }

  return true;
}

static void __page_print(const struct __page_object * const this){
  printf("** Printing info of page #%ld **\n", this->current_page_number);
  printf("**         ** TODO **         **\n");
  printf("**        Printing End        **\n");
}


static page_header_t __get_page_header(const struct __page_object * const this){
  return this->page.header;
}
static void __set_page_header(struct __page_object * const this,
    page_header_t *header){
  this->page.header = *header;
  this->write(this);
}

static enum page_type __get_page_type(const struct __page_object * const this){
  return this->type;
}
static void __set_page_type(struct __page_object * const this,
    enum page_type type){
  this->type = type;
}

static uint64_t __get_current_page_number(const struct __page_object * const this){
  return this->current_page_number;
}
static void __set_current_page_number(struct __page_object * const this,
    uint64_t page_number){
  this->current_page_number = page_number;
}


static uint32_t __get_number_of_keys(const struct __page_object * const this){
  return this->page.header.number_of_keys;
}

static void __set_number_of_keys(struct __page_object * const this,
    const uint32_t number_of_keys){
  this->page.header.number_of_keys = number_of_keys;
  this->write(this);
}


static internal_page_element_t* __get_key_and_offset(
    const struct __page_object * const this,
    const uint32_t idx) {
  if (this->type != INTERNAL_PAGE) {
    printf("(page_object_t->get_key_and_offset)\
 You've called wrong function.\n");
    printf("(page_object_t->get_key_and_offset)\
 This page is not an internal page.\n");
    printf("(page_object_t->get_key_and_offset)\
 Call get_record() method\n");
    return NULL;
  }

  return (internal_page_element_t*)&this->page.content.key_and_offsets[idx];
}

static void __set_key_and_offset(
    struct __page_object * const this,
    const uint32_t idx,
    const internal_page_element_t * const key_and_offset) {
  if (this->type != INTERNAL_PAGE) {
    printf("(page_object_t->set_key_and_offset)\
 You've called wrong function.\n");
    printf("(page_object_t->set_key_and_offset)\
 This page is not an internal page.\n");
    printf("(page_object_t->set_key_and_offset)\
 Call set_record() method\n");
    return;
  }

  this->page.content.key_and_offsets[idx] = *key_and_offset;
  this->write(this);
}

static record_t* __get_record(
    const struct __page_object * const this,
    const uint32_t idx) {
  if (this->type != LEAF_PAGE) {
    printf("(page_object_t->get_record)\
 You've called wrong function.\n");
    printf("(page_object_t->get_record)\
 This page is not a leaf page.\n");
    printf("(page_object_t->get_record)\
 Call get_key_and_offset() method\n");
    return NULL;
  }

  return (record_t*)&this->page.content.records[idx];
}

static void __set_record(
    struct __page_object * const this,
    const uint32_t idx,
    const record_t * const record) {
  if (this->type != LEAF_PAGE) {
    printf("(page_object_t->set_record)\
 You've called wrong function.\n");
    printf("(page_object_t->set_record)\
 This page is not a leaf page.\n");
    printf("(page_object_t->set_record)\
 Call set_key_and_offset() method\n");
    return;
  }

  this->page.content.records[idx] = *record;
  this->write(this);
}

static bool __has_room(const struct __page_object * const this) {
  if (this->type == INTERNAL_PAGE) {
      return this->page.header.number_of_keys < OFFSETS_PER_PAGE;
  } else if (this->type == LEAF_PAGE) {
      return this->page.header.number_of_keys < RECORD_PER_PAGE;
  } else {
    // Current page is invalid page.
    // This case should not be occurred.
    assert(false);
  }
}


bool __insert_record(struct __page_object * const this,
    const record_t * const record){
  if (this->type != LEAF_PAGE) {
    printf("(page_object_t->insert_record)\
 You've called wrong function.\n");
    printf("(page_object_t->insert_record)\
 This page is not a leaf page.\n");
    printf("(page_object_t->insert_record)\
 Call set_key_and_offset() method\n");
    return false;
  }

  // Check whether it has room to insert
  if ( ! this->has_room(this) ) {
    return false;
  }

  // Insert.
  uint64_t i, insertion_point = 0;
  // find the place to be inserted
  while (insertion_point < this->page.header.number_of_keys
      && this->page.content.records[insertion_point].key < record->key)
    insertion_point++;

  for (i = this->page.header.number_of_keys;
      i > insertion_point;
      i--) {
    memcpy(&this->page.content.records[i], 
        &this->page.content.records[i-1], sizeof(*record));
  }

  memcpy(&this->page.content.records[insertion_point], record, sizeof(*record));
  this->page.header.number_of_keys++;
  this->write(this);
  return true;
}

bool __insert_key_and_offset(struct __page_object * const this,
    const internal_page_element_t * const key_and_offset){
  if (this->type != INTERNAL_PAGE) {
    printf("(page_object_t->insert_key_and_offset)\
        You've called wrong function.\n");
    printf("(page_object_t->insert_key_and_offset)\
        This page is not an internal page.\n");
    printf("(page_object_t->insert_key_and_offset)\
        Call set_record() method\n");
    return false;
  }

  // Check whether it has room to insert
  if ( ! this->has_room(this) ) {
    return false;
  }

  // Insert.
  uint64_t i, insertion_point = 0;
  // find the place to be inserted
  while (insertion_point < this->page.header.number_of_keys
      && this->page.content.key_and_offsets[insertion_point].key
      < key_and_offset->key)
    insertion_point++;

  for (i = this->page.header.number_of_keys;
      i > insertion_point;
      i--) {
    memcpy(&this->page.content.key_and_offsets[i], 
        &this->page.content.key_and_offsets[i-1], sizeof(*key_and_offset));
  }

  memcpy(&this->page.content.key_and_offsets[insertion_point],
      key_and_offset, sizeof(*key_and_offset));
  this->page.header.number_of_keys++;
  this->write(this);
  return true;
}


void page_object_constructor(page_object_t * const this) {
  memset(this, 0, sizeof(*this));
  this->this = this;

  this->read = __page_read;
  this->write = __page_write;
  this->print = __page_print;

  this->get_header = __get_page_header;
  this->set_header = __set_page_header;

  this->get_type = __get_page_type;
  this->set_type = __set_page_type;

  this->get_current_page_number = __get_current_page_number;
  this->set_current_page_number = __set_current_page_number;

  this->get_number_of_keys = __get_number_of_keys;
  this->set_number_of_keys = __set_number_of_keys;

  this->get_key_and_offset = __get_key_and_offset;
  this->set_key_and_offset = __set_key_and_offset;

  this->get_record = __get_record;
  this->set_record = __set_record;

  this->has_room = __has_room;

  this->insert_record = __insert_record;
  this->insert_key_and_offset = __insert_key_and_offset;

}
