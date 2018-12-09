/*********************************************************
 * Your Name: Noah Ari 
 * Partner Name: Colby Morrison
 *********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <pthread.h>
#include <sys/mman.h>

int chunk_size = 2 << 12;
char **chunks;
int fill = 0;
int use = 0;
int count = 0;
long num_chunks = 0;
int err = 1;
int good = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;

int *pparse(char *chunk){
    int *out = (int *) malloc(chunk_size * 5);
    long count = 0;
    long fill = 0;
    char run = chunk[0];
    for(int i = 0; i < chunk_size; i++){
        char cur = chunk[i];
        if (cur == run)
            count++;
        else
        {
            out[fill] = count;
            out[fill + 1] = run;
            fill = fill + 2;
            // fwrite(&count, sizeof(count), 1, stdout);
            // fwrite(&run, sizeof(char), 1, stdout);
            run = cur;
            count = 1;
        }
    }
    out[fill] = count;
    out[fill + 1] = run;
    out[fill + 2] = '\0'; 
    // fwrite(&count, sizeof(count), 1, stdout);
    // fwrite(&run, sizeof(char), 1, stdout);
    return out;
}

void *consume(void *arg){
    for(int i = 0; i < num_chunks; i ++){
        if(pthread_mutex_lock(&mutex)){
            fprintf(stderr, "Lock error\n");
            return &err;
        }

        while(count == 0)
            if(pthread_cond_wait(&full, &mutex)){
                fprintf(stderr, "cond wait error\n");
                return &err;
            }

        if(use == num_chunks - 1)
            return &good; 

        char *chunk = chunks[use];
        use++;
        count--;
        if(pthread_cond_signal(&empty)){
                fprintf(stderr, "cond signal error\n");
                return &err;
            }
        if(pthread_mutex_unlock(&mutex)){
                fprintf(stderr, "cond unlock error\n");
                return &err;
            }
        int *parsed = pparse(chunk);
        int *cur = parsed;

        // Coordinate ordering with condition variables
        while(*cur != '\\0'){
            fwrite(*cur, sizeof(int), 1, stdout);
            cur++;
            fwrite(*cur, sizeof(char), 1, stdout);
            cur++;
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

    num_chunks = length/chunk_size;
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
    }

    // "Producer"
    for(long i = 0; i < num_chunks; i++){
        // Do this w/o lock
        char *contents = (char *) mmap(NULL, chunk_size, PROT_READ, MAP_FILE, fileno(fp), i*chunk_size);
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


