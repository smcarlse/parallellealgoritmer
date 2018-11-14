#include "bspedupack.h"

void bspsort(double *x, long n, long *nlout){

    /* Sorts an array x of n doubles by quicksort,
       without keeping track of the original indices.
 
       This is a sequential version, used as a reference for determining speedups.
    */

    int compare_doubles (const void *a, const void *b);

    *nlout= n;

    /* Sort block using system quicksort */
    qsort (x,n,sizeof(double),compare_doubles);

    
    return ;

} /* end bspsort */
