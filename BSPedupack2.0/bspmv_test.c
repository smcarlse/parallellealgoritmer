#include "bspedupack.h"

/* This is a test program which uses bspmv to multiply a 
   sparse matrix A and a dense vector u to obtain a dense vector v.
   The sparse matrix and its distribution are read from an input file.
   The dense vector v is initialized by the test program.
   The distribution of v is read from an input file.
   The distribution of u is read from another input file.

   The output vector is defined by
       u[i]= (sum: 0 <= j < n: a[i][j]*v[j]).
*/

#define DIV 0
#define MOD 1
#define NITERS 10
#define STRLEN 100
#define MAX_LINE_LENGTH 500 // Maximum length of header line
#define NPRINT 10  // Print NPRINT values per processor

typedef struct {long i,j;} indexpair;

long P;

double bspinput2triple(long *pnA, long *pnz, 
                     long **pia, long **pja, double **pa){
  
    /* This function reads a sparse matrix in distributed
       Matrix Market format from the input file and
       distributes matrix triples to the processors.
       The matrix must be square and contain numerical values
       (real or integer).

       The input consists of a number of header lines starting with '%'
       followed by one line
           m n nz p  (number of rows, columns, nonzeros, processors)
       followed by p+1 lines with the starting numbers
       of the processor parts
           Pstart[0]
           Pstart[1]
           ...
           Pstart[p]
       which means that processor q will get all nonzeros
       numbered Pstart[q]..Pstart[q+1]-1.
       This is followed by nz lines in the format
           i j a     (row index, column index, numerical value).
       The input indices are assumed by Matrix Market to start
       counting at one, but they are converted to start from zero.
       The triples are stored on the responsible processor
       into three local arrays ia, ja, a, in arbitrary order.
       
       Output:
       nA is the global matrix size.
       nz is the number of local nonzeros.
       a[k] is the numerical value of the k'th local nonzero,
            0 <= k < nz.
       ia[k] is the global row index of the k'th local nonzero.
       ja[k] is the global column index.

       The function called by P(0) returns the sum of the
       matrix values for checksum purposes.
    */

    long p= bsp_nprocs(); // p = number of processors obtained
    long s= bsp_pid(); // processor number
    double sum= 0.0;

    /* Initialize file pointer to NULL */
    FILE *fp = NULL;
    
    /* Initialize data and register global variables */
    long *Pstart= vecalloci(p+1);
    bsp_push_reg(pnA,sizeof(long));
    bsp_push_reg(pnz,sizeof(long));

    size_t tagsize= sizeof(indexpair);
    bsp_set_tagsize(&tagsize);
    bsp_sync();

    if (s==0){
        /* Open the matrix file and read the header */
        char filename[STRLEN];
        printf("Please enter the filename of the matrix distribution\n");
        scanf("%s",filename);
        fp=fopen(filename,"r");

        /* A is an mA by nA matrix with nzA nonzeros
           distributed over pA processors. */

        /* Read header */
        char line[MAX_LINE_LENGTH];
        long mA, nzA, pA;

        bool header_done= false;
        while (!header_done){
            fgets(line, MAX_LINE_LENGTH, fp);
            /* Skip lines starting with '%' */
            if (line[0] != '%') {
                sscanf(line,"%ld %ld %ld %ld\n", &mA, pnA, &nzA, &pA);
                header_done= true;
            }
        }
        if(mA != *pnA)
            bsp_abort("Error: matrix is not square");
        if(pA != p)
            bsp_abort("Error: p not equal to p(A)\n"); 

        /* Read distribution sizes */
        for (long q=0; q<=p; q++)
            fscanf(fp,"%ld\n", &Pstart[q]);
        for (long q=0; q<p; q++){
            bsp_put(q,pnA,pnA,0,sizeof(long));
            long nzq= Pstart[q+1]-Pstart[q];
            bsp_put(q,&nzq,pnz,0,sizeof(long));
        }
    }
    bsp_sync();


    /* Handle the processors one at a time, which saves buffer
       memory, at the expense of p-1 extra syncs. The buffer
       memory needed for communication of the input is at most
       the maximum amount of memory that a processor needs to
       store its nonzeros. */

    long length= *pnz+1; // number of local nonzeros 
                         // + 1 extra for a dummy
    *pa= vecallocd(length);
    *pia= vecalloci(length);  
    *pja= vecalloci(length);

    for (long q=0; q<p; q++){      
        indexpair t;
        if (s==0){
            /* Read the nonzeros from the matrix file and
               send them to their destination */
            for (long k=Pstart[q]; k<Pstart[q+1]; k++){
                double value;
                fscanf(fp,"%ld %ld %lf\n", &t.i, &t.j, &value);
                /* Convert indices to range 0..n-1, 
                   assuming they were in 1..n */
                t.i--;
                t.j--;
                /* Send a triple to P(q). 
                   The tag is an index pair (i,j).
                   The payload is a numerical value */
                bsp_send(q,&t,&value,sizeof(double));
                sum += value;
            }
        }
        bsp_sync();
        
        if (s==q){
            /* Store the received nonzeros */
            for (long k=0; k<(*pnz); k++){
                size_t status;
                bsp_get_tag(&status,&t);
                (*pia)[k]= t.i;
                (*pja)[k]= t.j;
                bsp_move(*pa+k,sizeof(double));
            }
        }
    }

    if (s==0)
        fclose(fp);
    bsp_pop_reg(pnz);
    bsp_pop_reg(pnA);
    vecfreei(Pstart);
    bsp_sync();

    return sum;
    
} /* end bspinput2triple */


