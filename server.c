#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>

#define MAX_WORDS 100
#define MAX_LENGTH 50
#define DICTIONARY_DIR "dictionary"

typedef struct {
    char english[MAX_LENGTH];
    char french[MAX_LENGTH];
} WordPair;

WordPair dictionary[MAX_WORDS];
int word_count = 0;

void load_dictionary() {
    struct dirent *entry;
    FILE *file;
    char path[256];
    char buffer[MAX_LENGTH * 2];
    
    word_count = 0; 
    
    DIR *dir = opendir(DICTIONARY_DIR);
    if (dir == NULL) {
        perror("Failed to open dictionary directory");
        exit(EXIT_FAILURE);
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
}

void handle_signal(int signal_) {
    if (word_count == 0) {
        printf("There is no words in dictionary.\n");
        return;
    }
    
    int random_index = rand() % word_count;
    if (signal_ == SIGUSR1) {
        printf("English: %s -> French: %s\n", dictionary[random_index].english, dictionary[random_index].french);
    } else if (signal_ == SIGUSR2) {
        printf("French: %s -> English: %s\n", dictionary[random_index].french, dictionary[random_index].english);
    }
}

void monitor_updates() {
    while (1) {
        sleep(5);
        load_dictionary();
        printf("Dictionary updated. Total words: %d\n", word_count);
    }
}

int main() {
    srand(time(NULL));
    load_dictionary();
    
    signal(SIGUSR1, handle_signal);
    signal(SIGUSR2, handle_signal);
    printf("Server running with PID: %d\n", getpid());
    printf("Server is running. Waiting for signals...\n");
    monitor_updates();
    return 0;
}
