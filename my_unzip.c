/*********************************************************
 * Your Name: Noah Ari 
 * Partner Name: Colby Morrison
 *********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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
        for(int i = 0; i < num; i++)
            printf("%c", asc); 
    }
    fclose(fp);
    return 0;
}





