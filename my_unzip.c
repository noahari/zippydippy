/*********************************************************
 * Your Name: Noah Ari 
 * Partner Name: Colby Morrison
 *********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int decode(char *in){
    char run = in[0];
    int count = 0;
    //iterate over compressed text input
    for (int i = 0; i < strlen(in); i++)
    {
	//cursor    
        char cur = in[i];
	//if the character is a digit: parse full digit into count
        if (isdigit(cur)){
	    //if it is a digit, the last count has an extra order of magnitude to account for before adding the new low order value
            count = count * 10 + c;
	}
	//otherwise it is the letter to print
        else{
	   while(count > 0){
	      fwrite(&run, sizeof(cur), 1, stdout);
	      count--;	      	   
	   }
        }
        run = cur; 
	count = 0;
    }
   return 0;
}





int main(int argc, char *argv[]) {
    
    FILE *fp = fopen(argv[1], "r");
    if(!fp)
        return 1;
    // How long is the file?
    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Allocate memory for contents of file (what if it's bigger than memory??)
    char *contents = (char*) malloc(length * sizeof(char));
    if(!contents)
        return 1;

    // Read whole file
    fread(contents, sizeof(char), length, fp);
    fclose(fp);
    // Decompress file
    decode(contents);
    // Free memory
    free(contents);
    return 0;
}
