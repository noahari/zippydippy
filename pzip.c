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
#include <unistd.h>

int chunk_size;
int fill = 0; // Next index to put in buffer
int use = 0; // Next index to get from buffer
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t print = PTHREAD_COND_INITIALIZER;

int *pparse(char *chunk){
    //printf("Strlen %ld, first char %c\n", strlen(chunk), chunk[0]);
    long chunkBound = chunk_size;
    if (strlen(chunk) < chunk_size)
        chunkBound = strlen(chunk);
    int *out = (int *) malloc(5 * chunkBound);
    if(!out){
       printf("Malloc error"); 
    }
    long my_count = 0;
    long my_fill = 0;
    char run = chunk[0];

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
    munmap(chunk, chunkBound);
   // printf("We have processed, first char: %c\n", chunk[0]);
    return out;
}


typedef struct __dasein{
    int **chunks;
    int *valid;
    void *fp;
    long num_chunks;
    int err;
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
    dasein* order = (dasein*) arg; 
    while(1){
        if(pthread_mutex_lock(&mutex)){
            fprintf(stderr, "Lock error\n");
            order->err = 1;
            return NULL; 
        }
        if(fill >= order->num_chunks){
            if(pthread_mutex_unlock(&mutex)){
                fprintf(stderr, "unlock error\n");
                order->err = 1;
                return NULL;
            }
            order->err = 0;
            return NULL;
        }
        int my_fill = fill;
       // printf("My fill %d\n", my_fill);
        char *contents = (char *) mmap(NULL, chunk_size, PROT_READ, MAP_PRIVATE, fileno(order -> fp), my_fill*chunk_size);
        fill++;
        if(pthread_mutex_unlock(&mutex)){
            fprintf(stderr, "unlock error\n");
            order->err = 1;
            return NULL;
        }

        // Parse chunk (will happen in parallel)
        int *parsed = pparse(contents);

        if(pthread_mutex_lock(&mutex)){
            fprintf(stderr, "Lock error\n");
            order->err = 1;
            return NULL;
        }
        order->chunks[my_fill] = parsed;
        order->valid[my_fill] = 1;
        if(pthread_cond_signal(&full)){
            fprintf(stderr, "cond signal error\n");
            order->err = 1;
            return NULL;
        }
        if(pthread_mutex_unlock(&mutex)){
            fprintf(stderr, "unlock error\n");
            order->err = 1;
            return NULL;
        }
        //printf("outlock 2 looping, num_chunks: %ld\n", num_chunks);
    }
    //printf("REturning!\n");
    order->err = 0;
    return NULL;
}

int consumer(dasein *order){
   while(use < order->num_chunks){
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
        }
        int *chunk = order->chunks[use]; 
        int *chunk_start = chunk;
        use++;
       // printf("Printing chunk %d\n", use);
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

    // Number of chunks must multiple of page size for mmap to work
    chunk_size = sysconf(_SC_PAGESIZE);
    long num_chunks = ceil((double) length/chunk_size);
    if(num_chunks < numproc){
        numproc = num_chunks;
    }
    //printf("num chunks %ld\n", num_chunks);

    // Create struct
    dasein *chunkster = anxiety(num_chunks);
    chunkster->num_chunks = num_chunks;
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
        if (chunkster->err){
            fprintf(stderr, "Producer error\n");
        }
	    if (ret){
		    fprintf(stderr, "Failed to join with thread number %d\n", i);
            fprintf(stderr, "Error code: %s\n", strerror(ret));
		    break;
	    }
    }
    free(chunkster->chunks);
    free(chunkster->valid);
    free(chunkster);
    return 0;
} 


