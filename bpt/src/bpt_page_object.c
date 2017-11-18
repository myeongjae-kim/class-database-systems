/* File Name : bpt_page_object.c
 * Author    : Kim, Myeong Jae
 * Due Date  : 2017-11-5 */

#include "bpt_page_object.h"
#include "bpt_free_page_manager.h"
#include "bpt_fd_table_map.h"
#include <string.h>
#include <assert.h>

extern header_object_t header_page[MAX_TABLE_NUM + 1];
extern buf_mgr_t buf_mgr;


/* Finds the appropriate place to
 * split a node that is too big into two.
 */
int64_t cut( int64_t length ) {
  if (length % 2 == 0)
    return length/2;
  else
    return length/2 + 1;
}


static bool __page_read(struct __page_object * const this){
  if (this->current_page_number == 0) {
    printf("(page_object_t->read) You are trying to read header page\
 which is forbidden\n");
    assert(false);
    return false;
  }

  if (this->frame != NULL){
    if (this->frame->table_id == this->table_id
      && this->frame->page_offset / PAGE_SIZE == this->current_page_number) {
      // same page. do nothing
    } else {
      // different page. release and request
      buf_mgr.release_frame(&buf_mgr, this->frame);
      this->frame = buf_mgr.request_frame(&buf_mgr,
          this->table_id, this->current_page_number);
      this->page = this->frame->page;
    }
  } else {
    // no frame. read it.
    this->frame = buf_mgr.request_frame(&buf_mgr,
        this->table_id, this->current_page_number);
    this->page = this->frame->page;
  }

  // Set page type.
  if (this->page->header.is_leaf) {
    this->type = LEAF_PAGE;
  } else {
    this->type = INTERNAL_PAGE;
  }

  return true;
}
static bool __page_write(struct __page_object * const this){
  if (this->current_page_number == 0) {
    printf("(page_object_t->write) You are trying to write header page\
 which is forbidden\n");
    printf("(page_object_t->write) call set_current_page_number() method\
 before calling write()\n");
    assert(false);
    return false;
  }

  this->frame->is_dirty = true;

  return true;
}

static void __page_print(const struct __page_object * const this){
  printf("** Printing info of page #%ld **\n", this->current_page_number);
  printf("**         ** TODO **         **\n");
  printf("**        Printing End        **\n");
}


static page_header_t __get_page_header(const struct __page_object * const this){
  return this->page->header;
}
static void __set_page_header(struct __page_object * const this,
    page_header_t *header){
  this->page->header = *header;
  /** this->write(this); */
}

static enum page_type __get_page_type(const struct __page_object * const this){
  return this->type;
}
static void __set_page_type(struct __page_object * const this,
    enum page_type type){
  this->type = type;
  if (type == LEAF_PAGE) {
    this->page->header.is_leaf = 1;
  } else if (type == INTERNAL_PAGE){
    this->page->header.is_leaf = 0;
  } else {
    fprintf(stderr, "(__set_page_type) wrong page type\n");
    assert(false);
    exit(1);
  }
  /** this->write(this); */
}

static int64_t __get_current_page_number(const struct __page_object * const this){
  return this->current_page_number;
}
static void __set_current_page_number(struct __page_object * const this,
    int64_t page_number){

  /* exception: this method does not write in spite of set methods */
  this->current_page_number = page_number;
}


static int32_t __get_number_of_keys(const struct __page_object * const this){
  return this->page->header.number_of_keys;
}

static void __set_number_of_keys(struct __page_object * const this,
    const int32_t number_of_keys){
  this->page->header.number_of_keys = number_of_keys;
  /** this->write(this); */
}


static internal_page_element_t* __get_key_and_offset(
    const struct __page_object * const this,
    const int32_t idx) {
  if (this->type != INTERNAL_PAGE) {
    printf("(page_object_t->get_key_and_offset)\
 You've called wrong function.\n");
    printf("(page_object_t->get_key_and_offset)\
 This page is not an internal page.\n");
    printf("(page_object_t->get_key_and_offset)\
 Call get_record() method\n");
    return NULL;
  }

  return (internal_page_element_t*)&this->page->content.key_and_offsets[idx];
}

static void __set_key_and_offset(
    struct __page_object * const this,
    const int32_t idx,
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

  this->page->content.key_and_offsets[idx] = *key_and_offset;
  /** this->write(this); */
}

static record_t* __get_record(
    const struct __page_object * const this,
    const int32_t idx) {
  if (this->type != LEAF_PAGE) {
    printf("(page_object_t->get_record)\
 You've called wrong function.\n");
    printf("(page_object_t->get_record)\
 This page is not a leaf page.\n");
    printf("(page_object_t->get_record)\
 Call get_key_and_offset() method\n");
    return NULL;
  }

  return (record_t*)&this->page->content.records[idx];
}

static void __set_record(
    struct __page_object * const this,
    const int32_t idx,
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

  this->page->content.records[idx] = *record;
  /** this->write(this); */
}

static bool __has_room(const struct __page_object * const this) {
  if (this->type == INTERNAL_PAGE) {
      return this->page->header.number_of_keys < (int64_t)OFFSETS_PER_PAGE;
  } else if (this->type == LEAF_PAGE) {
      return this->page->header.number_of_keys < (int64_t)RECORD_PER_PAGE;
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
 Call insert_key_and_offset() method\n");
    return false;
  }

  // Check whether it has room to insert
  if ( ! this->has_room(this) ) {
    return false;
  }

  // Insert.
  int64_t i, insertion_point = 0;
  // find the place to be inserted
  while (insertion_point < this->page->header.number_of_keys
      && this->page->content.records[insertion_point].key < record->key)
    insertion_point++;

  for (i = this->page->header.number_of_keys;
      i > insertion_point;
      i--) {
    memcpy(&this->page->content.records[i], 
        &this->page->content.records[i-1], sizeof(*record));
  }

  memcpy(&this->page->content.records[insertion_point], record, sizeof(*record));
  this->page->header.number_of_keys++;
  this->write(this);
  return true;
}


bool __insert_into_new_root(
    struct __page_object * const left, int64_t key,
    struct __page_object * const right){

  int64_t root_page_number = page_alloc(left->table_id);

  page_object_t root;
  page_object_constructor(&root, left->table_id, root_page_number);
  /** root.set_current_page_number(&root, root_page_number); */
  root.set_type(&root, INTERNAL_PAGE);

  root.page->header.one_more_page_offset =
    left->get_current_page_number(left) * PAGE_SIZE;

  root.page->content.key_and_offsets[0].key = key;
  root.page->content.key_and_offsets[0].page_offset =
    right->get_current_page_number(right) * PAGE_SIZE;
  root.page->header.number_of_keys++;

  left->page->header.linked_page_offset = root_page_number * PAGE_SIZE;
  right->page->header.linked_page_offset = root_page_number * PAGE_SIZE;

  left->write(left);
  right->write(right);
  root.write(&root);

  // write root offset to header page
  header_page[left->table_id].set_root_page_offset(&header_page[left->table_id],
      root_page_number * PAGE_SIZE);
  header_page[left->table_id].write(&header_page[left->table_id]);

  page_object_destructor(&root);

  return true;
}


