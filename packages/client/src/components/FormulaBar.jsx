export default function FormulaBar({ cellId, value, onChange }) {
  return (
    <div className="formula-bar">
      <span className="cell-ref">{cellId}</span>
      <span className="fx">fx</span>
      <input
        className="formula-input"
        value={value}
        onChange={(e) => onChange(e.target.value)}
        onKeyDown={(e) => {
          if (e.key === 'Enter') e.target.blur();
        }}
        placeholder="Enter value or formula (e.g. =A1+B1)"
      />
    </div>
  );
}
