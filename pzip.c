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
pthread_cond_t full = PTHREAD_COND_INITIALIZER;

// Struct to hold information about zipped chunks
typedef struct __dasein{
    // Array of pointers to each zipped
    int **chunks;
    // Bitmap, if ith entry is a 1, the ith entry in "chunks" is a valid zipped chunk
    int *valid;
    void *fp;
    long num_chunks;
} dasein;

// Initialization for struct
dasein *anxiety(int num_chunks){
    dasein *thing = malloc(sizeof(dasein));
    if(!thing){
        fprintf(stderr, "MALLOC ERROR: struct");
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

// Zips a chunk of size chunkBound
int *pparse(char *chunk, int chunkBound){
    // Malloc the worst case scenario
    int *out = (int *) malloc((5 * chunkBound) + 1);
    if(!out){
       printf("Malloc error"); 
    }
    long my_count = 0;
    long my_fill = 0;
    char run = chunk[0];
    // Find runs and put int-char pair into 'out' for each run 
    for(long i = 0; i < chunkBound; i++){
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
    // Add a -1 at the end to signal end of zipped section to consumer
    out[my_fill + 2] = -1; 
    munmap(chunk, chunkBound);
    return out;
}


void *producer(void *arg){ 
    dasein* order = (dasein*) arg; 
    while(1){
        if(pthread_mutex_lock(&mutex)){
            fprintf(stderr, "Lock error\n");
            return NULL; 
        }
        if(fill >= order->num_chunks){
            if(pthread_mutex_unlock(&mutex)){
                fprintf(stderr, "unlock error\n");
                return NULL;
            }
            return NULL;
        }
        int my_fill = fill;
        char *contents = (char *) mmap(NULL, chunk_size, PROT_READ, MAP_PRIVATE, fileno(order -> fp), my_fill*chunk_size);
        fill++;
        if(pthread_mutex_unlock(&mutex)){
            fprintf(stderr, "unlock error\n");
            return NULL;
        }
        long chunkBound = chunk_size;
        if (my_fill == order->num_chunks - 1)
            chunkBound = strlen(contents);

        // Parse chunk (will happen in parallel)
        int *parsed = pparse(contents, chunkBound);

        if(pthread_mutex_lock(&mutex)){
            fprintf(stderr, "Lock error\n");
            return NULL;
        }
        order->chunks[my_fill] = parsed;
        order->valid[my_fill] = 1;
        if(pthread_cond_signal(&full)){
            fprintf(stderr, "cond signal error\n");
            return NULL;
        }
        if(pthread_mutex_unlock(&mutex)){
            fprintf(stderr, "unlock error\n");
            return NULL;
        }
    }
    return NULL;
}

int consumer(dasein *order){
   while(use < order->num_chunks){
        if(pthread_mutex_lock(&mutex)){
            fprintf(stderr, "Lock error\n");
            return 1;
        }
        while(!order->valid[use]){
            if(pthread_cond_wait(&full, &mutex)){
               fprintf(stderr, "cond wait error\n");
               return 1;
            }
        }
        int *chunk = order->chunks[use]; 
        use++;
        int iter = 0;
        while(chunk[iter] != -1){
            fwrite(&chunk[iter], sizeof(int), 1, stdout);
            fwrite(&chunk[iter + 1], sizeof(char), 1, stdout);
            iter += 2;
        }
        if(pthread_mutex_unlock(&mutex)){
            fprintf(stderr, "unlock error\n");
            return 1;
        } 
        free(chunk);
    }
    return 0;
}


int main(int argc, char *argv[])
{
    //get_nprocs_conf = how many procs configured, nonconf is available
    long numproc = get_nprocs();
    FILE *fp = fopen(argv[1], "r");
    if(!fp){
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
    }

    int consume = consumer((void *)chunkster);
    if(consume)
        return 1;
  
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
    free(chunkster->chunks);
    free(chunkster->valid);
    free(chunkster);
    return 0;
} 


