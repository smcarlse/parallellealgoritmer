#include "bspedupack.h"

#define EPS 1.0e-15

void permute_rows(long M, long *Src, long *Dest, long nperm,
                  double **a, double *pa, long nlc, long jc, long jc1){

    /* This function permutes the rows of matrix A such that local row
       Src[i] moves to row Dest[i] of A, for 0 <= i < nperm.
       The permutation is only carried out for local column indices
       jc <= j < jc1.
       M = number of processor rows,
       nlc = length of local rows of matrix A 
           = the leading dimension of the storage format,
       A is stored as a 2D array a, which is also accessible
       as a 1D array pa.
    */

    if (jc1 <= jc)
        return;

    long pid= bsp_pid();
    long t= pid/M;

    for (long i=0; i<nperm; i++){
        /* Store row Src[i] of A in row r=Dest[i] on P(r%M,t) */
        long r= Dest[i];
        bsp_put(r%M+t*M,a[Src[i]/M],pa,((r/M)*nlc+jc)*sizeof(double),
                                            (jc1-jc)*sizeof(double));
    }

} /* end permute_rows */


void bsp_broadcast(double *x, long n, long src, long s0,
                   long stride, long p0, long phase){

    /* Broadcast the vector x of length n from processor src to
       processors s0+t*stride, 0 <= t < p0. Here n, p0 >= 1.
       The vector x must have been registered previously.
       Processors are numbered in 1D fashion.

       phase= phase of two-phase broadcast (0 or 1)
       Only one phase is performed, without synchronization.
    */

    if (n < 1 || p0 < 1)
        return;

    long s= bsp_pid(); /* processor number */
    long b= ( n%p0==0 ?  n/p0 : n/p0+1 ); /* broadcast block size */

    if ((phase==0 && s==src) ||
        (phase==1 && s0 <= s && s < s0+p0*stride &&  (s-s0)%stride==0)){
        /* participate */

        for (long t=0; t<p0; t++){
            long dest= s0+t*stride;
            long t1 = (phase==0 ? t : (s-s0)/stride);
                 // in phase 1: s= s0+t1*stride 

            long nbytes= MIN(b,n-t1*b)*sizeof(double);
            if (nbytes>0 && dest!=src)
                bsp_put(dest,&x[t1*b],x,t1*b*sizeof(double),nbytes);
        }
    }

} /* end bsp_broadcast1 */



