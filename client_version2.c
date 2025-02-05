#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <unistd.h>
#include <dirent.h>

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
    long msg_type; // Message type (1: Eng->Fr, 2: Fr->Eng)
    WordPair word;
} Message;

WordPair *shared_memory;
int word_count = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void load_files_to_queue(int msgid) {
    struct dirent *entry;
    char path[256], buffer[MAX_LENGTH * 2];

            FILE *file;
    DIR *dir = opendir(DICTIONARY_DIR);

    if (dir == NULL) {
        perror("Failed to open dictionary directory");
        exit(EXIT_FAILURE);
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".txt") != NULL) {
            snprintf(path, sizeof(path), "%s/%s", DICTIONARY_DIR, entry->d_name);
            file = fopen(path, "r");
            if (file == NULL) continue;

            while (fgets(buffer, sizeof(buffer), file) != NULL) {
                char *english = strtok(buffer, ";");
                char *french = strtok(NULL, "\n");
                if (english && french) {
                    Message msg = { .msg_type = rand() % 2 + 1 };
                    strncpy(msg.word.english, english, MAX_LENGTH);
                    strncpy(msg.word.french, french, MAX_LENGTH);
                    msgsnd(msgid, &msg, sizeof(Message) - sizeof(long), 0);
                }
            }
            fclose(file);
        }
    }
    closedir(dir);
}

void *translation_writer(void *arg) {
    int msgid = msgget(MSG_KEY, IPC_CREAT | 0666);
    if (msgid == -1) {
        perror("Failed to create message queue");
        exit(EXIT_FAILURE);
    }

    while (1) {
        load_files_to_queue(msgid);
        sleep(5); // Check for updates every 5 seconds
    }
}

void *translation_reader(void *arg) {
    int msgid = msgget(MSG_KEY, IPC_CREAT | 0666);
    if (msgid == -1) {
        perror("Failed to access message queue");
        exit(EXIT_FAILURE);
    }

    while (1) {
        Message msg;
        if (msgrcv(msgid, &msg, sizeof(Message) - sizeof(long), 0, IPC_NOWAIT) != -1) {
            pthread_mutex_lock(&mutex);
            if (word_count < MAX_WORDS) {
                if (msg.msg_type == 1) { // English->French
                    printf("English: %s -> French: %s\n", msg.word.english, msg.word.french);
                } else { // French->English
                    printf("French: %s -> English: %s\n", msg.word.french, msg.word.english);
                }
                shared_memory[word_count++] = msg.word;
            }
            pthread_mutex_unlock(&mutex);
        }
        sleep(1); // Avoid busy waiting
    }
}

int main() {
    int shmid = shmget(SHM_KEY, MAX_WORDS * sizeof(WordPair), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("Failed to create shared memory");
        exit(EXIT_FAILURE);
    }
    shared_memory = shmat(shmid, NULL, 0);
    if (shared_memory == (void *)-1) {
        perror("Failed to attach shared memory");
        exit(EXIT_FAILURE);
    }

    pthread_t writer_thread, reader_thread;
    pthread_create(&writer_thread, NULL, translation_writer, NULL);
    pthread_create(&reader_thread, NULL, translation_reader, NULL);

    pthread_join(writer_thread, NULL);
    pthread_join(reader_thread, NULL);

    shmdt(shared_memory);
    shmctl(shmid, IPC_RMID, NULL);
    return 0;
}
