#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdbool.h>

#define PORT 4001        
#define NUM_PLAYERS 3
#define MAX_WORD 64

typedef struct {
    char word[MAX_WORD];
    char masked[MAX_WORD];
    int score[NUM_PLAYERS];
    int current;
    int secret_word_length;     
} GameState;

char words[5][6] = {"HELLO", "BYE", "HOME", "LINUX", "KICK"};

void broadcast(int fds[], char *msg);
void send_to(int fd, char *msg);
int spin_wheel(void);
int reveal_letter(GameState *g, char letter);;

int main() {
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int client_fds[NUM_PLAYERS];
    srand(time(NULL));

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {

        perror("couldn't create socket");
        exit(EXIT_FAILURE);

    }

    printf("server socket created\n");

    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
         perror("couldn't bind");
         exit(EXIT_FAILURE);
    }

    printf("server socket bind done\n");

    if (listen(server_fd, NUM_PLAYERS) < 0) {
        perror("couldn't listen");
        exit(EXIT_FAILURE);
    }

    printf("server socket listen done\n");

    printf("Server listening on port %d...\n", PORT);

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    for (int i = 0; i < NUM_PLAYERS; i++) {

        client_fds[i] = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        char welcome_msg[64];
        sprintf(welcome_msg, "WELCOME P%d\n", i);
        send(client_fds[i], welcome_msg, strlen(welcome_msg), 0);
    }

    broadcast(client_fds, "ALL CONNECTED\nGame starts!!\n");

    GameState game;
    memset(&game, 0, sizeof(game));

    int random_world = rand() % 5;

    strcpy(game.word, words[random_world]);
    game.secret_word_length = strlen((game.word));

    for (int i = 0; i < game.secret_word_length; i++) {
        game.masked[i] = '_';
    }
    game.masked[game.secret_word_length] = '\0';
    game.current = 0;

    bool didPlayerSpin = false;

    while (1) {

        spin:

        char state_msg[128];
        sprintf(state_msg, "STATE %s\n", game.masked);
        broadcast(client_fds, state_msg);

        send_to(client_fds[game.current], "YOUR_TURN\n");

        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        int valueread = read(client_fds[game.current], buffer, sizeof(buffer));

        if (valueread <= 0) {
            break;
        }

        if (strncmp(buffer, "SPIN", 4) == 0) {
            int result = spin_wheel();
            
            if (result == -1) {
                game.score[game.current] = 0;
                send_to(client_fds[game.current], "SPIN_RESULT BANKRUPT\n");
                game.current = (game.current + 1) % NUM_PLAYERS;
            } 
            else if (result == 0) {
                send_to(client_fds[game.current], "SPIN_RESULT LOSE_TURN\n");
                game.current = (game.current + 1) % NUM_PLAYERS;
            } 
            else {
                char spin_msg[64];
                sprintf(spin_msg, "SPIN_RESULT %d POINTS\n", result);
                send_to(client_fds[game.current], spin_msg);
            }

            didPlayerSpin = true;

        }
        
        else if (strncmp(buffer, "GUESS_LETTER", 12) == 0) {

            if (!didPlayerSpin) {

                send_to(client_fds[game.current], "GUESS_LETTER PASSED. YOU HAVE TO SPIN FIRST\n");

                goto spin;

            }

            char guessed_char = buffer[13];
            int found = reveal_letter(&game, guessed_char);

            if (found > 0) {
                send_to(client_fds[game.current], "LETTER_RESULT HIT\n");

            } else {
                send_to(client_fds[game.current], "LETTER_RESULT MISS\n");
                game.current = (game.current + 1) % NUM_PLAYERS;
                didPlayerSpin = false;
            }

        }
        
        else if (strncmp(buffer, "GUESS_WORD", 10) == 0) {
            char guessed_word[MAX_WORD];
            sscanf(buffer + 11, "%s", guessed_word);
            if (strcmp(guessed_word, game.word) == 0) {
                char end_msg[128];
                sprintf(end_msg, "WORD_RESULT OK\nEND WINNER P%d\n", game.current);
                broadcast(client_fds, end_msg);
                for(int i = 0; i < NUM_PLAYERS; i++) close(client_fds[i]);
                close(server_fd);
                return 0;
            } else {
                send_to(client_fds[game.current], "WORD_RESULT WRONG\n");
                game.current = (game.current + 1) % NUM_PLAYERS;
            }
        }
    }
    return 0;
}

int spin_wheel() {
    int r = rand() % 10;
    if (r == 0) return -1;
    if (r == 1) return 0;
    return (rand() % 10 + 1) * 100;
}

int reveal_letter(GameState *g, char letter) {
    int count = 0;
    for (int i = 0; i < g->secret_word_length; i++) {
        if (g->word[i] == letter && g->masked[i] == '_') {
            g->masked[i] = letter;
            count++;
        }
    }
    return count;
}

void broadcast(int fds[], char *msg) {
    for (int i = 0; i < NUM_PLAYERS; i++) {
        send(fds[i], msg, strlen(msg), 0);
    }
}

void send_to(int fd, char *msg) {
    send(fd, msg, strlen(msg), 0);
}