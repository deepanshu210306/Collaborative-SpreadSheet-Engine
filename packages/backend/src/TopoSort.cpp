#include "TopoSort.hpp"
#include <queue>
#include <map>

TopoResult topologicalSort(const DependencyGraph& graph,
                           const std::set<std::string>* startNodes) {
    TopoResult result;
    result.hasCycle = false;

    std::set<std::string> nodes;
    std::map<std::string, int> inDegree;

    if (startNodes) {
        for (std::set<std::string>::const_iterator it = startNodes->begin();
             it != startNodes->end(); ++it) {
            nodes.insert(*it);
            std::set<std::string> all = graph.getAllDependents(*it);
            nodes.insert(all.begin(), all.end());
        }
    } else {
        const std::map<std::string, std::set<std::string> >& all = graph.nodes();
        for (std::map<std::string, std::set<std::string> >::const_iterator it = all.begin();
             it != all.end(); ++it) {
            nodes.insert(it->first);
        }
    }

    for (std::set<std::string>::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        inDegree[*it] = 0;
    }

    for (std::set<std::string>::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        const std::set<std::string>& deps = graph.getDependencies(*it);
        for (std::set<std::string>::const_iterator d = deps.begin(); d != deps.end(); ++d) {
            if (nodes.count(*d)) {
                inDegree[*it] += 1;
            }
        }
    }

    std::queue<std::string> q;
    for (std::map<std::string, int>::const_iterator it = inDegree.begin();
         it != inDegree.end(); ++it) {
        if (it->second == 0) q.push(it->first);
    }

    while (!q.empty()) {
        std::string id = q.front();
        q.pop();
        result.order.push_back(id);

        const std::set<std::string>& dependents = graph.getDependents(id);
        for (std::set<std::string>::const_iterator it = dependents.begin();
             it != dependents.end(); ++it) {
            if (!nodes.count(*it)) continue;
            inDegree[*it] -= 1;
            if (inDegree[*it] == 0) q.push(*it);
        }
    }

    result.hasCycle = result.order.size() != nodes.size();
    if (result.hasCycle) {
        for (std::set<std::string>::const_iterator it = nodes.begin();
             it != nodes.end(); ++it) {
            bool found = false;
            for (std::size_t i = 0; i < result.order.size(); ++i) {
                if (result.order[i] == *it) { found = true; break; }
            }
            if (!found) result.cycleNodes.push_back(*it);
        }
    }

    return result;
}

TopoResult getAffectedOrder(const DependencyGraph& graph, const std::string& changedId) {
    std::set<std::string> affected = graph.getAllDependents(changedId);
    affected.insert(changedId);
    return topologicalSort(graph, &affected);
}
