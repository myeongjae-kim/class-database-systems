#include "bptree.h"

int log_fd[MAX_TABLE];
int8_t *log_buf[MAX_TABLE];
int log_buf_size;
int log_cur_idx[MAX_TABLE];
const int LOG_SIZE = sizeof(log_t);

int open_log_file(int table_id){
  char log_file_name[10] = "./LOG";
  log_file_name[5] = table_id + '0';
  log_file_name[6] = '\0';

	int f = O_RDWR| O_CREAT | O_DIRECT | O_SYNC;
  log_fd[table_id] = open(log_file_name, f, DEF_DB_MODE);

  log_buf_size = 4096 * 5;
  log_buf[table_id] = (int8_t*)calloc(sizeof(*log_buf), log_buf_size);
  log_cur_idx[table_id] = 0;

  return 0;
}

int log_flush(int table_id) {
  // only append
  if (lseek(log_fd[table_id], 0, SEEK_END) < 0) {
    panic("lseek() error");
  }
  if (write(log_fd[table_id], log_buf[table_id],
        log_cur_idx[table_id]) < 0) {
    panic("write() error");
  }

  // reinitialize
  memset(log_buf[table_id], 0, log_buf_size);
  log_cur_idx[table_id] = 0;

  return 0;
}


int log_write(int table_id, log_t *log) {
  if (log_cur_idx[table_id] + LOG_SIZE >= log_buf_size) {
    log_flush(table_id);
  }

  memcpy(log_buf[table_id] + log_cur_idx[table_id], log, LOG_SIZE);
  log_cur_idx[table_id] += LOG_SIZE;

  return 0;
}


int close_log_file(int table_id){
  log_flush(table_id);
  close(log_fd[table_id]);
  log_fd[table_id] = 0;
  free(log_buf[table_id]);
  log_buf[table_id] = NULL;

  return 0;
}

int close_all_log_file() {
  for (int i = 0; i < MAX_TABLE; ++i) {
    if (log_fd[i] != 0) {
      close_log_file(i);
    }
  }

  return 0;
}
