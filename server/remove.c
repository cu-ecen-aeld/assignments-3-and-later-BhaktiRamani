#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <string.h>

int main()
{   
    const char *dir_path = "/home/bakri/Work/1_CU_Boulder/AESD/assignments-3-and-later-BhaktiRamani";
    const char *file_name = "aesd.txt";

    // Construct full file path
    char full_path[512]; // Make sure buffer is large enough
    snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, file_name);

    // Attempt to delete the file
    if (remove(full_path) == 0) {
        printf("File '%s' deleted successfully.\n", full_path);
    } else {
        perror("Error deleting file");
    }

    return 0;
}