long key(long i, long radix, long keytype){
   /* This function computes the key of an index i
      according to the keytype */
   
       if (keytype==DIV)
           return i/radix;
       else /* keytype=MOD */
           return i%radix;
           
} /* end key */


void sort(long n, long nz, long *ia, long *ja, double *a,
          long radix, long keytype){

   /* This function sorts the nonzero elements of an n by n
      sparse matrix A stored in triple format in arrays ia, ja, a.
      The sort is by counting. 

      If keytype=DIV, the triples are sorted by increasing value of
      ia[k] div radix.
      if keytype=MOD, the triples are sorted by increasing value of
      ia[k] mod radix.

      The sorting is stable: ties are decided so that the original
      precedences are maintained. For a complete sort by increasing
      index ia[k], this function should be called twice:
      first with keytype=MOD, then with keytype=DIV.
      
      Input: 
      n is the size of the matrix.
      nz is the number of nonzeros.
      a[k] is the numerical value of the k'th nonzero of the
           sparse matrix A, 0 <= k < nz.
      ia[k] is the row index of the k'th nonzero, 0 <= ia[k] < n.
      ja[k] is the column index of the k'th nonzero.
      The indices can be local or global.
      radix >= 1.
      
      Output: ia, ja, a in sorted order.
   */
   
   if (nz <= 0)
       return;

   long *ia1= vecalloci(nz);
   long *ja1= vecalloci(nz); 
   double *a1 = vecallocd(nz);
   
   /* Allocate bins */
   long nbins;
   if (keytype==DIV)
       nbins= (n%radix==0 ? n/radix : n/radix+1);
   else {
       // keytype=MOD
       nbins= radix;
   }
   long *startbin= vecalloci(nbins);
   long *lengthbin= vecalloci(nbins);
       
   /* Count the elements in each bin */
   for (long r=0; r<nbins; r++)
       lengthbin[r]= 0;
   for (long k=0; k<nz; k++){
       long r= key(ia[k],radix,keytype);
       lengthbin[r]++;
   }
    
   /* Compute the starting position of each bin */
   startbin[0]= 0;
   for (long r=1; r<nbins; r++)
       startbin[r]= startbin[r-1] + lengthbin[r-1];
       
   /* Enter the elements into the bins in temporary arrays (ia1,ja1,a1) */
   for (long k=0; k<nz; k++){
       long r= key(ia[k],radix,keytype);
       long k1= startbin[r];
       ia1[k1]= ia[k];
       ja1[k1]= ja[k];
       a1[k1] = a[k];
       startbin[r]++;
   }
  
   /* Copy the elements back to the orginal arrays */
   for (long k=0; k<nz; k++){
       ia[k]= ia1[k];
       ja[k]= ja1[k];
       a[k] = a1[k];
   }
   
   vecfreei(lengthbin);
   vecfreei(startbin);
   vecfreed(a1);
   vecfreei(ja1);
   vecfreei(ia1);
   
} /* end sort */


