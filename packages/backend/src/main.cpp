#include "WebSocketServer.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    std::cout.setf(std::ios::unitbuf);

    int port = 3001;
    if (argc > 1) {
        port = std::atoi(argv[1]);
        if (port <= 0) port = 3001;
    }

    WebSocketServer server(port);
    if (!server.start()) {
        std::cerr << "Failed to start C++ backend\n";
        return 1;
    }

    std::cout << "Spreadsheet Engine (C++) — topological sort, cycle detection, collab sync\n";
    std::cout << "Press Ctrl+C to stop.\n";
    server.run();
    return 0;
}
