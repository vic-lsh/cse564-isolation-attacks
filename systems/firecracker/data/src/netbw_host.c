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

#define SERVER_PORT 5001  // Traditional iperf port
#define BUFFER_SIZE 65536  // 64KB buffer
#define BACKLOG 5         // Pending connection queue size

volatile sig_atomic_t keep_running = 1;

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

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char *buffer;
    ssize_t bytes_read;
    unsigned long total_bytes = 0;
    struct timeval start_time, current_time;
    
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
    
    // Allocate buffer
    buffer = (char*)malloc(BUFFER_SIZE);
    if (!buffer) {
        perror("Memory allocation failed");
        close(server_fd);
        return 1;
    }
    
    // Set socket options for performance
    int rcvbuff = 1024 * 1024;  // 1MB receive buffer
    if (setsockopt(server_fd, SOL_SOCKET, SO_RCVBUF, &rcvbuff, sizeof(rcvbuff)) < 0) {
        perror("setsockopt failed");
    }
    
    while (keep_running) {
        // Accept connection
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("Accept failed");
            continue;
        }
        
        // Get client information
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("Client connected: %s\n", client_ip);
        
        // Set socket options for the client connection
        if (setsockopt(client_fd, SOL_SOCKET, SO_RCVBUF, &rcvbuff, sizeof(rcvbuff)) < 0) {
            perror("setsockopt failed");
        }
        
        // Reset counters and start timing
        total_bytes = 0;
        gettimeofday(&start_time, NULL);
        
        // Receive data
        while (keep_running) {
            bytes_read = recv(client_fd, buffer, BUFFER_SIZE, 0);
            if (bytes_read <= 0) {
                if (bytes_read < 0) {
                    perror("Error receiving data");
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
                    printf("\rReceived: %.2f MB, Speed: %.2f Mbps", 
                           total_bytes / (1024.0 * 1024.0), 
                           mbps);
                    fflush(stdout);
                }
            }
        }
        
        // Show final results
        gettimeofday(&current_time, NULL);
        double elapsed_secs = elapsed_time_sec(&start_time, &current_time);
        
        if (elapsed_secs > 0) {
            double mbps = (total_bytes * 8.0) / (elapsed_secs * 1000000.0);
            printf("\n\n--- Bandwidth Test Results ---\n");
            printf("Total data received: %.2f MB\n", total_bytes / (1024.0 * 1024.0));
            printf("Test duration:       %.2f seconds\n", elapsed_secs);
            printf("Average throughput:  %.2f Mbps\n\n", mbps);
        }
        
        close(client_fd);
        printf("Client disconnected. Waiting for new connections...\n");
    }
    
    // Clean up
    free(buffer);
    close(server_fd);
    
    return 0;
}
