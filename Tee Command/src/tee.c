#include<stdio.h>
#include<stdlib.h>

void tee_command(int argc, char* argv[])
{
    // Reads from STDIN and writes to both STDOUT and argv
    FILE** op_files = (FILE**) calloc (argc, sizeof(FILE*));
    for (int i=1; i<argc; i++)
    {
        op_files[i-1] = fopen(argv[i], "w"); 
    }

    // Buffer for reading (Setting limit to 4KB as default)
    char buffer[4096];
    int num_read;
    
    // Write to STDOUT and the files provided bytes are still left in the input stream
    while (( num_read = fread(buffer, 1, sizeof(buffer), stdin) ))
    {
        fwrite(buffer, 1, num_read, stdout);
        for (int i=0; i<argc-1; i++)
            fwrite(buffer, 1, num_read, op_files[i]);
    }
    
    // Close the files after writing
    for (int i=0; i<argc-1; i++)
        fclose(op_files[i]);

    // Free the loose pointer
    free(op_files);
}

int main(int argc, char* argv[])
{
    tee_command(argc, argv); 
    return 0;
}
