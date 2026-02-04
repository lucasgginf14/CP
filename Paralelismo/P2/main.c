#include <stdio.h>
#include <math.h>
#include </usr/include/x86_64-linux-gnu/mpi/mpi.h>

int MPI_FlattreeColectiva(void *buff , void *recvbuff , int count ,MPI_Datatype datatype , MPI_Op op , int root , MPI_Comm comm ){

    if(count < 0) return MPI_ERR_COUNT;
    if(buff == NULL) return MPI_ERR_BUFFER;
    if(comm != MPI_COMM_WORLD  || comm == MPI_COMM_NULL)  return MPI_ERR_COMM;

    int numprocs, rank;
    MPI_Comm_size ( comm , &numprocs);
    MPI_Comm_rank ( comm , &rank);

    if(datatype != MPI_DOUBLE) return MPI_ERR_TYPE;
    if(root < 0 || root >= numprocs) return MPI_ERR_ROOT; //comprobar se é mayor q o tamañano maximo doo comunicador - 1

    if(rank == 0){
        double recivido;
        double *sol = recvbuff;
        double *sum0 = buff;
        *sol += *sum0;
        for (int i = 1; i < numprocs; ++i) {
            MPI_Recv(&recivido, 1 , datatype, i , 0, MPI_COMM_WORLD ,MPI_STATUS_IGNORE);
            *sol += recivido;
        }
    }else{
        MPI_Send(buff , 1 , datatype , 0 , 0, MPI_COMM_WORLD);
    }
    return MPI_SUCCESS;
}

int MPI_BinomialBcast(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {

    // Verificaciones básicas
    if (comm != MPI_COMM_WORLD || comm == MPI_COMM_NULL)
        return MPI_ERR_COMM;
    int size, myrank;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &myrank);


    if (count < 0)
        return MPI_ERR_COUNT;
    if (buf == NULL)
        return MPI_ERR_BUFFER;
    if (root < 0 || root >= size)
        return MPI_ERR_ROOT;

    // Implementación de la función en sí

    double recivido;
    double* puntrecv = buf;

    int cnt = 0;
    for(int i = 1; i < size ; i *= 2){ //paso x ===> x + 2^x
        int destino = myrank + i;
        if(myrank < i){
            if(destino < size)
                MPI_Send(buf, count, datatype, destino, 1, comm);
        }else if(myrank < (i + pow(2, cnt))){
            MPI_Recv(&recivido, count, datatype, myrank - i, 1, comm, MPI_STATUS_IGNORE);
            *puntrecv = recivido;
        }
        cnt++;
    }
    return MPI_SUCCESS;
}

int main(int argc, char *argv[]){

    int numprocs, rank;
    double aux = 0;
    MPI_Init(&argc, &argv);

    MPI_Comm_size ( MPI_COMM_WORLD , &numprocs);
    MPI_Comm_rank ( MPI_COMM_WORLD , &rank);

    while(1) {
        aux = 0.0;
        if (rank != 0) {
            int n_rec;
            ///MPI_Bcast(&n_rec, 1, MPI_INT, 0, MPI_COMM_WORLD);
            MPI_BinomialBcast(&n_rec, 1, MPI_INT, 0 , MPI_COMM_WORLD);

            if(n_rec == 0) break;

            double h = 1.0 / (double) n_rec;
            double sum = 0.0;
            double x;
            for (int i = rank + 1; i <= n_rec; i += numprocs) {
                x = h * ((double) i - 0.5);
                sum += 4.0 / (1.0 + x * x);
            }

            MPI_FlattreeColectiva(&sum, &aux, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
        } else {

            int n;
            double pi, h, sum, x;

            printf("Enter the number of intervals: (0 quits) \n");
            scanf("%d", &n);

            ///MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
            MPI_BinomialBcast(&n , 1, MPI_INT, 0, MPI_COMM_WORLD);

            if (n == 0) break;  //para salir del bucle

            h = 1.0 / (double) n;
            sum = 0.0;

            for (int j = rank + 1; j <= n; j += numprocs) {
                x = h * ((double) j - 0.5);
                sum += 4.0 / (1.0 + x * x);
            }

            MPI_FlattreeColectiva(&sum, &aux, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

            double PI25DT = 3.141592653589793238462643;
            pi = h * aux;
            printf("pi is approximately %.16f, Error is %.16f\n", pi, fabs(pi - PI25DT));
        }
    }
    MPI_Finalize();
}