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

int **chunks; // Buffer
int chunk_size = 1 << 12;
int err = 0;
int good = 0;
int fill = 0; // Next index to put in buffer
int use = 0; // Next index to get from buffer
int count = 0; // Size of buffer
int write = 0; // Which chunk are we writing
long num_chunks = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t print = PTHREAD_COND_INITIALIZER;

int *pparse(char *chunk){
    //printf("Processing %s\n", chunk);
    int *out = (int *) malloc(5 * strlen(chunk));
    if(!out){
       printf("Malloc error"); 
    }
    long my_count = 0;
    long my_fill = 0;
    char run = chunk[0];
    for(int i = 0; i < strlen(chunk); i++){
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
    //printf("We have processed\n");
    return out;
}

void *producer(void *arg){ 
    // MALLOC AN INT!
    while(num_chunks > 0){
        if(pthread_mutex_lock(&mutex)){
            fprintf(stderr, "Lock error\n");
            return &err;
        }
    //structy struct cast arg holds fp & i
    // WHILE COUNT == MAX ??? 
        struct dasien* order = (struct dasien*) arg; 
        int my_fill = fill;
        char *contents = (char *) mmap(NULL, chunk_size, PROT_READ, MAP_PRIVATE, fileno(order -> fp), fill*chunk_size);
        fill++;
        if(pthread_mutex_unlock(&mutex)){
            fprintf(stderr, "unlock error\n");
            return &err;
        }

        // Parse chunk (will happen in parallel)
        int *parsed = pparse(contents);

        if(pthread_mutex_lock(&mutex)){
            fprintf(stderr, "Lock error\n");
            return &err;
        }
        order->chunks[my_fill] = parsed;
        order->valid[use] = 1;
        chunks[my_fill] = parsed;
        count++;
        num_chunks--;
        if(pthread_cond_signal(&full)){
            fprintf(stderr, "cond signal error\n");
            return &err;
        }
        if(pthread_mutex_unlock(&mutex)){
            fprintf(stderr, "unlock error\n");
            return &err;
        }
    }
    return &good;
}

int consumer(struct dasien *order){
   while(use < num_chunks){
       // printf("Wait!\n");
        if(pthread_mutex_lock(&mutex)){
            fprintf(stderr, "Lock error\n");
            return 1;
        }
        while(count == 0 && !order->valid[use]){
            if(pthread_cond_wait(&full, &mutex)){
               fprintf(stderr, "cond wait error\n");
               return 1;
            }
        }
        int *chunk = order->chunks[use]; 
        use++;
        count--;
        fwrite(chunk, sizeof(int), 1, stdout);
        fwrite(chunk + 1, sizeof(char), 1, stdout);
        if(pthread_mutex_unlock(&mutex)){
            fprintf(stderr, "unlock error\n");
            return 1;
        } 
        free(chunk);
    }
    return 0;
}

typedef struct __dasien{
    int **chunks;
    int *valid;
 //   int err; DEAL WITH THIS
}; dasien

struct dasien *anxiety(int num_chunks){
    struct dasien *thing = malloc(sizeof(struct dasien));
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
    if(num_chunks < numproc){
        numproc = num_chunks;
    }
    printf("num chunks %ld\n", num_chunks);

    // Create buffer 
    chunks = (int **) malloc(num_chunks * sizeof(int *)); 
    if(!chunks){
        fprintf(stderr, "Malloc error");
        return 1;
    }

    // Create producers
    //have to def outside so we can join later on # of threads created,
    // not # of threads we SHOULD have created
    int iter;
    pthread_t rope[numproc];
    for(iter = 0; iter < numproc; iter++){
        if (pthread_create(&rope[iter], NULL, producer, (void *)fp)){
		    fprintf(stderr, "Failed to create thread number %d", iter);
		    break;
	    }
        else printf("Created thread %d succesfully\n", iter);
    }

    int consume = consumer();
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

    free(chunks);
    return 0;
} 


