export function indexToCol(index) {
  let col = '';
  let n = index + 1;
  while (n > 0) {
    const rem = (n - 1) % 26;
    col = String.fromCharCode(65 + rem) + col;
    n = Math.floor((n - 1) / 26);
  }
  return col;
}

export function colToIndex(col) {
  let index = 0;
  for (let i = 0; i < col.length; i++) {
    index = index * 26 + (col.toUpperCase().charCodeAt(i) - 64);
  }
  return index - 1;
}

export function parseCellRef(ref) {
  const match = ref.match(/^([A-Z]+)(\d+)$/i);
  if (!match) return null;
  return { col: match[1].toUpperCase(), row: parseInt(match[2], 10) - 1 };
}

export function cellId(col, row) {
  return `${indexToCol(col)}${row + 1}`;
}

function expandRange(start, end) {
  const s = parseCellRef(start);
  const e = parseCellRef(end);
  if (!s || !e) return [];
  const cells = [];
  const minCol = Math.min(colToIndex(s.col), colToIndex(e.col));
  const maxCol = Math.max(colToIndex(s.col), colToIndex(e.col));
  const minRow = Math.min(s.row, e.row);
  const maxRow = Math.max(s.row, e.row);
  for (let r = minRow; r <= maxRow; r++) {
    for (let c = minCol; c <= maxCol; c++) cells.push(cellId(c, r));
  }
  return cells;
}

function extractRefs(expr) {
  const refs = new Set();
  for (const m of expr.matchAll(/([A-Z]+\d+):([A-Z]+\d+)/gi)) {
    expandRange(m[1], m[2]).forEach((r) => refs.add(r));
  }
  for (const m of expr.matchAll(/\b([A-Z]+\d+)\b/gi)) {
    refs.add(m[1].toUpperCase());
  }
  return [...refs];
}

export function parseFormula(raw) {
  if (!raw || !raw.startsWith('=')) {
    return { type: 'literal', value: raw, refs: [] };
  }
  const expr = raw.slice(1).trim();
  return { type: 'formula', expr, refs: extractRefs(expr) };
}

export function evaluateFormula(expr, getValue) {
  try {
    let evalExpr = expr.replace(/\b([A-Z]+\d+)\b/gi, (match) => {
      const val = getValue(match.toUpperCase());
      if (val === null || val === undefined || val === '') return '0';
      if (typeof val === 'string' && val.startsWith('#')) throw new Error(val);
      const num = Number(val);
      if (Number.isNaN(num)) throw new Error('#VALUE!');
      return String(num);
    });

    const reduce = (name, fn) => {
      evalExpr = evalExpr.replace(new RegExp(`${name}\\(([^)]+)\\)`, 'gi'), (_, inner) => {
        const parts = inner.split(',').map((p) => Number(p.trim()) || 0);
        return String(fn(parts));
      });
    };

    reduce('SUM', (p) => p.reduce((a, b) => a + b, 0));
    reduce('AVG', (p) => p.reduce((a, b) => a + b, 0) / (p.length || 1));
    reduce('MIN', (p) => Math.min(...p));
    reduce('MAX', (p) => Math.max(...p));

    if (!/^[\d\s+\-*/().]+$/.test(evalExpr)) throw new Error('#VALUE!');
    const result = Function(`"use strict"; return (${evalExpr})`)();
    if (typeof result !== 'number' || !Number.isFinite(result)) throw new Error('#VALUE!');
    return result;
  } catch (e) {
    if (e.message?.startsWith('#')) throw e;
    throw new Error('#ERROR!');
  }
}
