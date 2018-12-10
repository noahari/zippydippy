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
    printf("Processing %s\n", chunk);
    int *out = (int *) malloc(strlen(chunk) * 5);
    if(!out){
       printf("Malloc error"); 
    }
    long my_count = 0;
    long my_fill = 0;
    char run = chunk[0];
    for(int i = 0; i < strlen(chunk); i++){
        char cur = chunk[i];
        if (cur == run)
            count++;
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
    out[my_fill + 2] = '\0'; 
    printf("We have processed\n");
    return out;
}

void *consume(void *arg){ 
    for(int i = 0; i < num_chunks; i ++){
        if(pthread_mutex_lock(&mutex)){
            fprintf(stderr, "Lock error\n");
            return &err;
        }

        while(count == 0){
            printf("Wait!\n");
            if(pthread_cond_wait(&full, &mutex)){
                fprintf(stderr, "cond wait error\n");
                return &err;
            }
        }

        printf("Done! Use %d\n",use);
        // We're done, no more to read from buffer
        if(use == num_chunks)
            return &good; 

        // Get chunk to work on
        char *chunk = chunks[use];
        printf("Got a chunk! %s\n", chunk);
        use++;
        count--;
        if(pthread_cond_signal(&empty)){
            fprintf(stderr, "cond signal error\n");
            return &err;
        }
        if(pthread_mutex_unlock(&mutex)){
            fprintf(stderr, "unlock error\n");
            return &err;
        }
        
        // Parse chunk (will happen in parallel)
        int *parsed = pparse(chunk);
        printf("Got parsed chunk");
        int *cur = parsed;


        if(pthread_mutex_lock(&m2)){
            fprintf(stderr, "m2 lock error\n");
            return &err;
        }
        while(use != write)
            if(pthread_cond_wait(&print, &m2)){
                fprintf(stderr, "print cond wait error\n");
                return &err;
            }

        // Coordinate ordering with condition variables
        while(*cur != '\0'){
            fwrite(cur, sizeof(int), 1, stdout);
            fwrite(cur + 1, sizeof(char), 1, stdout);
            cur += 2;
        }

        write++;

        if(pthread_cond_signal(&print)){
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

    num_chunks = ceil((double) length/chunk_size);
    printf("num chunks %ld\n", num_chunks);
    pthread_t rope[numproc];

    // Create buffer 
    chunks = (char **) malloc(num_chunks * sizeof(char *)); 
    if(!chunks){
        fprintf(stderr, "Malloc error");
        return 1;
    }

    //have to def outside so we can join later on # of threads created,
    // not # of threads we SHOULD have created
    int iter;
    // Create consumers
    for(iter = 0; iter < numproc; iter++){
        if (pthread_create(&rope[iter], NULL, consume, NULL)){
		    fprintf(stderr, "Failed to create thread number %d", iter);
		    break;
	    }
        else printf("Created thread %d succesfully\n", iter);
    }

    // "Producer"
    for(long i = 0; i < num_chunks; i++){
        // Do this w/o lock
        printf("Mapping iteration %ld\n", i);
        char *contents = (char *) mmap(NULL, chunk_size, PROT_READ, MAP_PRIVATE, fileno(fp), i*chunk_size);
        if(contents == MAP_FAILED){
            fprintf(stderr, "map error %s", strerror(errno));
            return 1;
        }
        printf("Mapped chunk %ld. Contents: %s\n", i, contents);
        if(pthread_mutex_lock(&mutex)){
                fprintf(stderr, " lock error\n");
                return 1;
            }
        while(count == num_chunks)
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
	    if (pthread_join(rope[i], NULL)){
		    fprintf(stderr, "Failed to join with thread number %d", i);
		    break;
	    }
    }

    free(chunks);
    return 0;
} 


