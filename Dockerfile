# ---- Stage 1: build C++ backend ----
FROM gcc:12 AS backend-build
WORKDIR /src
COPY packages/backend/ ./
RUN g++ -std=c++14 -O2 -Wall -Iinclude \
    -o spreadsheet_server \
    src/main.cpp \
    src/DependencyGraph.cpp \
    src/TopoSort.cpp \
    src/FormulaParser.cpp \
    src/SpreadsheetEngine.cpp \
    src/WebSocketServer.cpp \
    -lpthread

# ---- Stage 2: build React frontend ----
FROM node:20-alpine AS frontend-build
WORKDIR /app
COPY package.json package-lock.json ./
COPY packages/client/package.json ./packages/client/
COPY packages/backend/package.json ./packages/backend/
RUN npm install --workspace=@spreadsheet/client
COPY packages/client/ ./packages/client/
RUN npm run build -w @spreadsheet/client

# ---- Stage 3: run (Nginx + C++ server) ----
FROM nginx:1.27-alpine
RUN apk add --no-cache libstdc++

COPY --from=backend-build /src/spreadsheet_server /usr/local/bin/spreadsheet_server
COPY --from=frontend-build /app/packages/client/dist /usr/share/nginx/html
COPY docker/nginx.conf /etc/nginx/conf.d/default.conf
COPY docker/start.sh /start.sh
RUN sed -i 's/\r$//' /start.sh && chmod +x /start.sh /usr/local/bin/spreadsheet_server

EXPOSE 80
CMD ["/start.sh"]
