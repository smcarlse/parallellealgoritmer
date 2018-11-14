#include "bspedupack.h"

void bspsort(double *x, long n, long *nlout){

    /* Sorts an array x of n doubles by samplesort,
       without keeping track of the original indices.
       On input, x is distributed by a block distribution
       over p processors, where p^2 <= n; x must have been
       registered already.
       On output, x is distributed by a block distribution
       with variable block size. The local output block size
       satisfies 1 <= nlout <= 2n/p + p.
    */

    int compare_doubles (const void *a, const void *b);
    int compare_items (const void *a, const void *b);
    void merge(char *x, char *tmp, long a, long b, long c, 
               size_t size,
               int (*compare)(const void *,const void *));
    void mergeparts(char *x, long *start, long p, size_t size,
                    int (*compare)(const void *,const void *));

    long p= bsp_nprocs(); // p = number of processors obtained
    long s= bsp_pid();    // s = processor number
    if (2*p*p > n)
        bsp_abort("Error: bspsort only works if 2*p*p <= n \n");

    /* Allocate and register */
    double *Sample= vecallocd(2*p*p);
    bsp_push_reg(Sample,2*p*p*sizeof(double));

    /* Set tag size, where tag will store the sender pid */
    size_t tag_size = sizeof(long);
    bsp_set_tagsize(&tag_size);
    bsp_sync();

    /****** Superstep (0)/(1) ******/

    /* Sort local block using system quicksort */
    long nl= nloc(p,s,n); // number of local elements
    qsort (x,nl,sizeof(double),compare_doubles);

    /* Determine p (nearly) equally spaced samples */
    int count = 0;
    long nlp= nl/(2*p) ;               // nlp >= 1 because nl >= p
    for (long i=0; i <= nl%(2*p) ; i++) { 
        count++;
        Sample[i]= x[i*(nlp+1)];     // subblocks of size nlp+1 
    }
    for (long i= nl%p+1; i<(2*p) ; i++)  {
        count++;
        Sample[i]= x[i*nlp + nl%(2*p)] ; // subblocks of size nlp 
    }
    printf(" sa mange samples hadde jeg %d, processor %d \n", count,s);
    
    /* Put samples in P(*) */
    for (long t=0; t<p ; t++)
        bsp_put(t,Sample,Sample,s*2*p*sizeof(double),
                2*p*sizeof(double));
    bsp_sync();

    /****** Superstep (2)/(3) ******/

    /* Copy weight of samples */
    Item *SampleItem= vecallocitem(2*p*p);
    for (long i=0; i < 2*p*p ; i++)
        SampleItem[i].weight= Sample[i];

    /* Add global index to samples */
    long blocktotal_s=0; // size of all blocks of processors < s
    for (long t=0; t<p ; t++){
        long blocktotal_t; // size of all blocks of processors < t
        long nt= nloc(p,t,n); // number of local elements of P(t)
        if (t <= n%p)
            blocktotal_t= t*(n/p+1);
        else 
            blocktotal_t= t*(n/p) + n%p;
        if (t==s)
            blocktotal_s= blocktotal_t; // keep for later use

        /* Determine global index of samples of P(t) */
        long ntp= nt/(2*p);
        for (long i=0; i <= nt%p ; i++)
            SampleItem[t*p+i].index= blocktotal_t + i*(ntp+1);
        for (long i= nt%p+1; i<p ; i++)
            SampleItem[t*p+i].index= blocktotal_t + i*ntp + nt%p;
    }

    /* Sort samples with their indices */
    long *start= vecalloci(2*p+1);
    for (long t=0; t<p ; t++)
        start[t]= t*p;
    start[p]= 2*p*p;
    mergeparts ((void *)SampleItem,start,p,sizeof(Item),
                compare_items);

    /* Choose p equidistant splitters from the samples */
    Item *Splitter= vecallocitem(p);
    for (long t=0; t<p ; t++)
        Splitter[t]= SampleItem[t*p] ;

    /* Send the values */
    long i= 0;
    for (long t=0; t<p ; t++){
        /* Send the values for P(t) */
        long i0= i;    // index of first value to be sent
        long count= 0; // number of values to be sent
        while (i < nl && 
               (t==p-1 || (x[i] < Splitter[t+1].weight) ||
                          (x[i] == Splitter[t+1].weight && 
                           blocktotal_s + i < Splitter[t+1].index)
               )){
            count++ ;
            i++ ;
        }
        if (count > 0)
            bsp_send(t,&s,&x[i0],count*sizeof(double));
    }
    bsp_sync();

    /****** Superstep (4) ******/

    /* Concatenate the received parts, in arbitrary order */
    unsigned int nparts_recvd; // <= p
    size_t nbytes_recvd;
    bsp_qsize(&nparts_recvd,&nbytes_recvd);

    start[0]= 0;
    for (long j=0; j<nparts_recvd; j++){
        size_t payload_size; // payload size in bytes
        long t; // id of sending processor, obtained but not used
        bsp_get_tag(&payload_size,&t);
        bsp_move(&x[start[j]],payload_size);
        start[j+1]= start[j] + payload_size/sizeof(double);
    }

    /* Determine the total number of doubles received,
       which is the output local block size */
    *nlout = start[nparts_recvd];

    /* Sort the local values for final output */
    mergeparts((void *)x,start,nparts_recvd,sizeof(double),
               compare_doubles);
    
    vecfreei(start);
    vecfreeitem(Splitter);
    vecfreeitem(SampleItem);
    bsp_pop_reg(Sample);
    vecfreed(Sample);

    return ;

} /* end bspsort */