void triple2icrs(long n, long nz, long *ia,  long *ja, double *a,
                 long *pnrows, long *pncols,
                 long **prowindex, long **pcolindex){

    /* This function converts a sparse matrix A given in triple
       format with global indices into a sparse matrix in
       incremental compressed row storage (ICRS) format with 
       local indices.

       The conversion needs time and memory O(nz + sqrt(n))
       on each processor, which is O(nz(A)/p + n/p + p).
       
       Input:
       n is the global size of the matrix.
       nz is the local number of nonzeros.
       a[k] is the numerical value of the k'th nonzero
            of the sparse matrix A, 0 <= k <nz.
       ia[k] is the global row index of the k'th nonzero.
       ja[k] is the global column index of the k'th nonzero.
  
       Output:
       nrows is the number of local nonempty rows
       ncols is the number of local nonempty columns
       rowindex[i] is the global row index of the i'th
                   local row, 0 <= i < nrows.
       colindex[j] is the global column index of the j'th
                   local column, 0 <= j < ncols.
       a[k] is the numerical value of the k'th local nonzero of the
            sparse matrix A, 0 <= k < nz. The array is sorted by
            increasing row index, ties being decided by 
            increasing column index.
       inc[k] is the increment in the local column index
           of the k'th local nonzero, compared to the column
           index of the (k-1)th nonzero if this nonzero is
           in the same row; otherwise, ncols is added
           to the difference.
           Here, the column index of the -1'th nonzero is 0.
   */
    
   /* radix is the smallest power of two >= sqrt(n)
      The div and mod operations are cheap for powers of two.
      A radix of about sqrt(n) minimizes memory and time. */

   long radix;
   for (radix=1; radix*radix<n; radix *= 2)
       ;
   
   /* Sort nonzeros by column index using radix sort */
   sort(n,nz,ja,ia,a,radix,MOD); // ja goes first
   sort(n,nz,ja,ia,a,radix,DIV);
   
   /* Count the number of local columns */
   long ncols= 0;
   long jglob_prev= -1;
   for (long k=0; k<nz; k++){
       long jglob= ja[k];
       if(jglob!=jglob_prev)
           /* new column index */
           ncols++;
       jglob_prev= jglob;
   }
   long *colindex= vecalloci(ncols);
   
   /* Convert global column indices to local ones.
      Initialize colindex */
   long j= 0;
   jglob_prev= -1;
   for (long k=0; k<nz; k++){
       long jglob= ja[k];
       if(jglob!=jglob_prev){
           colindex[j]= jglob;
           j++; 
           // j= first free position
       }
       ja[k]= j-1; // last entered local index
       jglob_prev= jglob;
   }
   
   /* Sort nonzeros by row index using radix sort */
   sort(n,nz,ia,ja,a,radix,MOD); // ia goes first
   sort(n,nz,ia,ja,a,radix,DIV);

   /* Count the number of local rows */
   long nrows= 0;
   long iglob_prev= -1;
   for (long k=0; k<nz; k++){
       long iglob= ia[k];
       if(iglob!=iglob_prev)
           /* new row index */
           nrows++;
       iglob_prev= iglob;
   }
   long *rowindex= vecalloci(nrows);
                              
   /* Convert global row indices to local ones.
      Initialize rowindex and inc */
   long i= 0;
   iglob_prev= -1;
   for (long k=0; k<nz; k++){
       long inck;
       if (k==0)
           inck= ja[k];
       else
           inck= ja[k] - ja[k-1];

       long iglob= ia[k]; 
       if(iglob!=iglob_prev){
           rowindex[i]= iglob;
           i++;
           if(k>0)
               inck += ncols;
       } 
       ia[k]= inck; // ia is used to store inc
       iglob_prev= iglob;
   }

   /* Add dummy triple */
   if (nz==0)
       ia[nz]= ncols; // value does not matter
   else 
       ia[nz]= ncols - ja[nz-1];
   ja[nz]= 0;                                                  
   a[nz]= 0.0;     
   
   *pnrows= nrows;
   *pncols= ncols;
   *prowindex= rowindex;
   *pcolindex= colindex;
   
} /* end triple2icrs */


