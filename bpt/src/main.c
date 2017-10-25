/* File Name : main.c
 * Author    : Kim, Myeong Jae
 * Due Date  : 2017-11-5
 *
 * This is a main file of disk-based b+tree */

#include "bpt.h"
#include <stdio.h>


// TODO: delete
#include "bpt_header_object.h"
extern header_object_t *header_page;
// Delete end

void print_page(uint64_t page_number);
void print_header_page();


int main(void)
{
  if (open_db("database") != 0) {
    printf("(main) DB Opening is failed.\n");  
  }

  printf("page_header_size: %ld\n", sizeof(page_header_t));
  printf("record_per_page: %ld\n", RECORD_PER_PAGE);
  printf("leaf_page size: %ld\n", sizeof(leaf_page_t));
  printf("offsets_per_page: %ld\n", OFFSETS_PER_PAGE);
  printf("iternal_page_size: %ld\n", sizeof(internal_page_t));




  print_page(1);

  uint64_t added_page_one;
  printf("Leaf Page is allocated. Number: %ld\n",
      added_page_one = leaf_page_alloc(1, 1));
  
  print_page(added_page_one);

  uint64_t added_page_two;
  printf("Internal Page is allocated. Number: %ld\n",
      added_page_two = internal_page_alloc(1, 1));
  
  print_page(added_page_two);

  delete_page(added_page_one);
  delete_page(added_page_two);



  print_header_page();

  return 0;
}
