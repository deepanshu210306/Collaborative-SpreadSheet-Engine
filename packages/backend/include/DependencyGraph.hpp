#pragma once
#include <string>
#include <map>
#include <set>
#include <vector>
#include <queue>

class DependencyGraph {
public:
    void addNode(const std::string& id);
    void removeNode(const std::string& id);
    void setDependencies(const std::string& id, const std::vector<std::string>& deps);

    const std::set<std::string>& getDependents(const std::string& id) const;
    const std::set<std::string>& getDependencies(const std::string& id) const;
    std::set<std::string> getAllDependents(const std::string& id) const;
    bool hasCycle(const std::string& startId) const;

    const std::map<std::string, std::set<std::string> >& nodes() const { return deps_; }
    std::size_t size() const { return deps_.size(); }

private:
    std::map<std::string, std::set<std::string> > deps_;
    std::map<std::string, std::set<std::string> > dependents_;
    static const std::set<std::string> empty_;
};
