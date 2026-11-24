#include "SpreadsheetEngine.hpp"
#include "FormulaParser.hpp"
#include "TopoSort.hpp"
#include <sstream>
#include <cstdlib>
#include <cmath>

SpreadsheetEngine::SpreadsheetEngine(int rows, int cols)
    : rows_(rows), cols_(cols), version_(0) {}

CellData SpreadsheetEngine::getCell(const std::string& id) const {
    std::map<std::string, CellData>::const_iterator it = cells_.find(id);
    if (it == cells_.end()) {
        CellData empty;
        empty.hasValue = false;
        return empty;
    }
    return it->second;
}

std::string SpreadsheetEngine::getDisplayValue(const std::string& id) const {
    CellData c = getCell(id);
    if (!c.error.empty()) return c.error;
    if (!c.hasValue) return "";
    return c.value;
}

double SpreadsheetEngine::lookupValue(const std::string& ref, void* ctx) {
    SpreadsheetEngine* self = static_cast<SpreadsheetEngine*>(ctx);
    CellData c = self->getCell(ref);
    if (!c.error.empty()) return 0;
    if (!c.hasValue || c.value.empty()) return 0;
    return std::atof(c.value.c_str());
}

void SpreadsheetEngine::evaluateCell(const std::string& id) {
    std::map<std::string, CellData>::iterator it = cells_.find(id);
    if (it == cells_.end()) return;

    ParsedFormula parsed = parseFormula(it->second.raw);
    if (!parsed.isFormula) {
        it->second.value = it->second.raw;
        it->second.hasValue = true;
        it->second.error.clear();
        return;
    }

    std::string err;
    double val = evaluateFormula(parsed.expr, &SpreadsheetEngine::lookupValue, this, err);
    if (!err.empty()) {
        it->second.error = err;
        it->second.hasValue = false;
        it->second.value.clear();
    } else {
        std::ostringstream oss;
        if (std::floor(val) == val && std::fabs(val) < 1e15) {
            oss << static_cast<long long>(val);
        } else {
            oss << val;
        }
        it->second.value = oss.str();
        it->second.hasValue = true;
        it->second.error.clear();
    }
}

SetCellResult SpreadsheetEngine::setCell(const std::string& id, const std::string& raw) {
    SetCellResult result;
    result.success = true;

    ParsedFormula parsed = parseFormula(raw);

    if (parsed.isFormula) {
        graph_.setDependencies(id, parsed.refs);
        if (graph_.hasCycle(id)) {
            CellData cell;
            cell.raw = raw;
            cell.error = "#CYCLE!";
            cell.hasValue = false;
            cells_[id] = cell;
            ++version_;
            result.success = false;
            result.error = "#CYCLE!";
            result.affected.push_back(id);
            return result;
        }
    } else {
        graph_.setDependencies(id, std::vector<std::string>());
    }

    CellData cell;
    cell.raw = raw;
    cell.error.clear();
    if (!parsed.isFormula) {
        cell.value = raw;
        cell.hasValue = true;
    } else {
        cell.hasValue = false;
    }
    cells_[id] = cell;

    TopoResult topo = getAffectedOrder(graph_, id);

    if (topo.hasCycle) {
        for (std::size_t i = 0; i < topo.cycleNodes.size(); ++i) {
            CellData& c = cells_[topo.cycleNodes[i]];
            c.error = "#CYCLE!";
            c.hasValue = false;
            c.value.clear();
        }
        ++version_;
        result.success = false;
        result.error = "#CYCLE!";
        result.affected = topo.cycleNodes;
        return result;
    }

    for (std::size_t i = 0; i < topo.order.size(); ++i) {
        evaluateCell(topo.order[i]);
    }

    ++version_;
    result.affected = topo.order;
    return result;
}

static std::string escapeJson(const std::string& s) {
    std::string out;
    for (std::size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (c == '"' || c == '\\') { out += '\\'; out += c; }
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    return out;
}

std::string SpreadsheetEngine::getSnapshotJson() const {
    std::ostringstream oss;
    oss << "{\"version\":" << version_ << ",\"cells\":{";
    bool first = true;
    for (std::map<std::string, CellData>::const_iterator it = cells_.begin();
         it != cells_.end(); ++it) {
        if (!first) oss << ",";
        first = false;
        oss << "\"" << escapeJson(it->first) << "\":{"
            << "\"raw\":\"" << escapeJson(it->second.raw) << "\",";
        if (it->second.hasValue) {
            oss << "\"value\":\"" << escapeJson(it->second.value) << "\",";
        } else {
            oss << "\"value\":null,";
        }
        if (it->second.error.empty()) {
            oss << "\"error\":null";
        } else {
            oss << "\"error\":\"" << escapeJson(it->second.error) << "\"";
        }
        oss << "}";
    }
    oss << "}}";
    return oss.str();
}

void SpreadsheetEngine::loadSnapshot(const std::map<std::string, CellData>& cells, int version) {
    cells_ = cells;
    graph_ = DependencyGraph();
    for (std::map<std::string, CellData>::const_iterator it = cells_.begin();
         it != cells_.end(); ++it) {
        ParsedFormula p = parseFormula(it->second.raw);
        if (p.isFormula) graph_.setDependencies(it->first, p.refs);
    }
    for (std::map<std::string, CellData>::const_iterator it = cells_.begin();
         it != cells_.end(); ++it) {
        evaluateCell(it->first);
    }
    version_ = version;
}
