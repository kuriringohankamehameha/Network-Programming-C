#include <stdio.h>
#include<stdlib.h>

int main()
{
    int* a = malloc(1);
    int b = 10;
    a = &b;
    printf("a = %d\n", *a);
    a = NULL;
    free(a);
    free(a);

}
