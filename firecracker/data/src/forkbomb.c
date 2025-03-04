#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>

int main() {
    pid_t pid;
    time_t current_time;
    char time_str[50];
    
    while(1) {
        pid = fork();
        
        if (pid == 0) {
            time(&current_time);
            struct tm *time_info = localtime(&current_time);
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", time_info);
            
            printf("New process - PID: %d, Time: %s\n", getpid(), time_str);
        }
    }
    
    return 0;
}