bool __insert_into_node(struct __page_object * const node,
    int64_t left_index, int64_t key,
    const struct __page_object * const right) {
  int64_t i;

  // Precondition: Node is not full.
  assert(node->get_number_of_keys(node) < (int64_t)OFFSETS_PER_PAGE);

  for (i = node->get_number_of_keys(node) - 1; i > left_index; --i) {
    node->page->content.key_and_offsets[i + 1].key = 
      node->page->content.key_and_offsets[i].key;

    node->page->content.key_and_offsets[i + 1].page_offset = 
      node->page->content.key_and_offsets[i].page_offset;
  }

  node->page->content.key_and_offsets[left_index + 1].page_offset = 
    right->get_current_page_number(right) * PAGE_SIZE;

  node->page->content.key_and_offsets[left_index + 1].key = key;
  node->set_number_of_keys(node, node->get_number_of_keys(node) + 1);
  node->write(node);

  return true;
}


int64_t __get_left_index(const struct __page_object* const parent,
    const struct __page_object * const left) {
  int64_t i;

  int64_t left_page_offset = left->get_current_page_number(left) * PAGE_SIZE;

  // If left page is in one_more_page_offset, return -1;
  // Otherwise, return index.
  if (parent->page->header.one_more_page_offset == left_page_offset) {
    return -1;
  }

  for (i = 0; i < (int64_t)OFFSET_ORDER; ++i) {
    if (parent->page->content.key_and_offsets[i].page_offset
        == left_page_offset) {
      return i;
    }
  }

  fprintf(stderr, "(__get_left_index) Left page offset must be in parent page\n");
  assert(false);
  exit(1);
}


bool __insert_into_parent(
    struct __page_object * const left, int64_t key,
    struct __page_object * const right);

bool __insert_into_node_after_splitting(struct __page_object * const old_node,
    int64_t left_index, int64_t key, struct __page_object * const right){
  // We do not need to care the one_more_page_offset in header
  int64_t i, j;

  // Make temporary record array whose size is RECORD_ORDER
  internal_page_element_t *temp_key_and_offsets =
    (internal_page_element_t*)calloc(
        OFFSET_ORDER, sizeof(*temp_key_and_offsets));

  if (temp_key_and_offsets == NULL) {
    perror("(__insert_into_node_after_splitting)");
    exit(1);
  }

  for (i = 0, j = 0; i < old_node->get_number_of_keys(old_node);
      ++i, ++j) {
    if (j == left_index + 1) {
      j++;
    }
    memcpy(&temp_key_and_offsets[j],
        &old_node->page->content.key_and_offsets[i],
        sizeof(temp_key_and_offsets[j]));
  }

  temp_key_and_offsets[left_index + 1].page_offset =
    right->get_current_page_number(right) * PAGE_SIZE;
  
  temp_key_and_offsets[left_index + 1].key = key;


    /* Create the new node and copy
     * half the keys and pointers to the
     * old and half to the new.
     */  


  int64_t split = cut(OFFSET_ORDER);

  int64_t new_page_number = page_alloc(old_node->table_id);
  struct __page_object new_node;
  page_object_constructor(&new_node, old_node->table_id, new_page_number);

  /** new_node.set_current_page_number(&new_node, new_page_number); */
  new_node.set_type(&new_node, INTERNAL_PAGE);

  // clear old node

  memset(old_node->page->content.records, 0,
      sizeof(old_node->page->content.records));
  old_node->set_number_of_keys(old_node, 0);

  for (i = 0; i < split - 1; ++i) {
    memcpy(&old_node->page->content.key_and_offsets[i],
        &temp_key_and_offsets[i],
        sizeof(old_node->page->content.key_and_offsets[i]));
    old_node->page->header.number_of_keys++;
  }

  new_node.page->header.one_more_page_offset =
    temp_key_and_offsets[i].page_offset;

  int64_t k_prime = temp_key_and_offsets[split - 1].key;

  for (++i, j = 0; i < (int64_t)OFFSET_ORDER; ++i, ++j) {
    memcpy(&new_node.page->content.key_and_offsets[j],
        &temp_key_and_offsets[i],
        sizeof(old_node->page->content.key_and_offsets[i]));
    new_node.page->header.number_of_keys++;
  }

  free(temp_key_and_offsets);
  new_node.page->header.linked_page_offset = 
    old_node->page->header.linked_page_offset;

  struct __page_object child;
  // Leftmost page
  page_object_constructor(&child, old_node->table_id,
     new_node.page->header.one_more_page_offset / PAGE_SIZE);

  /** child.set_current_page_number(&child, */
  /**     new_node.page->header.one_more_page_offset / PAGE_SIZE); */
  /** child.read(&child); */
  child.page->header.linked_page_offset =
    new_node.current_page_number * PAGE_SIZE;
  child.write(&child);

  // The others
  for (i = 0; i < new_node.page->header.number_of_keys; ++i) {
    child.set_current_page_number(&child,
        new_node.page->content.key_and_offsets[i].page_offset / PAGE_SIZE);
    child.read(&child);

    child.page->header.linked_page_offset =
      new_node.current_page_number * PAGE_SIZE;
    child.write(&child);
  }
  page_object_destructor(&child);

  // Do not forget to write pages.
  old_node->write(old_node);
  new_node.write(&new_node);

  bool rt_value = __insert_into_parent(old_node, k_prime, &new_node);
  page_object_destructor(&new_node);
  return rt_value;
}

bool __insert_into_parent(
    struct __page_object * const left, int64_t key,
    struct __page_object * const right){

  int64_t left_index;
  int64_t parent_offset;

  parent_offset = left->page->header.linked_page_offset;

  /* Case: new root */
  if (parent_offset == 0) {
    return __insert_into_new_root(left, key, right);
  }

  /* Case: leaf or node. (Remainder of
   * function body.)
   */

  /* Find the parent's pointer to the left
   * node.
   */

  struct __page_object parent;
  page_object_constructor(&parent, left->table_id
      ,parent_offset / PAGE_SIZE);

  /** parent.set_current_page_number(&parent, parent_offset / PAGE_SIZE); */
  /** parent.read(&parent); */
  assert(parent.get_type(&parent) == INTERNAL_PAGE);

  left_index = __get_left_index(&parent, left);

  /* Simple case: the new key fits into the node.
  */

  bool rt_value;
  if (parent.get_number_of_keys(&parent) < (int64_t)OFFSET_ORDER - 1) {
    rt_value = __insert_into_node(&parent, left_index, key, right);
    page_object_destructor(&parent);
    return rt_value;
  }


  /* Harder case:  split a node in order
   * to preserve the B+ tree properties.
   */

  rt_value =  __insert_into_node_after_splitting(
      &parent, left_index, key, right);

  page_object_destructor(&parent);

  return rt_value;
}