void bsplu(long M, long N, long n, long *pi, double **a){

    /* Compute LU decomposition of n by n matrix A with partial
       row pivoting.
       Processors are numbered in 2D fashion.
       Program text for P(s,t) = processor s+t*M,
       with 0 <= s < M and 0 <= t < N.
       A is distributed according to the M by N cyclic distribution.
       pi stores the output permutation, with a cyclically distributed
          copy in every processor column.
    */

    long pid= bsp_pid();
    long s= pid%M;
    long t= pid/M;

    long nlr=  nloc(M,s,n); /* number of local rows */
    long nlc=  nloc(N,t,n); /* number of local columns */

    long r; /* index of pivot row */
    bsp_push_reg(&r,sizeof(long));

    /* Set pointer for 1D access to A */
    double *pa;
    if (nlr>0)
        pa= a[0];
    else
        pa= NULL;
    bsp_push_reg(pa,nlr*nlc*sizeof(double));

    /* Initialize permutation vector pi */
    for (long i=0; i<nlr; i++)
        pi[i]= i*M+s; /* global row index */
    bsp_push_reg(pi,nlr*sizeof(long));

    double *Uk= vecallocd(nlc); bsp_push_reg(Uk,nlc*sizeof(double));
    double *Lk= vecallocd(nlr); bsp_push_reg(Lk,nlr*sizeof(double));
    double *Max= vecallocd(M);  bsp_push_reg(Max,M*sizeof(double));
    long *Imax= vecalloci(M);   bsp_push_reg(Imax,M*sizeof(long));

    bsp_sync();

    for (long k=0; k<n; k++){

        /****** Superstep (0)/(1) ******/
        long kr=  nloc(M,s,k); /* first local row with global index >= k */ 
        long kr1= nloc(M,s,k+1); 
        long kc=  nloc(N,t,k); 
        long kc1= nloc(N,t,k+1);


        if (k%N==t){   /* k=kc*N+t */
            /* Search for local absolute maximum in column k of A */
            double absmax= 0.0;
            long imax= -1;
            for (long i=kr; i<nlr; i++){
                if (fabs(a[i][kc])>absmax){
                    absmax= fabs(a[i][kc]);
                    imax= i;
                }
            } 

            double max= 0.0;
            if (absmax>0.0)
                max= a[imax][kc];

            /* Broadcast value and local index of maximum to P(*,t) */
            for (long s1=0; s1<M; s1++){
                bsp_put(s1+t*M,&max,Max,s*sizeof(double),sizeof(double));
                bsp_put(s1+t*M,&imax,Imax,s*sizeof(long),sizeof(long));
            }
        }
        bsp_sync();
        
        /****** Superstep (2)/(3) ******/
        if (k%N==t){
            /* Determine global absolute maximum (redundantly) */
            double absmax= 0.0;
            long smax= -1;
            for (long s1=0; s1<M; s1++){
                if (fabs(Max[s1])>absmax){
                    absmax= fabs(Max[s1]);
                    smax= s1;
                }
            }
            if (absmax > EPS){
                long imax= Imax[smax];
                r= imax*M+smax; /* global index of pivot row */
                double pivot= Max[smax];
                for (long i=kr; i<nlr; i++)
                    a[i][kc] /= pivot;
                if (s==smax)
                    a[imax][kc]= pivot; /* restore value of pivot */
            } else {
                bsp_abort("bsplu at stage %d: matrix is singular\n",k);
            }
        }
        /* Obtain index of pivot row */
        bsp_get(s+(k%N)*M,&r,0,&r,sizeof(long));
        bsp_sync();

        /****** Superstep (4)/(5) ******/
        long nperm= 0;
        long Src[2], Dest[2];
        if (k%M==s){
            /* Store pi(k) in pi(r) on P(r%M,t) */
            bsp_put(r%M+t*M,&pi[k/M],pi,(r/M)*sizeof(long),sizeof(long));
            Src[nperm]= k;
            Dest[nperm]= r;
            nperm++;
        }
        if (r%M==s){
            bsp_put(k%M+t*M,&pi[r/M],pi,(k/M)*sizeof(long),sizeof(long));
            Src[nperm]= r;
            Dest[nperm]= k;
            nperm++;
        }
        /* Swap rows k and r */
        permute_rows(M,Src,Dest,nperm,a,pa,nlc,0,nlc);

        bsp_sync();

        /****** Superstep (6) ******/
        /* Phase 0 of two-phase broadcasts */
        if (k%N==t){ 
            /* Store new column k in Lk */
            for (long i=kr1; i<nlr; i++)     
                Lk[i-kr1]= a[i][kc];
        }
        if (k%M==s){ 
            /* Store new row k in Uk */
            for (long j=kc1; j<nlc; j++)
                Uk[j-kc1]= a[kr][j];
        }
        bsp_broadcast(Lk,nlr-kr1,s+(k%N)*M,  s,M,N,0);
        bsp_broadcast(Uk,nlc-kc1,(k%M)+t*M,t*M,1,M,0);
        bsp_sync();
       
        /****** Superstep (7) ******/
        /* Phase 1 of two-phase broadcasts */
        bsp_broadcast(Lk,nlr-kr1,s+(k%N)*M,  s,M,N,1); 
        bsp_broadcast(Uk,nlc-kc1,(k%M)+t*M,t*M,1,M,1);
        bsp_sync();
      
        /****** Superstep (0') ******/
        /* Update of A */
        for (long i=kr1; i<nlr; i++){
            for (long j=kc1; j<nlc; j++)
                a[i][j] -= Lk[i-kr1]*Uk[j-kc1];
        }
    }
    bsp_pop_reg(Imax); vecfreei(Imax);
    bsp_pop_reg(Max);  vecfreed(Max);
    bsp_pop_reg(Lk);   vecfreed(Lk);
    bsp_pop_reg(Uk);   vecfreed(Uk);
    bsp_pop_reg(pi); 
    bsp_pop_reg(pa); 
    bsp_pop_reg(&r);

} /* end bsplu */
