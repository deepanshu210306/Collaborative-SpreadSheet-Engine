import { useState, useCallback } from 'react';
import { useCollaboration } from './hooks/useCollaboration.js';
import Grid from './components/Grid.jsx';
import FormulaBar from './components/FormulaBar.jsx';
import StatusBar from './components/StatusBar.jsx';

const ROWS = 50;
const COLS = 20;

export default function App() {
  const [cells, setCells] = useState({});
  const [selected, setSelected] = useState('A1');
  const [connected, setConnected] = useState(false);
  const [userCount, setUserCount] = useState(1);
  const [version, setVersion] = useState(0);

  const applySnapshot = useCallback((snapshot) => {
    if (!snapshot) return;
    setCells(snapshot.cells || {});
    if (typeof snapshot.version === 'number') setVersion(snapshot.version);
  }, []);

  const handleRemoteUpdate = useCallback(
    (msg) => {
      if (msg.type === 'cell_updated') {
        if (msg.cells) {
          setCells((prev) => ({ ...prev, ...msg.cells }));
        } else {
          setCells((prev) => ({
            ...prev,
            [msg.cellId]: {
              raw: msg.value,
              value: msg.result?.error ? null : (msg.display ?? msg.value),
              error: msg.result?.error || null,
            },
          }));
        }
        if (typeof msg.version === 'number') setVersion(msg.version);
      } else if (msg.type === 'init' || msg.type === 'sync') {
        applySnapshot(msg.snapshot);
        if (msg.users != null) setUserCount(msg.users);
      } else if (msg.type === 'users') {
        setUserCount(msg.count);
      }
    },
    [applySnapshot]
  );

  const { sendUpdate, clientId } = useCollaboration({
    onMessage: handleRemoteUpdate,
    onConnect: () => setConnected(true),
    onDisconnect: () => setConnected(false),
  });

  const handleCellChange = useCallback(
    (cellId, value) => {
      setCells((prev) => ({
        ...prev,
        [cellId]: { ...(prev[cellId] || {}), raw: value, value, error: null },
      }));
      sendUpdate({ type: 'cell_update', cellId, value });
    },
    [sendUpdate]
  );

  const selectedCell = cells[selected] || { raw: '', value: null, error: null };

  return (
    <div className="app">
      <header className="header">
        <h1>Collaborative Spreadsheet Engine</h1>
        <div className="header-meta">
          <span className={`status ${connected ? 'connected' : 'disconnected'}`}>
            {connected ? '● Connected' : '○ Offline'}
          </span>
          <span className="users">{userCount} user{userCount !== 1 ? 's' : ''} online</span>
          <span className="client-id">{clientId}</span>
        </div>
      </header>

      <FormulaBar
        cellId={selected}
        value={selectedCell.raw || ''}
        onChange={(val) => handleCellChange(selected, val)}
      />

      <Grid
        rows={ROWS}
        cols={COLS}
        cells={cells}
        selected={selected}
        onSelect={setSelected}
        onChange={handleCellChange}
        getDisplay={(id) => {
          const c = cells[id];
          if (!c) return '';
          if (c.error) return c.error;
          return c.value ?? '';
        }}
      />

      <StatusBar
        cellId={selected}
        value={selectedCell}
        cellCount={Object.keys(cells).length}
        version={version}
      />
    </div>
  );
}
