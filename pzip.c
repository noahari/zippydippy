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
int buf_size = 0;
int use = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t fill = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;

void *pparse(char *chunk){
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

void *consume(void *chunks){
    for(int i = 0; i < buf_size; i ++){
        int lock = pthread_mutex_lock(&mutex);
        if(lock){
            print("Lock error");
            return 1;
        }

        int my_use = use;
        if(use == buf_size)
            return 0;
        char *chunk = (char *)chunks + chunk_size*my_use;
        use++;
        pthread_cond_signal(&empty);
        pthread_mutex_unlock(&mutex);
        pparse(chunk);
    }
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

    // Map the file to address space
    void *contents = mmap(NULL, length, PROT_READ, NULL, fp, 0);
    fclose(fp);

    pthread_t rope[numproc];
    //have to def outside so we can join later on # of threads created,
    // not # of threads we SHOULD have created
    int iter;
    char **buf = (char **) malloc(length/chunk_size * sizeof(char *)); // CHECK MALLOC ERROR
    // Initial threads
    for(iter = 0; iter < numproc; iter++){
        if (pthread_create(&rope[iter], NULL, consume, contents)!= 0){
		    fprintf(stderr, "Failed to create thread number %d", iter);
		    break;
	    }
    }


    // THIS HERE IS PRODUCER CONSUMER SO LETS DO THAT YA WHOo



    // for (int i = 0; i < iter; i++){
    //         //not sure, but if not null for arg2 we could track the return
	//     //values of EACH thread by storing in an array? idk why we would
	//     //need to though for this project
	//     if (pthread_join(rope[i], NULL) != 0){
	// 	    fprintf(stderr, "Failed to join with thread number %d", i);
	// 	    break;
	//     }
    // }

    //parse(contents);
    // Free memory
    free(contents);
    return 0;
} 


