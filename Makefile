all:
    g++ scramble.cpp server.cpp main.cpp -o server -DMUTEX -std=c++11 -lpthread
