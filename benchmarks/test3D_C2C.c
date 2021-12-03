/*
* Harness software for benchmarking parallel FFT libraries.
* Benchmark: C2C 3-D transform, double precision
* Autor: Alan Ayala - ICL, UTK.
! USAGE:
    make clean; make -j; mpirun -n $NUM_RANKS ./test3D_CPU_C2C <library>
*/

#include "fiber_backends.h"
#include "fiber_utils.h"


int main(int argc, char** argv){
   
    // Get backend type from user
    int my_backend  = fiber_get_backend(argv[1]);

    MPI_Init(&argc, &argv);
    MPI_Comm comm = MPI_COMM_WORLD;

    int me;
    MPI_Comm_rank(comm, &me);

    int num_ranks;
    MPI_Comm_size(comm, &num_ranks);


    // Global grid
    int nx, ny, nz ;
    nx = atoi(argv[2]);
    ny = atoi(argv[3]);
    nz = atoi(argv[4]);

    int fftsize = nx*ny*nz;

    // select grid of processors with MPI built-in function
    int proc_grid[2] = {0,0};
    MPI_Dims_create(num_ranks, 2, proc_grid);
    printf("MPI_Dims_create [%d] proc_grid 1x%dx%d \n",  me, proc_grid[0], proc_grid[1]);

    proc_grid[0] = 1;
    // proc_grid[1] = 2;
    proc_grid[1] = num_ranks;
    
    // create cartesian grid
    int periods[2]  = {0,0};
    int mpi_reorder = 1;

    MPI_Comm cart_comm;
    MPI_Cart_create(comm, 2, proc_grid, periods, mpi_reorder, &cart_comm);

    int proc_rank;
    MPI_Comm_rank(cart_comm, &proc_rank);
    int proc_coords[2];
    MPI_Cart_coords(cart_comm, proc_rank, 2, proc_coords);


    // Boxes
    int box_low[3]  = {0, 0, 0};
    int box_high[3] = {0, 0, 0};

    // fast dimension
    box_low[0]  =  0;
    box_high[0] =  nx-1;
    // mid dimension
    box_low[1]  =  proc_coords[0] * (ny / proc_grid[0]);
    box_high[1] =  (proc_coords[0]+1) * (ny / proc_grid[0]) - 1; 
    // slow dimension
    box_low[2]  =  proc_coords[1] * (nz / proc_grid[1]);
    box_high[2] =  (proc_coords[1]+1) * (nz / proc_grid[1]) - 1; 

    int local_fft_size = (box_high[0]-box_low[0]+1)*(box_high[1]-box_low[1]+1)*(box_high[2]-box_low[2]+1);

   printf("[%d] proc_grid 1x%dx%d - [New MPI process %d] I am located at (%d, %d).   %d-%d-%d    %d-%d-%d   local_size={%d} \n", me, proc_grid[0], proc_grid[1],  proc_rank, proc_coords[0],proc_coords[1], \
                                        box_low[0], box_low[1], box_low[2], box_high[0], box_high[1], box_high[2], local_fft_size);

    fiber_complex *input  = calloc(local_fft_size, sizeof(fiber_complex));
    fiber_complex *output = calloc(local_fft_size, sizeof(fiber_complex));

    // Data Initialization
    int i;
    for(i=0; i<local_fft_size; i++){
        input[i].r = (double) i;
        input[i].i = (double) 0*i;
    }        
    input[0].r = 1;
    input[0].i = 2;

    // for(i=0; i<local_fft_size; i++) 
    //     printf("  %g+%gi, ", input[i].r, input[i].i);
    // printf("\n");        
    // printf("\n");      

    double timer[20];    
    int backend_options[20];
    backend_options[0] = 0; // forward/backward flag
    backend_options[1] = nx; // nx flag
    backend_options[2] = ny; // ny flag
    backend_options[3] = nz; // nz flag
    
    // ********************************
    // Compute forward (Z2Z) transform
    // ********************************
    fiber_execute_z2z[my_backend].function(box_low, box_high, box_low, box_high, comm, input, input, backend_options, timer);

    // Output after forward
    for(i=0; i<local_fft_size; i++) 
        printf(" %g+%gi, ", input[i].r, input[i].i);
    printf("\n");
    printf("\n");

    /*
    backend_options[0] = 1; // forward/backward flag
    fiber_execute_z2z[8].function(box_low, box_high, box_low, box_high, comm, input, input, backend_options, timer);

    // Output after backward
    for(i=0; i<local_fft_size; i++) {
        input[i].r /= fftsize;
        input[i].i /= fftsize;
        printf(" %g+%gi, ", input[i].r, input[i].i);
    }        
    printf("\n");
    printf("\n");

    // Error computation after backward
    double err = 0.0;
    for(i=1; i<local_fft_size; i++){
        if (fabs(input[i].r - (double) i ) > err)
            err = fabs(input[i].r - (double) i - 1);

        if( fabs(input[i].i) > 1.E-11 )
             err = fmax( fabs(input[0].i) , err);
    }

    err = fmax( fabs(input[0].r - (double) 1) , err);
    err = fmax( fabs(input[0].i - (double) 2) , err);

    // Print errors
    printf("%s: rank [%d] computed error |X - ifft(fft(X)) |_{max}: %1.6le\n", backends[my_backend], me, err);

    */


    // Data deallocation 
    free(input);
    free(output);


    MPI_Finalize();

    return 0;
}