bool __insert_record_after_splitting(struct __page_object * const this,
    const record_t * const record){
  int32_t i, j;


  if (this->type != LEAF_PAGE) {
    printf("(page_object_t->insert_record)\
        You've called wrong function.\n");
    printf("(page_object_t->insert_record)\
        This page is not a leaf page.\n");
    printf("(page_object_t->insert_record)\
        Call insert_key_and_offset_after_splitting() method\n");
    return false;
  }

  struct __page_object * const leaf = this;



  // Pseudo code
  //
  // Make a new page buffer.
  // Make temporary record array whose size is RECORD_ORDER
  // Set header of page.
  //
  // Find insertion_index to find a page to insert.
  // Copy all key and value to temporary record array.
  // While copying, if we meet insertion_index, increase iterater by 1.
  //
  // Insert record to temporary record array
  //
  // Make leaf clear.
  // Call split function.
  //
  // Old leaf page has records from 0 to split - 1
  // New leaf page has records from split to the end
  //
  // Free temporary record array
  //
  // link new and old page.
  //
  // Call insert_into_parent


  // Make a new page buffer.
  // Set header of page.

  // page_alloc returns empty page
  int64_t new_leaf_page_number = page_alloc(this->table_id);

  page_object_t new_leaf;
  // link page object and physical page
  page_object_constructor(&new_leaf, this->table_id, new_leaf_page_number);

  /** new_leaf.set_current_page_number(&new_leaf, new_leaf_page_number); */

  // set type as leaf.
  new_leaf.set_type(&new_leaf, LEAF_PAGE);

  // Make temporary record array whose size is RECORD_ORDER
  record_t *temp_records = (record_t*)calloc(RECORD_ORDER,
      sizeof(*temp_records));

  int64_t insertion_index = 0;
  while (insertion_index < (int64_t)RECORD_ORDER - 1
      && leaf->get_record(leaf, insertion_index)->key < record->key) {
    insertion_index++;
  }

  int32_t leaf_page_number_of_keys = leaf->get_number_of_keys(leaf);
  for (i = 0, j = 0; i < leaf_page_number_of_keys; ++i, j++) {
    if (i == insertion_index) {
      j++;
    }

    record_t * leaf_record = leaf->get_record(leaf, i);

    temp_records[j].key = leaf_record->key;
    memcpy(temp_records[j].value, leaf_record->value,
        sizeof(temp_records[j].value));
  }

  temp_records[insertion_index].key = record->key;
  memcpy(temp_records[insertion_index].value, record->value,
      sizeof(temp_records[j].value));

  // clear leaf page
  memset(leaf->page->content.records, 0,
      sizeof(leaf->page->content.records));
  leaf->set_number_of_keys(leaf, 0);

  int64_t split = cut(RECORD_ORDER - 1);

  for (i = 0; i < split; ++i) {
    memcpy(leaf->page->content.records[i].value,
        temp_records[i].value, sizeof(temp_records[i].value));
    leaf->page->content.records[i].key = temp_records[i].key;
    leaf->set_number_of_keys(leaf, leaf->get_number_of_keys(leaf) + 1);
  }


  for (i = split, j = 0; i < (int64_t)RECORD_ORDER; ++i, j++) {
    memcpy(new_leaf.page->content.records[j].value,
        temp_records[i].value, sizeof(temp_records[i].value));
    new_leaf.page->content.records[j].key = temp_records[i].key;
    new_leaf.set_number_of_keys(&new_leaf,
        new_leaf.get_number_of_keys(&new_leaf) + 1);
  }

  free(temp_records);

  // get link from old
  new_leaf.page->header.one_more_page_offset =
    leaf->page->header.one_more_page_offset;


  // link page from old to new
  leaf->page->header.one_more_page_offset =
    new_leaf.get_current_page_number(&new_leaf) * PAGE_SIZE;

  // Parent
  new_leaf.page->header.linked_page_offset =
    leaf->page->header.linked_page_offset;

  int64_t new_key = new_leaf.get_record(&new_leaf, 0)->key;

  // Write data to disk.
  leaf->write(leaf);
  new_leaf.write(&new_leaf);

  // call insert_into_parent function
  bool rt_value =  __insert_into_parent(leaf, new_key, &new_leaf);

  page_object_destructor(&new_leaf);

  return rt_value;
}



/** bool __insert_key_and_offset(struct __page_object * const this,
 *     const internal_page_element_t * const key_and_offset){
 *   if (this->type != INTERNAL_PAGE) {
 *     printf("(page_object_t->insert_key_and_offset)\
 *         You've called wrong function.\n");
 *     printf("(page_object_t->insert_key_and_offset)\
 *         This page is not an internal page.\n");
 *     printf("(page_object_t->insert_key_and_offset)\
 *         Call set_record() method\n");
 *     return false;
 *   }
 *
 *   // Check whether it has room to insert
 *   if ( ! this->has_room(this) ) {
 *     return false;
 *   }
 *
 *   // Insert.
 *   int64_t i, insertion_point = 0;
 *   // find the place to be inserted
 *   while (insertion_point < this->page->header.number_of_keys
 *       && this->page->content.key_and_offsets[insertion_point].key
 *       < key_and_offset->key)
 *     insertion_point++;
 *
 *   for (i = this->page->header.number_of_keys;
 *       i > insertion_point;
 *       i--) {
 *     memcpy(&this->page->content.key_and_offsets[i],
 *         &this->page->content.key_and_offsets[i-1], sizeof(*key_and_offset));
 *   }
 *
 *   memcpy(&this->page->content.key_and_offsets[insertion_point],
 *       key_and_offset, sizeof(*key_and_offset));
 *   this->page->header.number_of_keys++;
 *   this->write(this);
 *   return true;
 * } */


void __remove_record_from_page(struct __page_object * const this,
    const int64_t key) {
  int32_t i;
  assert(this->get_type(this) == LEAF_PAGE);

  // Find index of target record
  for (i = 0; i < (int32_t)RECORD_ORDER; ++i) {
    if (this->page->content.records[i].key == key) {
      break;
    }
  }
  assert(i < (int32_t)RECORD_ORDER);

  // Remove the key and shift other keys accordingly.
  // i is the idx of target record

  // Remove
  memset(&this->page->content.records[i], 0,
      sizeof(this->page->content.records[i]));

  // Compaction
  for (i++; i < this->page->header.number_of_keys; ++i) {
    memcpy(&this->page->content.records[i-1],
        &this->page->content.records[i],
        sizeof(this->page->content.records[i-1]));
  }

  // Remove redundancy
  // i-1 is last key idex
  // If no compaction, no redundancy is exist.
  /** if (1 < i && i < this->page->header.number_of_keys) {
    *   assert(memcmp(&this->page->content.records[i-1],
    *         &this->page->content.records[i-2],
    *         sizeof(this->page->content.records[i-1])) == 0);
    *   memset(&this->page->content.records[i-1], 0,
    *       sizeof(this->page->content.records[i-1]));
    * } */

  // Remove redundancy
  memset(&this->page->content.records[i-1], 0,
      sizeof(this->page->content.records[i-1]));

  // One key fewer
  this->page->header.number_of_keys--;

  // Below is happed only page is root page
  if (this->page->header.number_of_keys == 0) {
    // Check whether current page is root page
    assert(this->current_page_number * PAGE_SIZE
        == header_page[this->table_id].get_root_page_offset(&header_page[this->table_id]));
  }

  this->write(this);
}


int64_t __get_page_number_of_left(const struct __page_object * const this){
  struct __page_object __parent_page;
  page_object_constructor(&__parent_page, this->table_id,
      this->page->header.linked_page_offset / PAGE_SIZE);

  /** __parent_page.set_current_page_number(&__parent_page,
    *     this->page->header.linked_page_offset / PAGE_SIZE); */
  /** __parent_page.read(&__parent_page); */

  // Make it const to forbbid changing parent page's value
  const struct __page_object * const parent_page = &__parent_page;

  int64_t i;
  for (i = 0; i < parent_page->page->header.number_of_keys; ++i) {
    if (parent_page->page->content.key_and_offsets[i].page_offset
        == this->current_page_number * PAGE_SIZE) {
      // current page is found.
      break;
    }
  }

  // current page must exist!
  assert(i != parent_page->page->header.number_of_keys);

  // return page number of left page.
  //
  // (i - 1) means the left page.
  // Even though i == 0, it is okay.
  

  int64_t rt_value;
  if (i == 0) {
    rt_value = parent_page->page->header.one_more_page_offset / PAGE_SIZE;
  } else{
    rt_value = parent_page->page->content.key_and_offsets[i - 1].page_offset
      / PAGE_SIZE;
  }

  page_object_destructor(&__parent_page);
  return rt_value;
}


