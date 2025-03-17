#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h>
#include <time.h>
#include <sys/time.h>

#define TEST_DIR "./victim_test_dir"
#define NUM_FILES 1000
#define NUM_ITERATIONS 5

// Get current time in microseconds
long long get_microseconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000LL + tv.tv_usec;
}

// Clean up test files completely
void cleanup_test_environment() {
    printf("Cleaning up test environment...\n");
    
    char cmd[PATH_MAX];
    snprintf(cmd, PATH_MAX, "rm -rf %s", TEST_DIR);
    int ret = system(cmd);
    
    if (ret != 0) {
        printf("Warning: Cleanup command returned %d\n", ret);
        
        // Fallback manual cleanup if system command fails
        DIR *dir = opendir(TEST_DIR);
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                    continue;
                
                char file_path[PATH_MAX];
                snprintf(file_path, PATH_MAX, "%s/%s", TEST_DIR, entry->d_name);
                unlink(file_path);
            }
            closedir(dir);
            rmdir(TEST_DIR);
        }
    } else {
        printf("Test environment cleaned up successfully\n");
    }
}

// Create test directory with files
void setup_test_environment() {
    printf("Setting up test environment...\n");
    
    // First clean up any existing test directory
    cleanup_test_environment();
    
    // Create fresh test directory
    if (mkdir(TEST_DIR, 0755) == -1) {
        perror("Failed to create test directory");
        exit(EXIT_FAILURE);
    }
    
    // Create test files
    for (int i = 0; i < NUM_FILES; i++) {
        char file_path[PATH_MAX];
        snprintf(file_path, PATH_MAX, "%s/file_%d.txt", TEST_DIR, i);
        
        int fd = open(file_path, O_CREAT | O_WRONLY, 0644);
        if (fd == -1) {
            perror("Failed to create test file");
            continue;
        }
        
        // Write small amount of data
        write(fd, "test data", 9);
        close(fd);
    }
    
    printf("Created %d test files in %s\n", NUM_FILES, TEST_DIR);
}

// Function already replaced in previous update

// Perform file lookup tests
double test_file_lookups() {
    long long start_time, end_time;
    struct stat statbuf;
    char file_path[PATH_MAX];
    
    start_time = get_microseconds();
    
    // Perform stat on all files to measure lookup time
    for (int i = 0; i < NUM_FILES; i++) {
        snprintf(file_path, PATH_MAX, "%s/file_%d.txt", TEST_DIR, i);
        if (stat(file_path, &statbuf) == -1) {
            // File not found, not an error for our test
            continue;
        }
    }
    
    end_time = get_microseconds();
    
    // Return time in milliseconds
    return (end_time - start_time) / 1000.0;
}

// Perform directory listing test
double test_directory_listing() {
    long long start_time, end_time;
    
    start_time = get_microseconds();
    
    // Open and read directory entries multiple times
    for (int i = 0; i < 10; i++) {
        DIR *dir = opendir(TEST_DIR);
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                // Just read each entry
            }
            closedir(dir);
        }
    }
    
    end_time = get_microseconds();
    
    // Return time in milliseconds
    return (end_time - start_time) / 1000.0;
}

// Main function to run the victim program
int main(int argc, char *argv[]) {
    int skip_cleanup = 0;
    
    // Check if cleanup should be skipped
    if (argc > 1 && strcmp(argv[1], "--skip-cleanup") == 0) {
        skip_cleanup = 1;
    }
    
    printf("Dentry Cache Victim Program - Performance Measurement\n");
    printf("====================================================\n\n");
    
    // Set up test files
    setup_test_environment();
    
    // Warm up - drop any cached files
    sync();
    system("echo 3 > /proc/sys/vm/drop_caches 2>/dev/null || echo 'Failed to drop caches (requires sudo)'");
    
    // Run the tests
    printf("Running performance tests...\n");
    
    double total_lookup_time = 0;
    double total_listing_time = 0;
    
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        double lookup_time = test_file_lookups();
        double listing_time = test_directory_listing();
        
        printf("Iteration %d: File lookup time: %.2f ms, Directory listing time: %.2f ms\n", 
               i+1, lookup_time, listing_time);
        
        total_lookup_time += lookup_time;
        total_listing_time += listing_time;
        
        // Small delay between iterations
        usleep(100000); // 100ms
    }
    
    // Print average results
    printf("\nAverage Results:\n");
    printf("Average file lookup time: %.2f ms\n", total_lookup_time / NUM_ITERATIONS);
    printf("Average directory listing time: %.2f ms\n", total_listing_time / NUM_ITERATIONS);
    
    printf("\nTo see the impact of dentry cache pollution:\n");
    printf("1. First run this program to get baseline performance\n");
    printf("2. Run the dentry_attack program\n");
    printf("3. Run this program again to see degraded performance\n");
    printf("Note: Pass --skip-cleanup as an argument to keep test files\n");
    
    // Clean up unless explicitly told not to
    if (!skip_cleanup) {
        cleanup_test_environment();
    } else {
        printf("Skipping cleanup as requested. Test files remain in %s\n", TEST_DIR);
    }
    
    return EXIT_SUCCESS;
}
