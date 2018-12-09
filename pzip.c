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

char *pparse(char *chunk){
    long count = 0;
    char run = chunk[0];
    for(int i = 0; i < chunk_size; i++){
        char cur = chunk[i];
        if (cur == run)
            count++;
        else
        {
            // fwrite(&count, sizeof(count), 1, stdout);
            // fwrite(&run, sizeof(char), 1, stdout);
            run = cur;
            count = 1;
        }
    }
    // fwrite(&count, sizeof(count), 1, stdout);
    // fwrite(&run, sizeof(char), 1, stdout);
    return 0;
}

void *consume(void *arg){
    for(int i = 0; i < num_chunks; i ++){
        if(pthread_mutex_lock(&mutex)){
            fprintf(stderr, "Lock error\n");
            return &err;
        }

        while(count == 0)
            if(pthread_cond_wait(&full, &mutex)){
                fprintf(stderr, " error\n");
                return &err;
            }

        if(use == num_chunks - 1)
            return &good; 

        char *chunk = chunks[use];
        use++;
        count--;
        if(pthread_cond_signal(&empty)){
                fprintf(stderr, " error\n");
                return &err;
            }
        if(pthread_mutex_unlock(&mutex)){
                fprintf(stderr, " error\n");
                return &err;
            }
        pparse(chunk);
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
    // // Map the file to address space
    // void *contents = mmap(NULL, length, PROT_READ, NULL, fp, 0);
    // fclose(fp);

    pthread_t rope[numproc];

    // Create buffer, calloc to initalize to zero
    chunks = (char **) malloc(num_chunks * sizeof(char *)); // CHECK MALLOC ERROR
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
                fprintf(stderr, " error\n");
                return 1;
            }
        while(count == num_chunks)
            if(pthread_cond_wait(&empty, &mutex)){
                fprintf(stderr, " error\n");
                return 1;
            }
        chunks[fill] = contents;
        fill++;
        count++;
        if(pthread_cond_signal(&full)){
                fprintf(stderr, " error\n");
                return 1;
            }
        if(pthread_mutex_unlock(&mutex)){
                fprintf(stderr, " error\n");
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


