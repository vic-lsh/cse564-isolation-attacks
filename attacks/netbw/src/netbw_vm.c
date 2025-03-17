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
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>

#define SERVER_PORT 5001  // Traditional iperf port
#define BUFFER_SIZE 65536  // 64KB buffer size
#define TEST_DURATION 10   // Default test duration in seconds

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

// Function to determine server IP based on eth0 IP
char* get_server_ip() {
    struct ifaddrs *ifaddr, *ifa;
    static char server_ip[INET_ADDRSTRLEN];
    char host[NI_MAXHOST];
    
    // Get list of network interfaces
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return NULL;
    }
    
    // Find eth0 interface with IPv4 address
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;
            
        // Only interested in eth0 and IPv4
        if (strcmp(ifa->ifa_name, "eth0") == 0 && 
            ifa->ifa_addr->sa_family == AF_INET) {
            
            // Convert network address to string
            struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
            inet_ntop(AF_INET, &(sa->sin_addr), host, NI_MAXHOST);
            
            // Copy the IP address to modify
            strncpy(server_ip, host, INET_ADDRSTRLEN);
            
            // Extract the last digit and subtract one
            char *last_octet = strrchr(server_ip, '.') + 1;
            int octet_value = atoi(last_octet);
            octet_value--;
            
            // Replace the last octet
            sprintf(last_octet, "%d", octet_value);
            
            break;
        }
    }
    
    freeifaddrs(ifaddr);
    return server_ip;
}

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in server_addr;
    char *buffer;
    ssize_t bytes_sent;
    unsigned long total_bytes = 0;
    struct timeval start_time, current_time;
    double elapsed_secs;
    int duration = TEST_DURATION;
    char *server_ip;
    struct hostent *host;

    
    // Check for custom duration
    if (argc > 1) {
        duration = atoi(argv[1]);
        if (duration <= 0) {
            duration = TEST_DURATION;
        }
    }
    
    // Set up signal handling
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    
    // Get server IP based on eth0
    // server_ip = get_server_ip();
    // server_ip = "127.0.0.1";
    server_ip = "host.docker.internal";
    if (server_ip == NULL) {
        fprintf(stderr, "Failed to determine server IP from eth0. Please check your network configuration.\n");
        return 1;
    }
    host = gethostbyname(server_ip);
    if (host == NULL) {
        fprintf(stderr, "Error resolving hostname %s\n", server_ip);
        return 1;
    }
    
    printf("Determined server IP: %s\n", server_ip);
    
    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error opening socket");
        return 1;
    }
    
    // Prepare server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    memcpy(&server_addr.sin_addr, host->h_addr, host->h_length);
    
    // if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
    //     perror("Invalid address");
    //     close(sockfd);
    //     return 1;
    // }
    
    // Connect to server
    printf("Attempting to connect to %s:%d...\n", server_ip, SERVER_PORT);
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        printf("Make sure you have a server listening at %s:%d\n", server_ip, SERVER_PORT);
        close(sockfd);
        return 1;
    }
    
    printf("Connected successfully. Starting bandwidth test for %d seconds...\n", duration);
    
    // Allocate buffer and fill with random data
    buffer = (char*)malloc(BUFFER_SIZE);
    if (!buffer) {
        perror("Memory allocation failed");
        close(sockfd);
        return 1;
    }
    
    // Fill buffer with some pattern
    for (int i = 0; i < BUFFER_SIZE; i++) {
        buffer[i] = (char)(i % 256);
    }
    
    // Set socket options for performance
    int sendbuff = 1024 * 1024;  // 1MB send buffer
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sendbuff, sizeof(sendbuff)) < 0) {
        perror("setsockopt failed");
    }
    
    // Start timing
    gettimeofday(&start_time, NULL);
    current_time = start_time;
    
    // Send data as fast as possible
    while (keep_running && elapsed_time_sec(&start_time, &current_time) < duration) {
        bytes_sent = send(sockfd, buffer, BUFFER_SIZE, 0);
        if (bytes_sent < 0) {
            perror("Error sending data");
            break;
        }
        
        total_bytes += bytes_sent;
        
        // Update time and print progress every ~1 second
        if (total_bytes % (20 * BUFFER_SIZE) == 0) {
            gettimeofday(&current_time, NULL);
            elapsed_secs = elapsed_time_sec(&start_time, &current_time);
            
            if (elapsed_secs > 0) {
                double mbps = (total_bytes * 8.0) / (elapsed_secs * 1000000.0);
                printf("\rTime: %.2f sec, Sent: %.2f MB, Speed: %.2f Mbps", 
                       elapsed_secs, 
                       total_bytes / (1024.0 * 1024.0), 
                       mbps);
                fflush(stdout);
            }
        }
    }
    
    // Calculate final results
    gettimeofday(&current_time, NULL);
    elapsed_secs = elapsed_time_sec(&start_time, &current_time);
    
    if (elapsed_secs > 0) {
        double mbps = (total_bytes * 8.0) / (elapsed_secs * 1000000.0);
        double mb_sent = total_bytes / (1024.0 * 1024.0);
        
        printf("\n\n--- Bandwidth Test Results ---\n");
        printf("Total bytes sent:     %.2f MB\n", mb_sent);
        printf("Test duration:        %.2f seconds\n", elapsed_secs);
        printf("Average throughput:   %.2f Mbps\n", mbps);
    }
    
    // Clean up
    free(buffer);
    close(sockfd);
    
    return 0;
}
