/*
 * Author: Oskar Bero
 */
#ifndef PA5_MYMAL_H
#define PA5_MYMAL_H

#include <stddef.h>

/*
 * Typedef a pointer to mem_block to pt_block for convenience
 */
typedef struct mem_block *pt_block;

/*
 * mem_block struct is the data structure for meta data of an
 * allocated memory block in the heap. It has a doubly-linked list
 * structure and holds information about block size, the next and
 * previous blocks of memory, free memory flag. Ther are also a void*
 * ptr_check and char[1] last_byte to use for comparison of addresses,
 * The line and function meta data is for __LINE__
 * and __FILE__ passed into my_malloc
 */
struct mem_block {
    int                 size;
    pt_block            next;
    pt_block            prev;
    int                 free;
    int                 line;
    char    *           file;
    int                 signature;

};


/*
 * My own implementation of malloc, which allocates memory from the heap.
 * If needed it extends the heap using sbrk( ). After getting a memory block
 * from sbrk( ) my_malloc puts a mem_block struct in the front and returns
 * the pointer to memory right after new mem_block. It takes the size,
 * __LINE__ and __FILE__ as arguments. Line and file are stored in the mem_block
 * for debuging and error printing purposes.
 */
void *my_malloc(int size , int n_line , char *f_name );

/*
 * This is my own implementation of the free function that goes with my_malloc.
 * This free releases the memory allocated by my_malloc and attempts to fuse blocks
 * (using combine_blocks( ) ) before and after the recently freed block as a measure of
 * preventing memory fragmentation(to a degree). n_line and f_name are for __LINE__
 * and __FILE__ for debug and error printing purposes
 */
void my_free( void * p , int n_line , char *f_name );

#endif