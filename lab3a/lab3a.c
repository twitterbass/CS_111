// NAME: John Tran
// EMAIL: johntran627@gmail.com
// UID: 005183495

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "ext2_fs.h"

#define FILE_MODE 0x8000
#define DIRECTORY_MODE 0x4000
#define SYMBOLIC_LINK_MODE 0xA000

#define FILE_TYPE 'f'
#define DIRECTORY_TYPE 'd'
#define SYMBOLIC_LINK_TYPE 's'

#define FREE_BLOCK 'B'
#define FREE_INODE 'I'

int fs_fd;

struct ext2_super_block *super_block;
struct ext2_group_desc *group_desc;

int open_handler(char *pathname, int flags) {
  int status = open(pathname, flags);
  if (status == -1) {
    fprintf(stderr, "open() failed - %s\n", strerror(errno));
    exit(1);
  }
  return status;
}

int pread_handler(int fd, void *buf, int count, int offset) {
  int status = pread(fd, buf, count, offset);
  if (status == -1) {
    fprintf(stderr, "Failed to pread a file descriptor: %s\n", strerror(errno));
    exit(1);
  }
  return status;
}

int get_block_size() {
  return 1024 << super_block->s_log_block_size;
}

int get_num_blocks() {
  return get_block_size() / sizeof(int);
}

int is_inode_free(struct ext2_inode *inode) {
  return (inode->i_mode == 0 || inode->i_links_count == 0);
}

char get_inode_file_type(struct ext2_inode *inode) {
  if ((inode->i_mode & 0XF000) == FILE_MODE) return FILE_TYPE;
  if ((inode->i_mode & 0XF000) == DIRECTORY_MODE) return DIRECTORY_TYPE;
  if ((inode->i_mode & 0XF000) == SYMBOLIC_LINK_MODE) return SYMBOLIC_LINK_TYPE;
  return '?';
}

void get_time(char *time_str, unsigned int time) {
  time_t rawtime = time;
  struct tm *info = gmtime(&rawtime);
  strftime(time_str, 128, "%m/%d/%y %H:%M:%S", info);
}

void print_super_block_summary() {
  pread(fs_fd, super_block, sizeof(struct ext2_super_block), 1024);
  fprintf(stdout, "%s,%d,%d,%d,%d,%d,%d,%d\n", 
	  "SUPERBLOCK", 
	  super_block->s_blocks_count,         
	  super_block->s_inodes_count,         
	  get_block_size(),   
	  super_block->s_inode_size,           
	  super_block->s_blocks_per_group,     
	  super_block->s_inodes_per_group,     
	  super_block->s_first_ino             
	  );
}

void print_group_summary() {
  pread(fs_fd, group_desc, sizeof(struct ext2_group_desc), 2048);
  fprintf(stdout, "%s,%d,%d,%d,%d,%d,%d,%d,%d\n",
	  "GROUP",
	  0,                                
	  super_block->s_blocks_count,      
	  super_block->s_inodes_count,      
	  group_desc->bg_free_blocks_count, 
	  group_desc->bg_free_inodes_count, 
	  group_desc->bg_block_bitmap,      
	  group_desc->bg_inode_bitmap,      
	  group_desc->bg_inode_table        
	  );
}

void print_free_entry_summary(char entry_type) {	
  int num_bitmap;
  int block_size = get_block_size();
  char key[8];

  if (entry_type == FREE_BLOCK) {
    num_bitmap = group_desc->bg_block_bitmap;
    strcpy(key, "BFREE");
  }
  else if (entry_type == FREE_INODE) {
    num_bitmap = group_desc->bg_inode_bitmap;
    strcpy(key, "IFREE");
  }

  char buf;
  int i;
  for (i = 0; i < block_size; i++) {
    pread_handler(fs_fd, &buf, sizeof(char), num_bitmap * block_size + i);
    int j;
    for (j = 0; j < 8; j++) {
      if (!(buf & (1 << j))) 
	fprintf(stdout, "%s,%d\n", key, 8 * i + j + 1);
    }
  }
}

