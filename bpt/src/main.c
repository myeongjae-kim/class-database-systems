/* File Name : main.c
 * Author    : Kim, Myeong Jae
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

void print_page(uint64_t page_number);
void print_header_page();

#ifdef DBG
void parameter_check(void) {
  printf("page_header_size: %ld\n", sizeof(page_header_t));
  assert(sizeof(page_header_t) == 128);

  printf("record_per_page: %ld\n", RECORD_PER_PAGE);
  assert(RECORD_PER_PAGE + 1 == 32);

  printf("leaf_page size: %ld\n", sizeof(leaf_page_t));
  assert(sizeof(leaf_page_t) == 4096);

  printf("offsets_per_page: %ld\n", OFFSETS_PER_PAGE);
  assert(OFFSETS_PER_PAGE + 1 == 249);

  printf("iternal_page_size: %ld\n", sizeof(internal_page_t));
  assert(sizeof(internal_page_t) == 4096);

  printf("page_t size: %ld\n", sizeof(page_t));
  assert(sizeof(page_t) == 4096);


  printf("(parameter_check) parameter checking is successful.\n");
}
#endif

enum command_case {INVALID, OPEN, INSERT, FIND, DELETE};

#define COMMAND_CASE_NUM 5
char* commands[] = {"", "open", "insert", "find", "delete"};

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
  char* input_iterator;
  int64_t key;
  uint32_t i;

  uint64_t temp_pages[20];

  if (open_db("database") != 0) {
    printf("(main) DB Opening is failed.\n");  
    exit(1);
  }

#ifdef DBG
  parameter_check();
#endif

  for (i = 0; i < 10; ++i) {
    temp_pages[i] = page_alloc();
  }
  temp_pages[10] = page_alloc();
  print_header_page();

  
  free_page_clean();

  for (i = 0; i < 11; ++i) {
    if(page_free(temp_pages[i]) == false) {
      assert(false);
    }
  }

  free_page_clean();


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
        // call open_db();

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
        for (i = 0; i < sizeof(command_input); ++i) {
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
        } else if (input_iterator == NULL) {
          printf("(insert) <value> does not exist.\n");
          printf("(insert) Usage > insert <key> <value>\n");
          break;
        }


#ifdef DBG
        printf("key: %ld, value: %s\n", key, input_iterator);
#endif
        insert(key, input_iterator);

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
        // call find(key);

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
        // call delete(key);

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

  return 0;
}
