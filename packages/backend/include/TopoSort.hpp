#pragma once
#include "DependencyGraph.hpp"
#include <string>
#include <vector>
#include <set>

struct TopoResult {
    std::vector<std::string> order;
    bool hasCycle;
    std::vector<std::string> cycleNodes;
};

TopoResult topologicalSort(const DependencyGraph& graph,
                           const std::set<std::string>* startNodes = 0);

TopoResult getAffectedOrder(const DependencyGraph& graph, const std::string& changedId);
