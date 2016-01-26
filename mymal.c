/*
 * Author: Oskar Bero
 */
#include "mymal.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_SIZE      	sizeof(struct mem_block)
#define SIGN                1010101010
#define RED                 "\x1b[2;31;31m"
#define RESET               "\x1b[0m"


/* prototypes of utility functions which the user does not need to know of in mymal.c */
int  is_valid_memblock( void * , int , char*);
void set_meta( pt_block , int , char * );
void trim_to_size( pt_block , int);
void mem_leak_check( void );
void *get_block_struct(void *p);
pt_block combine_blocks(pt_block);
pt_block heap_extend( pt_block , int);
pt_block get_free_block(pt_block *last, int size);


static struct mem_block     *root;
int                         num_allocated;

/* MALLOC FUNCTIONS */
void *my_malloc(int size , int n_line , char *f_name ){

    /* TODO - testing index remove before handing in */
    pt_block prev;
    pt_block p;

    if(size < 0)
    {
        printf("%s==>ERROR You cannot allocate negative amount of memmory (failure)%s\n", RED, RESET);
        return NULL;
    }
    else if(size == 0){
        printf("%s==>ERROR You shouldn't allocate 0 memmory (failure) %s\n", RED, RESET);
        return NULL;
    }



    if(root) { /* not first malloc , root is not null */

        /* start by finding a block of matching size that is free, use prev = root to traverse list*/
        prev = root;
        p = get_free_block(&prev, size); /*get a free block using prev to traverse  , and size to compare store in p */

        if (p != NULL) {

            /* try and fit the memory we found with get_free_block( ) to exact size requested.
             * if size of our block - the size requested is still bigger than BLOCK_SIZE + 8 where +8 is an
             * arbitrary # of bytes chosen to fit at least 1 int.
             * */
            if((p->size - size) >= (BLOCK_SIZE + 8)){
                trim_to_size(p, size);
            }
            p->free = 0; /* p is no longer free */

        }
        else{
            /* no free block of the size requested -> extend the heap */
            p = heap_extend(prev, size);

            set_meta(p,n_line,f_name);

            if (p == NULL) { /* p still null -> sbrk failed in extending heap return null */
                return (NULL);
            }

        }

    }
    else{
        /*add a memory leak check at exit */
        atexit(&mem_leak_check);
        /* first run of malloc -> get heap space, we dont have a last so pass in null */
        p = heap_extend( NULL , size);

        /*p is NULL only if sbrk( ) failed in heap_extend( ) */
        if(p == NULL){
            /* error for not enough memory in the heap - something really got fucked up */
            return NULL;
        }
        /* save line and file malloc was ran from and set a signature for later recognition */
        set_meta(p, n_line, f_name);

        /*the first malloc'd space is p , set root to p*/
        root = p;

        /* first malloc successful -> num allocated = 0 because on return of not null we increment that number */
        num_allocated = 0;

    }

    /* Return either the first block of memory if root was NULL or new block of memory other wise. Either stored in p*/
    num_allocated++; /*increment the number of allocated blocks */

    return (char *)p+BLOCK_SIZE; /*return the memory block + 1 which is 1x sizeof(struct mem_block)*/
}

void my_free( void  * p , int n_line , char *f_name ){

    pt_block    b;

    if(p == NULL){

        printf("%s==>Error - passed in pointer is NULL%s\n", RED, RESET);
        return;
    }
    if(is_valid_memblock(p, n_line, f_name)) {


        /*get the pointer to memory that includes the struct mem_block (need to free that too)*/
        b = get_block_struct(p);
        b->free = 1;  /* this section of memory (including the struct) is now marked free) */
        /*set all the bytes in bto null */


        /* check if the block before this one is free , attempt to combine this block and the previous one */
        if (b->prev != NULL && b->prev->free == 1) {
            /* here I pass the previous block into combine_blocks. combine blocks will use the prev block
             * and check next (in this case the block we are freeing) and attempt to combine the free space */

            char *file = b->file; /*save meta data */
            int   line = b->line;

            b = combine_blocks(b->prev);
            set_meta(b,line,file);

        }
        /*if we have a next block - attempt to combine it  with current*/
        if (b->next != NULL) {
            /* there is no assignmnet of the returned pointer to b here because:
             * 1. If there was a previous free block we already added it to b which now holds the whole section of b and
             *      b->prev.
             * 2. If there was no previous block, we are only combining current b and b->next, which means the memory still
             *         begins in the spot b currently points to.
             */
            combine_blocks(b);

        }
        else { /* we are at the end of the heap, no next block */

            /* check for previous block, the combine functions earlier only check for a free previous block */
            if (b->prev) {
                /* we have a previous node, we set it's next pointer to null since the current memory is freed*/
                b->prev->next = NULL;

            }
            else { /*no previous means that we are freeing the last bit of memmory still allocated */


                /* last bit of memory is cleaned so we have no list of nodes , set root to null */
                root = NULL;

                /* set the breakpoint for the heap to our initial address
                * (should equal the value of the very first sbrk(0) - b should point to that value now*/
                brk(b);


            }

        }
        /*decrement the number of blocks allocated*/
        num_allocated--;

    }
}



/*
 * This function is called by atexit( ) notifys the programmer that he has memory leaks and lets you know the # of them
 */
