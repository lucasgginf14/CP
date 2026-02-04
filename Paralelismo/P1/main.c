#include <stdio.h>
#include <math.h>
#include </usr/include/x86_64-linux-gnu/mpi/mpi.h>

int main(int argc, char *argv[]){

    int numprocs, rank;

    MPI_Init(&argc, &argv);

    MPI_Comm_size ( MPI_COMM_WORLD , &numprocs);
    MPI_Comm_rank ( MPI_COMM_WORLD , &rank);

    while(1) {

        if (rank != 0) {
            int n_rec;
            MPI_Recv(&n_rec, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            if(n_rec == 0) break;       //para salir de bucle

            double h = 1.0 / (double) n_rec;
            double sum = 0.0;
            double x;
            for (int i = rank + 1; i <= n_rec; i += numprocs) {
                x = h * ((double) i - 0.5);
                sum += 4.0 / (1.0 + x * x);
            }

            MPI_Send(&sum, 1, MPI_DOUBLE, 0, rank, MPI_COMM_WORLD);//supoño q o proceso root e siempre 0
        } else {

            int n;
            double pi, h, sum, x;

            printf("Enter the number of intervals: (0 quits) \n");
            scanf("%d", &n);


            for (int j = 1; j < numprocs; j++)
                MPI_Send(&n, 1, MPI_INT, j, rank, MPI_COMM_WORLD);

            if (n == 0) break;  //para salir del bucle

            h = 1.0 / (double) n;
            sum = 0.0;

            for (int j = rank + 1; j <= n; j += numprocs) {
                x = h * ((double) j - 0.5);
                sum += 4.0 / (1.0 + x * x);
            }

            double acc = sum, aux;
            for (int j = 1; j < numprocs; j++) {
                MPI_Recv(&aux, 1, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                acc = acc + aux;
            }

            double PI25DT = 3.141592653589793238462643;
            pi = h * acc;
            printf("pi is approximately %.16f, Error is %.16f\n", pi, fabs(pi - PI25DT));
        }
    }
    MPI_Finalize();
}