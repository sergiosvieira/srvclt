#include "server.h"
#include <sys/socket.h> /** socklen_t **/
#include <sys/types.h>
#include <netdb.h> /** struct addrinfo **/
#include <unistd.h> /** close **/

//#define DEBUG

int create_server_socket(struct addrinfo* ptr,
                         int& option_value)
{
    int server_socket = 0;
    if ((server_socket = socket(ptr->ai_family,
                                ptr->ai_socktype,
                                ptr->ai_protocol)) == -1)
    {
        perror("Could not open socket");
        return -1;
    }
    if (setsockopt(server_socket,
                   SOL_SOCKET,
                   SO_REUSEADDR,
                   &option_value,
                   sizeof(int)) == -1)
    {
        perror("Socket setsockopt() failed");
        //std::cout << "Close Server " << server_socket << "\n";
        close(server_socket);
        return -1;
    }
    if (bind(server_socket,
             ptr->ai_addr,
             ptr->ai_addrlen) == -1)
    {
        perror("Socket bind() failed");
        //std::cout << "Close Server " << server_socket << "\n";
        close(server_socket);
        return -1;
    }
    return server_socket;
}

int init_server()
{
    int server_socket = 0;
    struct addrinfo hints{},
    *res = nullptr;
    
    int option_value = 1;
    
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    if (getaddrinfo(nullptr, kPort, &hints, &res) != 0)
    {
        perror("getaddrinfo() failed");
        pthread_exit(nullptr);
        return -1;
    }
    struct addrinfo *ptr = nullptr;
    for (ptr = res; ptr != nullptr; ptr = ptr->ai_next)
    {
        server_socket = create_server_socket(ptr, option_value);
        if (server_socket == -1) continue;
        if (listen(server_socket, kMaxConnectionsQueue) == -1)
        {
            perror("Socket listen() failed");
            //std::cout << "Close Server " << server_socket << "\n";
            close(server_socket);
            continue;
        }
        break;
    }
    freeaddrinfo(res);
    if (ptr == nullptr)
    {
        fprintf(stderr, "Could not find a socket to bind to.\n");
        pthread_exit(nullptr);
        return -1;
    }
    return server_socket;
}

void print_server_state()
{
    std::string state = "";
    switch (server_state)
    {
        case ServerState::IDLE:
            state = "IDLE";
            break;
        case ServerState::SEND_WORD:
            state = "SEND WORD";
            break;
        case ServerState::PLAYING:
            state = "PLAYING";
            break;
        case ServerState::GAME_OVER:
            state = "GAME_OVER";
            break;            
    }
#ifdef DEBUG
    std::cout << "Server State: " << state << "\n";
#endif
}

void print_active_connections()
{
#ifdef DEBUG
    std::cout << "Active Connections: " << active_connections << "\n";
#endif
}

void *accept_clients(void *args)
{
    struct sockaddr_storage client_addr;
    socklen_t client_addr_length = sizeof(struct sockaddr_storage);
    int client_socket = 0;
    pthread_t worker_thread;
    int server_socket = init_server();
    if (server_socket == -1) 
    {
        pthread_exit(nullptr);
        exit(-1);
    }
    ServerArgs s_args = *(ServerArgs*)args;
    std::string selected_word = s_args.selected_word;
    std::string scrambled_word = s_args.scrambled_word;
    int players = s_args.number_of_players;
    while (true)
    {
        print_server_state();
        if (server_state == ServerState::IDLE)
        {
            if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_length)) == -1)
            {
                perror("Could not accept() connection");
                continue;
            }
            pthread_mutex_lock (&lock);
            connections.push_back(client_socket);
            pthread_mutex_unlock (&lock);
            if (connections.size() == players)
            {
                pthread_mutex_lock (&lock);
                server_state = ServerState::PLAYING;
                pthread_mutex_unlock (&lock);
                std::string msg = "Scrambled word is " + scrambled_word;
                for (auto socket : connections)
                {
                    if (send(socket, msg.c_str(), msg.size(), 0) <= 0)
                    {
                        perror("Socket send() failed");
                        close(socket);
                        auto it = std::find(connections.begin(),
                                            connections.end(),
                                            socket);
                        connections.erase(it);
                        continue;
                    }
                    ClientArgs c_args;
                    c_args.socket = socket;
                    c_args.scrambled_word = scrambled_word;
                    c_args.selected_word = selected_word;
                    if (pthread_create(&worker_thread, nullptr, service_single_client, &c_args) != 0)
                    {
                        perror("Could not create a worker thread");
                        close(socket);
                        close(server_socket);
                        break;
                    }
                }
            }
        } 
        else if (server_state == ServerState::PLAYING)
        {
        }
        else if (server_state == ServerState::GAME_OVER)
        {            
            for (auto socket : connections)
            {
                std::string msg = "game_over";
                if (send(socket, msg.c_str(), msg.size(), 0) <= 0)
                {
                    perror("Socket send() failed");
                }
                close(socket);
            }            
            pthread_mutex_lock (&lock);
            connections.clear();
            server_state = ServerState::IDLE;
            pthread_mutex_unlock (&lock);
        }
    }
    pthread_exit(nullptr);
}

void *service_single_client(void *args)
{
    ClientArgs *c_args = (ClientArgs*)args;
    int client_socket = c_args->socket;
    std::string selected_word = c_args->selected_word;
    ssize_t rec_bytes = 0;
    char buffer[100] = {};
    pthread_detach(pthread_self());
    pthread_mutex_lock (&lock);
    ++active_connections;
    pthread_mutex_unlock (&lock);
    while(true)
    {
        memset(buffer, '\0', sizeof(buffer));
        rec_bytes = recv(client_socket,
                         buffer,
                         sizeof(buffer),
                         0);
        if (rec_bytes == -1)
        {
            perror("Socket recv() failed");
            close(client_socket);
            break;
        }
        else if (rec_bytes > 0)
        {
            buffer[rec_bytes] = '\0';
            ptrdiff_t index = find(connections.begin(), connections.end(), client_socket) - connections.begin();
            std::string guess = std::string(buffer);
            if (selected_word == guess)
            {
                std::cout << "Player " << index + 1 << " WINS!!\n";
                std::string msg = "You are Correct!";
                if (send(client_socket, msg.c_str(), msg.size(), 0) <= 0)
                {
                    perror("Socket send() failed");
                    break;
                }                
                pthread_mutex_lock (&lock);
                server_state = ServerState::GAME_OVER;
                pthread_mutex_lock (&lock);
                break;
            }
            else
            {
                std::cout << "Received "
                << guess
                << " - player" << index + 1
                << " wrong\n";
                std::string msg = "Wrong! Try again";
                if (send(client_socket, msg.c_str(), msg.size(), 0) <= 0)
                {
                    perror("Socket send() failed");
                    break;                    
                }
            }
        }
    }
    pthread_mutex_lock (&lock);
    --active_connections;
    pthread_mutex_unlock (&lock);
    //close(client_socket);
    pthread_exit(nullptr);
}