void mem_leak_check( void ){

    if(num_allocated == 0 && root == NULL){
        /*good to go no memory leaks! */
        return;
    }
    else{
        printf( "%s\n========================E X I T======================================\n"
                        "==>ERROR: Memory leaks in your code %i pointer(s) remain allocated: %s\n",RED, num_allocated, RESET);
        while(root != NULL){
            if(root->free == 0){
                printf("%s\t    -Memmory allocated at Line #%i in File %s not freed!%s\n", RED, root->line, root->file , RESET);
            }
            root = root->next;
        }


    }
    return;
}

/*
 * combine_blocks function checks the next block of memory in front of
 * the mem_block we pass in, in an attempt to combine the size of both
 * into one single free mem_block.
 */
pt_block combine_blocks(pt_block p){

    /* if there is a block ahead and it's free */
    if(p->next != NULL && p->next->free == 1) {
        /* add to size of the current block  the size of next block plus the size of mem_block  */
        p->size += (p->next->size) + BLOCK_SIZE;
        p->next = p->next->next; /*fused the next block, set its next pointer to what the old block next pointed to*/
        if (p->next != NULL) {
            p->next->prev = p;
        }
    }

    return p;
}

/*
 * Since when get_free_block looks for a first block of memory that would at very least fit the requested size,
 * it can result in wasting the excess space, i.e internal fragmentation. In order to prevent that this function
 * attempts to cuts the passed in block to requested size, and puts a new mem_block struct in the front of the memory
 * that's left with free = 1 and size = to remaining size of the block.
 */
void trim_to_size(pt_block p,int size){

    pt_block        new;
    new = p + size;
    new->size = p->size - size - BLOCK_SIZE;
    new->next = p->next;
    new->free = 1;

    p->size = size;
    p->next = new;

}

/* sets the __LINE__ and __FILE__ for a given mem_block . Util to not repeat code */
void set_meta(pt_block p, int n_line, char* f_name) {
    p->line = n_line;
    p->file = f_name;
    p->signature = SIGN;
}

/*
 * This function just subtracts the BLOCK_SIZE from pointer p.
 * It is here to avoid pointer arithmatic on void * ptr, we get a
 * temp char * and do the arithmatic there. Then return the resulting pointer.c
 */
void *get_block_struct(void *p){
    char *temp;
    temp = p;
    p = (temp-BLOCK_SIZE);
    return p;
}

/*
 * pointer p passed in is the pointer to the beginning of the (assumedly) allocated memory we are trying to free.
 * First this function checks if the root is null (aka we didnt allocate anything yet). Then there is a check for p being
 * in range from beginning of the heap to the breakpoint of the heap, if p is not in that bound
 * then the memory trying to be freed is not (my)malloc'd. If p is in range we check the signature for the node, if its
 * not the correct signature , the memory was not allocated using (my)malloc. Then we check if it's already freed, print
 * error accordingly. If both the checks pass we return a 1 indicating that p points to a valid memory block;
 */
int is_valid_memblock(void *p, int n_line, char *f_name) {

    pt_block b = get_block_struct(p);

    /* check if we are trying to free before any memory has even been allocated i.e root still null */
    if(root){

        /* cast root to void * to just get the raw pointer */
        if ( p >(void *) root &&  p < sbrk( 0 ) ) {


            /* compare the signature saved in the struct ! */
            if (b->signature != SIGN) {
                printf("%s==>>ERROR in %s\n==>>LINE #%i - freeing pointer not returned by (my)malloc%s\n",RED,f_name, n_line, RESET);
                return 0;
            }
            else if( b->free ){

                printf( "%s==>>ERROR in %s\n==>>LINE #%i - the pointer was already free'd%s\n" , RED , f_name,n_line, RESET);
                return 0;
            }
            else {
                /*success*/

                return 1;
            }

        }
        else {
            printf("%s==>>ERROR corrupted memory%s\n", RED, RESET);
            return 0;
        }

    }
        printf( "%s==>>ERROR in %s\n==>>LINE #%i - the pointer argument was never malloc'd%s\n", RED , f_name,n_line, RESET);
        return 0;

}

/*
 * heap_extend is a function made to do exactly that - extend the heap.
 * It is separate as to not clutter my_malloc( ).
 */
pt_block heap_extend( pt_block last , int size ){

    /* pointer to mem_block for operations and return value in this function */
    pt_block            p;

    /* Get the breakpoint of the heap */
    p = sbrk( 0 );
    /*extend heap by specified size + BLOCK_SIZE */

    if(sbrk(BLOCK_SIZE + size) ==(void *)-1){
        /* sbrk failed and wew didnt get any memory */
        return NULL;
    }

    /* Give our new memory a size and set next to be null we already need to extend aka we're at
     * the end of the heap
     */
    p->size = size;
    p->next = NULL;
    p->prev = last; /*TODO - test this line */
    p->signature = SIGN;
    /* if there is a previous mem_block struct set it's next ptr to p */
    if( last != NULL) {
        last->next = p;
    }
    /* Not a free block anymore ( 0 ) */
    p->free = 0;

    /* p here is pointer to our new block of memory with the new mem_block struct */
    return p;
}

/*
 * get_free_block (get free block) traverses the linked list of mem_block
 * structs to find first free block that has memory >= size
 */
pt_block get_free_block(pt_block *last, int size) {

    /* get the beginning of the linked list of mem_block structs */
    pt_block p = root;


    /* while we have a struct at p and its not of the right size and free
     * we keep looking
     */
    while (p != NULL && !(p->free && p->size >= size)) {
        /*keep the pointer to the last struct visited */
        *last = p;
        /* move to next block */
        p = p->next;
    }
    /*return what we found! */

    return p;
}


