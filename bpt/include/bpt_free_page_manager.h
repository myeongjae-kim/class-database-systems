#ifndef __BPT_FREE_PAGE_OBJECT_H__
#define __BPT_FREE_PAGE_OBJECT_H__

#include "bpt.h"

void free_page_manager_init();

int64_t page_alloc();
bool page_free(const int64_t page_number);

void free_page_clean();

#endif
