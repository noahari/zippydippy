/*********************************************************
 * Your Name: Noah Ari 
 * Partner Name: Colby Morrison
 *********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <sys/sysinfo.h>
#include <pthread.h>
#include <sys/mman.h>

int chunk_size = 1 << 12;
int err = 0;
int good = 0;
int fill = 0; // Next index to put in buffer
int use = 0; // Next index to get from buffer
int count = 0; // Size of buffer
int write = 0; // Which chunk are we writing
long num_chunks = 0;
long num_chunks_cp = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t print = PTHREAD_COND_INITIALIZER;

int *pparse(char *chunk){
    //printf("Strlen %ld, first char %c\n", strlen(chunk), chunk[0]);
    int *out = (int *) malloc(5 * strlen(chunk));
    if(!out){
       printf("Malloc error"); 
    }
    long my_count = 0;
    long my_fill = 0;
    char run = chunk[0];
    long chunkBound = chunk_size;
    if (strlen(chunk) < chunk_size)
        chunkBound = strlen(chunk);
    for(long i = 0; i < chunkBound; i++){
        // printf("Starting loop %ld, first char %c\n", i, chunk[0]);
        char cur = chunk[i];
        if (cur == run)
            my_count++;
        else{
            out[my_fill] = my_count;
            out[my_fill + 1] = run;
            my_fill += 2;
            run = cur;
            my_count = 1;
        }
    }
    out[my_fill] = my_count;
    out[my_fill + 1] = run;
    out[my_fill + 2] = -1; 
   // printf("We have processed, first char: %c\n", chunk[0]);
    return out;
}


typedef struct __dasein{
    int **chunks;
    int *valid;
    void *fp;
    //   int err; DEAL WITH THIS
} dasein;

//this seems highly redundant, why dont we just define a struct then malloc(sizeof(struct))?
dasein *anxiety(int num_chunks){
    dasein *thing = malloc(sizeof(dasein));
    if(!thing){
        fprintf(stderr, "MALLOC ERROR: death");
        return NULL;
    }

    thing->chunks = malloc(num_chunks * sizeof(int *));
    if(!thing->chunks){
        fprintf(stderr, "MALLOC ERROR: chunks");
        return NULL;
    }

    thing->valid = calloc(num_chunks, sizeof(int));
    if(!thing->valid){
        fprintf(stderr, "MALLOC ERROR: invalid");
        return NULL;
    }

    return thing;
}


void *producer(void *arg){ 
    // MALLOC AN INT!
    while(1){
        if(pthread_mutex_lock(&mutex)){
            fprintf(stderr, "Lock error\n");
            return &err;
        }
        if(fill >= num_chunks){
            if(pthread_mutex_unlock(&mutex)){
                fprintf(stderr, "unlock error\n");
                return &err;
            }
            return &good;
        }
    //structy struct cast arg holds fp & i
    // WHILE COUNT == MAX ??? 
        dasein* order = (dasein*) arg; 
        int my_fill = fill;
       // printf("My fill %d\n", my_fill);
        char *contents = (char *) mmap(NULL, chunk_size, PROT_READ, MAP_PRIVATE, fileno(order -> fp), my_fill*chunk_size);
        fill++;
        //num_chunks--;
        if(pthread_mutex_unlock(&mutex)){
            fprintf(stderr, "unlock error\n");
            return &err;
        }

        // Parse chunk (will happen in parallel)
        int *parsed = pparse(contents);
        //printf("Got parsed: %p. Fill: %d\n", parsed, my_fill);

        if(pthread_mutex_lock(&mutex)){
            fprintf(stderr, "Lock error\n");
            return &err;
        }
        order->chunks[my_fill] = parsed;
        order->valid[my_fill] = 1;
        count++;
        if(pthread_cond_signal(&full)){
            fprintf(stderr, "cond signal error\n");
            return &err;
        }
        if(pthread_mutex_unlock(&mutex)){
            fprintf(stderr, "unlock error\n");
            return &err;
        }
        //printf("outlock 2 looping, num_chunks: %ld\n", num_chunks);
    }
    //printf("REturning!\n");
    return &good;
}

int consumer(dasein *order){
   while(use < num_chunks_cp){
       // printf("Wait!\n");
        if(pthread_mutex_lock(&mutex)){
            fprintf(stderr, "Lock error\n");
            return 1;
        }
        while(!order->valid[use]){
            //printf("Waiting. Use %d\n", use);
            if(pthread_cond_wait(&full, &mutex)){
               fprintf(stderr, "cond wait error\n");
               return 1;
            }
            //printf("Signaled. count: %d, use: %d, valid use: %d \n", count, use, order->valid[use]);
        }
        int *chunk = order->chunks[use]; 
        int *chunk_start = chunk;
        use++;
        count--;
        //printf("Printing chunk %d\n", use);
        while(*chunk != -1){
            fwrite(chunk, sizeof(int), 1, stdout);
            fwrite(chunk + 1, sizeof(char), 1, stdout);
            chunk += 2;
        }
        if(pthread_mutex_unlock(&mutex)){
            fprintf(stderr, "unlock error\n");
            return 1;
        } 
        free(chunk_start);
    }
    return 0;
}


int main(int argc, char *argv[])
{
    //get_nprocs_conf = how many procs configured, nonconf is available
    long numproc = get_nprocs();
    FILE *fp = fopen(argv[1], "r");
    if(!fp){
        fclose(fp);
        return 1;
    }
    // How long is the file?
    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Number of chunks
    num_chunks = ceil((double) length/chunk_size);
    num_chunks_cp = num_chunks;
    if(num_chunks < numproc){
        numproc = num_chunks;
    }
    //printf("num chunks %ld\n", num_chunks);

    // Create struct
    dasein *chunkster = anxiety(num_chunks);
    chunkster->fp = fp;
    // Create producers
    //have to def outside so we can join later on # of threads created,
    // not # of threads we SHOULD have created
    int iter;
    pthread_t rope[numproc];
    for(iter = 0; iter < numproc; iter++){
        if (pthread_create(&rope[iter], NULL, producer, (void *)chunkster)){
		    fprintf(stderr, "Failed to create thread number %d", iter);
		    break;
	    }
        //else printf("Created thread %d succesfully\n", iter);
    }

    int consume = consumer((void *)chunkster);
    if(consume)
        return 1;
  
    // printf("Waiting to join\n");

    for (int i = 0; i < iter; i++){
            //not sure, but if not null for arg2 we could track the return
	    //values of EACH thread by storing in an array? idk why we would
	    //need to though for this project
        int ret = pthread_join(rope[i], NULL);
	    if (ret){
		    fprintf(stderr, "Failed to join with thread number %d\n", i);
            fprintf(stderr, "Error code: %s\n", strerror(ret));
		    break;
	    }
    }

    free(chunkster);
    return 0;
} 


