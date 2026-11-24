CXX = g++
CXXFLAGS = -std=c++14 -O2 -Wall -Iinclude
LDFLAGS = -lws2_32

SRCS = src/main.cpp \
       src/DependencyGraph.cpp \
       src/TopoSort.cpp \
       src/FormulaParser.cpp \
       src/SpreadsheetEngine.cpp \
       src/WebSocketServer.cpp

OBJS = $(SRCS:.cpp=.o)
TARGET = spreadsheet_server.exe

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	del /Q src\*.o $(TARGET) 2>nul || rm -f src/*.o $(TARGET)

.PHONY: all clean
