#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <sys/mman.h>
#include "compress.h"
#include "chunk_archive.h"
#include "queue.h"
#include "options.h"

#define CHUNK_SIZE (1024*1024)
#define QUEUE_SIZE 20

#define COMPRESS 1
#define DECOMPRESS 0

struct chunks_in{
    int i;
    int fd;
    int offset;
    int chunks;
    struct options opt;
    queue *in;
};

struct chunks_out{
    int i;
    int chunks;
    queue *out;
    archive ar;
};

struct cositas{
    int numero_fio;
    int *cnt;
    pthread_mutex_t *mutexCnt;
    queue *in;
    queue *out;
};

struct fios_info{
    pthread_t id;
    struct cositas *cositas;
};

struct fios_info_ch_in{
    pthread_t id;
    struct chunks_in *cositas;
};

struct fios_info_ch_out{
    pthread_t id;
    struct chunks_out *cositas;
};

// take chunks from queue in, run them through process (compress or decompress), send them to queue out
void* worker(void* items) {///2º seccion critica
    struct cositas *cousas = items;
    int cnt;
    chunk ch, res;
    while(1) {
        pthread_mutex_lock(cousas->mutexCnt);
            if(*cousas->cnt > 0){
            cnt = *cousas->cnt;
            cnt--;
            *cousas->cnt = cnt;
            pthread_mutex_unlock(cousas->mutexCnt);

            ch = q_remove(*cousas->in);

            res = zcompress(ch);
            free_chunk(ch);

            q_insert(*cousas->out, res);
        }else{
            pthread_mutex_unlock(cousas->mutexCnt);
            break;
        }
    }
    return NULL;
}///2º seccion critica

// read input file and send chunks to the in queue
void* read_chunks(void* items){
    struct chunks_in *ch_in = items;
    chunk ch;
    for(ch_in->i=0; ch_in->i < ch_in->chunks; ch_in->i++) {///1º seccion critica
        ch = alloc_chunk(ch_in->opt.size);

        ch_in->offset=lseek(ch_in->fd, 0, SEEK_CUR);

        ch->size = read(ch_in->fd, ch->data, ch_in->opt.size);
        ch->num    = ch_in->i;
        ch->offset = ch_in->offset;

        q_insert(*ch_in->in, ch);
    }///1º seccion critica
    return NULL;
}

// send chunks to the output archive file
void* send_chunks(void* items){
    struct chunks_out *ch_out = items;
    chunk ch;
    for(ch_out->i=0; ch_out->i < ch_out->chunks; ch_out->i++) {///3º seccion critica
        ch = q_remove(*ch_out->out);
        add_chunk(ch_out->ar, ch);
        free_chunk(ch);
    }///3º seccion critica
    return NULL;
}

