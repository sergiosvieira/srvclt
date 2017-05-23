#ifndef server_h
#define server_h

#include <iostream>
#include <vector>

using VectorOfSockets = std::vector<int>;

static pthread_mutex_t lock;
static const char* kPort = "11978";
static const int kMaxConnectionsQueue = 5;
static int active_connections = 0;
static VectorOfSockets connections = {};

typedef struct
{
    int socket = 0;
    std::string selected_word = "";
    std::string scrambled_word = "";
} ClientArgs;

typedef struct
{
	int number_of_players = 0;
    std::string selected_word = "";
    std::string scrambled_word = "";
} ServerArgs;

enum class ServerState
{
    IDLE,
    SEND_WORD,
    PLAYING,
    GAME_OVER
};

static ServerState server_state = ServerState::IDLE;

void *accept_clients(void *args);
void *service_single_client(void *args);
void *server_playing(void *args);

#endif /* server_h */
