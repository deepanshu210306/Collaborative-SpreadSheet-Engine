#include "WebSocketServer.hpp"
#include "Sha1.hpp"
#include "JsonUtil.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define closesocket close
#endif

namespace {

bool setNonBlocking(SocketHandle s) {
#ifdef _WIN32
    u_long mode = 1;
    return ioctlsocket(s, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(s, F_GETFL, 0);
    return fcntl(s, F_SETFL, flags | O_NONBLOCK) == 0;
#endif
}

std::string extractHeader(const std::string& req, const std::string& name) {
    std::string key = name + ":";
    std::size_t pos = req.find(key);
    if (pos == std::string::npos) {
        // case-insensitive search
        std::string lower = req;
        for (std::size_t i = 0; i < lower.size(); ++i)
            lower[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(lower[i])));
        std::string lkey = key;
        for (std::size_t i = 0; i < lkey.size(); ++i)
            lkey[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(lkey[i])));
        pos = lower.find(lkey);
        if (pos == std::string::npos) return "";
    }
    pos += key.size();
    while (pos < req.size() && (req[pos] == ' ' || req[pos] == '\t')) ++pos;
    std::size_t end = req.find("\r\n", pos);
    if (end == std::string::npos) end = req.size();
    return req.substr(pos, end - pos);
}

} // namespace

WebSocketServer::WebSocketServer(int port)
    : port_(port), listenSock_(INVALID_SOCKET), running_(false),
      clientCounter_(0), engine_(100, 26) {}

WebSocketServer::~WebSocketServer() {
    stop();
}

bool WebSocketServer::start() {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "WSAStartup failed\n";
        return false;
    }
#endif

    listenSock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock_ == INVALID_SOCKET) {
        std::cerr << "socket() failed\n";
        return false;
    }

    int opt = 1;
#ifdef _WIN32
    setsockopt(listenSock_, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&opt), sizeof(opt));
