#include "server.h"
#include <sys/socket.h> /** socklen_t **/
#include <sys/types.h>
#include <netdb.h> /** struct addrinfo **/
#include <unistd.h> /** close **/

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
        close(server_socket);
        return -1;
    }
    if (bind(server_socket,
             ptr->ai_addr,
             ptr->ai_addrlen) == -1)
    {
        perror("Socket bind() failed");
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
        if (listen(server_socket, kMaxConnections) == -1)
        {
            perror("Socket listen() failed");
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

#define DEBUG

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
    struct sockaddr_storage *client_addr = nullptr;
    socklen_t client_addr_length = sizeof(struct sockaddr_storage);
    int client_socket = 0;
    ClientArgs *c_args = nullptr;
    pthread_t worker_thread;
    int server_socket = init_server();
    if (server_socket == -1) exit(-1);
    while (true)
    {
        print_server_state();
        if (server_state == ServerState::IDLE)
        {
            client_addr = new struct sockaddr_storage();
            if ((client_socket = accept(server_socket, (struct sockaddr *) client_addr, &client_addr_length)) == -1)
            {
                delete client_addr;
                perror("Could not accept() connection");
                continue;
            }
            c_args = new ClientArgs();
            c_args->socket = client_socket;
            if (pthread_create(&worker_thread, nullptr, service_single_client, c_args) != 0)
            {
                perror("Could not create a worker thread");
                delete client_addr;
                delete c_args;
                close(client_socket);
                close(server_socket);
                pthread_exit(nullptr);
            }
            print_active_connections();
            if (active_connections == 2)
            {
                server_state = ServerState::SEND_WORD;
            }
        }
    }
    pthread_exit(nullptr);
}

void *service_single_client(void *args)
{
    ClientArgs *c_args = (ClientArgs*)args;
    int client_socket = c_args->socket,
    rec_bytes = 0;
    char buffer[100] = {};
    pthread_detach(pthread_self());
    pthread_mutex_lock (&lock);
    ++active_connections;
    fprintf(stderr, "+ Number of connections is %d\n", active_connections);
    pthread_mutex_unlock (&lock);
    bool ready = false;
    while(true)
    {
#ifdef DEBUG
        std::cout << "Client socket id: " << client_socket << "\n";
#endif
        rec_bytes = recv(client_socket,
                         buffer,
                         sizeof(buffer),
                         0);
        if (rec_bytes == -1)
        {
            perror("Socket recv() failed");
            close(client_socket);
            pthread_exit(nullptr);
            break;
        }
        if (server_state == ServerState::SEND_WORD &&
            ready == false)
        {
            std::string msg = "Scrambled word is ";
            if (send(client_socket, msg.c_str(), msg.size(), 0) <= 0)
            {
                perror("Socket send() failed");
                close(client_socket);
            }
            else
            {
                ready = true;
            }
        }
    }
    pthread_mutex_lock (&lock);
    --active_connections;
    fprintf(stderr, "- Number of connections is %d\n", active_connections);
    pthread_mutex_unlock (&lock);
    close(client_socket);
    pthread_exit(nullptr);
}