void __get_k_prime_index_and_its_value(
    const struct __page_object * const this,
    const int64_t neighbor_page_number, const bool neighbor_is_right,
    int64_t * const k_prime_index, int64_t * const k_prime) {

  struct __page_object __parent_page;
  page_object_constructor(&__parent_page, this->table_id,
     this->page->header.linked_page_offset / PAGE_SIZE);

  /** __parent_page.set_current_page_number(&__parent_page,
    *     this->page->header.linked_page_offset / PAGE_SIZE);
    * __parent_page.read(&__parent_page); */

  // Make it const to forbbid changing parent page's value
  const struct __page_object * const parent_page = &__parent_page;

  // Extra case. Neighbor page is in left.
  if (parent_page->page->header.one_more_page_offset / PAGE_SIZE
      == neighbor_page_number) {
    // neighbor page is found.
    *k_prime_index = 0;
    *k_prime = parent_page->page->content.key_and_offsets[0].key;
    return;
  }

  // Normal case. neighbor page is in right of current page.
  int64_t i;
  for (i = 0; i < parent_page->page->header.number_of_keys; ++i) {
    if (parent_page->page->content.key_and_offsets[i].page_offset
        == neighbor_page_number * PAGE_SIZE) {
      // neighbor page is found.
      *k_prime_index = i;
      *k_prime = parent_page->page->content.key_and_offsets[i].key;
      break;
    }
  }

  // When neighbor is in left, increase k_prime_index by 1
  if (neighbor_is_right == false) {
    i++;
    *k_prime_index = i;
    *k_prime = parent_page->page->content.key_and_offsets[i].key;
  }


  // neighbor page must exist!
  assert(i != parent_page->page->header.number_of_keys);

  page_object_destructor(&__parent_page);

}

void  __remove_key_and_offset_from_page(struct __page_object * const this,
    const int64_t key) {
  int32_t i;
  assert(this->get_type(this) == INTERNAL_PAGE);
  // Find index of target key_and_offset

  for (i = 0; i < (int32_t)OFFSET_ORDER; ++i) {
    if (this->page->content.key_and_offsets[i].key == key) {
      break;
    }
  }
  assert(i < (int32_t)OFFSET_ORDER);

  // Remove the key and shift other keys accordingly.
  // i is the idx of target record

  // Remove
  memset(&this->page->content.key_and_offsets[i], 0,
      sizeof(this->page->content.key_and_offsets[i]));

  // Compaction
  for (i++; i < this->page->header.number_of_keys; ++i) {
    memcpy(&this->page->content.key_and_offsets[i-1],
        &this->page->content.key_and_offsets[i],
        sizeof(this->page->content.key_and_offsets[i-1]));
  }

  // Remove redundancy
  // i-1 is last key idex
  /** if (1 < i && i < this->page->header.number_of_keys) {
    *   assert(memcmp(&this->page->content.key_and_offsets[i-1],
    *         &this->page->content.key_and_offsets[i-2],
    *         sizeof(this->page->content.key_and_offsets[i-1])) == 0);
    *   memset(&this->page->content.key_and_offsets[i-1], 0,
    *       sizeof(this->page->content.key_and_offsets[i-1]));
    * } */


  // Remove redundancy
    memset(&this->page->content.key_and_offsets[i-1], 0,
        sizeof(this->page->content.key_and_offsets[i-1]));


  // One key fewer
  this->page->header.number_of_keys--;

  // Below is happed only page is root page
  if (this->page->header.number_of_keys == 0) {
    // Check whether current page is root page
    assert(this->current_page_number * PAGE_SIZE
        == header_page[this->table_id].get_root_page_offset(&header_page[this->table_id]));
  }

  this->write(this);
}

// neighbor is right one.
int64_t __get_neighbor_index(struct __page_object * const parent,
    int64_t child_page_number) {
  int64_t i;

  if (parent->page->header.one_more_page_offset
      == child_page_number * PAGE_SIZE) {
    return 0;
  }

  for (i = 0; i < parent->page->header.number_of_keys; ++i) {
    if (parent->page->content.key_and_offsets[i].page_offset
        == child_page_number * PAGE_SIZE) {
      return i + 1;
    }
  }

  // Child page is not in parent. Error!
  fprintf(stderr, "(__get_neighbor_index) Child page is not in parent.\n");
  assert(false);
  exit(1);
}

bool __delete_key_and_offset_of_key(struct __page_object * const this,
    const int64_t key);

bool __coalesce_nodes(struct __page_object * this,
    struct __page_object * neighbor_page, const int64_t k_prime) {

  /* Swap neighbor with node if node is on the
   * extreme right and neighbor is to its left.
   */

  /** if (neighbor_is_right == false) {
    *   struct __page_object * temp = this;
    *   this = neighbor_page;
    *   neighbor_page = temp;
    * } */

  // Now, 'this' is in left, and 'neighbor_page' is in right.


  /* Starting point in 'this' for copying
   * keys and pointers from 'neighbor_page'.
   */

  int64_t this_insertion_index = this->get_number_of_keys(this);

  /* Case:  nonleaf node.
   * Append k_prime and the following pointer.
   * Append all pointers and keys from the neighbor.
   */

  this->page->content.key_and_offsets[this_insertion_index].key = k_prime;
  this->page->content.key_and_offsets[this_insertion_index].page_offset
    = neighbor_page->page->header.one_more_page_offset;
  this->page->header.number_of_keys++;

  int i,j, neighbor_end = neighbor_page->get_number_of_keys(neighbor_page);

  for (i = this_insertion_index + 1, j = 0; j < neighbor_end; ++i, ++j) {
    memcpy(&this->page->content.key_and_offsets[i],
        &neighbor_page->page->content.key_and_offsets[j],
        sizeof(this->page->content.key_and_offsets[i])
        );
    this->page->header.number_of_keys++;
  }


  /* All children must now point up to the same parent.
  */
  struct __page_object child_page;
  page_object_constructor(&child_page, this->table_id,
    this->page->content.key_and_offsets[0].page_offset / PAGE_SIZE);
  for (i = 0; i < this->page->header.number_of_keys; ++i) {
    child_page.set_current_page_number(&child_page,
        this->page->content.key_and_offsets[i].page_offset / PAGE_SIZE);
    child_page.read(&child_page);
    child_page.page->header.linked_page_offset = 
      this->current_page_number * PAGE_SIZE;
    child_page.write(&child_page);
  }

  this->write(this);

  // Get parent and remove neighbor from parent.
  struct __page_object parent;
  page_object_constructor(&parent, this->table_id,
      this->page->header.linked_page_offset / PAGE_SIZE);

  /** parent.set_current_page_number(&parent, */
  /**     this->page->header.linked_page_offset / PAGE_SIZE); */
  /** parent.read(&parent); */

  // delete key_and_offset from parent.
  bool result = __delete_key_and_offset_of_key(&parent, k_prime);

  // remove neighbor page
  if(page_free(this->table_id, neighbor_page->get_current_page_number(neighbor_page))
      == false) {
    fprintf(stderr, "(__coalesce_leaves) page_free() failed.\n");
    assert(false);
    exit(1);
  }

  if (parent.get_number_of_keys(&parent) == 0) {
    // It is occurred only in root
    assert(parent.get_current_page_number(&parent) * PAGE_SIZE
        == header_page[this->table_id].get_root_page_offset(&header_page[this->table_id]));

    // Remove root
    // TODO
    if(page_free(this->table_id, parent.get_current_page_number(&parent))
        == false) {
      fprintf(stderr, "(__coalesce_leaves) page_free() failed.\n");
      assert(false);
      exit(1);
    }

    // Make 'this' as root
    header_page[this->table_id].set_root_page_offset(&header_page[this->table_id],
        this->get_current_page_number(this) * PAGE_SIZE);
    this->page->header.linked_page_offset = 0;
    this->write(this);
  }

  page_object_destructor(&child_page);
  page_object_destructor(&parent);

  return result;

}

