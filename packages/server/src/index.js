import { WebSocketServer } from 'ws';
import { SpreadsheetEngine } from '@spreadsheet/engine';
import { createServer } from 'http';
import { readFileSync, existsSync } from 'fs';
import { join, extname } from 'path';
import { fileURLToPath } from 'url';

const __dirname = fileURLToPath(new URL('.', import.meta.url));
const PORT = process.env.PORT || 3001;

const engine = new SpreadsheetEngine(100, 26);
const clients = new Map();
let clientCounter = 0;

const MIME = {
  '.html': 'text/html',
  '.js': 'application/javascript',
  '.css': 'text/css',
  '.json': 'application/json',
  '.svg': 'image/svg+xml',
};

const server = createServer((req, res) => {
  const distPath = join(__dirname, '../../client/dist');
  let filePath = join(distPath, req.url === '/' ? 'index.html' : req.url.split('?')[0]);

  if (!existsSync(filePath) || !filePath.startsWith(distPath)) {
    filePath = join(distPath, 'index.html');
  }

  if (existsSync(filePath)) {
    const ext = extname(filePath);
    res.writeHead(200, { 'Content-Type': MIME[ext] || 'text/plain' });
    res.end(readFileSync(filePath));
  } else {
    res.writeHead(404);
    res.end('Not found — run npm run build in client for production');
  }
});

const wss = new WebSocketServer({ server });

function broadcast(message) {
  const data = JSON.stringify(message);
  for (const [, ws] of clients) {
    if (ws.readyState === 1) ws.send(data);
  }
}

function cellsPayload(affected) {
  const cells = {};
  for (const id of affected) {
    const c = engine.getCell(id);
    cells[id] = { raw: c.raw, value: c.value, error: c.error };
  }
  return cells;
}

wss.on('connection', (ws) => {
  const clientId = `user-${++clientCounter}`;
  clients.set(clientId, ws);

  ws.send(JSON.stringify({
    type: 'init',
    clientId,
    snapshot: engine.getSnapshot(),
    users: clients.size,
  }));

  broadcast({ type: 'users', count: clients.size });

  ws.on('message', (raw) => {
    try {
      const msg = JSON.parse(raw.toString());

      if (msg.type === 'cell_update') {
        const { cellId, value } = msg;
        const result = engine.setCell(cellId, value);
        broadcast({
          type: 'cell_updated',
          cellId,
          value,
          display: engine.getDisplayValue(cellId),
          result,
          cells: cellsPayload(result.affected || [cellId]),
          version: engine.version,
          sender: clientId,
        });
      } else if (msg.type === 'sync_request') {
        ws.send(JSON.stringify({
          type: 'sync',
          snapshot: engine.getSnapshot(),
        }));
      }
    } catch (e) {
      ws.send(JSON.stringify({ type: 'error', message: e.message }));
    }
  });

  ws.on('close', () => {
    clients.delete(clientId);
    broadcast({ type: 'users', count: clients.size });
  });
});

server.listen(PORT, () => {
  console.log(`JS spreadsheet server on http://localhost:${PORT}`);
  console.log(`WebSocket on ws://localhost:${PORT}`);
});
