#pragma once
#include "DependencyGraph.hpp"
#include <string>
#include <map>
#include <vector>

struct CellData {
    std::string raw;
    std::string value;
    std::string error;
    bool hasValue;
};

struct SetCellResult {
    bool success;
    std::string error;
    std::vector<std::string> affected;
};

class SpreadsheetEngine {
public:
    SpreadsheetEngine(int rows = 100, int cols = 26);

    CellData getCell(const std::string& id) const;
    std::string getDisplayValue(const std::string& id) const;
    SetCellResult setCell(const std::string& id, const std::string& raw);

    std::string getSnapshotJson() const;
    void loadSnapshot(const std::map<std::string, CellData>& cells, int version);

    int version() const { return version_; }
    const DependencyGraph& graph() const { return graph_; }

private:
    void evaluateCell(const std::string& id);
    static double lookupValue(const std::string& ref, void* ctx);

    int rows_;
    int cols_;
    int version_;
    DependencyGraph graph_;
    std::map<std::string, CellData> cells_;
};
