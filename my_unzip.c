/*********************************************************
 * Your Name: Noah Ari 
 * Partner Name: Colby Morrison
 *********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
int decode(char *in, int len){
    int count = 0;
    //iterate over compressed text input
    for (int i = 0; i < len; i++)
    {
	//cursor    
        char cur = in[i];
        int code = (int) cur;
        printf("%d: %d\n", i, code);
	//if the character is a digit: parse full digit into count
        if ((i + 1) % 5 != 0){
            
	    //if it is a digit, the last count has an extra order of magnitude to account for before adding the new low order value
            count += code;
	}
	//otherwise it is the letter to print
        else{
	   while(count > 0){
           if(code > 0)
	        printf("%c",cur);
	      count--;	      	   
	   }
	   //reset count inside else block after loop to protect from errors
           count = 0;
       	} 
    }
   return 0;
}





int main(int argc, char *argv[]) {
    FILE *fp = fopen(argv[1], "rb");
    if(!fp){
        fclose(fp);
        return 1;
    }

    // How long is the file?
    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    for(int i = 0; i < length; i++){
        int num;
        char asc;
        int readn = fread(&num, sizeof(int), 1, fp);
        if(!readn){
            fclose(fp);
            return 1;
        }
        int readc = fread(&asc, sizeof(char), 1, fp);
        if(!readc){
            fclose(fp);
            return 1;
        }
        for(int i = 0; i < num; i++){
            printf("%c", asc); 
        }
         // Decompress file
       // decode(contents, length);
    }
    fclose(fp);
    return 0;
}





