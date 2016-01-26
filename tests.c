#include <stdio.h>
#include <string.h>
#include "mymal.h"
#include <stdlib.h>

#define malloc( x )     my_malloc( x , __LINE__ , __FILE__ )
#define free( x )       my_free( x , __LINE__ , __FILE__ )

int
main() {


    char *t1,*t6;
    char *t2;
    char *t3;
    int *t4, i;
    char *t5;
    char *t7;
    void *negative_memory;

    /*failure on basis of allocating negative memory */
    negative_memory = malloc(-100);
    /*failure on basis of allocating 0 memory (would result in allocating just the struct otherwise - wastes space */
    negative_memory = malloc(0);

    t1 = (char *)malloc(100);
    t2 = (char *)malloc(200);
    t3 = (char *)malloc(300);
    t4 = (int *)malloc(400);
    t5 = (char *)malloc(1);
    t6 = (char *)malloc(4);
    t7 = (char *)malloc(5);

    /*JUST SO THE -Wall doesnt complain about unused pointer */
    *t4 = 4;
    t4 = (int *)negative_memory;

    /* put some characters in t7 to check if malloc doesnt complain about it */
    for(i = 0; i < 5; i++)
    {
        t7[i] = 'A';
    }


    free(t1); /*success */
    free(t5);
    free(t2+10); /*corrupted pointer tests ALSO t2 is never freed so not allocated memory test*/
    free(t6);
    free(t3);
    free(t1); /*double free tests */
    free(t7);

    /* t4 not freed , complain at exit */
    exit(EXIT_SUCCESS);
}