export function topologicalSort(graph, startNodes = null) {
  const inDegree = new Map();
  const nodes = new Set();

  const collect = (id) => {
    if (nodes.has(id)) return;
    nodes.add(id);
    inDegree.set(id, 0);
  };

  if (startNodes) {
    for (const id of startNodes) {
      collect(id);
      for (const dep of graph.getAllDependents(id)) collect(dep);
    }
  } else {
    for (const id of graph.nodes.keys()) collect(id);
  }

  for (const id of nodes) {
    for (const dep of graph.getDependencies(id)) {
      if (nodes.has(dep)) inDegree.set(id, (inDegree.get(id) || 0) + 1);
    }
  }

  const queue = [];
  for (const [id, degree] of inDegree) {
    if (degree === 0) queue.push(id);
  }

  const order = [];
  while (queue.length > 0) {
    const id = queue.shift();
    order.push(id);
    for (const dependent of graph.getDependents(id)) {
      if (!nodes.has(dependent)) continue;
      const newDegree = inDegree.get(dependent) - 1;
      inDegree.set(dependent, newDegree);
      if (newDegree === 0) queue.push(dependent);
    }
  }

  const hasCycle = order.length !== nodes.size;
  const cycleNodes = hasCycle ? [...nodes].filter((id) => !order.includes(id)) : [];
  return { order, hasCycle, cycleNodes };
}

export function getAffectedOrder(graph, changedId) {
  const affected = graph.getAllDependents(changedId);
  affected.add(changedId);
  return topologicalSort(graph, affected);
}
