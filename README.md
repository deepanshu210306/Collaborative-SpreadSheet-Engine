# Collaborative Spreadsheet Engine

Real-time collaborative spreadsheet with a **Node.js / JavaScript** backend and a **React** frontend.

## Tech Stack

- **JavaScript** — calculation engine (DAG, topological sort, memory pooling, formulas)
- **Node.js + WebSockets** — real-time collaboration server
- **React** — browser spreadsheet UI

## Features

- Formulas: `+`, `-`, `*`, `/`, `SUM`, `AVG`, `MIN`, `MAX`
- Dependency graph + topological sort (incremental recompute)
- Cycle detection (`#CYCLE!`)
- Memory pooling
- Real-time multi-user sync (WebSocket, server-authoritative)

## Quick Start

```bash
npm install
npm run dev
```

- UI: http://localhost:5173
- Server / WebSocket: http://localhost:3001

Open two browser tabs to test collaboration.

## Structure

```
packages/
  engine/   JS calc engine (topo sort, cycles, formulas)
  server/   Node.js WebSocket server
  client/   React UI
```

## Docker

```bash
docker compose up --build
```

Open: **http://localhost:3001**
