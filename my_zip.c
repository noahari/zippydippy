/*********************************************************
 * Your Name: Colby Morrison 
 * Partner Name: Noah Ari
 *********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int parse(char *in)
{
    char run = in[0];
    int count = 0;
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

int main(int argc, char *argv[])
{
    FILE *fp = fopen(argv[1], "r");
    if(!fp){
        fclose(fp);
        return 1;
    }
    // How long is the file?
    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Allocate memory for contents of file (what if it's bigger than memory??)
    char *contents = malloc(length + 1);
    if(!contents){
        fclose(fp);
        return 1;
    }

    // Read whole file
    int done = fread(contents, length, 1, fp);
    if(!done){
        fclose(fp);
        return 1;
    }
    fclose(fp);
    // Parse file
    parse(contents);
    // Free memory
    free(contents);
    return 0;
}
