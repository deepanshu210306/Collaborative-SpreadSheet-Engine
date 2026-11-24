CXX = g++
CXXFLAGS = -std=c++14 -O2 -Wall -Iinclude

ifeq ($(OS),Windows_NT)
  LDFLAGS = -lws2_32
  TARGET = spreadsheet_server.exe
else
  LDFLAGS = -lpthread
  TARGET = spreadsheet_server
endif

SRCS = src/main.cpp \
       src/DependencyGraph.cpp \
       src/TopoSort.cpp \
       src/FormulaParser.cpp \
       src/SpreadsheetEngine.cpp \
       src/WebSocketServer.cpp

OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(TARGET) spreadsheet_server.exe

.PHONY: all clean
