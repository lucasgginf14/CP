#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "compress.h"
#include "chunk_archive.h"
#include "queue.h"
#include "options.h"

#include <pthread.h>  //EJERCICIO 2


#define CHUNK_SIZE (1024*1024)
#define QUEUE_SIZE 20

#define COMPRESS 1
#define DECOMPRESS 0

struct cositas{
    int numero_fio;

    queue *in;
    queue *out;
};

struct fios_info{
    pthread_t id;
    struct cositas *cositas;
};

// take chunks from queue in, run them through process (compress or decompress), send them to queue out
void* worker(void* items) {///2º seccion critica
    struct cositas *cousas = items;
    chunk ch, res;
    while(q_elements(*cousas->in) > 0) {
       ch = q_remove(*cousas->in);

       res = zcompress(ch);
        free_chunk(ch);

        q_insert(*cousas->out, res);
    }
    return NULL;
}///2º seccion critica

// Compress file taking chunks of opt.size from the input file,
// inserting them into the in queue, running them using a worker,
// and sending the output from the out queue into the archive file
void comp(struct options opt) {
    int fd, chunks, i, offset;
    char comp_file[256];
    struct stat st;
    archive ar;
    queue in, out;
    chunk ch;

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

    in  = q_create(opt.queue_size);  //COLA DE ENTRADA
    out = q_create(opt.queue_size);  //COLA DE SALIDA

    // read input file and send chunks to the in queue  COMIENZA LA PARTE MAS IMPORTANTE    EJERCICIO 3
    for(i=0; i<chunks; i++) {
        ch = alloc_chunk(opt.size);

        offset=lseek(fd, 0, SEEK_CUR);

        ch->size   = read(fd, ch->data, opt.size);
        ch->num    = i;
        ch->offset = offset;

        q_insert(in, ch);
    }

    struct fios_info *fios;
    ///inicialización threads workers
    printf("Creando %d threads\n", opt.num_threads);
    fios = malloc(sizeof(struct fios_info) * opt.num_threads);

    if(fios == NULL) {
        printf("falta memoria\n");
        exit(1);
    }

    // compression of chunks from in to out

    for (int j = 0; j < opt.num_threads; ++j) {
        fios[j].cositas = malloc(sizeof (struct cositas));
        fios[j].cositas->numero_fio = j;
        fios[j].cositas->in = &in;
        fios[j].cositas->out = &out;
        if (0 != pthread_create(&fios[j].id, NULL, worker, fios[j].cositas)) {
            printf("Imposible crear o fio #%d", j);
            exit(1);
        }
    }

    for (int j = 0; j < opt.num_threads; ++j)///esperamos acabe compresion no?
        pthread_join(fios[j].id, NULL);

    ///liberar toda memoria usada y tal
    for (int j = 0; j < opt.num_threads; ++j)
        free(fios[j].cositas);

    // send chunks to the output archive file   EJERCICIO 3
    for(i=0; i<chunks; i++) {
        ch = q_remove(out);
        if(ch != NULL) {  //EJERCICIO 2
            add_chunk(ar, ch);
            free_chunk(ch);
        }
    }

    free(fios);
    close_archive_file(ar);
    close(fd);
    q_destroy(in);
    q_destroy(out);

    printf("Archivo comprimido con exito!\n");
}


// Decompress file taking chunks of size opt.size from the input file

void decomp(struct options opt) {   //NON HAI QUE TOCALO
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