bool __redistribute_nodes(struct __page_object * this,
    struct __page_object * neighbor_page, const bool neighbor_is_right,
    const int64_t k_prime_index, const int64_t k_prime) {

  assert(this->get_type(this) == INTERNAL_PAGE
      && neighbor_page->get_type(neighbor_page) == INTERNAL_PAGE);

  // Here is two case
  // 1. this <- neighbor_page
  // 2. neighbor_page -> this.
  //
  // Always, record is moved from neighbor to this.
  // The difference is the location of each pages.

  // Get parent page.
  assert(this->page->header.linked_page_offset
      == neighbor_page->page->header.linked_page_offset);
  struct __page_object parent;
  page_object_constructor(&parent, this->table_id,
      this->page->header.linked_page_offset / PAGE_SIZE);

  /** parent.set_current_page_number(&parent,
    *     this->page->header.linked_page_offset / PAGE_SIZE);
    * parent.read(&parent); */


  // 1. this <- neighbor_page
  if (neighbor_is_right) {

    // Copy key_and_offset to temp
    internal_page_element_t temp_key_and_offset;
    temp_key_and_offset.key = k_prime;
    temp_key_and_offset.page_offset =
      neighbor_page->page->header.one_more_page_offset;

    // set new k_prime
    // new key must be bigger than old key
    assert(parent.page->content.key_and_offsets[k_prime_index].key
        < neighbor_page->page->content.key_and_offsets[0].key);
    // k_prime value is okay?
    // TODO: below assertion can be removed.
    // TODO: argument k_prime can be removed.
    assert(parent.page->content.key_and_offsets[k_prime_index].key
        == k_prime);


    parent.page->content.key_and_offsets[k_prime_index].key
      = neighbor_page->page->content.key_and_offsets[0].key;

    // Should we change the grand parent's k_prime?
    // No. grand parent's k_prime is a first key of 'this'.
    // We don't need to care about it.

    // remove neighbor's one_more_page_offset and compaction
    neighbor_page->page->header.one_more_page_offset = 
      neighbor_page->page->content.key_and_offsets[0].page_offset;

    int64_t i;
    int64_t n_of_key_in_neighbor = neighbor_page->page->header.number_of_keys;
    for (i = 1; i < n_of_key_in_neighbor; ++i) {
      memcpy(&neighbor_page->page->content.key_and_offsets[i-1],
          &neighbor_page->page->content.key_and_offsets[i],
          sizeof(neighbor_page->page->content.key_and_offsets[i-1]));
    }

    memset(&neighbor_page->page->content.key_and_offsets[n_of_key_in_neighbor - 1], 0,
        sizeof(neighbor_page->page->content.key_and_offsets[n_of_key_in_neighbor - 1]));
    neighbor_page->page->header.number_of_keys--;


    // set new record
    this->set_key_and_offset(this, 
        this->page->header.number_of_keys, &temp_key_and_offset);

    // increae num_of_key of this
    this->page->header.number_of_keys++;


    // change parent of added record's page.

    struct __page_object child_page;
    page_object_constructor(&child_page, this->table_id,
      temp_key_and_offset.page_offset / PAGE_SIZE);

    /** child_page.current_page_number =
      *   temp_key_and_offset.page_offset / PAGE_SIZE;
      * child_page.read(&child_page); */

    child_page.page->header.linked_page_offset =
      this->current_page_number * PAGE_SIZE;

    child_page.write(&child_page);

    page_object_destructor(&child_page);
  } else {
    // 2. neighbor_page -> this

    // Precondition check

    // k_prime value is okay?
    assert(parent.page->content.key_and_offsets[k_prime_index].key
        == k_prime);

    // new key must be smaller than old key
    assert(parent.page->content.key_and_offsets[k_prime_index].key
        > neighbor_page->page->content.
        key_and_offsets[neighbor_page->page->header.number_of_keys - 1].key);






    // get last item of neighbor_page
    internal_page_element_t temp_key_and_offset;
    memcpy( &temp_key_and_offset,
        &neighbor_page->page->content.
        key_and_offsets[neighbor_page->page->header.number_of_keys - 1],
        sizeof(temp_key_and_offset));

    // remove last of neighbor_page
    neighbor_page->page->header.number_of_keys--;
    memset(&neighbor_page->page->content.
        key_and_offsets[neighbor_page->page->header.number_of_keys],
        0, sizeof(temp_key_and_offset));


    // make a space in this
    int64_t i;
    for (i = this->page->header.number_of_keys; i > 0; --i) {
      memcpy(&this->page->content.key_and_offsets[i],
          &this->page->content.key_and_offsets[i - 1],
          sizeof(this->page->content.key_and_offsets[i]));
    }
    memset(&this->page->content.key_and_offsets[0], 0,
        sizeof(this->page->content.key_and_offsets[0]));

    this->page->header.number_of_keys++;

    // Get key from parent( == k_prime) ,
    // and key offset from neighbor_page
    this->page->content.key_and_offsets[0].key = k_prime;

    this->page->content.key_and_offsets[0].page_offset =
      this->page->header.one_more_page_offset;

    this->page->header.one_more_page_offset = 
      temp_key_and_offset.page_offset;

    // set parent. change k_prime to new value
    parent.page->content.key_and_offsets[k_prime_index].key =
      temp_key_and_offset.key;


    // change parent of added record's page.

    struct __page_object child_page;
    page_object_constructor(&child_page, this->table_id,
      temp_key_and_offset.page_offset / PAGE_SIZE);

    /** child_page.current_page_number =
      *   temp_key_and_offset.page_offset / PAGE_SIZE;
      * child_page.read(&child_page); */

    child_page.page->header.linked_page_offset =
      this->current_page_number * PAGE_SIZE;

    child_page.write(&child_page);

    page_object_destructor(&child_page);
  }



  // Write to disk
  this->write(this);
  neighbor_page->write(neighbor_page);
  parent.write(&parent);

  page_object_destructor(&parent);


  return true;


}

