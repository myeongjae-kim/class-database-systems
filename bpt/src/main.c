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
  char __command_input[256];
  char* command_input = __command_input;

  if (open_db("database") != 0) {
    printf("(main) DB Opening is failed.\n");  
    exit(1);
  }

#ifdef DBG
  parameter_check();
#endif
  print_page(1);
  print_header_page();
  add_free_page();
  add_free_page();
  add_free_page();
  print_header_page();

  print_page(1);


  printf("> ");
  while(fgets(command_input, sizeof(command_input), stdin) != NULL){

    if (*command_input == '\n') {
      continue;
    }

    switch ( decode_command(command_input) ) {
      case OPEN:
        printf("Open\n");
        command_input += strlen(commands[OPEN]);
        if (*command_input != ' ') {
          printf("(open) Invalid argument\n");
          break;
        }

        printf("%s\n", command_input);

        break;
      case INSERT:
        printf("Insert\n");
        command_input += strlen(commands[INSERT]);
        if (*command_input != ' ') {
          printf("(insert) Invalid argument\n");
          break;
        }

        printf("%s\n", command_input);

        break;
      case FIND:
        printf("Find\n");
        command_input += strlen(commands[FIND]);
        if (*command_input != ' ') {
          printf("(find) Invalid argument\n");
          break;
        }

        printf("%s\n", command_input);

        break;
      case DELETE:
        printf("Delete\n");
        command_input += strlen(commands[DELETE]);
        if (*command_input != ' ') {
          printf("(delete) Invalid argument\n");
          break;
        }
        printf("%s\n", command_input);

        break;

      case INVALID:
        printf("Invalid\n");
        break;

      default:
        printf("Default\n");
        break;
    }


    printf("> ");
  };



  /**   while ((instruction = getchar()) != EOF) {
   *     // When a user just push enter
   *     if (instruction == '\n') {
   *       printf("> ");
   *       continue;
   *     }
   *
   *     must_be_space_bar = getchar();
   *     if (must_be_space_bar == ' ') {
   *       // Read the value
   *       scanf("%[^\n]", value_buffer);
   *
   *       switch (instruction) {
   *         case 'i':
   *           printf("[i]:\n%s\n", value_buffer);
   *           break;
   *         case 'f':
   *         case 'p':
   *           printf("[p]:\n%s\n", value_buffer);
   *           break;
   *         default:
   *           printf("Input right command.\n");
   *           break;
   *       }
   *     } else {
   *       printf("More than two character command not exists\n");
   *     }
   *
   *     while (must_be_space_bar != '\n' && getchar() != (int)'\n');
   *     printf("> ");
   *   } */
  printf("\n");

  return 0;
}
