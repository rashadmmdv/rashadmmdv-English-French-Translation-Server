// File: unified_server_ipc.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <signal.h>
#include <dirent.h>
#include <unistd.h>

#define MAX_WORDS 100
#define MAX_LENGTH 50
#define DICTIONARY_DIR "dictionary"
#define SHM_KEY 1234
#define MSG_KEY 5678

typedef struct {
    char english[MAX_LENGTH];
    char french[MAX_LENGTH];
} WordPair;

typedef struct {
    long msg_type; // Message type (1: Client Request, 2: Server Response)
    char word[MAX_LENGTH];
    int direction; // 1 for English->French, 2 for French->English
    char response[MAX_LENGTH];
} Message;

WordPair *dictionary;
int word_count = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void load_dictionary() {
    pthread_mutex_lock(&mutex);
    word_count = 0; // Reset dictionary

    struct dirent *entry;
    FILE *file;
    char path[256], buffer[MAX_LENGTH * 2];
    
    DIR *dir = opendir(DICTIONARY_DIR);
    if (dir == NULL) {
        perror("Failed to open dictionary directory");
        pthread_mutex_unlock(&mutex);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".txt") != NULL) {
            snprintf(path, sizeof(path), "%s/%s", DICTIONARY_DIR, entry->d_name);
            file = fopen(path, "r");
            if (file == NULL) {
                perror("Failed to open file");
                continue;
            }

            while (fgets(buffer, sizeof(buffer), file) != NULL && word_count < MAX_WORDS) {
                char *english = strtok(buffer, ";");
                char *french = strtok(NULL, "\n");
                if (english && french) {
                    strncpy(dictionary[word_count].english, english, MAX_LENGTH);
                    strncpy(dictionary[word_count].french, french, MAX_LENGTH);
                    word_count++;
                }
            }
            fclose(file);
        }
    }
    closedir(dir);
    pthread_mutex_unlock(&mutex);
}

char *translate(const char *input, int direction) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < word_count; i++) {
        if (direction == 1 && strcmp(input, dictionary[i].english) == 0) {
            pthread_mutex_unlock(&mutex);
            return dictionary[i].french;
        } else if (direction == 2 && strcmp(input, dictionary[i].french) == 0) {
            pthread_mutex_unlock(&mutex);
            return dictionary[i].english;
        }
    }
    pthread_mutex_unlock(&mutex);
    return NULL; // Translation not found
}

void handle_requests() {
    int msgid = msgget(MSG_KEY, IPC_CREAT | 0666);
    if (msgid == -1) {
        perror("Failed to create message queue");
        exit(EXIT_FAILURE);
    }

    Message msg;
    while (1) {
        // Wait for a client request
        if (msgrcv(msgid, &msg, sizeof(Message) - sizeof(long), 1, 0) != -1) {
            char *result = translate(msg.word, msg.direction);
            if (result) {
                snprintf(msg.response, MAX_LENGTH, "%s", result);
            } else {
                snprintf(msg.response, MAX_LENGTH, "Translation not found.");
            }

            // Send response back to client
            msg.msg_type = 2;
            msgsnd(msgid, &msg, sizeof(Message) - sizeof(long), 0);
        }
    }
}

void *monitor_folder(void *arg) {
    while (1) {
        sleep(5); // Check for updates every 5 seconds
        load_dictionary();
        printf("Dictionary updated. Total words: %d\n", word_count);
    }
}

int main() {
    int shmid = shmget(SHM_KEY, MAX_WORDS * sizeof(WordPair), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("Failed to create shared memory");
        exit(EXIT_FAILURE);
    }
    dictionary = shmat(shmid, NULL, 0);
    if (dictionary == (void *)-1) {
        perror("Failed to attach shared memory");
        exit(EXIT_FAILURE);
    }

    load_dictionary();

    pthread_t folder_thread;
    pthread_create(&folder_thread, NULL, monitor_folder, NULL);

    printf("Server is running...\n");
    handle_requests();

    shmdt(dictionary);
    shmctl(shmid, IPC_RMID, NULL);
    return 0;
}
