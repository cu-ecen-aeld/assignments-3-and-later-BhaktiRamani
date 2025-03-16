// #include <stdio.h>
// #include <stdlib.h>
// #include <fcntl.h>
// #include <unistd.h>
// #include <string.h>
// #include <errno.h>

// #define DEVICE "/dev/aesdchar"
// #define TEST_STR1 "Hello, AESD!\n"
// #define TEST_STR2 "Partial write"
// #define TEST_STR3 " completed!\n"
// #define READ_BUF_SIZE 1024

// void test_write(int fd, const char *str) {
//     ssize_t bytes_written = write(fd, str, strlen(str));
//     if (bytes_written < 0) {
//         perror("Write failed");
//         exit(EXIT_FAILURE);
//     }
//     printf("Wrote %zd bytes: \"%s\"\n", bytes_written, str);
// }

// void test_read(int fd) {
//     char buffer[READ_BUF_SIZE] = {0};
//     ssize_t bytes_read = read(fd, buffer, READ_BUF_SIZE - 1);
//     printf("bytes_read = %ld\n", bytes_read);
//     if (bytes_read < 0) {
//         perror("Read failed");
//         exit(EXIT_FAILURE);
//     }
//     printf("Read %zd bytes: \"%s\"\n", bytes_read, buffer);
// }

// int main() {
//     int fd = open(DEVICE, O_RDWR | O_TRUNC);
//     if (fd < 0) {
//         perror("Failed to open device file");
//         return EXIT_FAILURE;
//     }

//     printf("Testing complete write...\n");
//     test_write(fd, TEST_STR1);

//     printf("Testing partial write...\n");
//     test_write(fd, TEST_STR2);
//     test_write(fd, TEST_STR3); // Completing the partial write

//     lseek(fd, 0, SEEK_SET);  // Reset file position for reading
//     printf("Reading back data...\n");
//     test_read(fd);

//     close(fd);
//     return EXIT_SUCCESS;
// }

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define DEVICE "/dev/aesdchar"
#define TEST_STR1 "Hello, AESD!\n"
#define TEST_STR2 "Partial write"
#define TEST_STR3 " completed!\n"
#define READ_BUF_SIZE 1024

void test_write(int fd, const char *str) {
    ssize_t bytes_written = write(fd, str, strlen(str));
    if (bytes_written < 0) {
        perror("Write failed");
        exit(EXIT_FAILURE);
    }
    printf("Wrote %zd bytes: \"%s\"\n", bytes_written, str);
    printf("\n");
}

void test_read(int fd) {
    char buffer[READ_BUF_SIZE] = {0};
    ssize_t bytes_read = read(fd, buffer, READ_BUF_SIZE - 1);
    printf("bytes_read = %ld\n", bytes_read);
 
    if (bytes_read < 0) {
        perror("Read failed");
        exit(EXIT_FAILURE);
    }
    printf("Read %zd bytes: \"%s\"\n", bytes_read, buffer);
    printf("\n");
    
    // Print hex dump for better debugging
    // printf("Hex dump of read data:\n");
    // for (int i = 0; i < bytes_read; i++) {
    //     printf("%02x ", buffer[i]);
    //     if ((i+1) % 16 == 0) printf("\n");
    // }
    printf("\n");
}

void test_circular_buffer() {
    printf("\n========== TESTING CIRCULAR BUFFER ==========\n");
    
    int fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device file");
        exit(EXIT_FAILURE);
    }
    
    // Write 10 numbered entries
    for (int i = 1; i <= 10; i++) {
        char buf[20];
        sprintf(buf, "write%d\n", i);
        test_write(fd, buf);
    }
    
    // Read back all entries
    printf("Reading all entries...\n");
    lseek(fd, 0, SEEK_SET);
    test_read(fd);
    
    // Add one more entry (which should push out the first entry)
    printf("\nAdding one more entry to test circular buffer...\n");
    test_write(fd, "write11\n");
    
    // Read again to see if first entry was pushed out
    printf("Reading entries after overflow...\n");
    lseek(fd, 0, SEEK_SET);
    test_read(fd);
    
    // Test partial writes combining to form one entry
    printf("\nTesting partial writes...\n");
    test_write(fd, "part");
    test_write(fd, "ial");
    test_write(fd, "_write\n");
    
    // Read again
    printf("Reading after partial writes...\n");
    lseek(fd, 0, SEEK_SET);
    test_read(fd);
    
    close(fd);
}

int main() {
    int fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device file");
        return EXIT_FAILURE;
    }

    printf("Testing complete write...\n");
    test_write(fd, "Write 1");
    test_write(fd, "Write 2");
    test_write(fd, "Write 3");
    test_write(fd, "Write 4");
    test_write(fd, "Write 5");
    test_write(fd, "Write 6");
    test_write(fd, "Write 7");
    test_write(fd, "Write 8");
    test_write(fd, "Write 9");
    test_write(fd, "Write 10");
    printf("Reading the file after 10 writes\n");
    //lseek(fd, 0, SEEK_SET);  // Reset file position for reading
    test_read(fd);
    printf("Adding 11th element - BUFFERFLOW\n");
    test_write(fd, "Write 11");
    //lseek(fd, 0, SEEK_SET);  // Reset file position for reading
    test_read(fd);
    


    // printf("Testing partial write...\n");
    // test_write(fd, TEST_STR2);
    // test_write(fd, TEST_STR3); // Completing the partial write

    // lseek(fd, 0, SEEK_SET);  // Reset file position for reading
    // printf("Reading back data...\n");
    // test_read(fd);

    close(fd);
    
    // // Now test the circular buffer functionality
    // test_circular_buffer();
    
    return EXIT_SUCCESS;
}