import { indexToCol } from '../utils.js';
import Cell from './Cell.jsx';

export default function Grid({ rows, cols, cells, selected, onSelect, onChange, getDisplay }) {
  const colHeaders = Array.from({ length: cols }, (_, i) => indexToCol(i));

  return (
    <div className="grid-container">
      <table className="grid">
        <thead>
          <tr>
            <th className="corner" />
            {colHeaders.map((col) => (
              <th key={col} className="col-header">{col}</th>
            ))}
          </tr>
        </thead>
        <tbody>
          {Array.from({ length: rows }, (_, row) => (
            <tr key={row}>
              <td className="row-header">{row + 1}</td>
              {colHeaders.map((col) => {
                const id = `${col}${row + 1}`;
                return (
                  <Cell
                    key={id}
                    id={id}
                    raw={cells[id]?.raw ?? ''}
                    display={getDisplay(id)}
                    isSelected={selected === id}
                    hasError={!!cells[id]?.error}
                    onSelect={() => onSelect(id)}
                    onChange={(val) => onChange(id, val)}
                  />
                );
              })}
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  );
}
