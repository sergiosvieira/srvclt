#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>

int main(int argc, char *argv[])
{
    int clientSocket;
    struct addrinfo hints{}, // Used to provide hints to getaddrinfo()
           *res = nullptr, // Used to return the list of addrinfo's
           *p = nullptr;   // Used to iterate over this list
    char *host = nullptr, *port = nullptr;
    char buffer[100 + 1]; // +1 for '\0'
    ssize_t nbytes = 0;
    int opt = 0;

    while ((opt = getopt(argc, argv, "h:p:")) != -1)
        switch (opt)
        {
            case 'h':
                host = strdup(optarg);
                break;
            case 'p':
                port = strdup(optarg);
                break;
            default:
                printf("Unknown option\n"); exit(1);
        }

    if(host == NULL || port == NULL)
    {
        printf("USAGE: client -h HOST -p PORT\n");
        exit(1);
    }
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, port, &hints, &res) != 0)
    {
        perror("getaddrinfo() failed");
        exit(-1);
    }
    for(p = res;p != NULL; p = p->ai_next) 
    {
        if ((clientSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) 
        {
            perror("Could not open socket");
            continue;
        }
        if (connect(clientSocket, p->ai_addr, p->ai_addrlen) == -1) 
        {
            close(clientSocket);
            perror("Could not connect to socket");
            continue;
        }
        break;
    }
    freeaddrinfo(res);
    while (true)
    {
      memset(buffer, '\0', sizeof(buffer));
      nbytes = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
      if (nbytes == 0)
      {
          perror("Server closed the connection");
          close(clientSocket);
          exit(-1);
      }
      else if (nbytes == -1)
      {
          perror("Socket recv() failed");
          close(clientSocket);
          exit(-1);
      }
      else
      {
          buffer[nbytes] = '\0';
          std::string server_message = std::string(buffer);
          if (server_message == "game_over")
          {
            std::cout << "Game Over!! You lost!\n";
            break;
          } 
          else if (server_message == "You are Correct!")
          {
            std::cout << "You are Correct!\n";
            break;
          }
          std::cout << server_message << "\n";
          std::string guess = "";
          while (guess.size() == 0)
          {
              std::cout << "Enter guess: ";
              std::cin >> guess;
              if (send(clientSocket, guess.c_str(), guess.size(), 0) <= 0)
              {
                perror("Socket send() failed");
                close(clientSocket);
              }                  
          }              
      }
    }
    close(clientSocket);
    return 0;
}
