# Collaborative Spreadsheet Engine

Real-time collaborative spreadsheet with a **full C++ backend** and a **React** frontend.

## Tech Stack

- **C++** — calculation engine + WebSocket collaboration server
- **React** — browser spreadsheet UI

## Features

- Formulas: `+`, `-`, `*`, `/`, `SUM`, `AVG`, `MIN`, `MAX`
- Dependency graph + topological sort (incremental recompute)
- Cycle detection (`#CYCLE!`)
- Memory pooling
- Real-time multi-user sync (C++ WebSocket, server-authoritative)

## Quick Start

Needs **Node.js** (for the React UI) and **g++** (MinGW on Windows).

```bash
npm install
npm run build:backend
npm run dev
```

- UI: http://localhost:5173
- Backend: ws://localhost:3001

## Structure

```
packages/
  backend/   C++ engine + WebSocket server
  client/    React UI
scripts/     Launches the C++ server
```

## Collaboration

1. `npm run dev`
2. Open http://localhost:5173 in two tabs
3. Edit cells — updates sync through the C++ backend