void bspinputvec(const char *filename,
                 long *pn, long *pnv, long **pvindex){
  
    /* This function reads the distribution of a dense
       vector v from the input file and initializes the
       corresponding local index array.
       The input consists of one line
           n p    (number of components, processors)
       followed by n lines in the format
           i proc (index, processor number),
       where i=1,2,...,n.
       
       Input:
       filename is the name of the input file.

       Output:
       n is the global length of the vector.
       nv is the local length.
       vindex[i] is the global index corresponding to
                 the local index i, 0 <= i < nv.
    */

    long p= bsp_nprocs(); // p = number of processors obtained
    long s= bsp_pid(); // processor number

    /* Initialize fp and Nv */
    FILE *fp = NULL;
    long *Nv = NULL; // Nv[q] = number of components of P(q)

    bsp_push_reg(pn,sizeof(long));
    bsp_push_reg(pnv,sizeof(long));
    bsp_sync();

    if (s==0){
        /* Open the file and read the header */
        long procsv;
        fp=fopen(filename,"r");
        fscanf(fp,"%ld %ld\n", pn, &procsv);
        if(procsv!=p)
            bsp_abort("Error: p not equal to p(vec)\n"); 
        for (long q=0; q<p; q++)
            bsp_put(q,pn,pn,0,sizeof(long));
    }
    bsp_sync();

    /* The owner of the global index i and its
       local index are stored in temporary arrays 
       which are distributed cyclically. */
    long n = *pn;
    long np= nloc(p,s,n);
    long *tmpproc= vecalloci(np);
    long *tmpind= vecalloci(np);
    bsp_push_reg(tmpproc,np*sizeof(long));
    bsp_push_reg(tmpind,np*sizeof(long));
    bsp_sync();

    if (s==0){
        /* Allocate component counters */
        Nv= vecalloci(p);
        for (long q=0; q<p; q++)
            Nv[q]= 0;
    }

    long b= (n%p==0 ? n/p : n/p+1); // block size for vector read
    for (long q=0; q<p; q++){
        if(s==0){
            /* Read the owners of the vector components from
               file and put owner and local index into a
               temporary location. This is done n/p
               components at a time to save memory  */
            for (long k=q*b; k<(q+1)*b && k<n; k++){
                long i, proc, ind;
                fscanf(fp,"%ld %ld\n", &i, &proc);
                /* Convert index and processor number
                   to ranges 0..n-1 and 0..p-1,
                   assuming they were in 1..n and 1..p */
                i--;  
                proc--;
                ind= Nv[proc];
                if(i!=k)
                    bsp_abort("Error: i not equal to index \n");
                bsp_put(i%p,&proc,tmpproc,(i/p)*sizeof(long),sizeof(long));
                bsp_put(i%p,&ind,tmpind,(i/p)*sizeof(long),sizeof(long));
                Nv[proc]++;
            }
        }
        bsp_sync();
    }

    if(s==0){
        for (long q=0; q<p; q++)
            bsp_put(q,&Nv[q],pnv,0,sizeof(long));
    }
    bsp_sync();

    /* Store the components at their final destination */
    long *vindex= vecalloci(*pnv);  
    bsp_push_reg(vindex,(*pnv)*sizeof(long));
    bsp_sync();

    for (long k=0; k<np; k++){
        long globk= k*p+s;
        bsp_put(tmpproc[k],&globk,vindex,
                tmpind[k]*sizeof(long),sizeof(long));
    }
    bsp_sync();

    bsp_pop_reg(vindex);
    bsp_pop_reg(tmpind);
    bsp_pop_reg(tmpproc);
    bsp_pop_reg(pnv);
    bsp_pop_reg(pn);
    bsp_sync();

    vecfreei(Nv);
    vecfreei(tmpind);
    vecfreei(tmpproc);

    *pvindex= vindex;

} /* end bspinputvec */


