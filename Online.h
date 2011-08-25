
using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <net/if.h>

#include <vector>
#include <iostream>
#include <fstream>
#include <cstring>
#include <complex>

// the port server and client will connect to
#define DEFAULTPORT 1618
// the maximum number of bytes sent in a single packet
#define MAXDATASIZE 64
// how many pending connections the queue will hold
#define BACKLOG 10

// connected becomes true once a client finds a server or vice versa
bool connected = false;
// listen on the socket sockfd, and add the new connection on newfd
int sockfd, portfd;
// my address's information
struct sockaddr_in my_addr;
// connector's address's information
struct sockaddr_in their_addr;
// size of our socket in memory
socklen_t sin_size;
// variable for garbage collection
struct sigaction sa;

struct IPv4 {
    unsigned char b1, b2, b3, b4;
};

// method run for cleanup
void sigchld_handler(int);
// set up a server, specifying a port
void initServer(int);
// set up a client, specifying a target IP and a port
void initClient(int, char*);
// check if port is valid - return default port otherwise
int validPort(int);
// close all open sockets
void closeConnections();
// send across the connection
void sendMessage(string);
// receive from the connection
string recvMessage(bool);
// pull the next packet from the connection - packets are " " delimited
string nextToken();
// marshall a vector<int> into a string
string listToString(vector<int>*);
// unmarshall a vector from a string
vector<int>* stringToList(string);

