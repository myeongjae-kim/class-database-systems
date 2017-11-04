/* File Name : main.c Author    : Kim, Myeong Jae
 * Due Date  : 2017-11-5
 *
 * This is a main file of disk-based b+tree */

#include "bpt.h"
#include <stdio.h>
#include <string.h>

#ifdef DBG
#include <assert.h>
#endif


// TODO: delete
#include "bpt_header_object.h"
#include "bpt_page_object.h"
#include "bpt_free_page_manager.h"
extern header_object_t *header_page;
extern int32_t db;
// Delete end

void print_page(int64_t page_number);
void print_header_page();


void testing(void) {
/**   printf("(testing)\n");
  *   char* ptr;
  *
  *   int32_t i;
  *   for (i = 1; i <= 1000000; ++i) {
  *     if ((ptr = find(i)) == NULL) {
  *       fprintf(stderr, "key %d is not found\n", i);
  *       exit(1);
  *     }
  *     printf("\r%s                               ", ptr);
  *   }
  *   printf("                                      \n"); */
  print_all();
}


#ifdef DBG
void parameter_check(void) {
  printf("page_header_size: %ld\n", sizeof(page_header_t));
  assert(sizeof(page_header_t) == 128);

  printf("record_per_page: %ld\n", RECORD_PER_PAGE);
  assert(RECORD_PER_PAGE + 1 == 32);

  printf("offsets_per_page: %ld\n", OFFSETS_PER_PAGE);
  assert(OFFSETS_PER_PAGE + 1 == 249);

  printf("page_t size: %ld\n", sizeof(struct __page));
  assert(sizeof(struct __page) == 4096);


  printf("(parameter_check) parameter checking is successful.\n");
}

bool *debug_vector;
int64_t debug_vector_size;
int64_t debug_vector_max_idx = 1;
bool *not_found;

void check_post_condition() {
  page_object_t page_buffer;
  page_object_constructor(&page_buffer);
  page_buffer.set_current_page_number(&page_buffer,
      header_page->get_root_page_offset(header_page) / PAGE_SIZE);
  page_buffer.read(&page_buffer);

  while (page_buffer.get_type(&page_buffer) != LEAF_PAGE) {
    assert(page_buffer.get_type(&page_buffer) != INVALID_PAGE);

    page_buffer.set_current_page_number(&page_buffer,
        page_buffer.page.header.one_more_page_offset / PAGE_SIZE);
    page_buffer.read(&page_buffer);
  }

  // Leaf is found.
  int64_t i;
  for (i = 0; i < 110000; ++i) {
    not_found[i] = true;
  }

  while(1){
    assert(page_buffer.get_type(&page_buffer) == LEAF_PAGE);
    for (i = 0; i < page_buffer.page.header.number_of_keys; ++i) {

      /** printf("[page #%ld] key:%ld, value:%s\n",
        *     page_buffer.get_current_page_number(&page_buffer),
        *     page_buffer.page.content.records[i].key,
        *     page_buffer.page.content.records[i].value); */

      not_found[page_buffer.page.content.records[i].key] = false;
    }
    if (page_buffer.page.header.one_more_page_offset != 0) {
      page_buffer.set_current_page_number(&page_buffer,
          page_buffer.page.header.one_more_page_offset / PAGE_SIZE);
      page_buffer.read(&page_buffer);
    } else {
      break;
    }
  }

  printf("** Correctness check **\n");
  for (i = 1; i <= 100000; ++i) {
    if (debug_vector[i] != not_found[i]) {
      printf("Problem occurred. key:%ld\n", i);
      assert(false);
    }
  }
}
#endif

enum command_case {INVALID, OPEN, INSERT, FIND, DELETE, TEST};

#define COMMAND_CASE_NUM 6
char* commands[] = {"", "open", "insert", "find", "delete", "test"};

enum command_case decode_command(char* command) {
  enum command_case return_case;

  for (return_case = OPEN; return_case < COMMAND_CASE_NUM; ++return_case) {
    if (strncmp(command, commands[return_case],
          strlen(commands[return_case])) == 0) {
      return return_case;
    }
  }

  return INVALID;
}