void bspmv_test(){
    void bspmv_init(long n, long nrows, long ncols,
               long nv, long nu, long *rowindex, long *colindex,
               long *vindex, long *uindex,
               long *srcprocv, long *srcindv,
               long *destprocu, long *destindu);
    void bspmv(long n, long nz, long nrows, long ncols,
               double *a, long *inc,
               long *srcprocv, long *srcindv,
               long *destprocu, long *destindu,
               long nv, long nu, double *v, double *u);
    
    bsp_begin(P);
    long p= bsp_nprocs(); /* p=P */
    long s= bsp_pid();
    
    /* Input of sparse matrix into triple storage */
    long n, nz, *ia, *ja;
    double *a;
    double suma = bspinput2triple(&n,&nz,&ia,&ja,&a);

    /* Convert data structure to incremental compressed row storage */
    long nrows, ncols, *rowindex, *colindex;
    triple2icrs(n,nz,ia,ja,a,&nrows,&ncols,&rowindex,&colindex);
    
    /* Read vector distributions */
    char vfilename[STRLEN], ufilename[STRLEN]; // only used by P(0)
    if (s==0){
        printf("Please enter the filename of the v-vector distribution\n");
        scanf("%s",vfilename);
    }
    long nv, *vindex;
    bspinputvec(vfilename,&n,&nv,&vindex);

    if (s==0){ 
        printf("Please enter the filename of the u-vector distribution\n");
        scanf("%s",ufilename);
    }
    long nu, *uindex;
    bspinputvec(ufilename,&n,&nu,&uindex);

    if (s==0){
        printf("Sparse matrix-vector multiplication");
        printf(" using %ld processors\n",p);
    }

    /* Initialize input vector v */
    double *v= vecallocd(nv);
    for (long i=0; i<nv; i++){
        v[i]= 1.0;
    }
    double *u= vecallocd(nu);
    
    if (s==0){
        printf("Initialization for matrix-vector multiplications\n");
        fflush(stdout);
    }
    bsp_sync(); 
    double time0= bsp_time();
    
    long *srcprocv= vecalloci(ncols);
    long *srcindv= vecalloci(ncols);
    long *destprocu= vecalloci(nrows);
    long *destindu= vecalloci(nrows);
    bspmv_init(n,nrows,ncols,nv,nu,
               rowindex,colindex,vindex,uindex,
               srcprocv,srcindv,destprocu,destindu);

    if (s==0){
        printf("Start of %ld matrix-vector multiplications.\n",
               (long)NITERS);
        fflush(stdout);
    }
    bsp_sync(); 
    double time1= bsp_time();
    
    for (long iter=0; iter<NITERS; iter++)
        bspmv(n,nz,nrows,ncols,a,ia,srcprocv,srcindv,
              destprocu,destindu,nv,nu,v,u);
    bsp_sync();
    double time2= bsp_time();
    
    if (s==0){
        printf("End of matrix-vector multiplications.\n");
        printf("Initialization took only %.6lf seconds.\n",time1-time0);
        printf("Each matvec took only %.6lf seconds.\n", 
                      (time2-time1)/(double)NITERS);
        printf("The computed solution is (<= %ld values per processor):\n",
                (long)NPRINT);
        fflush(stdout);
    }

    for (long i=0; i < NPRINT && i<nu; i++){
        long iglob= uindex[i];
        printf("proc=%ld i=%ld, u=%lf \n",s,iglob,u[i]);
    }

    /* Check error by computing
           sum (u[i] : 0 <= i < n) = sum (a[i][j] : 0 <= i, j < n) */
  
    double *SumU= vecallocd(p);
    bsp_push_reg(SumU,p*sizeof(double));
    bsp_sync();

    double sumu= 0.0;
    for (long i=0; i<nu; i++)
        sumu += u[i];
    bsp_put(0,&sumu, SumU, s*sizeof(double), sizeof(double));
    bsp_sync();

    if (s==0){
        double totalU= 0.0;
        for (long t=0; t<p; t++)
            totalU += SumU[t];
        printf("Sum(u)=%lf sum(A)=%lf checksum error = %lf\n",
               totalU,suma,fabs(totalU-suma));
    }
    bsp_pop_reg(SumU);
    bsp_sync();
    vecfreed(SumU); 

    vecfreei(destindu); vecfreei(destprocu); 
    vecfreei(srcindv);  vecfreei(srcprocv); 
    vecfreed(u);        vecfreed(v);
    vecfreei(uindex);   vecfreei(vindex);
    vecfreei(rowindex); vecfreei(colindex); 
    vecfreei(ja);       vecfreei(ia);
    vecfreed(a);

    bsp_end();
    
} /* end bspmv_test */


int main(int argc, char **argv){

    bsp_init(bspmv_test, argc, argv);

    /* sequential part */
    printf("How many processors do you want to use?\n");
    fflush(stdout);

    scanf("%ld",&P);
    if (P > bsp_nprocs()){
        printf("Sorry, only %u processors available.\n",
                bsp_nprocs());
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    /* SPMD part */
    bspmv_test();

    /* sequential part */
    exit(EXIT_SUCCESS);

} /* end main */
