export default function StatusBar({ cellId, value, cellCount, version }) {
  return (
    <div className="status-bar">
      <span>Cell: {cellId}</span>
      <span>
        {value.error
          ? `Error: ${value.error}`
          : value.raw?.startsWith('=')
            ? `Formula: ${value.raw}`
            : `Value: ${value.value ?? ''}`}
      </span>
      <span>Cells: {cellCount}</span>
      <span>v{version}</span>
    </div>
  );
}
