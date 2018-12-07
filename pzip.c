/*********************************************************
 * Your Name: Noah Ari 
 * Partner Name: Colby Morrison
 *********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <pthread.h>

int parse(char *in)
{
    char run = in[0];
    long count = 0;
    for (int i = 0; i < strlen(in); i++)
    {
        char cur = in[i];
        if (cur == run)
            count++;
        else
        {
            fwrite(&count, sizeof(count), 1, stdout);
            fwrite(&run, sizeof(char), 1, stdout);
            run = cur;
            count = 1;
        }
    }
    fwrite(&count, sizeof(count), 1, stdout);
    fwrite(&run, sizeof(char), 1, stdout);
    return 0;
}

typedef struct __chunk{
    int size;
    char *contents;
} chunk;

void *pparse(void *in){
    chunk *my_chunk = (chunk *) in;
    for(int i = 0; i < my_chunk->size; i++){
        char cur = in[i];
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

int main(int argc, char *argv[])
{
    //get_nprocs_conf = how many procs configured, nonconf is available
    long numproc = get_nprocs();
    long chunk_size = 2^12;
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
    int chunk_count[length/chunk_size] = {0};
    //have to def outside so we can join later on # of threads created,
    // not # of threads we SHOULD have created
    int iter;
    // Initial threads
    for(iter = 0; iter < numproc; iter++){
        if (pthread_create(&rope[iter], NULL, pparse, contents + (chunk_size * iter)!= 0){
		    fprintf(stderr, "Failed to create thread number %d", iter);
		    break;
	    }
        chunk_count[iter] = 1;
    }

    // THIS HERE IS PRODUCER CONSUMER SO LETS DO THAT YA WHOo
    for (int i = 0; i < iter; i++){
            //not sure, but if not null for arg2 we could track the return
	    //values of EACH thread by storing in an array? idk why we would
	    //need to though for this project
	    if (pthread_join(rope[i], NULL) != 0){
		    fprintf(stderr, "Failed to join with thread number %d", i);
		    break;
	    }
    }

    //parse(contents);
    // Free memory
    free(contents);
    return 0;
} 


