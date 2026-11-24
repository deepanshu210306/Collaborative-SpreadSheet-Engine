# ---- Build React frontend ----
FROM node:20-alpine AS frontend-build
WORKDIR /app
COPY package.json package-lock.json ./
COPY packages/client/package.json ./packages/client/
COPY packages/engine/package.json ./packages/engine/
COPY packages/server/package.json ./packages/server/
RUN npm install
COPY packages/client/ ./packages/client/
RUN npm run build -w @spreadsheet/client

# ---- Run Node server (serves UI + WebSocket) ----
FROM node:20-alpine
WORKDIR /app
COPY package.json package-lock.json ./
COPY packages/client/package.json ./packages/client/
COPY packages/engine/package.json ./packages/engine/
COPY packages/server/package.json ./packages/server/
RUN npm install --omit=dev
COPY packages/engine/ ./packages/engine/
COPY packages/server/ ./packages/server/
COPY --from=frontend-build /app/packages/client/dist ./packages/client/dist

ENV PORT=3001
EXPOSE 3001
CMD ["npm", "run", "start", "-w", "@spreadsheet/server"]
