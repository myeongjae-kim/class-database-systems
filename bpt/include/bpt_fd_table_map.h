/* File Name : bpt_fd_table_map.h
 * Author    : Kim, Myeong Jae
 * Due Date  : 2017-11-22 */

#ifndef __BPT_FD_TABLE_MAP_H__
#define __BPT_FD_TABLE_MAP_H__

#define MAX_TABLE_NUM 10

#include <stdint.h>
#include <stdbool.h>
#include "bpt_header_object.h"


// Set new 
int32_t set_new_table_of_fd(const int32_t fd);
int32_t get_fd_of_table(const int32_t table_id);
bool delete_table(const int32_t table_id);
bool remove_table_id_mapping(const int32_t table_id);
void remove_all_mapping_and_close();

#endif
