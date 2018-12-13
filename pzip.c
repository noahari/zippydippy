/*********************************************************
 * Your Name: Noah Ari 
 * Partner Name: Colby Morrison
 *********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/sysinfo.h>
#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>

// Struct to hold shared information 
typedef struct __dasein{
    // Array of pointers to each zipped
    int **chunks;
    // Bitmap, if ith entry is a 1, the ith entry in "chunks" is a valid zipped chunk
    int *valid;
    void *fp;
    long num_chunks;
    int chunk_size;
    int fill;
    int use;
    pthread_mutex_t mutex;
    pthread_cond_t full; 
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

    thing->fill = 0;
    thing->use = 0;
    pthread_mutex_init(&thing->mutex, NULL);
    pthread_cond_init(&thing->full, NULL);

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
    //Create struct for parsed data storage and maintaining order
    dasein* order = (dasein*) arg; 
    //loop, we return inside for all paths
    while(1){
        //LOCK
        if(pthread_mutex_lock(&(order->mutex))){
            fprintf(stderr, "Lock error\n");
            return NULL; 
        }
        //check if we are out of chunks, if so return
        if(order->fill >= order->num_chunks){
            //UNLOCK
            if(pthread_mutex_unlock(&(order->mutex))){
                fprintf(stderr, "unlock error\n");
                return NULL;
            }
            return NULL;
        }
        //
        int my_fill = order->fill;
        order->fill++;
        //UNLOCK
        if(pthread_mutex_unlock(&(order->mutex))){
            fprintf(stderr, "unlock error\n");
            return NULL;
        }
        char *contents = (char *) mmap(NULL, order->chunk_size, PROT_READ, MAP_PRIVATE, fileno(order -> fp), my_fill*order->chunk_size);
        //store the chunk size separately in order to check the case
        //the chunk is smaller, e.g. the chunk at the end of a 2 chunk
        //4.1 KB file
        long chunkBound = order->chunk_size;
        if (my_fill == order->num_chunks - 1)
            chunkBound = strlen(contents);

        // Parse chunk (will happen in parallel)
        int *parsed = pparse(contents, chunkBound);

        //LOCK
        if(pthread_mutex_lock(&(order->mutex))){
            fprintf(stderr, "Lock error\n");
            return NULL;
        }
        //Store data value, and note that this chunk is now valid
        order->chunks[my_fill] = parsed;
        order->valid[my_fill] = 1;
        //signals the consumer that something valid was stored in the
        //buffer of the struct to print
        if(pthread_cond_signal(&(order->full))){
            fprintf(stderr, "cond signal error\n");
            return NULL;
        }
        //UNLOCK
        if(pthread_mutex_unlock(&(order->mutex))){
            fprintf(stderr, "unlock error\n");
            return NULL;
        }
    }
    return NULL;
}

int consumer(dasein *order){
   //loops over global variable use so we print in order
   //and we only try to print equal to the number of
   //chunks that are actually there
   while(order->use < order->num_chunks){
        int iter = 0;
        //LOCK
        if(pthread_mutex_lock(&(order->mutex))){
            fprintf(stderr, "Lock error\n");
            return 1;
        }
        //If we are active or signalled and are not ready to print the
        //correct chunk in terms of order, we spin
        while(!order->valid[order->use]){
            if(pthread_cond_wait(&(order->full), &(order->mutex))){
               fprintf(stderr, "cond wait error\n");
               return 1;
            }
        }
        //save what chunk we are on and then increment the use var
        //so that when we loop over the first while we will be checking
        //for the correct chunk to print in order
        int *chunk = order->chunks[order->use]; 
        order->use++;
        //loop over the chunk, printing until we hit the boundary
        //we defined as a value of -1
        while(chunk[iter] != -1){
            fwrite(&chunk[iter], sizeof(int), 1, stdout);
            fwrite(&chunk[iter + 1], sizeof(char), 1, stdout);
            iter += 2;
        }
        //UNLOCK
        if(pthread_mutex_unlock(&(order->mutex))){
            fprintf(stderr, "unlock error\n");
            return 1;
        } 
        //Free the chunk value we saved
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

    // Number of chunks must be a multiple of page size for mmap to work
    long chunk_size = sysconf(_SC_PAGESIZE);
    long num_chunks = ceil((double) length/chunk_size);
    if(num_chunks < numproc){
        numproc = num_chunks;
    }

    // Create struct and initialize
    dasein *chunkster = anxiety(num_chunks);
    chunkster->num_chunks = num_chunks;
    chunkster->chunk_size = chunk_size;
    chunkster->fp = fp;
    // Create a thread for each processor each running producer
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

    //Calls consume on the struct so the main thread can print
    int consume = consumer((void *)chunkster);
    //error of failed print
    if(consume){
        fprintf(stderr, "Failed Print");
        return 1;
    }
    //loops over the rope waiting for all threads to finish before 
    //freeing data that threads may use and returning
    //free memory of the struct and the individually malloc'd fields
    free(chunkster->chunks);
    free(chunkster->valid);
    free(chunkster);
    return 0;
} 


