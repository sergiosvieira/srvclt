#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include "scramble.h"
#include "server.h"

void get_options(int argc,
                 char * argv[],
                 int& number_of_players)
{
    int opt = 0;
    char *players = nullptr;
    /* Use getopt to fetch the host and port */
    while ((opt = getopt(argc, argv, "n:")) != -1)
        switch (opt)
    {
        case 'n':
            players = strdup(optarg);
            number_of_players = std::atoi(players);
            break;
        default:
            printf("Unknown option\n"); exit(1);
    }
    if(players == nullptr)
    {
        printf("USAGE: server -n <number of players>\n");
        exit(1);
    }
}

std::string init_game()
{
    std::cout << "Scramble Game Starts\n";
    std::string word = selectRandomWord(preset);
    return shuffleString(word);
}

int main(int argc, char * argv[])
{
    int players = 0;
    get_options(argc, argv, players);
    std::string selected_word = init_game();
    pthread_t server_thread;
    sigset_t new_sigset;
    sigemptyset (&new_sigset);
    sigaddset(&new_sigset, SIGPIPE);
    if (pthread_sigmask(SIG_BLOCK, &new_sigset, nullptr) != 0)
    {
        perror("Unable to mask SIGPIPE");
        exit(-1);
    }
    pthread_mutex_init(&lock, nullptr);
    if (pthread_create(&server_thread, nullptr, accept_clients, nullptr) < 0)
    {
        perror("Could not create server thread");
        exit(-1);
    }
    pthread_join(server_thread, nullptr);
#ifdef MUTEX
    pthread_mutex_destroy(&lock);
#endif
    pthread_exit(nullptr);
    return 0;
}
