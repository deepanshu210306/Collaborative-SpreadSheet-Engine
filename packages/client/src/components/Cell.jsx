import { useState, useRef, useEffect } from 'react';

export default function Cell({ id, raw, display, isSelected, hasError, onSelect, onChange }) {
  const [editing, setEditing] = useState(false);
  const [editValue, setEditValue] = useState(raw);
  const inputRef = useRef(null);

  useEffect(() => {
    if (editing && inputRef.current) {
      inputRef.current.focus();
      inputRef.current.select();
    }
  }, [editing]);

  useEffect(() => {
    if (!editing) setEditValue(raw);
  }, [raw, editing]);

  const commit = () => {
    setEditing(false);
    if (editValue !== raw) {
      onChange(editValue);
    }
  };

  const handleKeyDown = (e) => {
    if (e.key === 'Enter') {
      commit();
    } else if (e.key === 'Escape') {
      setEditValue(raw);
      setEditing(false);
    } else if (e.key === 'Tab') {
      e.preventDefault();
      commit();
    }
  };

  if (editing) {
    return (
      <td className={`cell editing ${isSelected ? 'selected' : ''}`}>
        <input
          ref={inputRef}
          value={editValue}
          onChange={(e) => setEditValue(e.target.value)}
          onBlur={commit}
          onKeyDown={handleKeyDown}
        />
      </td>
    );
  }

  return (
    <td
      className={`cell ${isSelected ? 'selected' : ''} ${hasError ? 'error' : ''}`}
      onClick={() => { onSelect(); setEditing(true); }}
      onDoubleClick={() => setEditing(true)}
    >
      {display}
    </td>
  );
}
