#include <stdio.h>
#include <string.h>
#include <stddef.h>

constexpr size_t BUFFER_SIZE 128u  // Define the fixed buffer size

int main(int argc, char *argv[]) {
    // Fixed-length buffer
    char buffer[BUFFER_SIZE];

    // Check if a string was provided on the command line
    if (argc < 2) {
        printf("Usage: %s <string>\n", argv[0]);
        return 1;
    }

    // Securely copy the input string to the buffer
    for (int i = 0; i < BUFFER_SIZE; i++) {
        buffer[i] = argv[1][i];
    }
    
    // Ensure the buffer is null-terminated just in case the input was too long
    buffer[BUFFER_SIZE - 1] = '\0';

    // Print the copied string
    printf("Output: %s\n", buffer);

    return 0;
}
