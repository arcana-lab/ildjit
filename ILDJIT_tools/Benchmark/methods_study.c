#include <stdio.h>
#include <lex.yy.h>

extern FILE *yyin;

int main (int argc, char **argv){
        FILE    *inputFile;

        if (argc <= 1){
                fprintf(stderr, "Give the file name as input\n");
                return -1;
        }
        inputFile       = fopen(argv[1], "r");
        if (inputFile == NULL) {
                fprintf(stderr, "ERROR: Cannot open the file %s\n", argv[1]);
                return -1;
        }

        yyin    = inputFile;
        yylex();

        fclose(inputFile);
        
        return 0;
}
