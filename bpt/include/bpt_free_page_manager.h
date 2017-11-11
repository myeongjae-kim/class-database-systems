/* File Name : bpt_free_page_manager.h
 * Author    : Kim, Myeong Jae
 * Due Date  : 2017-11-5 */

#ifndef __BPT_FREE_PAGE_OBJECT_H__
#define __BPT_FREE_PAGE_OBJECT_H__

#include "bpt.h"

void free_page_manager_init(int table_id);

int64_t page_alloc(int table_id);
bool page_free(int table_id, const int64_t page_number);

void free_page_clean(int table_id);

#endif
