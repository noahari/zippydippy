/*********************************************************
 * Your Name: Colby Morrison 
 * Partner Name: Noah Ari
 *********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *parse(char *in){
    char run = in[0];
    int count = 0;
    char *out = (char *) malloc(strlen(in) * sizeof(char));
    for(int i = 0; i < strlen(in); i++){
        char cur = in[i];
        printf("%c\n", cur);
        if (cur == run)
            count++;
        else{
            char runStr[sizeof(count) + 1];
            snprintf(runStr, sizeof(count), "%x", count); // Add count to runStr
            strcat(runStr, &run); // Add run char to runStr
            strcat(out, runStr); // Add runStr to count
            run = cur;
            count = 0;
        }
    }
    return out;
}

int main(int argc, char *argv[]) {
    char *parsed = parse(argv[1]);
    fwrite(parsed, sizeof(char), strlen(parsed), stdout);
    free(parsed);
    return 0;
}
