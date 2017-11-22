/* File Name : bpt_fd_table_map.c
 * Author    : Kim, Myeong Jae
 * Due Date  : 2017-11-22 */

#include "bpt_fd_table_map.h"
#include "bpt_free_page_manager.h"

#include <stdio.h>
#include <stdlib.h>

static int32_t fd_to_table_map[MAX_TABLE_NUM + 1];

// We do not use zero idx;
static int32_t *map_iter = fd_to_table_map + 1;
static const int32_t * const map_iter_begin = fd_to_table_map + 1;
static const int32_t * const map_iter_end = fd_to_table_map + MAX_TABLE_NUM + 1;

// header pages
header_object_t header_page[MAX_TABLE_NUM + 1];

int32_t get_fd_of_table(const int32_t table_id) {
  return fd_to_table_map[table_id];
}

// Error if return value is below zero.
int32_t set_new_table_of_fd(const int32_t fd) {
  int i = 0;

  // fd should not be zero.
  assert(fd != 0);

  while (i < MAX_TABLE_NUM) {
    // empty table is found
    if (*map_iter == 0) {
      *map_iter = fd;
      int32_t table_id = (map_iter - (map_iter_begin - 1));

      // set new header
      header_object_constructor(&header_page[table_id], table_id);

      return table_id;
    }
  
    // Go to find position
    map_iter++;
    if (map_iter == map_iter_end) {
      map_iter = (int32_t*)map_iter_begin;
    }

    i++;
  }

  // Error is occurrred!
  // Full of table
  fprintf(stderr, "(set_new_table) We cannot generate new table.\n");
  fprintf(stderr, "(set_new_table) Full of table_id array. Max: %d\n", MAX_TABLE_NUM);
  return -1;
}

// This function is called in close_table
bool remove_table_id_mapping(const int32_t table_id) {
  if (fd_to_table_map[table_id] == 0) {
    fprintf(stderr, "(delete_table) trying to delete empty table\n");
    assert(false);
    return false;
  }

  // destruct header information
  header_object_destructor(&header_page[table_id]);
  fd_to_table_map[table_id] = 0;
  return true;
}

void remove_all_mapping_and_close() {
  int i;
  for (i = 0; i < MAX_TABLE_NUM + 1; ++i) {
    if (fd_to_table_map[i] != 0) {
      if(close(fd_to_table_map[i]) < 0) {
        perror("remove_all_mapping_and_close");
        exit(1);
      }
      fd_to_table_map[i] = 0;
    }
  }
}