#else
    setsockopt(listenSock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(static_cast<unsigned short>(port_));

    if (bind(listenSock_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "bind() failed on port " << port_ << "\n";
        closesocket(listenSock_);
        listenSock_ = INVALID_SOCKET;
        return false;
    }

    if (listen(listenSock_, 16) == SOCKET_ERROR) {
        std::cerr << "listen() failed\n";
        closesocket(listenSock_);
        listenSock_ = INVALID_SOCKET;
        return false;
    }

    setNonBlocking(listenSock_);
    running_ = true;
    std::cout << "C++ Spreadsheet backend running on ws://localhost:" << port_ << std::endl;
    return true;
}

void WebSocketServer::stop() {
    running_ = false;
    for (std::map<std::string, Client>::iterator it = clients_.begin();
         it != clients_.end(); ++it) {
        closesocket(it->second.socket);
    }
    clients_.clear();
    if (listenSock_ != INVALID_SOCKET) {
        closesocket(listenSock_);
        listenSock_ = INVALID_SOCKET;
    }
#ifdef _WIN32
    WSACleanup();
#endif
}

bool WebSocketServer::acceptClient() {
    sockaddr_in clientAddr;
#ifdef _WIN32
    int len = sizeof(clientAddr);
#else
    socklen_t len = sizeof(clientAddr);
#endif
    SocketHandle s = accept(listenSock_, reinterpret_cast<sockaddr*>(&clientAddr), &len);
    if (s == INVALID_SOCKET) return false;

    setNonBlocking(s);
    Client c;
    c.socket = s;
    std::ostringstream oss;
    oss << "user-" << (++clientCounter_);
    c.id = oss.str();
    c.handshakeDone = false;
    clients_[c.id] = c;
    std::cout << "Client connected: " << c.id << "\n";
    return true;
}

bool WebSocketServer::doHandshake(Client& client, const std::string& request) {
    std::string key = extractHeader(request, "Sec-WebSocket-Key");
    if (key.empty()) return false;

    std::string accept = base64Encode(sha1(key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"));

    std::ostringstream resp;
    resp << "HTTP/1.1 101 Switching Protocols\r\n"
         << "Upgrade: websocket\r\n"
         << "Connection: Upgrade\r\n"
         << "Sec-WebSocket-Accept: " << accept << "\r\n\r\n";

    std::string r = resp.str();
    int sent = send(client.socket, r.c_str(), static_cast<int>(r.size()), 0);
    if (sent <= 0) return false;

    client.handshakeDone = true;
    sendInit(client);

    std::ostringstream usersMsg;
    usersMsg << "{\"type\":\"users\",\"count\":" << clients_.size() << "}";
    broadcast(usersMsg.str(), client.id);
    return true;
}

void WebSocketServer::sendInit(Client& client) {
    std::ostringstream oss;
    oss << "{\"type\":\"init\",\"clientId\":\"" << client.id
        << "\",\"snapshot\":" << engine_.getSnapshotJson()
        << ",\"users\":" << clients_.size() << "}";
    sendText(client, oss.str());
}

bool WebSocketServer::sendText(Client& client, const std::string& message) {
    std::vector<unsigned char> frame;
    frame.push_back(0x81); // FIN + text

    std::size_t len = message.size();
    if (len < 126) {
        frame.push_back(static_cast<unsigned char>(len));
    } else if (len < 65536) {
        frame.push_back(126);
        frame.push_back(static_cast<unsigned char>((len >> 8) & 0xFF));
        frame.push_back(static_cast<unsigned char>(len & 0xFF));
    } else {
        frame.push_back(127);
        for (int i = 7; i >= 0; --i) {
            frame.push_back(static_cast<unsigned char>((len >> (i * 8)) & 0xFF));
        }
    }
    frame.insert(frame.end(), message.begin(), message.end());

    int sent = send(client.socket,
                    reinterpret_cast<const char*>(&frame[0]),
                    static_cast<int>(frame.size()), 0);
    return sent > 0;
}

bool WebSocketServer::recvFrames(Client& client) {
    char buf[4096];
    int n = recv(client.socket, buf, sizeof(buf), 0);
    if (n == 0) return false;
    if (n < 0) {
#ifdef _WIN32
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK) return true;
#else
        if (errno == EWOULDBLOCK || errno == EAGAIN) return true;
#endif
        return false;
    }

    client.buffer.append(buf, n);

    if (!client.handshakeDone) {
        std::size_t end = client.buffer.find("\r\n\r\n");
        if (end != std::string::npos) {
            std::string req = client.buffer.substr(0, end + 4);
            client.buffer.erase(0, end + 4);
            if (!doHandshake(client, req)) return false;
        }
        return true;
    }

    while (client.buffer.size() >= 2) {
        unsigned char b0 = static_cast<unsigned char>(client.buffer[0]);
        unsigned char b1 = static_cast<unsigned char>(client.buffer[1]);
        int opcode = b0 & 0x0F;
        bool masked = (b1 & 0x80) != 0;
        std::size_t payloadLen = b1 & 0x7F;
        std::size_t headerLen = 2;

        if (payloadLen == 126) {
            if (client.buffer.size() < 4) break;
            payloadLen = (static_cast<unsigned char>(client.buffer[2]) << 8) |
                         static_cast<unsigned char>(client.buffer[3]);
            headerLen = 4;
        } else if (payloadLen == 127) {
            if (client.buffer.size() < 10) break;
            payloadLen = 0;
            for (int i = 0; i < 8; ++i) {
                payloadLen = (payloadLen << 8) |
                             static_cast<unsigned char>(client.buffer[2 + i]);
            }
            headerLen = 10;
        }

        std::size_t maskOffset = headerLen;
        if (masked) headerLen += 4;
        if (client.buffer.size() < headerLen + payloadLen) break;

        std::string payload;
        payload.resize(payloadLen);
        if (masked) {
            unsigned char mask[4];
            for (int i = 0; i < 4; ++i)
                mask[i] = static_cast<unsigned char>(client.buffer[maskOffset + i]);
            for (std::size_t i = 0; i < payloadLen; ++i) {
                payload[i] = client.buffer[headerLen + i] ^ mask[i % 4];
            }
        } else {
            payload.assign(client.buffer.begin() + headerLen,
                           client.buffer.begin() + headerLen + payloadLen);
        }

        client.buffer.erase(0, headerLen + payloadLen);

        if (opcode == 0x8) return false; // close
        if (opcode == 0x9) { // ping -> pong
            std::vector<unsigned char> pong;
            pong.push_back(0x8A);
            pong.push_back(static_cast<unsigned char>(payload.size()));
            pong.insert(pong.end(), payload.begin(), payload.end());
            send(client.socket, reinterpret_cast<const char*>(&pong[0]),
                 static_cast<int>(pong.size()), 0);
            continue;
        }
        if (opcode == 0x1 || opcode == 0x2) {
            onMessage(client, payload);
        }
    }
    return true;
}

void WebSocketServer::onMessage(Client& client, const std::string& payload) {
    std::string type = jsonGetString(payload, "type");

    if (type == "cell_update") {
        std::string cellId = jsonGetString(payload, "cellId");
        std::string value = jsonGetString(payload, "value");
        SetCellResult result = engine_.setCell(cellId, value);

        std::ostringstream oss;
        oss << "{\"type\":\"cell_updated\",\"cellId\":\"" << jsonEscape(cellId)
            << "\",\"value\":\"" << jsonEscape(value)
            << "\",\"display\":\"" << jsonEscape(engine_.getDisplayValue(cellId))
            << "\",\"result\":{\"success\":" << (result.success ? "true" : "false")
            << ",\"error\":" << (result.error.empty() ? "null" : ("\"" + jsonEscape(result.error) + "\""))
            << "},\"version\":" << engine_.version()
            << ",\"sender\":\"" << client.id
            << "\",\"cells\":{";

        for (std::size_t i = 0; i < result.affected.size(); ++i) {
            const std::string& id = result.affected[i];
            CellData c = engine_.getCell(id);
            if (i > 0) oss << ",";
            oss << "\"" << jsonEscape(id) << "\":{"
                << "\"raw\":\"" << jsonEscape(c.raw) << "\",";
            if (c.hasValue) {
                oss << "\"value\":\"" << jsonEscape(c.value) << "\",";
            } else {
                oss << "\"value\":null,";
            }
            if (c.error.empty()) {
                oss << "\"error\":null";
            } else {
                oss << "\"error\":\"" << jsonEscape(c.error) << "\"";
            }
            oss << "}";
        }
        oss << "}}";
        broadcast(oss.str());
    } else if (type == "sync_request") {
        std::ostringstream oss;
        oss << "{\"type\":\"sync\",\"snapshot\":" << engine_.getSnapshotJson() << "}";
        sendText(client, oss.str());
    }
}

void WebSocketServer::broadcast(const std::string& message, const std::string& excludeId) {
    std::vector<std::string> dead;
    for (std::map<std::string, Client>::iterator it = clients_.begin();
         it != clients_.end(); ++it) {
        if (it->first == excludeId) continue;
        if (!it->second.handshakeDone) continue;
        if (!sendText(it->second, message)) {
            dead.push_back(it->first);
        }
    }
    for (std::size_t i = 0; i < dead.size(); ++i) removeClient(dead[i]);
}

void WebSocketServer::removeClient(const std::string& id) {
    std::map<std::string, Client>::iterator it = clients_.find(id);
    if (it == clients_.end()) return;
    closesocket(it->second.socket);
    clients_.erase(it);
    std::cout << "Client disconnected: " << id << "\n";

    std::ostringstream oss;
    oss << "{\"type\":\"users\",\"count\":" << clients_.size() << "}";
    broadcast(oss.str());
}

void WebSocketServer::handleClient(Client& client) {
    if (!recvFrames(client)) {
        removeClient(client.id);
    }
}

void WebSocketServer::run() {
    while (running_) {
        acceptClient();

        std::vector<std::string> ids;
        for (std::map<std::string, Client>::iterator it = clients_.begin();
             it != clients_.end(); ++it) {
            ids.push_back(it->first);
        }
        for (std::size_t i = 0; i < ids.size(); ++i) {
            std::map<std::string, Client>::iterator it = clients_.find(ids[i]);
            if (it != clients_.end()) handleClient(it->second);
        }

#ifdef _WIN32
        Sleep(10);
#else
        usleep(10000);
#endif
    }
}
