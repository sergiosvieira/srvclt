#ifndef server_h
#define server_h

#include <iostream>

static pthread_mutex_t lock;
static const char* kPort = "11978";
static const int kMaxConnections = 5;
static int active_connections = 0;

typedef struct
{
    int socket = 0;
    std::string word = "";
} ClientArgs;

enum class ServerState
{
    IDLE,
    SEND_WORD,
    PLAYING
};

static ServerState server_state = ServerState::IDLE;

void *accept_clients(void *args);
void *service_single_client(void *args);

#endif /* server_h */
