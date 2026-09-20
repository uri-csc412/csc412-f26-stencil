#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    // creates a 1024 byte buffer to store data, will this be enough memory to store data?
    char buffer[1024];

    // Read data from stdin and write it to stdout (standard output)
    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        // pass data from the buffer to a function to process the data
        fputs(buffer, stdout);
    }

    return 0;
}