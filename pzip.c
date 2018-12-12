/*********************************************************
 * Your Name: Noah Ari 
 * Partner Name: Colby Morrison
 *********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
// #include <sys/sysinfo.h>
#include <pthread.h>
#include <sys/mman.h>

char **chunks; // Buffer
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

void *consume(void *arg){ 
    while(num_chunks > 0){
        if(pthread_mutex_lock(&mutex)){
            fprintf(stderr, "Lock error\n");
            return &err;
        }

        int my_use = use;

        while(count == 0){
            // printf("Wait!\n");
            if(pthread_cond_wait(&full, &mutex)){
                fprintf(stderr, "cond wait error\n");
                return &err;
            }
        }

        // printf("Done! Use %d\n",my_use);
        // We're done, no more to read from buffer
        // if(my_use == num_chunks)
        //     return &good; 

        // Get chunk to work on
        char *chunk = chunks[use];
        // printf("Got a chunk! %s\n", chunk);
        use++;
        count--;
        num_chunks--;
        printf("My use: %d, use %d\n", my_use, use); 
        if(pthread_cond_signal(&empty)){
            fprintf(stderr, "cond signal error\n");
            return &err;
        }
        if(pthread_mutex_unlock(&mutex)){
            fprintf(stderr, "unlock error\n");
            return &err;
        }

        // printf("%d\n", write);
        
        // Parse chunk (will happen in parallel)
        int *parsed = pparse(chunk);
        // printf("Got parsed chunk\n");
        int *cur = parsed;

        // for(int i = 0; i < strlen(chunk); i++){
        //     // printf("%d\n",cur[i]);
        // }


        if(pthread_mutex_lock(&m2)){
            fprintf(stderr, "m2 lock error\n");
            return &err;
        }

        // printf("Acquired lock 2\n");

        while(my_use != write){
            // printf("Waiting!\n");
            if(pthread_cond_wait(&print, &m2)){
                fprintf(stderr, "print cond wait error\n");
                return &err;
            }
        }

        // Coordinate ordering with condition variables
        while(*cur != -1){
            // printf("Writing int %d\n", *cur);
            // printf("Writing char %c\n", *(cur+1));
            fwrite(cur, sizeof(int), 1, stdout);
            fwrite(cur + 1, sizeof(char), 1, stdout);
            cur = cur + 2;
        }

        write++;

        if(pthread_cond_broadcast(&print)){
            fprintf(stderr, "print cond signal error\n");
            return &err;
       }

        if(pthread_mutex_unlock(&m2)){
                fprintf(stderr, "unlock error\n");
                return &err;
        }
        free(parsed);
    }
    return &good;
}

int main(int argc, char *argv[])
{
    //get_nprocs_conf = how many procs configured, nonconf is available
    // long numproc = get_nprocs();
    long numproc = 2;
    FILE *fp = fopen(argv[1], "r");
    if(!fp){
        fclose(fp);
        return 1;
    }
    // How long is the file?
    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    num_chunks = ceil((double) length/chunk_size);
    if(num_chunks < numproc){
        numproc = num_chunks;
    }
    printf("num chunks %ld\n", num_chunks);


    // Create buffer 
    chunks = (char **) malloc(num_chunks * sizeof(char *)); 
    if(!chunks){
        fprintf(stderr, "Malloc error");
        return 1;
    }

    // Create consumers
    //have to def outside so we can join later on # of threads created,
    // not # of threads we SHOULD have created
    int iter;
    pthread_t rope[numproc];
    for(iter = 0; iter < numproc; iter++){
        if (pthread_create(&rope[iter], NULL, consume, NULL)){
		    fprintf(stderr, "Failed to create thread number %d", iter);
		    break;
	    }
        // else printf("Created thread %d succesfully\n", iter);
    }

    // "Producer"
    for(long i = 0; i < num_chunks; i++){
        // Do this w/o lock
        // printf("Mapping iteration %ld\n", i);
        char *contents = (char *) mmap(NULL, chunk_size, PROT_READ, MAP_PRIVATE, fileno(fp), i*chunk_size);
        if(contents == MAP_FAILED){
            fprintf(stderr, "map error %s", strerror(errno));
            return 1;
        }
        // printf("Mapped chunk %ld. Contents: %s\n", i, contents);
        if(pthread_mutex_lock(&mutex)){
                fprintf(stderr, " lock error\n");
                return 1;
            }
        while(count == num_chunks) // Is this superflouous?? 
            if(pthread_cond_wait(&empty, &mutex)){
                fprintf(stderr, "cond wait error\n");
                return 1;
            }
        chunks[fill] = contents;
        fill++;
        count++;
        if(pthread_cond_signal(&full)){
                fprintf(stderr, "cond signal error\n");
                return 1;
            }
        if(pthread_mutex_unlock(&mutex)){
                fprintf(stderr, "unlock error\n");
                return 1;
            }
    }
    printf("Waiting to join\n");

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


