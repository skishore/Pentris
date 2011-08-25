
// class for building a server or client and connecting to another computer
#include "Online.h"

void sigchld_handler(int s) {
    while(wait(NULL) > 0);
}

int validPort(int PORT) {
    if ((PORT > 1024) && (PORT < 65536))
        return PORT;
    return DEFAULTPORT;
}

void initServer(int PORT) {
    char* address;
    struct ifreq ifr;

    PORT = validPort(PORT);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        cout << "Error opening socket" << endl;
        exit(0);
    }   

    my_addr.sin_family = AF_INET;         // host byte order
    my_addr.sin_port = htons(PORT);     // short, network byte order
    my_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
    memset(&(my_addr.sin_zero), '\0', 8); // zero the rest of the struct

    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
        cout << "Error binding port" << endl;
        exit(0);
    } 

    if (listen(sockfd, BACKLOG) == -1) {
        cout << "Error listening" << endl;
        exit(0);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        cout << "Error in garbage collection" << endl;
        exit(0);
    }

    ifr.ifr_addr.sa_family = AF_INET;  
    strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ-1);
    // get the network address with ioctl
    ioctl(sockfd, SIOCGIFADDR, &ifr);
    address = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
    
    cout << "Server running at IP " << address << ", port " << PORT << endl;

    while (connected == false) {  // main accept() loop
        sin_size = sizeof(struct sockaddr_in);
        if ((portfd = accept(sockfd, (struct sockaddr*)&their_addr, &sin_size)) == -1) {
            cout << "Error accepting connection" << endl;
            continue;
        }

        if (fcntl(portfd, F_SETFL, fcntl(portfd, F_GETFL) | O_NONBLOCK) == -1) 
            cout << "Error making socket nonblocking with fcntl" << endl;

        cout << "Received a connection from IP " << inet_ntoa(their_addr.sin_addr) << endl;
        connected = true;
    }
}

void initClient(int PORT, char* destIP) {
    PORT = validPort(PORT);
    
    if ((portfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        cout << "Error opening socket" << endl;
        exit(0);
    }

    their_addr.sin_family = AF_INET;    // host byte order
    their_addr.sin_port = htons(PORT);  // short, network byte order
    their_addr.sin_addr.s_addr = inet_addr(destIP);
    memset(&(their_addr.sin_zero), '\0', 8);  // zero the rest of the struct

    if (connect(portfd, (struct sockaddr*)&their_addr, sizeof(struct sockaddr)) == -1) {
        cout << "Error connecting to host" << endl;
        exit(0);
    } 
        
    if (fcntl(portfd, F_SETFL, fcntl(portfd, F_GETFL) | O_NONBLOCK) == -1) 
        cout << "Error making socket nonblocking with fcntl" << endl;
    
    cout << "Connected to host at IP " << destIP << ", port " << PORT << endl;
    connected = true;
}

string vectorToString(vector<int>* v) {
    stringstream stream;
    for (int i = 0; i < v->size(); i++) {
        if (i > 0)
            stream << ",";
        stream << (*v)[i];
    }
    stream << " ";
    
    return stream.str();
}

vector<int>* stringToVector(const string s) {
    int num;
    
    vector<int>* v = new vector<int>();
    stringstream stream(s);
    while (stream >> num) {
        v->push_back(num);
        stream.ignore(1);
    }
    return v;
}

void sendMessage(string message) {
    const char* buf = message.c_str();
   
    if (send(portfd, buf, strlen(buf), 0) == -1)
        cout << "Error sending message " << message << endl;
}

string recvMessage(bool peek=false) {
    int numbytes;
    char* buf = new char[MAXDATASIZE];

    int flags = 0;
    if (peek == true)
        flags = MSG_PEEK;

    if ((numbytes = recv(portfd, buf, MAXDATASIZE-1, flags)) == -1) {
        if (errno != EWOULDBLOCK) {
            cout << "Error in recv()" << endl;
        } // else cout << "No bytes in the socket" << endl;
        return "";
    }

    string ans(buf, numbytes);
    delete buf;
    return ans;
}

string nextToken() {
    string word;
    stringstream stream(recvMessage(true));
    stream >> word;
 
    int size = word.length() + 1;

    if (word != "") {
        char* buf = new char[size];
        if (recv(portfd, buf, size, 0) == -1)
            if (errno != EWOULDBLOCK) {
                cout << "Error in recv()" << endl;
            } // else cout << "No bytes in the socket" << endl;
    }
    
    return word;
}

void closeConnections() {
    close(portfd);
    close(sockfd);
    cout << "Connections closed." << endl;
}

