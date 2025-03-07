#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>

// Function for quicksort comparison
int compare(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

// Function to merge two sorted arrays
void merge(int arr[], int left[], int right[], int left_size, int right_size) {
    int i = 0, j = 0, k = 0;
    
    while (i < left_size && j < right_size) {
        if (left[i] <= right[j]) {
            arr[k++] = left[i++];
        } else {
            arr[k++] = right[j++];
        }
    }
    
    // Copy remaining elements
    while (i < left_size) {
        arr[k++] = left[i++];
    }
    
    while (j < right_size) {
        arr[k++] = right[j++];
    }
}

// Custom sorting function that uses chunks to sort a large array
void chunk_sort(int arr[], size_t size, size_t chunk_size) {
    // Sort the array in chunks to improve cache locality
    for (size_t i = 0; i < size; i += chunk_size) {
        size_t current_chunk_size = (i + chunk_size < size) ? chunk_size : (size - i);
        qsort(arr + i, current_chunk_size, sizeof(int), compare);
    }
    
    // Now merge the sorted chunks
    int *temp = (int*)malloc(size * sizeof(int));
    if (!temp) {
        printf("Memory allocation failed!\n");
        return;
    }
    
    size_t chunk_count = (size + chunk_size - 1) / chunk_size;
    size_t current_size = chunk_size;
    
    while (chunk_count > 1) {
        for (size_t i = 0; i < size; i += 2 * current_size) {
            size_t left_size = current_size;
            size_t right_size = (i + current_size < size) ? 
                            ((i + 2 * current_size < size) ? current_size : (size - i - current_size)) : 0;
            
            if (right_size > 0) {
                // Copy chunks to temp buffer
                memcpy(temp, arr + i, left_size * sizeof(int));
                memcpy(temp + left_size, arr + i + left_size, right_size * sizeof(int));
                
                // Merge back to original array
                merge(arr + i, temp, temp + left_size, left_size, right_size);
            }
        }
        
        current_size *= 2;
        chunk_count = (chunk_count + 1) / 2;
    }
    
    free(temp);
}

// Calculate optimal chunk size based on array size
size_t calculate_chunk_size(size_t array_size) {
    // For very small arrays, use the array size
    if (array_size <= 1024) {
        return array_size;
    }
    
    // For medium arrays, use 1/8 of the array size (power of 2)
    if (array_size <= 1048576) { // 2^20
        return array_size >> 3;  // divide by 8
    }
    
    // For large arrays, target roughly 1M elements per chunk,
    // but keep it a power of 2 for better cache alignment
    size_t log2_size = (size_t)log2((double)array_size);
    size_t log2_chunk = (log2_size > 20) ? 20 : log2_size - 3; // 2^20 = ~1M elements
    return 1UL << log2_chunk;
}

int main(int argc, char *argv[]) {
    // Default power is 26 (2^26 = 67,108,864 elements)
    int power = 26;
    
    // Parse command line arguments
    if (argc > 1) {
        power = atoi(argv[1]);
        if (power < 10 || power > 30) {
            printf("Power must be between 10 (1K elements) and 30 (1B elements).\n");
            printf("Usage: %s [power_of_two]\n", argv[0]);
            return 1;
        }
    } else {
        printf("No power of two specified, using default: 2^%d\n", power);
    }
    
    // Calculate array size
    size_t array_size = 1UL << power; // 2^power
    
    // Calculate chunk size
    size_t chunk_size = calculate_chunk_size(array_size);
    
    printf("Array size: 2^%d = %zu elements (%zu MB)\n", 
           power, array_size, (array_size * sizeof(int)) / (1024 * 1024));
    printf("Chunk size: %zu elements (%zu KB)\n", 
           chunk_size, (chunk_size * sizeof(int)) / 1024);
    
    // Allocate memory for the array
    printf("Allocating memory...\n");
    int *data = (int*)malloc(array_size * sizeof(int));
    if (!data) {
        printf("Memory allocation failed! Try reducing power.\n");
        return 1;
    }
    
    // Initialize with random values
    printf("Initializing array with random values...\n");
    srand(time(NULL));
    for (size_t i = 0; i < array_size; i++) {
        data[i] = rand();
    }
    
    // Start timer
    printf("Starting sort...\n");
    clock_t start = clock();
    
    // Sort the array using our custom chunked sort
    chunk_sort(data, array_size, chunk_size);
    
    // End timer
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    
    // Verify the array is sorted
    printf("Verifying sort...\n");
    int verification_step = array_size > 100000000 ? 10000000 : array_size / 10;
    for (size_t i = 1; i < array_size; i++) {
        if (data[i] < data[i-1]) {
            printf("Sort failed at index %zu!\n", i);
            break;
        }
        
        // Print progress periodically
        if (i % verification_step == 0) {
            printf("Verified %zu elements (%.1f%%)...\n", 
                   i, (100.0 * i) / array_size);
        }
    }
    
    printf("Sort completed in %.2f seconds.\n", time_spent);
    printf("First few elements: %d, %d, %d, %d, %d\n", 
           data[0], data[1], data[2], data[3], data[4]);
    printf("Last few elements: %d, %d, %d, %d, %d\n", 
           data[array_size-5], data[array_size-4], data[array_size-3], 
           data[array_size-2], data[array_size-1]);
    
    // Free memory
    free(data);
    return 0;
}
