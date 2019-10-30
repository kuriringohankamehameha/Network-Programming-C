#include <stdio.h>
#include <stdlib.h>

int main() {
    int i;
    FILE *fp;

    fp=fopen("bigfakefile.txt","w");

    for(i=0;i<(1);i++) {
        fseek(fp,(1024*1024), SEEK_CUR);
        fprintf(fp,"C");
    }

    fclose(fp);
    return 0;
}

