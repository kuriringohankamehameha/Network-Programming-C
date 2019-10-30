#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>

/* Performance Analysis of mmap() vs write()
 * when writing 1GB data into a file
 */

#define GIGABYTE 1024*1024*1024
#define MEGABYTE 1024*1024

void mmap_filewrite(int input_size) {
    int fd = open("mmap_testfile.txt", O_RDWR | O_CREAT, (mode_t)0600);
    int size = input_size; // 1 GB file

    lseek(fd, size, SEEK_SET);
    write(fd, "A", 1); // Write a dummy byte to ensure no SIGBUS on mmap() access

    void* page_address = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    printf("Mapped at %p\n", page_address);

    if (page_address == (void*) -1 ) {
        fprintf(stderr, "Error");
        return;
    }

    char* chars = page_address;
    for (int i=0; i<input_size; i++) {
        if (i % (1024) == 0) {
            chars[i] = 'L';
        }
    }

    munmap(page_address, size);
    close(fd);
    return;
}

void write_filewrite() {
    int fd = open("write_testfile.txt", O_RDWR | O_CREAT, (mode_t)0600);

    for (int i=0; i<1024*1024; i++) {
        write(fd, "L", 1);
        lseek(fd, 1024, SEEK_CUR);
    }

    close(fd);
    return;
}

int main() {
    clock_t start, end;
    float seconds;

    start = clock();
    mmap_filewrite(GIGABYTE);
    end = clock();
    seconds = (float)(end - start) / CLOCKS_PER_SEC;
    printf("Time taken for mmap(): %.3lf seconds\n", seconds);
    
    start = clock();
    write_filewrite();
    end = clock();
    seconds = (float)(end - start) / CLOCKS_PER_SEC;
    printf("Time taken for write(): %.3lf seconds\n", seconds);
    return 0;
}
