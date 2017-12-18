#include "bptree.h"

/* Open new file and return table id
 */
int open_file(conn *c, const char *file_path){
	int f = O_RDWR| O_CREAT | O_DIRECT | O_SYNC;
	int i;

  // ****** Parsing file name start ******
  // Check file name
  i = 0;
  while (file_path[i] != 0) {
    i++;
  }
  // i is at the end of string
  
  i--;
  // i is at the end of last character 
  while (file_path[i] != '/' && i > 0) {
    i--;
  }

  if (i == 0) {
    // i is at the first file name
    // do nothing
  } else {
    // i is at last '/'
    i++;
    // i is at the first file name
  }

  // now i is the last character of file
  if (strncmp("DATA", &file_path[i], 4 * sizeof(char)) != 0) {
    goto err;
  }

  // DATA
  i += 4;

  int table_id = atoi(&file_path[i]);

  // ****** Parsing file name end ******

  if (!c->tbls[table_id].is_used) {
    c->tbls[table_id].c = c;
    c->tbls[table_id].table_id = table_id;
    c->tbls[table_id].bm.fd = open(file_path, f, DEF_DB_MODE);
    c->tbls[table_id].is_used = true;
    return table_id;
  }

  /** for (i = 0; i < MAX_TABLE; i++){
    *   if (!c->tbls[i].is_used){
    *     c->tbls[i].c = c;
    *     c->tbls[i].table_id = i;
    *     c->tbls[i].bm.fd = open(file_path, f, DEF_DB_MODE);
    *     c->tbls[i].is_used = true;
    *     return i;
    *   }
    * } */

err:

  //panic("open_file"); 
  return E_FULL_TABLE;
}

/* Close the file
*/
void close_file(table *t){
  close(t->bm.fd);
  memset(t, 0, sizeof(table));
}

/* Extend the file
*/
void extend_file(table *t, hpage *hp){
  hblock *hb = B(hp);
  int new_num_page;
  int sz;
  uint8_t buf[BLOCK_SIZE * 2];
  fblock *blk = (fblock*) ALIGN_UP((uintptr_t) buf, BLOCK_SIZE);
  hb->free = hb->num_page * BLOCK_SIZE;
  memset(blk, 0, BLOCK_SIZE);
#ifdef NUM_EXTEND_PAGE
  new_num_page = hb->num_page + NUM_EXTEND_PAGE;
#else
  new_num_page = hb->num_page * 2;
#endif /* NUM_EXTEND_PAGE */
  for (sz = hb->num_page + 1; sz < new_num_page; sz++){
    blk->next = sz * BLOCK_SIZE;
    write_block(t, blk, (sz - 1) * BLOCK_SIZE);
  }
  blk->next = ADDR_NOT_EXIST;
  write_block(t, blk, (sz - 1) * BLOCK_SIZE);
  hb->num_page = new_num_page;
}

/* Read one block from file
*/
void read_block(table *t, void *p, addr ad){
  int fd = t->bm.fd;
  int nr;

  if (!ALIGNED(ad)){
    ad = ALIGN_DOWN(ad, BLOCK_SIZE);
  }
  if ((nr = pread(fd, p, BLOCK_SIZE, ad)) == -1 || nr != BLOCK_SIZE){
    panic("pread"); 
  }
}

/* Allocate one block from file.
 * If the file is full, extend the file.
 */
addr alloc_block(table *t){
  hpage *hp = get_hpage(t);
  addr ad;
  fpage *fp;
  set_dirty(hp);

  if (B(hp)->free == ADDR_NOT_EXIST){
    extend_file(t, hp);
  }
  ad = B(hp)->free;
  fp = get_fpage(t, ad);
  set_dirty(fp);
  release_page(t, fp);

  B(hp)->free = B(fp)->next;
  release_page(t, hp);
  return ad;
}

/* Free the block from file for reuse.
*/
void free_block(table *t, void *b){
  hpage *hp = get_hpage(t);
  hblock *hb = B(hp);
  fpage *fp = (fpage*)b;
  fblock *fb = B(fp);

  memset(fb, 0, BLOCK_SIZE);
  fb->next = hb->free;
  set_dirty(fp);

  hb->free = fp->offset;
  set_dirty(hp);
  release_page(t, hp);
}

/* Write one block to file
*/
void write_block(table *t, void *b, addr ad){
  int fd = t->bm.fd;
  int nr;
  if (!ALIGNED(ad)){
    ad = ALIGN_DOWN(ad, BLOCK_SIZE);
  }
  if ((nr = pwrite(fd, b, BLOCK_SIZE, ad)) == -1 || nr != BLOCK_SIZE){
    panic("pwrite"); 
  }
}