// Compress file taking chunks of opt.size from the input file,
// inserting them into the in queue, running them using a worker,
// and sending the output from the out queue into the archive file
void comp(struct options opt) {
    int fd, chunks;
    int i = 0;
    int offset = 0;
    char comp_file[256];
    struct stat st;
    archive ar;
    queue in, out;

    if((fd=open(opt.file, O_RDONLY))==-1) {
        printf("Cannot open %s\n", opt.file);
        exit(0);
    }

    fstat(fd, &st);
    chunks = st.st_size/opt.size+(st.st_size % opt.size ? 1:0);

    if(opt.out_file) {
        strncpy(comp_file,opt.out_file,255);
    } else {
        strncpy(comp_file, opt.file, 255);
        strncat(comp_file, ".ch", 255);
    }

    ar = create_archive_file(comp_file);

    in  = q_create(opt.queue_size);
    out = q_create(opt.queue_size);

    pthread_mutex_t *mutexCnt;
    mutexCnt = malloc(sizeof (pthread_mutex_t));
    pthread_mutex_init(mutexCnt , NULL);

    struct fios_info *fios;
    struct fios_info_ch_in *fios_ch_in;
    struct fios_info_ch_out *fios_ch_out;

    printf("Creando thread chunks lectura\n");
    fios_ch_in = malloc(sizeof(struct fios_info_ch_in));

    if(fios_ch_in == NULL){
        printf("falta memoria\n");
        exit(1);
    }

    printf("Leyendo archivo...\n");
    fios_ch_in->cositas = malloc(sizeof(struct chunks_in));
    fios_ch_in->cositas->i = i;
    fios_ch_in->cositas->opt = opt;
    fios_ch_in->cositas->fd = fd;
    fios_ch_in->cositas->offset = offset;
    fios_ch_in->cositas->chunks = chunks;
    fios_ch_in->cositas->in = &in;
    if (0 != pthread_create(&fios_ch_in->id, NULL, read_chunks, fios_ch_in->cositas)) {
        printf("Imposible crear o fio de lectura de chunks\n");
        exit(1);
    }

    ///inicialización threads workers
    printf("Creando %d threads\n" , opt.num_threads);
    fios = malloc(sizeof (struct fios_info) * opt.num_threads);

    if(fios == NULL){
         printf("falta memoria\n");
         exit(1);
     }

    int *shr;
    shr = mmap(NULL , sizeof (int) , PROT_READ | PROT_WRITE , MAP_ANONYMOUS | MAP_SHARED , 0,0);
    *shr = chunks;

    // compression of chunks from in to out
    printf("Comprimiendo archivo...\n");
    for (int j = 0; j < opt.num_threads; ++j) {
        fios[j].cositas = malloc(sizeof (struct cositas));
        fios[j].cositas->numero_fio = j;
        fios[j].cositas->cnt = shr;
        fios[j].cositas->mutexCnt = mutexCnt;
        fios[j].cositas->in = &in;
        fios[j].cositas->out = &out;
        if (0 != pthread_create(&fios[j].id, NULL, worker, fios[j].cositas)) {
            printf("Imposible crear o fio #%d", j);
            exit(1);
        }
    }

    printf("Creando thread chunks escritura\n");

    fios_ch_out = malloc(sizeof(struct fios_info_ch_out));

    if(fios_ch_out == NULL){
        printf("falta memoria\n");
        exit(1);
    }

    printf("Escribiendo archivo compirmido...\n");
    fios_ch_out->cositas = malloc(sizeof(struct chunks_out));
    fios_ch_out->cositas->i = i;
    fios_ch_out->cositas->chunks = chunks;
    fios_ch_out->cositas->ar = ar;
    fios_ch_out->cositas->out = &out;
    if (0 != pthread_create(&fios_ch_out->id, NULL, send_chunks, fios_ch_out->cositas)) {
        printf("Imposible crear o fio de escritura de chunks\n");
        exit(1);
    }

    pthread_join(fios_ch_in->id, NULL);///esperamos termine la lectura no?

    for (int j = 0; j < opt.num_threads; ++j)///esperamos acabe compresion no?
        pthread_join(fios[j].id, NULL);

    pthread_join(fios_ch_out->id, NULL);///esperamos termine la escritura no?

    ///liberar toda memoria usada y tal
    for (int j = 0; j < opt.num_threads; ++j)
        free(fios[j].cositas);

    free(fios);
    free(fios_ch_in->cositas);
    free(fios_ch_in);
    free(fios_ch_out->cositas);
    free(fios_ch_out);

    close_archive_file(ar);
    close(fd);
    q_destroy(in);
    q_destroy(out);

    printf("Archivo comprimido con exito!\n");
}


// Decompress file taking chunks of size opt.size from the input file

void decomp(struct options opt) {
    int fd, i;
    char uncomp_file[256];
    archive ar;
    chunk ch, res;

    if((ar=open_archive_file(opt.file))==NULL) {
        printf("Cannot open archive file\n");
        exit(0);
    }

    if(opt.out_file) {
        strncpy(uncomp_file, opt.out_file, 255);
    } else {
        strncpy(uncomp_file, opt.file, strlen(opt.file) -3);
        uncomp_file[strlen(opt.file)-3] = '\0';
    }

    if((fd=open(uncomp_file, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH))== -1) {
        printf("Cannot create %s: %s\n", uncomp_file, strerror(errno));
        exit(0);
    }

    for(i=0; i<chunks(ar); i++) {
        ch = get_chunk(ar, i);

        res = zdecompress(ch);
        free_chunk(ch);

        lseek(fd, res->offset, SEEK_SET);
        write(fd, res->data, res->size);
        free_chunk(res);
    }

    close_archive_file(ar);
    close(fd);
}

int main(int argc, char *argv[]) {
    struct options opt;

    opt.compress    = COMPRESS;
    opt.num_threads = 3;
    opt.size        = CHUNK_SIZE;
    opt.queue_size  = QUEUE_SIZE;
    opt.out_file    = NULL;

    read_options(argc, argv, &opt);

    if(opt.compress == COMPRESS) comp(opt);
    else decomp(opt);
}
