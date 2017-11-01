#include "bpt_page_object.h"
#include "bpt_free_page_manager.h"
#include <string.h>
#include <assert.h>

extern int32_t db;

extern header_object_t *header_page;

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

  go_to_page_number(this->current_page_number);
  memset(&this->page, 0, PAGE_SIZE);
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
    assert(false);
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
  if (type == LEAF_PAGE) {
    this->page.header.is_leaf = 1;
  } else if (type == INTERNAL_PAGE){
    this->page.header.is_leaf = 0;
  } else {
    fprintf(stderr, "(__set_page_type) wrong page type\n");
    assert(false);
    exit(1);
  }
  this->write(this);
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
  return this->page.header.number_of_keys;
}

static void __set_number_of_keys(struct __page_object * const this,
    const int32_t number_of_keys){
  this->page.header.number_of_keys = number_of_keys;
  this->write(this);
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

  return (internal_page_element_t*)&this->page.content.key_and_offsets[idx];
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

  this->page.content.key_and_offsets[idx] = *key_and_offset;
  this->write(this);
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

  return (record_t*)&this->page.content.records[idx];
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

  this->page.content.records[idx] = *record;
  this->write(this);
}

static bool __has_room(const struct __page_object * const this) {
  if (this->type == INTERNAL_PAGE) {
      return this->page.header.number_of_keys < (int64_t)OFFSETS_PER_PAGE;
  } else if (this->type == LEAF_PAGE) {
      return this->page.header.number_of_keys < (int64_t)RECORD_PER_PAGE;
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


bool __insert_into_new_root(
    struct __page_object * const left, int64_t key,
    struct __page_object * const right){

  int64_t root_page_number = page_alloc();

  page_object_t root;
  page_object_constructor(&root);

  root.set_current_page_number(&root, root_page_number);
  root.set_type(&root, INTERNAL_PAGE);

  root.page.header.one_more_page_offset =
    left->get_current_page_number(left) * PAGE_SIZE;

  root.page.content.key_and_offsets[0].key = key;
  root.page.content.key_and_offsets[0].page_offset =
    right->get_current_page_number(right) * PAGE_SIZE;
  root.page.header.number_of_keys++;

  left->page.header.linked_page_offset = root_page_number * PAGE_SIZE;
  right->page.header.linked_page_offset = root_page_number * PAGE_SIZE;

  left->write(left);
  right->write(right);
  root.write(&root);

  // write root offset to header page
  header_page->set_root_page_offset(header_page,
      root_page_number * PAGE_SIZE);

  return true;
}


bool __insert_into_node(struct __page_object * const node,
    int64_t left_index, int64_t key,
    const struct __page_object * const right) {
  int64_t i;

  // Precondition: Node is not full.
  assert(node->get_number_of_keys(node) < (int64_t)OFFSETS_PER_PAGE);

  //TODO
  for (i = node->get_number_of_keys(node) - 1; i > left_index; --i) {
    node->page.content.key_and_offsets[i + 1].key = 
      node->page.content.key_and_offsets[i].key;

    node->page.content.key_and_offsets[i + 1].page_offset = 
      node->page.content.key_and_offsets[i].page_offset;
  }

  node->page.content.key_and_offsets[left_index + 1].page_offset = 
    right->get_current_page_number(right) * PAGE_SIZE;

  node->page.content.key_and_offsets[left_index + 1].key = key;
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
  if (parent->page.header.one_more_page_offset == left_page_offset) {
    return -1;
  }

  for (i = 0; i < (int64_t)OFFSET_ORDER; ++i) {
    if (parent->page.content.key_and_offsets[i].page_offset
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

//TODO: Definitely here is the problem place.
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
        &old_node->page.content.key_and_offsets[i],
        sizeof(temp_key_and_offsets[j]));
  }

  temp_key_and_offsets[left_index + 1].page_offset =
    right->get_current_page_number(right) * PAGE_SIZE;
  
  temp_key_and_offsets[left_index + 1].key = key;


    /* Create the new node and copy
     * half the keys and pointers to the
     * old and half to the new.
     */  


  //TODO
  int64_t split = cut(OFFSET_ORDER);

  int64_t new_page_number = page_alloc();
  struct __page_object new_node;
  page_object_constructor(&new_node);

  new_node.set_current_page_number(&new_node, new_page_number);
  new_node.set_type(&new_node, INTERNAL_PAGE);

  // clear old node

  memset(old_node->page.content.records, 0,
      sizeof(old_node->page.content.records));
  old_node->set_number_of_keys(old_node, 0);

  for (i = 0; i < split - 1; ++i) {
    memcpy(&old_node->page.content.key_and_offsets[i],
        &temp_key_and_offsets[i],
        sizeof(old_node->page.content.key_and_offsets[i]));
    old_node->page.header.number_of_keys++;
  }

  new_node.page.header.one_more_page_offset =
    temp_key_and_offsets[i].page_offset;

  int64_t k_prime = temp_key_and_offsets[split - 1].key;

  for (++i, j = 0; i < (int64_t)OFFSET_ORDER; ++i, ++j) {
    memcpy(&new_node.page.content.key_and_offsets[j],
        &temp_key_and_offsets[i],
        sizeof(old_node->page.content.key_and_offsets[i]));
    new_node.page.header.number_of_keys++;
  }

  free(temp_key_and_offsets);
  new_node.page.header.linked_page_offset = 
    old_node->page.header.linked_page_offset;

  struct __page_object child;
  page_object_constructor(&child);

  // Leftmost page
  child.set_current_page_number(&child,
      new_node.page.header.one_more_page_offset / PAGE_SIZE);
  child.read(&child);
  child.page.header.linked_page_offset =
    new_node.current_page_number * PAGE_SIZE;
  child.write(&child);

  // The others
  for (i = 0; i < new_node.page.header.number_of_keys; ++i) {
    child.set_current_page_number(&child,
        new_node.page.content.key_and_offsets[i].page_offset / PAGE_SIZE);
    child.read(&child);

    child.page.header.linked_page_offset =
      new_node.current_page_number * PAGE_SIZE;
    child.write(&child);
  }

  // Do not forget to write pages.
  old_node->write(old_node);
  new_node.write(&new_node);

  return __insert_into_parent(old_node, k_prime, &new_node);
}

bool __insert_into_parent(
    struct __page_object * const left, int64_t key,
    struct __page_object * const right){

  int64_t left_index;
  int64_t parent_offset;

  parent_offset = left->page.header.linked_page_offset;

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
  page_object_constructor(&parent);

  parent.set_current_page_number(&parent, parent_offset / PAGE_SIZE);
  parent.read(&parent);
  assert(parent.get_type(&parent) == INTERNAL_PAGE);

  left_index = __get_left_index(&parent, left);

  /* Simple case: the new key fits into the node.
  */

  if (parent.get_number_of_keys(&parent) < (int64_t)OFFSET_ORDER - 1) {
    return __insert_into_node(&parent, left_index, key, right);
  }


  /* Harder case:  split a node in order
   * to preserve the B+ tree properties.
   */

  return __insert_into_node_after_splitting(
      &parent, left_index, key, right);
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

  //TODO: Implementing
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
  int64_t new_leaf_page_number = page_alloc();

  page_object_t new_leaf;
  page_object_constructor(&new_leaf);

  // link page object and physical page
  new_leaf.set_current_page_number(&new_leaf, new_leaf_page_number);

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
  memset(leaf->page.content.records, 0,
      sizeof(leaf->page.content.records));
  leaf->set_number_of_keys(leaf, 0);

  int64_t split = cut(RECORD_ORDER - 1);

  for (i = 0; i < split; ++i) {
    memcpy(leaf->page.content.records[i].value,
        temp_records[i].value, sizeof(temp_records[i].value));
    leaf->page.content.records[i].key = temp_records[i].key;
    leaf->set_number_of_keys(leaf, leaf->get_number_of_keys(leaf) + 1);
  }


  for (i = split, j = 0; i < (int64_t)RECORD_ORDER; ++i, j++) {
    memcpy(new_leaf.page.content.records[j].value,
        temp_records[i].value, sizeof(temp_records[i].value));
    new_leaf.page.content.records[j].key = temp_records[i].key;
    new_leaf.set_number_of_keys(&new_leaf,
        new_leaf.get_number_of_keys(&new_leaf) + 1);
  }

  free(temp_records);

  // link page from old to new
  leaf->page.header.one_more_page_offset =
    new_leaf.get_current_page_number(&new_leaf) * PAGE_SIZE;

  // Parent
  new_leaf.page.header.linked_page_offset =
    leaf->page.header.linked_page_offset;

  int64_t new_key = new_leaf.get_record(&new_leaf, 0)->key;

  // Write data to disk.
  leaf->write(leaf);
  new_leaf.write(&new_leaf);

  // call insert_into_parent function
  return __insert_into_parent(leaf, new_key, &new_leaf);
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
 *   while (insertion_point < this->page.header.number_of_keys
 *       && this->page.content.key_and_offsets[insertion_point].key
 *       < key_and_offset->key)
 *     insertion_point++;
 *
 *   for (i = this->page.header.number_of_keys;
 *       i > insertion_point;
 *       i--) {
 *     memcpy(&this->page.content.key_and_offsets[i],
 *         &this->page.content.key_and_offsets[i-1], sizeof(*key_and_offset));
 *   }
 *
 *   memcpy(&this->page.content.key_and_offsets[insertion_point],
 *       key_and_offset, sizeof(*key_and_offset));
 *   this->page.header.number_of_keys++;
 *   this->write(this);
 *   return true;
 * } */


void __remove_record_from_page(struct __page_object * const this,
    const int64_t key) {
  int32_t i;
  assert(this->get_type(this) == LEAF_PAGE);

  // Find index of target record
  for (i = 0; i < (int32_t)RECORD_ORDER; ++i) {
    if (this->page.content.records[i].key == key) {
      break;
    }
  }
  assert(i < (int32_t)RECORD_ORDER);

  // Remove the key and shift other keys accordingly.
  // i is the idx of target record

  // Remove
  memset(&this->page.content.records[i], 0,
      sizeof(this->page.content.records[i]));

  // Compaction
  for (i++; i < this->page.header.number_of_keys; ++i) {
    memcpy(&this->page.content.records[i-1],
        &this->page.content.records[i],
        sizeof(this->page.content.records[i-1]));
  }

  // Remove redundancy
  // i-1 is last key idex
  assert(memcmp(&this->page.content.records[i-1],
        &this->page.content.records[i-2],
        sizeof(&this->page.content.records[i-1]) == 0));
  memset(&this->page.content.records[i-1], 0,
      sizeof(this->page.content.records[i-1]));

  // One key fewer
  this->page.header.number_of_keys--;

  // Below is happed only page is root page
  if (this->page.header.number_of_keys == 0) {
    // Check whether current page is root page
    assert(this->current_page_number * PAGE_SIZE
        == header_page->get_root_page_offset(header_page));
  }

  this->write(this);
}


int64_t __get_page_number_of_left(const struct __page_object * const this){
  struct __page_object __parent_page;
  page_object_constructor(&__parent_page);

  __parent_page.set_current_page_number(&__parent_page,
      this->page.header.linked_page_offset / PAGE_SIZE);
  __parent_page.read(&__parent_page);

  // Make it const to forbbid changing parent page's value
  const struct __page_object * const parent_page = &__parent_page;

  int64_t i;
  for (i = 0; i < parent_page->page.header.number_of_keys; ++i) {
    if (parent_page->page.content.key_and_offsets[i].page_offset
        == this->current_page_number * PAGE_SIZE) {
      // current page is found.
      break;
    }
  }

  // current page must exist!
  assert(i != parent_page->page.header.number_of_keys);

  // return page number of left page.
  //
  // (i - 1) means the left page.
  // Even though i == 0, it is okay.
  return (*(parent_page->page.content.key_and_offsets + (i - 1) )).page_offset
    / PAGE_SIZE;
}


void __get_k_prime_index_and_its_value(
    const struct __page_object * const this,
    const int64_t neighbor_page_number,
    int64_t * const k_prime_index, int64_t * const k_prime) {

  struct __page_object __parent_page;
  page_object_constructor(&__parent_page);

  __parent_page.set_current_page_number(&__parent_page,
      this->page.header.linked_page_offset / PAGE_SIZE);
  __parent_page.read(&__parent_page);

  // Make it const to forbbid changing parent page's value
  const struct __page_object * const parent_page = &__parent_page;

  int64_t i;
  for (i = 0; i < parent_page->page.header.number_of_keys; ++i) {
    if (parent_page->page.content.key_and_offsets[i].page_offset
        == neighbor_page_number * PAGE_SIZE) {
      // neighbor page is found.
      break;
    }
  }

  // neighbor page must exist!
  assert(i != parent_page->page.header.number_of_keys);

  *k_prime_index = i;
  *k_prime = parent_page->page.content.key_and_offsets[i].key;
}


bool __delete_record_of_key(struct __page_object * const this,
    const int64_t key){
  // This function is called only current page is LEAF_PAGE.
  assert(this->get_type(this) == LEAF_PAGE);

  // Remove key and value from page.

  __remove_record_from_page(this, key);


  /* Case:  Record deletion from the root. 
   */

  // Do nothing.
  if (this->get_current_page_number(this) * PAGE_SIZE
      == header_page->get_root_page_offset(header_page) * PAGE_SIZE) {
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
    this->page.header.one_more_page_offset / PAGE_SIZE;

  // It means that current page is rightmost page.
  // neighbor will be the left page of current page.
  if (neighbor_page_number == 0) {
    neighbor_page_number = __get_page_number_of_left(this);
  }

  // At least one neighbor page is exists
  // becuase current page is not a root.
  assert(neighbor_page_number != 0);

  int64_t k_prime_index = -1;
  int64_t k_prime = -1;
  __get_k_prime_index_and_its_value(
      this, neighbor_page_number,
      &k_prime_index, &k_prime);

  assert(k_prime_index != -1 && k_prime != -1);

  //TODO: Capacity?

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
  /** this->insert_key_and_offset = __insert_key_and_offset; */
  this->insert_record_after_splitting =
    __insert_record_after_splitting;


  this->delete_record_of_key = __delete_record_of_key;

}
