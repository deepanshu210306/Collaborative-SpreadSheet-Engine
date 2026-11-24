#pragma once
#include "SpreadsheetEngine.hpp"
#include <string>
#include <map>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET SocketHandle;
#else
typedef int SocketHandle;
#endif

struct Client {
    SocketHandle socket;
    std::string id;
    std::string buffer;
    bool handshakeDone;
};

class WebSocketServer {
public:
    explicit WebSocketServer(int port = 3001);
    ~WebSocketServer();

    bool start();
    void run();
    void stop();

private:
    bool acceptClient();
    void handleClient(Client& client);
    bool doHandshake(Client& client, const std::string& request);
    bool sendText(Client& client, const std::string& message);
    bool recvFrames(Client& client);
    void broadcast(const std::string& message, const std::string& excludeId = "");
    void removeClient(const std::string& id);
    void onMessage(Client& client, const std::string& payload);
    void sendInit(Client& client);

    int port_;
    SocketHandle listenSock_;
    bool running_;
    int clientCounter_;
    std::map<std::string, Client> clients_;
    SpreadsheetEngine engine_;
};
