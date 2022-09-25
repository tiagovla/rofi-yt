#include <stdio.h>
#include <stdlib.h>

void int2hex(int a, char *buffer, int buf_size) {
    buffer += (buf_size - 1);
    for (int i = 7; i >= 0; i--) {
        int j = (a >> (i * 4)) & 0xF;
        if (j <= 9)
            *buffer-- = j + '0';
        else
            *buffer-- = j + 'A' - 10;
    }
}

// function to save char* to file
void save_to_file(char *filename, char *content) {
    FILE *fptr;
    fptr = fopen(filename, "w");
    if (fptr == NULL) {
        printf("Error!");
        exit(1);
    }
    fprintf(fptr, "%s", content);
    fclose(fptr);
}
