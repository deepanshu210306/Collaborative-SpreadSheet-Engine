export class MemoryPool {
  constructor(factory, initialSize = 64) {
    this.factory = factory;
    this.pool = [];
    this.active = 0;
    for (let i = 0; i < initialSize; i++) this.pool.push(factory());
  }

  acquire() {
    this.active++;
    return this.pool.length > 0 ? this.pool.pop() : this.factory();
  }

  release(obj) {
    this.active--;
    this.pool.push(obj);
  }

  get stats() {
    return { pooled: this.pool.length, active: this.active };
  }
}

export const nodePool = new MemoryPool(() => ({
  id: '',
  raw: '',
  value: null,
  error: null,
  deps: [],
  dependents: new Set(),
}));