void print_one_inode_info(int inode_number, struct ext2_inode *inode) {
  char creation_time[128];
  char modification_time[128];
  char access_time[128];
  
  get_time(creation_time, inode->i_ctime);
  get_time(modification_time, inode->i_mtime);
  get_time(access_time, inode->i_atime);
  
  fprintf(stdout, "%s,%d,%c,%o,%d,%d,%d,%s,%s,%s,%d,%d",
	  "INODE",
	  inode_number,                
	  get_inode_file_type(inode),                    
	  (inode->i_mode & 0X0FFF),   
	  inode->i_uid,                
	  inode->i_gid,                
	  inode->i_links_count,        
	  creation_time,          
	  modification_time, 
	  access_time,       
	  inode->i_size,               
	  inode->i_blocks              
	  );

  if (get_inode_file_type(inode) != SYMBOLIC_LINK_TYPE || inode->i_size >= 6) {
    int i;
    for (i = 0; i < 15; i++)
      fprintf(stdout, ",%d", inode->i_block[i]); 
  }
  fprintf(stdout, "\n");
}

void print_inode_summary() {
  int i;
  struct ext2_inode inode;
  
  for (i = 0; i < (int) super_block->s_inodes_count; i++) {
    pread_handler(fs_fd, &inode, sizeof(struct ext2_inode), 
	       group_desc->bg_inode_table * get_block_size() + i * sizeof(struct ext2_inode));

    if (is_inode_free(&inode)) continue;	
    print_one_inode_info(i + 1, &inode);
  }
}

void print_one_directory_info(struct ext2_dir_entry *dir_entry, int inode_number, int logical_offset) {
  fprintf(stdout, "%s,%d,%d,%d,%d,%d,'%s'\n", 
	  "DIRENT",
	  inode_number,        
	  logical_offset,      
	  dir_entry->inode,    
	  dir_entry->rec_len,  
	  dir_entry->name_len, 
	  dir_entry->name       
	  );
}

void print_single_indirect_directory(struct ext2_inode *inode, int inode_number, int index) {
  struct ext2_dir_entry dir_entry;

  int block_size = get_block_size();
  int num_blocks = get_num_blocks();

  int i;
  for (i = 0; i < num_blocks; i++) {
    int block_number;
	pread_handler(fs_fd, &block_number, sizeof(int), (inode->i_block[index] * block_size + sizeof(int) * i));
    if (block_number == 0) continue; 
    int j;
    for (j = 0; j < block_size; j += dir_entry.rec_len) {
      pread_handler(fs_fd, &dir_entry, sizeof(struct ext2_dir_entry), block_number * block_size + j);
      if (dir_entry.inode == 0) continue; 
      int offset = (12 + i) * block_size + j;
      print_one_directory_info(&dir_entry, inode_number, offset);
    }
  }
}

void print_double_indirect_directory(struct ext2_inode *inode, int inode_number, int index) {
  int indirect_block_number;
  int num_blocks = get_num_blocks();

  int i;
  for (i = 0; i < num_blocks; i++) {
    pread_handler(fs_fd, &indirect_block_number, sizeof(int), (inode->i_block[index] * get_block_size() + sizeof(int) * i));
    if (indirect_block_number == 0) continue; 
    print_single_indirect_directory(inode, inode_number, indirect_block_number);
  }
}

void print_triple_indirect_directory(struct ext2_inode *inode, int inode_number, int index) {
  int double_indirect_block_number;
  int num_blocks = get_num_blocks();

  int i;
  for (i = 0; i < num_blocks; i++) {
    pread_handler(fs_fd, &double_indirect_block_number, sizeof(int), (inode->i_block[index] * get_block_size() + sizeof(int) * i));
    if (double_indirect_block_number == 0) continue;
    print_double_indirect_directory(inode, inode_number, double_indirect_block_number);
  }
}

void print_directory_entry_summary() {
  int a;
  int block_size = get_block_size();
  struct ext2_inode inode;

  for (a = 0; a < (int) super_block->s_inodes_count; a++) {
    pread_handler(fs_fd, &inode, sizeof(struct ext2_inode), 
	       group_desc->bg_inode_table * block_size + a * sizeof(struct ext2_inode));
    if (is_inode_free(&inode)) continue;

    if (get_inode_file_type(&inode) != DIRECTORY_TYPE) continue;
		
    struct ext2_dir_entry dir_entry;
    int i;
    for (i = 0; i < EXT2_NDIR_BLOCKS; i++) {
      if (inode.i_block[i] == 0) continue; 
      int j;
      for (j = 0; j < block_size; j += dir_entry.rec_len) {
	pread_handler(fs_fd, &dir_entry, sizeof(struct ext2_dir_entry), inode.i_block[i] * block_size + j);
	if (dir_entry.inode == 0) continue; 
	int offset = i * block_size + j;
	print_one_directory_info(&dir_entry, a + 1, offset);			
      }
    }
    	
    if (inode.i_block[12] != 0)  
      print_single_indirect_directory(&inode, a + 1, 12);
	
    if (inode.i_block[13] != 0) 
      print_double_indirect_directory(&inode, a + 1, 13);
		
    if (inode.i_block[14] != 0) 	
      print_triple_indirect_directory(&inode, a + 1, 14);  
  }
}


