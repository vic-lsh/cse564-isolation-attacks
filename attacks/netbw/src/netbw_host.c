#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <pthread.h>

#define SERVER_PORT 5001  // Traditional iperf port
#define BUFFER_SIZE 65536  // 64KB buffer
#define BACKLOG 5         // Pending connection queue size

volatile sig_atomic_t keep_running = 1;

// Mutex for thread-safe printing
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

// Signal handler for clean termination
void sig_handler(int signo) {
    if (signo == SIGINT || signo == SIGTERM) {
        printf("\nReceived termination signal. Shutting down...\n");
        keep_running = 0;
    }
}

// Function to calculate elapsed time in seconds
double elapsed_time_sec(struct timeval *start, struct timeval *end) {
    return (end->tv_sec - start->tv_sec) + 
           (end->tv_usec - start->tv_usec) / 1000000.0;
}

// Structure to pass connection information to thread
typedef struct {
    int client_fd;
    struct sockaddr_in client_addr;
} connection_data_t;

// Thread function to handle client connection
void *handle_client(void *arg) {
    connection_data_t *conn = (connection_data_t *)arg;
    int client_fd = conn->client_fd;
    struct sockaddr_in client_addr = conn->client_addr;
    
    // Get client information
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    
    pthread_mutex_lock(&print_mutex);
    printf("Thread %lu: Client connected: %s\n", pthread_self(), client_ip);
    pthread_mutex_unlock(&print_mutex);
    
    // Set socket options for the client connection
    int rcvbuff = 1024 * 1024;  // 1MB receive buffer
    if (setsockopt(client_fd, SOL_SOCKET, SO_RCVBUF, &rcvbuff, sizeof(rcvbuff)) < 0) {
        perror("setsockopt failed");
    }
    
    // Allocate buffer for this thread
    char *buffer = (char*)malloc(BUFFER_SIZE);
    if (!buffer) {
        perror("Memory allocation failed");
        close(client_fd);
        free(conn);
        pthread_exit(NULL);
    }
    
    // Reset counters and start timing
    unsigned long total_bytes = 0;
    struct timeval start_time, current_time;
    gettimeofday(&start_time, NULL);
    
    // Receive data
    ssize_t bytes_read;
    while (keep_running) {
        bytes_read = recv(client_fd, buffer, BUFFER_SIZE, 0);
        if (bytes_read <= 0) {
            if (bytes_read < 0) {
                pthread_mutex_lock(&print_mutex);
                perror("Error receiving data");
                pthread_mutex_unlock(&print_mutex);
            }
            break;  // Client disconnected or error
        }
        
        total_bytes += bytes_read;
        
        // Update and show progress every ~100MB
        if (total_bytes % (100 * 1024 * 1024) == 0) {
            gettimeofday(&current_time, NULL);
            double elapsed_secs = elapsed_time_sec(&start_time, &current_time);
            
            if (elapsed_secs > 0) {
                double mbps = (total_bytes * 8.0) / (elapsed_secs * 1000000.0);
                
                pthread_mutex_lock(&print_mutex);
                printf("Thread %lu: Received: %.2f MB, Speed: %.2f Mbps\n", 
                       pthread_self(),
                       total_bytes / (1024.0 * 1024.0), 
                       mbps);
                pthread_mutex_unlock(&print_mutex);
            }
        }
    }
    
    // Show final results
    gettimeofday(&current_time, NULL);
    double elapsed_secs = elapsed_time_sec(&start_time, &current_time);
    
    if (elapsed_secs > 0) {
        double mbps = (total_bytes * 8.0) / (elapsed_secs * 1000000.0);
        
        pthread_mutex_lock(&print_mutex);
        printf("\n--- Thread %lu: Bandwidth Test Results ---\n", pthread_self());
        printf("Total data received: %.2f MB\n", total_bytes / (1024.0 * 1024.0));
        printf("Test duration:       %.2f seconds\n", elapsed_secs);
        printf("Average throughput:  %.2f Mbps\n\n", mbps);
        printf("Thread %lu: Client disconnected.\n", pthread_self());
        pthread_mutex_unlock(&print_mutex);
    }
    
    // Clean up
    free(buffer);
    close(client_fd);
    free(conn);
    
    pthread_exit(NULL);
}

int main() {
    int server_fd;
    struct sockaddr_in server_addr;
    
    // Set up signal handling
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    
    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Error opening socket");
        return 1;
    }
    
    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server_fd);
        return 1;
    }
    
    // Prepare server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces
    server_addr.sin_port = htons(SERVER_PORT);
    
    // Bind socket to address
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return 1;
    }
    
    // Listen for connections
    if (listen(server_fd, BACKLOG) < 0) {
        perror("Listen failed");
        close(server_fd);
        return 1;
    }
    
    printf("Server listening on port %d...\n", SERVER_PORT);
    
    // Set socket options for performance
    int rcvbuff = 1024 * 1024;  // 1MB receive buffer
    if (setsockopt(server_fd, SOL_SOCKET, SO_RCVBUF, &rcvbuff, sizeof(rcvbuff)) < 0) {
        perror("setsockopt failed");
    }
    
    // Main server loop
    while (keep_running) {
        // Prepare to accept connection
        connection_data_t *conn = malloc(sizeof(connection_data_t));
        if (!conn) {
            perror("Memory allocation failed");
            continue;
        }
        
        socklen_t client_len = sizeof(conn->client_addr);
        
        // Accept connection
        conn->client_fd = accept(server_fd, (struct sockaddr*)&conn->client_addr, &client_len);
        if (conn->client_fd < 0) {
            perror("Accept failed");
            free(conn);
            continue;
        }
        
        // Create thread to handle this connection
        pthread_t thread_id;
        int result = pthread_create(&thread_id, NULL, handle_client, (void *)conn);
        if (result != 0) {
            perror("Thread creation failed");
            close(conn->client_fd);
            free(conn);
            continue;
        }
        
        // Detach thread to allow it to clean up itself when done
        pthread_detach(thread_id);
    }
    
    // Clean up main thread resources
    close(server_fd);
    pthread_mutex_destroy(&print_mutex);
    
    return 0;
}