bool __coalesce_nodes_when_parent_is_root(
    struct __page_object * this, 
    struct __page_object * const parent,
    struct __page_object * neighbor) {
  const int table_id = this->table_id;

  // Here, only special case.
  assert(this->page->header.linked_page_offset
      == neighbor->page->header.linked_page_offset);
  assert(this->page->header.linked_page_offset
      == parent->current_page_number * PAGE_SIZE);

  // Please...

  // coalesce this and neighbor.
  // 'this' will be a new root.


  /* Swap neighbor with node if node is on the
   * extreme right and neighbor is to its left.
   */

  /** if (neighbor_is_right == false) {
   *   struct __page_object * temp = this;
   *   this = neighbor;
   *   neighbor = temp;
   * } */

  // Now, 'this' is in left, and 'neighbor_page' is in right.
  struct __page_object leftmost;
  page_object_constructor(&leftmost, this->table_id,
      neighbor->page->header.one_more_page_offset / PAGE_SIZE);

  /** leftmost.set_current_page_number(&leftmost,
    *     neighbor->page->header.one_more_page_offset / PAGE_SIZE);
    * leftmost.read(&leftmost); */

  assert(leftmost.get_type(&leftmost) == LEAF_PAGE);

  // control one_more_page_offset of neighbor...
  this->page->content.key_and_offsets[this->page->header.number_of_keys].key = 
    leftmost.page->content.records[0].key;
  this->page->content.
    key_and_offsets[this->page->header.number_of_keys].page_offset = 
    neighbor->page->header.one_more_page_offset;

  this->page->header.number_of_keys++;



  int i,j, neighbor_end = neighbor->get_number_of_keys(neighbor);

  for (i = this->page->header.number_of_keys, j = 0;
      j < neighbor_end; ++i, ++j) {

    memcpy(&this->page->content.key_and_offsets[i],
        &neighbor->page->content.key_and_offsets[j],
        sizeof(this->page->content.key_and_offsets[i])
        );
    this->page->header.number_of_keys++;
  }


  /* All children must now point up to the same parent.
  */
  struct __page_object child_page;
  page_object_constructor(&child_page, this->table_id,
        this->page->content.key_and_offsets[0].page_offset / PAGE_SIZE);
  for (i = 0; i < this->page->header.number_of_keys; ++i) {
    child_page.set_current_page_number(&child_page,
        this->page->content.key_and_offsets[i].page_offset / PAGE_SIZE);
    child_page.read(&child_page);
    child_page.page->header.linked_page_offset = 
      this->current_page_number * PAGE_SIZE;
    child_page.write(&child_page);
  }


  // free parent and neighbor
  if(page_free(this->table_id, neighbor->get_current_page_number(neighbor))
      == false) {
    fprintf(stderr, "(__coalesce_leaves_when...) page_free() failed.\n");
    assert(false);
    exit(1);
  }

  if(page_free(this->table_id, parent->get_current_page_number(parent))
      == false) {
    fprintf(stderr, "(__coalesce_leaves_when...) page_free() failed.\n");
    assert(false);
    exit(1);
  }

  // Make 'this' as root
  header_page[table_id].set_root_page_offset(&header_page[table_id],
      this->get_current_page_number(this) * PAGE_SIZE);
  this->page->header.linked_page_offset = 0;
  this->write(this);


  page_object_destructor(&leftmost);
  page_object_destructor(&child_page);

  return true;
}

bool __redistribute_nodes_when_parent_is_root(
    struct __page_object * this, 
    struct __page_object * const parent,
    struct __page_object * neighbor) {

  // Here, only special case.
  assert(this->page->header.linked_page_offset
      == neighbor->page->header.linked_page_offset);
  assert(this->page->header.linked_page_offset
      == parent->current_page_number * PAGE_SIZE);

  // 'this' is in left, and 'neighbor_page' is in right.
  // parent has only one element.
  assert(parent->page->header.number_of_keys == 1);


  struct __page_object leftmost;
  page_object_constructor(&leftmost, this->table_id,
      neighbor->page->header.one_more_page_offset / PAGE_SIZE);

  /** leftmost.set_current_page_number(&leftmost,
    *     neighbor->page->header.one_more_page_offset / PAGE_SIZE);
    * leftmost.read(&leftmost); */

  assert(leftmost.get_type(&leftmost) == INTERNAL_PAGE);


  internal_page_element_t temp_key_and_offset;
  temp_key_and_offset.key = leftmost.page->content.key_and_offsets[0].key;
  temp_key_and_offset.page_offset = neighbor->page->header.one_more_page_offset;

  // add temp_key_and_offset to 'this'
  memcpy(&this->page->content.key_and_offsets[this->page->header.number_of_keys],
      &temp_key_and_offset, sizeof(temp_key_and_offset));
  this->page->header.number_of_keys++;

  // remove one key from neighbor.
  neighbor->page->header.one_more_page_offset = 
    neighbor->page->content.key_and_offsets[0].page_offset;

  int64_t i;
  for (i = 1; i < neighbor->page->header.number_of_keys; ++i) {
    memcpy(&neighbor->page->content.key_and_offsets[i - 1],
        &neighbor->page->content.key_and_offsets[i],
        sizeof(neighbor->page->content.key_and_offsets[i - 1]));
  }
  i--; // i is pointing the last element
  memset(&neighbor->page->content.key_and_offsets[i], 0,
      sizeof(neighbor->page->content.key_and_offsets[i]));
  neighbor->page->header.number_of_keys--;


  // change parent's key
  parent->page->content.key_and_offsets[0].key
    = neighbor->page->content.key_and_offsets[0].key;

  // change leftmost's parent
  // Now neighbor's leftmost is in this's rightmost.
  leftmost.page->header.linked_page_offset = 
    this->current_page_number * PAGE_SIZE;


  // write
  this->write(this);
  parent->write(parent);
  neighbor->write(neighbor);
  leftmost.write(&leftmost);


  page_object_destructor(&leftmost);

  return true;
}

