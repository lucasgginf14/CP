#include <stdio.h>
#include </usr/include/x86_64-linux-gnu/mpi/mpi.h>
#include <sys/time.h>

//NOTA DE ESTA PRÁCTICA P3 = 0.65/0.75

#define DEBUG 1

#define N 5

int main(int argc, char *argv[] ) {

    MPI_Init(&argc, &argv);
    int numprocs, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    float result2[N];
    int T_COMPUTACION;
    int t_comp1;
    int TiempoComunicacion;
    int t_comp_i[numprocs];
    int t_comm_i[numprocs];


    if (rank == 0) {
        int i, j;
        float matrix[N][N];
        float vector[N];
        float result[N / numprocs];
        struct timeval tv1, tv2;

        /* Initialize Matrix and Vector */
        for (i = 0; i < N; i++) {
            vector[i] = i;
            for (j = 0; j < N; j++) {
                matrix[i][j] = i + j;
            }
        }

        /// N/P numero de filas q se pasan

        /// (N/P) * N Numero de elementos

        if ((N % numprocs) == 0) {
            float rowVals[N / numprocs][N];
            int send[numprocs], displays[numprocs], cnt = 0;
            for (int k = 0; k < numprocs; ++k) {
                send[k] = (N / numprocs) * N;
                displays[k] = cnt;
                cnt += ((N / numprocs) * N);
            }

            gettimeofday(&tv1, NULL);
            MPI_Bcast(vector, N, MPI_FLOAT, 0, MPI_COMM_WORLD);
            MPI_Scatterv(matrix, send, displays, MPI_FLOAT, rowVals, (N / numprocs) * N, MPI_FLOAT, 0, MPI_COMM_WORLD);
            gettimeofday(&tv2, NULL);
            int t_comm1 = (tv2.tv_usec - tv1.tv_usec) + 1000000 * (tv2.tv_sec - tv1.tv_sec);

            gettimeofday(&tv1, NULL);
            for(int i=0;i<N/numprocs;i++) {
                result[i]=0;
                for(int j=0;j<N;j++) {
                    result[i] += rowVals[i][j]*vector[j];
                }
            }
            gettimeofday(&tv2, NULL);
            t_comp1 = (tv2.tv_usec - tv1.tv_usec) + 1000000 * (tv2.tv_sec - tv1.tv_sec);

            int eachProc[numprocs];
            cnt = 0;
            for (int i = 0; i < numprocs; ++i) {
                eachProc[i] = N / numprocs;
                displays[i] = cnt;
                cnt += (N / numprocs);
            }

            t_comp_i[0] = t_comp1;

            gettimeofday(&tv1, NULL);
            MPI_Gatherv(result, N / numprocs, MPI_FLOAT, result2, eachProc, displays, MPI_FLOAT, 0, MPI_COMM_WORLD);
            MPI_Gather(&t_comp_i, 1, MPI_INT, t_comp_i, 1, MPI_INT, 0, MPI_COMM_WORLD);
            MPI_Gather(&t_comm_i, 1, MPI_INT, t_comm_i, 1, MPI_INT, 0, MPI_COMM_WORLD);
            gettimeofday(&tv2, NULL);
            int t_comm2 = (tv2.tv_usec - tv1.tv_usec) + 1000000 * (tv2.tv_sec - tv1.tv_sec);

            TiempoComunicacion = t_comm1 + t_comm2;
            t_comm_i[0] = TiempoComunicacion;

        } else {
            int rows_proc = N/numprocs;
            int diff = N % numprocs;

            float rowVals[(N / numprocs) + diff][N];
            int send[numprocs], displays[numprocs], cnt = 0;

            for (int k = 0; k < numprocs; ++k) {
                if (k == 0)
                    send[k] = (rows_proc + diff) * N;
                else
                    send[k] = (rows_proc * N);

                displays[k] = cnt;
                cnt += send[k];
            }

            gettimeofday(&tv1, NULL);
            MPI_Bcast(vector, N, MPI_FLOAT, 0, MPI_COMM_WORLD);
            MPI_Scatterv(matrix, send, displays, MPI_FLOAT, rowVals, send[rank], MPI_FLOAT, 0, MPI_COMM_WORLD);
            gettimeofday(&tv2, NULL);
            int t_comm1 = (tv2.tv_usec - tv1.tv_usec) + 1000000 * (tv2.tv_sec - tv1.tv_sec);

            gettimeofday(&tv1, NULL);
            for(int i = 0; i < N / numprocs + diff; i++) {
                result[i] = 0;
                for(int j = 0; j < N; j++) {
                    result[i] += rowVals[i][j]*vector[j];
                }
            }
            gettimeofday(&tv2, NULL);
            t_comp1 = (tv2.tv_usec - tv1.tv_usec) + 1000000 * (tv2.tv_sec - tv1.tv_sec);

            int eachProcs[numprocs];
            cnt = 0;
            for (int k = 0; k < numprocs; ++k) {
                if(k == 0)
                    eachProcs[k] = rows_proc + diff;
                else
                    eachProcs[k] = rows_proc;

                displays[k] = cnt;
                cnt += eachProcs[k];
            }

            t_comp_i[0] = t_comp1;

            gettimeofday(&tv1, NULL);
            MPI_Gatherv(result, eachProcs[rank], MPI_FLOAT, result2, eachProcs, displays, MPI_FLOAT, 0, MPI_COMM_WORLD);
            MPI_Gather(&t_comp_i, 1, MPI_INT, t_comp_i, 1, MPI_INT, 0, MPI_COMM_WORLD);
            MPI_Gather(&t_comm_i, 1, MPI_INT, t_comm_i, 1, MPI_INT, 0, MPI_COMM_WORLD);
            gettimeofday(&tv2, NULL);
            int t_comm2 = (tv2.tv_usec - tv1.tv_usec) + 1000000 * (tv2.tv_sec - tv1.tv_sec);
            TiempoComunicacion = t_comm1 + t_comm2;
            t_comm_i[0] = TiempoComunicacion;

        }

        if (DEBUG){
            for(i=0;i<N;i++) {
                printf("%f \n ",result2[i]);
            }
        } else {
            int computacionTotal = 0, comunicacionesTotal = 0;
            for (int k = 0; k <= numprocs; ++k) {
                if(k != numprocs){
                    printf("Rank: %d\t"
                           "Tiempo Comunicacion = %d µs\t"
                           "Tiempo Computacion = %d µs\n" , k, t_comm_i[k], t_comp_i[k]);
                    computacionTotal += t_comp_i[k];
                    comunicacionesTotal += t_comm_i[k];
                }else{
                    printf("Tiempo Comunicaciones Total = %d\n"
                           "Tiempo Computacion Total = %d\n" , comunicacionesTotal, computacionTotal);
                }

            }
        }

    } else {
        float matrix[N][N]; // esto facelo estatico sin reservar memoria, por esto -0.1

        float rowVals[N / numprocs][N];
        int send[numprocs], displays[numprocs];

        struct timeval tv1, tv2;
        float result[N / numprocs];
        float vector[N];

        gettimeofday(&tv1, NULL);
        MPI_Bcast(vector, N, MPI_FLOAT, 0, MPI_COMM_WORLD);
        MPI_Scatterv(matrix, send, displays, MPI_FLOAT, rowVals, (N / numprocs) * N, MPI_FLOAT, 0, MPI_COMM_WORLD);
        gettimeofday(&tv2, NULL);

        int tcomm1 = (tv2.tv_usec - tv1.tv_usec) + 1000000 * (tv2.tv_sec - tv1.tv_sec);


        gettimeofday(&tv1, NULL);
        for (int i = 0; i < N / numprocs; i++) {
            result[i] = 0;
            for (int j = 0; j < N; j++) {
                result[i] += rowVals[i][j] * vector[j];
            }
        }
        gettimeofday(&tv2, NULL);

        T_COMPUTACION = (tv2.tv_usec - tv1.tv_usec) + 1000000 * (tv2.tv_sec - tv1.tv_sec);

        int eachProc[numprocs], cnt = 0;
        for (int i = 0; i < numprocs; ++i) {
            eachProc[i] = N / numprocs;
            displays[i] = cnt;
            cnt += N / numprocs;
        }

        gettimeofday(&tv1, NULL);
        MPI_Gatherv(result, N / numprocs, MPI_FLOAT, result2, eachProc, displays, MPI_FLOAT, 0, MPI_COMM_WORLD);
        MPI_Gather(&T_COMPUTACION, 1, MPI_INT, t_comp_i, 1, MPI_INT, 0, MPI_COMM_WORLD);
        gettimeofday(&tv2, NULL);

        int tcomm2 = (tv2.tv_usec - tv1.tv_usec) + 1000000 * (tv2.tv_sec - tv1.tv_sec);

        int tcommTotal = tcomm1 + tcomm2;
        MPI_Gather(&tcommTotal, 1, MPI_INT, t_comm_i, 1, MPI_INT, 0, MPI_COMM_WORLD);


    }
    MPI_Finalize();
}