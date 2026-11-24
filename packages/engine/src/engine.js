import { DependencyGraph } from './graph.js';
import { getAffectedOrder } from './topoSort.js';
import { parseFormula, evaluateFormula } from './formulaParser.js';

export class SpreadsheetEngine {
  constructor(rows = 100, cols = 26) {
    this.rows = rows;
    this.cols = cols;
    this.graph = new DependencyGraph();
    this.cells = new Map();
    this.version = 0;
  }

  getCell(id) {
    return this.cells.get(id) ?? { raw: '', value: null, error: null };
  }

  getValue(id) {
    const cell = this.getCell(id);
    if (cell.error) return cell.error;
    return cell.value ?? '';
  }

  getDisplayValue(id) {
    const cell = this.getCell(id);
    if (cell.error) return cell.error;
    if (cell.value === null || cell.value === '') return '';
    return String(cell.value);
  }

  setCell(id, raw) {
    const parsed = parseFormula(raw);

    if (parsed.type === 'formula') {
      this.graph.setDependencies(id, parsed.refs);
      if (this.graph.hasCycle(id)) {
        this.cells.set(id, { raw, value: null, error: '#CYCLE!' });
        this.version++;
        return { success: false, error: '#CYCLE!', affected: [id] };
      }
    } else {
      this.graph.setDependencies(id, []);
    }

    this.cells.set(id, {
      raw,
      value: parsed.type === 'literal' ? raw : null,
      error: null,
    });

    const { order, hasCycle, cycleNodes } = getAffectedOrder(this.graph, id);

    if (hasCycle) {
      for (const cid of cycleNodes) {
        const c = this.cells.get(cid);
        if (c) {
          c.error = '#CYCLE!';
          c.value = null;
        }
      }
      this.version++;
      return { success: false, error: '#CYCLE!', affected: [...cycleNodes] };
    }

    for (const cid of order) this._evaluateCell(cid);
    this.version++;
    return { success: true, affected: order };
  }

  _evaluateCell(id) {
    const cell = this.cells.get(id);
    if (!cell) return;
    const parsed = parseFormula(cell.raw);
    if (parsed.type === 'literal') {
      cell.value = cell.raw;
      cell.error = null;
      return;
    }
    try {
      cell.value = evaluateFormula(parsed.expr, (ref) => this.getValue(ref));
      cell.error = null;
    } catch (e) {
      cell.value = null;
      cell.error = e.message || '#ERROR!';
    }
  }

  getSnapshot() {
    const data = {};
    for (const [id, cell] of this.cells) {
      data[id] = { raw: cell.raw, value: cell.value, error: cell.error };
    }
    return { version: this.version, cells: data };
  }

  loadSnapshot(snapshot) {
    this.cells.clear();
    this.graph = new DependencyGraph();
    for (const [id, cell] of Object.entries(snapshot.cells || {})) {
      this.cells.set(id, { ...cell });
      const parsed = parseFormula(cell.raw);
      if (parsed.type === 'formula') this.graph.setDependencies(id, parsed.refs);
    }
    for (const id of this.cells.keys()) this._evaluateCell(id);
    this.version = snapshot.version || 0;
  }
}