bool __delete_key_and_offset_of_key(struct __page_object * const this,
    const int64_t key){
  // This function is called only current page is INTERNAL_PAGE.
  assert(this->get_type(this) == INTERNAL_PAGE);
  int table_id = this->table_id;

  // Remove key and value from page.

  __remove_key_and_offset_from_page(this, key);

  /* Case:  key_and_offset deletion from the root. 
  */

  // Do nothing.
  if (this->get_current_page_number(this) * PAGE_SIZE
      == header_page[table_id].get_root_page_offset(&header_page[table_id])) {
    return true;
  }

  // TODO: Below codes are not yet debugged.



  /* Case: deletion from a key_and_offset below the root.
   * (Rest of function body.)
   */

  // This page is not a root. number_of_keys cannot down to zero.

  /* Determine minimum allowable size of node,
   * to be preserved after deletion.
   */

  int64_t min_keys = cut(OFFSET_ORDER - 1); // min_keys = 124
  if (this->get_number_of_keys(this) >= min_keys) {
    return true;
  }

  /* Case:  node falls below minimum.
   * Either coalescence or redistribution
   * is needed.
   */

  /* Find the appropriate neighbor node with which
   * to coalesce.
   * Also find the key (k_prime) in the parent
   * between the pointer to node n and the pointer
   * to the neighbor.
   */

  // To get neighbor's index,
  // Get parent page first.

  struct __page_object parent;
  page_object_constructor(&parent, this->table_id,
      this->page->header.linked_page_offset / PAGE_SIZE);

  /** parent.set_current_page_number(&parent,
    *     this->page->header.linked_page_offset / PAGE_SIZE);
    * parent.read(&parent); */

  // Get neighbor's index,
  // Here is the start of hell...
  int64_t neighbor_index = __get_neighbor_index(&parent,
      this->get_current_page_number(this));

  // If it is true, it is normal case: 'this' is left, 'neighbor' is right
  // If it is false, 'this' is rightmost page. 'neighbor' should be left
  bool neighbor_is_right = neighbor_index < parent.page->header.number_of_keys;

  // k_prime_index will be a real neighbor_index
  int64_t k_prime_index = neighbor_is_right ?
    neighbor_index : neighbor_index - 2;

  int64_t k_prime;

  if (k_prime_index == -1) {
    // Here is last problem place!!!
    // This case is occurred only when parent is root.
    assert(parent.current_page_number * PAGE_SIZE
        == header_page[table_id].get_root_page_offset(&header_page[table_id]));
    k_prime = -1;
    /** assert(false); */
  } else {
    k_prime = parent.page->content.key_and_offsets[k_prime_index].key;
  }


  // Get neighbor page
  struct __page_object neighbor;
  page_object_constructor(&neighbor, this->table_id,
      parent.page->content.key_and_offsets[k_prime_index].page_offset
      / PAGE_SIZE);

  /** neighbor.set_current_page_number(&neighbor,
    *     parent.page->content.key_and_offsets[k_prime_index].page_offset
    *     / PAGE_SIZE);
    * neighbor.read(&neighbor); */

  int64_t capacity = OFFSET_ORDER;


  if (neighbor_is_right == false) {
    k_prime_index++;
    k_prime = parent.page->content.key_and_offsets[k_prime_index].key;
  }


  /* Coalescence. */

  bool rt_value;
  // Below 1 is the one_more_page_offset of neighbor
  if (1 + neighbor.get_number_of_keys(&neighbor)
      + this->get_number_of_keys(this) < capacity){


    if (k_prime_index == -1) {
      //TODO Below will be debugged later...
      rt_value =  __coalesce_nodes_when_parent_is_root(
          &neighbor, &parent, this);
    } else {

      // When neighbor is left, k_prime which will be deleted should be corrected
      if (neighbor_is_right == false) {
        rt_value =  __coalesce_nodes(&neighbor, this, k_prime);
      } else {
        rt_value =  __coalesce_nodes(this, &neighbor, k_prime);
      }
    }
  }

  /* Redistribution. */
  else {
    //TODO: case should be splitted.
    if (k_prime_index == -1) {
      //TODO
      /** assert(false); // It can be happen? why not? */
      rt_value =  __redistribute_nodes_when_parent_is_root(
          &neighbor, &parent, this);
    } else {
      rt_value =  __redistribute_nodes(this, &neighbor, neighbor_is_right,
          k_prime_index, k_prime);
    }
  }



  page_object_destructor(&parent);
  page_object_destructor(&neighbor);

  return rt_value;
}



