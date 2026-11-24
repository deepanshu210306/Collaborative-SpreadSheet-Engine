import { useEffect, useRef, useCallback, useState } from 'react';

const WS_URL = `ws://${window.location.hostname}:3001`;

export function useCollaboration({ onMessage, onConnect, onDisconnect }) {
  const wsRef = useRef(null);
  const [clientId, setClientId] = useState(null);
  const reconnectTimer = useRef(null);
  const callbacks = useRef({ onMessage, onConnect, onDisconnect });
  callbacks.current = { onMessage, onConnect, onDisconnect };

  const sendUpdate = useCallback((msg) => {
    if (wsRef.current?.readyState === WebSocket.OPEN) {
      wsRef.current.send(JSON.stringify(msg));
    }
  }, []);

  useEffect(() => {
    const connect = () => {
      const ws = new WebSocket(WS_URL);
      wsRef.current = ws;

      ws.onopen = () => {
        callbacks.current.onConnect?.();
        if (reconnectTimer.current) {
          clearTimeout(reconnectTimer.current);
          reconnectTimer.current = null;
        }
      };

      ws.onmessage = (event) => {
        const msg = JSON.parse(event.data);
        if (msg.type === 'init') {
          setClientId(msg.clientId);
        }
        callbacks.current.onMessage?.(msg);
      };

      ws.onclose = () => {
        callbacks.current.onDisconnect?.();
        reconnectTimer.current = setTimeout(connect, 2000);
      };

      ws.onerror = () => ws.close();
    };

    connect();
    return () => {
      if (reconnectTimer.current) clearTimeout(reconnectTimer.current);
      wsRef.current?.close();
    };
  }, []);

  return { sendUpdate, clientId };
}
