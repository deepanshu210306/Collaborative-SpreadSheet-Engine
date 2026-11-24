export class DependencyGraph {
  constructor() {
    this.nodes = new Map();
  }

  addNode(id) {
    if (!this.nodes.has(id)) {
      this.nodes.set(id, { id, deps: new Set(), dependents: new Set() });
    }
    return this.nodes.get(id);
  }

  setDependencies(id, deps) {
    const node = this.addNode(id);
    const newDeps = new Set(deps);

    for (const oldDep of node.deps) {
      if (!newDeps.has(oldDep)) this.nodes.get(oldDep)?.dependents.delete(id);
    }
    for (const dep of newDeps) {
      this.addNode(dep);
      this.nodes.get(dep).dependents.add(id);
    }
    node.deps = newDeps;
  }

  getDependents(id) {
    return this.nodes.get(id)?.dependents ?? new Set();
  }

  getDependencies(id) {
    return this.nodes.get(id)?.deps ?? new Set();
  }

  getAllDependents(id) {
    const visited = new Set();
    const queue = [id];
    while (queue.length > 0) {
      const current = queue.shift();
      const node = this.nodes.get(current);
      if (!node) continue;
      for (const dep of node.dependents) {
        if (!visited.has(dep)) {
          visited.add(dep);
          queue.push(dep);
        }
      }
    }
    visited.delete(id);
    return visited;
  }

  hasCycle(startId) {
    const visited = new Set();
    const stack = new Set();
    const dfs = (id) => {
      if (stack.has(id)) return true;
      if (visited.has(id)) return false;
      visited.add(id);
      stack.add(id);
      const node = this.nodes.get(id);
      if (node) {
        for (const dep of node.deps) {
          if (dfs(dep)) return true;
        }
      }
      stack.delete(id);
      return false;
    };
    return dfs(startId);
  }
}