extern const int8_t empty_page_dummy[PAGE_SIZE];
bool __coalesce_leaves(struct __page_object * this,
    struct __page_object * neighbor_page,
    const int64_t k_prime) {
  const int table_id = this->table_id;
  // Normal situations: 'this' is left. 'neighbor_page' is right.

  // If 'neighbor_page' is left and 'this' is right, swap.
  // (When 'this' is the rightmost page in a node
  /** if (this->page->header.one_more_page_offset == 0) {
   *   struct __page_object * temp = this;
   *   this = neighbor_page;
   *   neighbor_page = temp;
   * } */

  // Now, 'neighbor_page' is the right sibling of 'this'
  // Coalescence!


  /* Starting point in 'this' for copying
   * keys and pointers from 'neighbor_page'.
   */

  int64_t this_insertion_index = this->get_number_of_keys(this);

  /* In a leaf, append the keys and values of
   * neighbor_page to 'this'.
   * Set the this's one_more_page_offset to point to
   * what had been neighbor_page's one_more_page_offset.
   */

  int64_t i,j;
  for (i = this_insertion_index, j = 0;
      j < neighbor_page->page->header.number_of_keys; ++i, j++) {
    memcpy(&this->page->content.records[i],
        &neighbor_page->page->content.records[j],
        sizeof(this->page->content.records[i]));
    this->page->header.number_of_keys++;
  }
  this->page->header.one_more_page_offset = 
    neighbor_page->page->header.one_more_page_offset;

  this->write(this);
  /** neighbor_page->write(neighbor_page); */

  // Get parent and remove neighbor from parent.
  struct __page_object parent;
  page_object_constructor(&parent, this->table_id,
      this->page->header.linked_page_offset / PAGE_SIZE);

  /** parent.set_current_page_number(&parent,
    *     this->page->header.linked_page_offset / PAGE_SIZE);
    * parent.read(&parent); */

  bool result = __delete_key_and_offset_of_key(&parent, k_prime);


  // remove neighbor page.
  if(page_free(this->table_id, neighbor_page->get_current_page_number(neighbor_page))
      == false) {
    fprintf(stderr, "(__coalesce_leaves) page_free() failed.\n");
    assert(false);
    exit(1);
  }

  // If parent is free page, just return
  if (memcmp(&parent.page->header.is_leaf,
      &empty_page_dummy[sizeof(int64_t)],
      PAGE_SIZE - sizeof(int64_t)) == 0) {
    // go to return
    
    
    // If not, Check whether parent is root
  } else {
    /** if (parent.get_number_of_keys(&parent) == 1) { */
    if (parent.get_number_of_keys(&parent) == 0) {
      // It is occurred only in root
      assert(parent.get_current_page_number(&parent) * PAGE_SIZE
          == header_page[table_id].get_root_page_offset(&header_page[table_id]));

      // Remove root
      // TODO
      if(page_free(this->table_id, parent.get_current_page_number(&parent))
          == false) {
        fprintf(stderr, "(__coalesce_leaves) page_free() failed.\n");
        assert(false);
        exit(1);
      }

      // Make 'this' as root
      header_page[table_id].set_root_page_offset(&header_page[table_id],
          this->get_current_page_number(this) * PAGE_SIZE);
      this->page->header.linked_page_offset = 0;
      this->write(this);
    }
  }

  page_object_destructor(&parent);

  return result;
  }


  bool __redistribute_leaves(struct __page_object * this,
      struct __page_object * neighbor_page, const bool neighbor_is_right,
      const int64_t k_prime_index, const int64_t k_prime) {

    assert(this->get_type(this) == LEAF_PAGE
        && neighbor_page->get_type(neighbor_page) == LEAF_PAGE);

    // Here is two case
    // 1. this <- neighbor_page
    // 2. neighbor_page -> this.
    //
    // Always, record is moved from neighbor to this.
    // The difference is the location of each pages.

    // Get parent
    assert(this->page->header.linked_page_offset
        == neighbor_page->page->header.linked_page_offset);
    struct __page_object parent;
    page_object_constructor(&parent, this->table_id,
        this->page->header.linked_page_offset / PAGE_SIZE);

    /** parent.set_current_page_number(&parent,
     *     this->page->header.linked_page_offset / PAGE_SIZE);
     * parent.read(&parent); */


    // 1. this <- neighbor_page
    if (neighbor_is_right) {

      // Copy first record to temp
      record_t temp_record;
      memcpy(&temp_record, &neighbor_page->page->content.records[0],
          sizeof(temp_record));

      // remove neighbor's first record and compaction
      int64_t i;
      int64_t n_of_key_in_neighbor = neighbor_page->page->header.number_of_keys;
      for (i = 1; i < n_of_key_in_neighbor; ++i) {
        memcpy(&neighbor_page->page->content.records[i-1],
            &neighbor_page->page->content.records[i],
            sizeof(neighbor_page->page->content.records[i-1]));
      }

      memset(&neighbor_page->page->content.records[n_of_key_in_neighbor - 1], 0,
          sizeof(neighbor_page->page->content.records[n_of_key_in_neighbor - 1]));
      neighbor_page->page->header.number_of_keys--;


      // set new record
      this->set_record(this, 
          this->page->header.number_of_keys, &temp_record);

      // increae num_of_key of this
      this->page->header.number_of_keys++;

      // set new k_prime
      // new key must be bigger than old key
      assert(parent.page->content.key_and_offsets[k_prime_index].key
          < neighbor_page->page->content.records[0].key);
      // k_prime value is okay?
      // TODO: below assertion can be removed.
      // TODO: argument k_prime can be removed.
      assert(parent.page->content.key_and_offsets[k_prime_index].key
          == k_prime);








      parent.page->content.key_and_offsets[k_prime_index].key
        = neighbor_page->page->content.records[0].key;
    } else {
      // 2. neighbor_page -> this.
      // copy neighbor's last to this
      record_t temp_record;
      memcpy(&temp_record,
          &neighbor_page->page->content.
          records[neighbor_page->page->header.number_of_keys - 1],
          sizeof(temp_record));

      // remove neighbor's last record
      memset(&neighbor_page->page->content.
          records[neighbor_page->page->header.number_of_keys - 1],
          0, sizeof(neighbor_page->page->content.records[0]));
      neighbor_page->page->header.number_of_keys--;

      // empty first of 'this'

      int64_t i;
      int64_t n_of_key_in_this = this->page->header.number_of_keys;
      for (i = n_of_key_in_this; i > 0; --i) {
        memcpy(&this->page->content.records[i],
            &this->page->content.records[i-1],
            sizeof(this->page->content.records[i]));
      }
      this->page->header.number_of_keys++;
      memcpy(&this->page->content.records[0], &temp_record,
          sizeof(temp_record));


      // set new k_prime
      // new key must be smaller than old key

      assert(parent.page->content.key_and_offsets[k_prime_index].key
          > this->page->content.records[0].key);

      // k_prime value is okay?
      // TODO: below assertion can be removed.
      // TODO: argument k_prime can be removed.
      assert(parent.page->content.key_and_offsets[k_prime_index].key
          == k_prime);



      parent.page->content.key_and_offsets[k_prime_index].key
        = this->page->content.records[0].key;
    }

    // Write to disk
    this->write(this);
    neighbor_page->write(neighbor_page);
    parent.write(&parent);


    page_object_destructor(&parent);

    return true;
  }

  bool __is_this_rightmost_in_parent(const struct __page_object * const this) {
    struct __page_object parent;
    page_object_constructor(&parent, this->table_id,
        this->page->header.linked_page_offset / PAGE_SIZE);

    /** parent.set_current_page_number(&parent,
     *     this->page->header.linked_page_offset / PAGE_SIZE);
     * parent.read(&parent); */

    bool rt_value;
    if (parent.page->content.
        key_and_offsets[parent.page->header.number_of_keys - 1].page_offset
        == this->current_page_number * PAGE_SIZE) {
      rt_value =  true;
    } else {
      rt_value =  false;
    }

    page_object_destructor(&parent);
    return rt_value;
  }


  bool __delete_record_of_key(struct __page_object * const this,
      const int64_t key){
    const int table_id = this->table_id;
    // This function is called only current page is LEAF_PAGE.
    assert(this->get_type(this) == LEAF_PAGE);

    // Remove key and value from page.

    __remove_record_from_page(this, key);


    /* Case:  Record deletion from the root. 
    */

    // Do nothing.
    if (this->get_current_page_number(this) * PAGE_SIZE
        == header_page[table_id].get_root_page_offset(&header_page[table_id])) {
      return true;
    }


    /* Case:  deletion from a node below the root.
     * (Rest of function body.)
     */

    /* Determine minimum allowable size of node,
     * to be preserved after deletion.
     */

    int64_t min_keys = cut(RECORD_ORDER - 1); // min_keys == 16
    if (this->get_number_of_keys(this) >= min_keys) {
      return true;
    }

    /* Case:  node falls below minimum.
     * Either coalescence or redistribution
     * is needed.
     */

    /* Find the appropriate neighbor node with which
     * to coalesce.
     * Also find the key (k_prime) in the parent
     * between the pointer to node n and the pointer
     * to the neighbor.
     */

    // neighbor is right sibling
    int64_t neighbor_page_number =
      this->page->header.one_more_page_offset / PAGE_SIZE;

    // It means that current page is rightmost page.
    // neighbor will be the left page of current page.



    // When neighbor_page_number == 0, it means that it is the last page
    // of whole B+Tree

    /** if (neighbor_page_number == 0) { */
    bool neighbor_is_right = true;
    if (neighbor_page_number == 0
        || __is_this_rightmost_in_parent(this)) {
      neighbor_page_number = __get_page_number_of_left(this);
      neighbor_is_right = false;
    }

    // At least one neighbor page is exists
    // becuase current page is not a root.
    assert(neighbor_page_number != 0);

    int64_t k_prime_index = -1;
    int64_t k_prime = -1;
    __get_k_prime_index_and_its_value(
        this, neighbor_page_number, neighbor_is_right,
        &k_prime_index, &k_prime);

    assert(k_prime_index != -1 && k_prime != -1);

    // Capacity?
    int64_t capacity = (int64_t)RECORD_ORDER;


    // Get neighbor page
    struct __page_object neighbor_page;
    page_object_constructor(&neighbor_page, this->table_id,
        neighbor_page_number);

    /** neighbor_page.set_current_page_number(&neighbor_page, neighbor_page_number);
     * neighbor_page.read(&neighbor_page); */

    /* Coalescence. */
    bool rt_value;
    if (neighbor_page.page->header.number_of_keys
        + this->page->header.number_of_keys
        < capacity) {

      if (neighbor_is_right == true) {
        rt_value = __coalesce_leaves(this, &neighbor_page, k_prime);
      } else {
        rt_value = __coalesce_leaves(&neighbor_page, this, k_prime);
      }
    }

    /* Redistribution. */

    else {
      rt_value = __redistribute_leaves(
          this, &neighbor_page, neighbor_is_right, k_prime_index, k_prime);
    }

    page_object_destructor(&neighbor_page);
    return rt_value;
  }


  void page_object_constructor(page_object_t * const this,
      const int32_t table_id,
      const int64_t page_number) {
    memset(this, 0, sizeof(*this));
    this->this = this;
    this->table_id = table_id;
    this->frame = NULL;
    this->current_page_number = page_number;

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
    /** this->insert_key_and_offset = __insert_key_and_offset; */
    this->insert_record_after_splitting =
      __insert_record_after_splitting;


    this->delete_record_of_key = __delete_record_of_key;


    // Read it for cache
    this->read(this);
  }


  // release frame
  void page_object_destructor(page_object_t * const this) {
    if (this->frame != NULL) {
      buf_mgr.release_frame(&buf_mgr, this->frame);
    }
    memset(this, 0, sizeof(*this));
  }
