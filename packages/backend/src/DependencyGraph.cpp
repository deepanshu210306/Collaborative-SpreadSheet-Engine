#include "DependencyGraph.hpp"

const std::set<std::string> DependencyGraph::empty_;

void DependencyGraph::addNode(const std::string& id) {
    if (deps_.find(id) == deps_.end()) {
        deps_[id] = std::set<std::string>();
        dependents_[id] = std::set<std::string>();
    }
}

void DependencyGraph::removeNode(const std::string& id) {
    if (deps_.find(id) == deps_.end()) return;

    for (std::set<std::string>::const_iterator it = deps_[id].begin();
         it != deps_[id].end(); ++it) {
        dependents_[*it].erase(id);
    }
    for (std::set<std::string>::const_iterator it = dependents_[id].begin();
         it != dependents_[id].end(); ++it) {
        deps_[*it].erase(id);
    }
    deps_.erase(id);
    dependents_.erase(id);
}

void DependencyGraph::setDependencies(const std::string& id,
                                      const std::vector<std::string>& deps) {
    addNode(id);
    std::set<std::string> newDeps(deps.begin(), deps.end());

    for (std::set<std::string>::const_iterator it = deps_[id].begin();
         it != deps_[id].end(); ++it) {
        if (newDeps.find(*it) == newDeps.end()) {
            dependents_[*it].erase(id);
        }
    }

    for (std::set<std::string>::const_iterator it = newDeps.begin();
         it != newDeps.end(); ++it) {
        addNode(*it);
        dependents_[*it].insert(id);
    }

    deps_[id] = newDeps;
}

const std::set<std::string>& DependencyGraph::getDependents(const std::string& id) const {
    std::map<std::string, std::set<std::string> >::const_iterator it = dependents_.find(id);
    if (it == dependents_.end()) return empty_;
    return it->second;
}

const std::set<std::string>& DependencyGraph::getDependencies(const std::string& id) const {
    std::map<std::string, std::set<std::string> >::const_iterator it = deps_.find(id);
    if (it == deps_.end()) return empty_;
    return it->second;
}

std::set<std::string> DependencyGraph::getAllDependents(const std::string& id) const {
    std::set<std::string> visited;
    std::queue<std::string> q;
    q.push(id);

    while (!q.empty()) {
        std::string current = q.front();
        q.pop();
        const std::set<std::string>& deps = getDependents(current);
        for (std::set<std::string>::const_iterator it = deps.begin();
             it != deps.end(); ++it) {
            if (visited.insert(*it).second) {
                q.push(*it);
            }
        }
    }
    visited.erase(id);
    return visited;
}

bool DependencyGraph::hasCycle(const std::string& startId) const {
    std::set<std::string> visited;
    std::set<std::string> stack;

    struct Dfs {
        const DependencyGraph* g;
        std::set<std::string>* visited;
        std::set<std::string>* stack;

        bool run(const std::string& id) {
            if (stack->count(id)) return true;
            if (visited->count(id)) return false;
            visited->insert(id);
            stack->insert(id);
            const std::set<std::string>& deps = g->getDependencies(id);
            for (std::set<std::string>::const_iterator it = deps.begin();
                 it != deps.end(); ++it) {
                if (run(*it)) return true;
            }
            stack->erase(id);
            return false;
        }
    };

    Dfs dfs;
    dfs.g = this;
    dfs.visited = &visited;
    dfs.stack = &stack;
    return dfs.run(startId);
}
