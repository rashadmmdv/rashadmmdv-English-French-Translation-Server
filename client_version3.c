// File: unified_client_ipc.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#define MSG_KEY 5678
#define MAX_LENGTH 50

typedef struct {
    long msg_type; // Message type (1: Client Request, 2: Server Response)
    char word[MAX_LENGTH];
    int direction; // 1 for English->French, 2 for French->English
    char response[MAX_LENGTH];
} Message;

int main() {
    int msgid = msgget(MSG_KEY, IPC_CREAT | 0666);
    if (msgid == -1) {
        perror("Failed to access message queue");
        exit(EXIT_FAILURE);
    }

    Message msg;
    char input[MAX_LENGTH];
    while (1) {
        printf("Enter translation request (format: direction:word) or 'exit': ");
        fgets(input, sizeof(input), stdin);

        if (strncmp(input, "exit", 4) == 0) {
            break;
        }

        // Parse input
        sscanf(input, "%d:%s", &msg.direction, msg.word);
        msg.msg_type = 1; // Client Request

        // Send request to server
        msgsnd(msgid, &msg, sizeof(Message) - sizeof(long), 0);

        // Wait for server response
        if (msgrcv(msgid, &msg, sizeof(Message) - sizeof(long), 2, 0) != -1) {
            printf("Response: %s\n", msg.response);
        }
    }

    return 0;
}