int main(void)
{
  char command_input[256];
  char* input_iterator, *find_result;
  int64_t key;
  int32_t i;
  bool open_is_called = false;

#ifdef fsync
  putchar('\n');
  printf("*******************************************************\n");
  printf("*******************************************************\n");
  printf("*********************  Warning!!  *********************\n");
  printf("** Remove #define fsync(x) to make DB unbuffered I/O **\n");
  printf("*******************************************************\n");
  printf("*******************************************************\n");
  putchar('\n');
#endif
  
#ifdef DBG
  parameter_check();

  debug_vector_size = 110000;
  debug_vector = calloc(debug_vector_size, sizeof(*debug_vector));
  debug_vector_max_idx = 0;
  not_found = calloc(110000, sizeof(*not_found));
#endif

#ifndef fsync
  // Make standard I/O Unbuffered.
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stdin, NULL, _IONBF, 0);
#endif

  // initializing buffer.
  memset(command_input, 0 , sizeof(command_input));

  // Open database first
  printf("> ");
  while(fgets(command_input, sizeof(command_input), stdin) != NULL){
    input_iterator = command_input;
    key = 0;

    if (*input_iterator == '\n') {
      continue;
    }

    // Remove linebreak
    input_iterator = strtok(input_iterator, "\n");

    switch ( decode_command(input_iterator) ) {
      case OPEN:
        input_iterator += strlen(commands[OPEN]);
        if (*input_iterator != ' ') {
          printf("(open) Argument is invalid.\n");
          printf("(open) Usage > open <pathname>\n");
          break;
        }
        // Skip space bar
        input_iterator++;

#ifdef DBG
        printf("pathname: %s\n", input_iterator);
#endif
        if (open_db(input_iterator) != 0) {
          printf("(main) DB Opening is failed.\n");  
          exit(1);
        }


        open_is_called = true;
        break;
      case INSERT:
      case FIND:
      case DELETE:
      case TEST:
      case INVALID:
      default:
        printf("DB is not ye opened. Open database first.\n");
        printf("Usage: > open dbname\n");
        break;
    }

    if (open_is_called) {
      break;
    }
    printf("> ");
  }

  if (open_is_called == false) {
    // The input is EOF.
    return 0;
  }

  // get other commands

  printf("> ");
  while(fgets(command_input, sizeof(command_input), stdin) != NULL){
    input_iterator = command_input;
    key = 0;

    if (*input_iterator == '\n') {
      continue;
    }

    // Remove linebreak
    input_iterator = strtok(input_iterator, "\n");

    switch ( decode_command(input_iterator) ) {
      case OPEN:
        input_iterator += strlen(commands[OPEN]);
        if (*input_iterator != ' ') {
          printf("(open) Argument is invalid.\n");
          printf("(open) Usage > open <pathname>\n");
          break;
        }
        // Skip space bar
        input_iterator++;

#ifdef DBG
        printf("pathname: %s\n", input_iterator);
#endif
        if (open_db(input_iterator) != 0) {
          printf("(main) DB Opening is failed.\n");  
          exit(1);
        }


        break;
      case INSERT:
        input_iterator += strlen(commands[INSERT]);
        if (*input_iterator != ' ') {
          printf("(insert) Argument is invalid.\n");
          printf("(insert) Usage > insert <key> <value>\n");
          break;
        }
        // Skip space bar
        input_iterator++;

        // Parse 
        for (i = 0; i < (int64_t)sizeof(command_input); ++i) {
          if (input_iterator[i] == ' ') {
            break;
          }
        }
        input_iterator[i] = '\0';
        key = atol(input_iterator);
        input_iterator += i + 1;

        // Parsed input check
        if (key == 0) {
          printf("(insert) key conversion to integer is failed.\n");
          printf("(insert) Usage > insert <key> <value>\n");
          break;
        } else if(input_iterator == NULL
            ||*input_iterator == ' '
            || *input_iterator == '\0') {
          printf("(insert) <value> does not exist.\n");
          printf("(insert) Usage > insert <key> <value>\n");
          break;
        } 

#ifdef DBG
        printf("key: %ld, value: %s\n", key, input_iterator);
#endif

#ifdef DBG
        // Insert only if key is not exist.
        if (find(key) == NULL) {
          insert(key, input_iterator);

          debug_vector_max_idx++;
        }
#endif


        break;
      case FIND:
        input_iterator += strlen(commands[FIND]);
        if (*input_iterator != ' ') {
          printf("(find) Argument is invalid.\n");
          printf("(find) Usage > find <key>\n");
          break;
        }
        // Skip space bar
        input_iterator++;

        if ((key = atol(input_iterator)) == 0) {
          printf("(find) key conversion to integer is failed.\n");
          printf("(find) Usage > find <key>\n");
          break;
        }

#ifdef DBG
        printf("key: %ld\n", key);
#endif
        if((find_result = find(key)) != NULL) {
          printf("Key #%ld is found. Value : %s\n", key, find_result);
        } else {
          printf("Key #%ld is not found.\n", key);
        }

        break;
      case DELETE:
        input_iterator += strlen(commands[DELETE]);
        if (*input_iterator != ' ') {
          printf("(delete) Argument is invalid.\n");
          printf("(delete) Usage > delete <key>\n");
          break;
        }
        // Skip space bar
        input_iterator++;


        if ((key = atol(input_iterator)) == 0) {
          printf("(delete) key conversion to integer is failed.\n");
          printf("(delete) Usage > delete <key>\n");
          break;
        }

#ifdef DBG
        printf("key: %ld\n", key);
#endif

        if ((find_result = find(key)) != NULL) {
          delete(key);
          printf("(delete)[key: %ld, value: %s] is deleted.\n"
              ,key , find_result);

#ifdef DBG
          debug_vector[key] = true;
          /** for (i = 1; i < debug_vector_max_idx; ++i) {
            *   if (debug_vector[i] == false) {
            *     if (find(i) == NULL) {
            *       printf("!! key: %d is lost!\n", i);
            *       assert(false);
            *     }
            *   }
            * } */
          /** check_post_condition(); */
#endif
        } else {
          printf("(delete) key %ld is not found.\n", key);
          assert(false);
        }

        break;

      case TEST:
        testing();
        break;


      case INVALID:
      default:
        printf(" Invalid command is inputted\n");
        printf(" Usage:\n");
        printf("  > open <pathname>\n");
        printf("  > insert <key> <value>\n");
        printf("  > find <key>\n");
        printf("  > delete <key>\n");
        break;
    }


    printf("> ");
  };


#ifdef DBG
  free(debug_vector);
  free(not_found);
#endif

  return 0;
}