int print_indirect_block_helper(int indirect_block_number, int inode_number, int offset, int index, int level) {
  int block_number; 

  pread_handler(fs_fd, &block_number, sizeof(int), 
	     indirect_block_number * get_block_size() + index * sizeof(int));

  if (block_number == 0) return block_number; 

  fprintf(stdout, "%s,%d,%d,%d,%d,%d\n",
	  "INDIRECT",
	  inode_number,           
	  level,                                       
	  offset,                                  
	  indirect_block_number,  
	  block_number            
	  );

  return block_number;
}

void print_single_indirect_block(struct ext2_inode *inode, int inode_number) {
  int num_blocks = get_num_blocks();
  int i;
  for (i = 0; i < num_blocks; i++) {
    print_indirect_block_helper(inode->i_block[12], inode_number, (12 + i), i, 1);
  }
}

void print_double_indirect_block( struct ext2_inode *inode, int inode_number) {
  
  int indirect_block_number;

  int num_blocks = get_num_blocks();
  int i;
  for (i = 0; i < num_blocks; i++) {
    int double_offset = 12 + (i + 1) * num_blocks;
    indirect_block_number = print_indirect_block_helper(inode->i_block[13], inode_number, double_offset, i, 2);
    if (indirect_block_number == 0) continue;
    
    int j;
    for (j = 0; j < num_blocks; j++) {
      int single_offset = 12 + (i + 1) * num_blocks + j;
      print_indirect_block_helper(indirect_block_number, inode_number, single_offset, j, 1);
    }
  }
}

void print_triple_indirect_block(struct ext2_inode *inode, int inode_number) {
  int double_indirect_block_number;
  int indirect_block_number;

  int num_blocks = get_num_blocks();
  int i;
  for (i = 0; i < num_blocks; i++) {
    int triple_offset = 12 + ((i + 1) * num_blocks + 1) * num_blocks;
    double_indirect_block_number = print_indirect_block_helper(inode->i_block[14], inode_number, triple_offset, i, 3);
    if (double_indirect_block_number == 0) continue;
  int j;
    for (j = 0; j < num_blocks; j++) {
      int double_offset = 12 + ((i + 1) * num_blocks + j + 1) * num_blocks;
      indirect_block_number = print_indirect_block_helper(double_indirect_block_number, inode_number, double_offset, j, 2);
      if (indirect_block_number == 0) continue;
      int k;
      for (k = 0; k < num_blocks; k++) {
	int single_offset = double_offset + k;
	print_indirect_block_helper(indirect_block_number, inode_number, single_offset, k, 1);	
      }
    }
  }
}

void print_indirect_block_reference_summary() {
  struct ext2_inode inode;

  int num_inodes = super_block->s_inodes_count;
  int i;
  for (i = 0; i < num_inodes; i++) {
    pread_handler(fs_fd, &inode, sizeof(struct ext2_inode), 
	       group_desc->bg_inode_table * get_block_size() + i * sizeof(struct ext2_inode));
    if (is_inode_free(&inode)) continue;
    char file_type = get_inode_file_type(&inode);
		
    if (file_type != FILE_TYPE && file_type != DIRECTORY_TYPE) continue;

    if (inode.i_block[12]  != 0) 
      print_single_indirect_block(&inode, i + 1);
    if (inode.i_block[13] != 0) 
      print_double_indirect_block(&inode, i + 1);
    if (inode.i_block[14] != 0) 
      print_triple_indirect_block(&inode, i + 1);
  }
}

void print_summaries() {
  print_super_block_summary();
  print_group_summary();
  print_free_entry_summary(FREE_BLOCK);
  print_free_entry_summary(FREE_INODE);
  print_inode_summary();
  print_directory_entry_summary();
  print_indirect_block_reference_summary();
}

void malloc_error_check(void *ptr) {
  if (ptr == NULL) {
    fprintf(stderr, "malloc() failed - %s\n", strerror(errno));
    exit(1);
  }
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Invalid number of arguments.]\n");
    exit(1);
  }

  fs_fd = open_handler(argv[1], O_RDONLY);

  super_block = malloc(sizeof(struct ext2_super_block));
  malloc_error_check(super_block); 
  group_desc = malloc(sizeof(struct ext2_group_desc));
  malloc_error_check(group_desc);
  
  print_summaries();
  
  close(fs_fd);
  free(super_block);
  free(group_desc);

  exit(0);
}


