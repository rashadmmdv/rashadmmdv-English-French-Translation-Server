#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#define NUM_REQUESTS 100

int main() {
    pid_t server_pid;
    printf("Type your server PID: ");
    scanf("%d", &server_pid);
    
    srand(time(NULL));
    for (int i = 0; i < NUM_REQUESTS; i++) {
        int signal_type = rand() % 2 == 0 ? SIGUSR1 : SIGUSR2;
        if (signal_type)
        {
            printf("Signal type SIGUSR1 is sent");
        }
        else
        {
            printf("Signal type SIGUSR2 is sent");
        }
        kill(server_pid, signal_type);
        sleep(1);
    }
    
    printf("The end.\n");
    return 0;
}
