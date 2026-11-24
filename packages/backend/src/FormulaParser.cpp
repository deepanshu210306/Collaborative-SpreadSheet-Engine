#include "FormulaParser.hpp"
#include <cctype>
#include <cstdlib>
#include <cmath>
#include <sstream>
#include <set>
#include <algorithm>

namespace {

std::string toUpper(std::string s) {
    for (std::size_t i = 0; i < s.size(); ++i) {
        s[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[i])));
    }
    return s;
}

bool isCellRef(const std::string& s, std::size_t pos, std::size_t& end) {
    if (pos >= s.size() || !std::isalpha(static_cast<unsigned char>(s[pos]))) return false;
    std::size_t i = pos;
    while (i < s.size() && std::isalpha(static_cast<unsigned char>(s[i]))) ++i;
    if (i == pos || i >= s.size() || !std::isdigit(static_cast<unsigned char>(s[i]))) return false;
    while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) ++i;
    end = i;
    return true;
}

int colToIndex(const std::string& col) {
    int index = 0;
    for (std::size_t i = 0; i < col.size(); ++i) {
        index = index * 26 + (std::toupper(static_cast<unsigned char>(col[i])) - 'A' + 1);
    }
    return index - 1;
}

std::string indexToCol(int index) {
    std::string col;
    int n = index + 1;
    while (n > 0) {
        int rem = (n - 1) % 26;
        col = static_cast<char>('A' + rem) + col;
        n = (n - 1) / 26;
    }
    return col;
}

bool parseCell(const std::string& ref, int& col, int& row) {
    std::size_t i = 0;
    while (i < ref.size() && std::isalpha(static_cast<unsigned char>(ref[i]))) ++i;
    if (i == 0 || i >= ref.size()) return false;
    col = colToIndex(ref.substr(0, i));
    row = std::atoi(ref.c_str() + i) - 1;
    return row >= 0;
}

std::vector<std::string> expandRange(const std::string& start, const std::string& end) {
    std::vector<std::string> cells;
    int sc, sr, ec, er;
    if (!parseCell(start, sc, sr) || !parseCell(end, ec, er)) return cells;
    int minC = std::min(sc, ec), maxC = std::max(sc, ec);
    int minR = std::min(sr, er), maxR = std::max(sr, er);
    for (int r = minR; r <= maxR; ++r) {
        for (int c = minC; c <= maxC; ++c) {
            std::ostringstream oss;
            oss << indexToCol(c) << (r + 1);
            cells.push_back(oss.str());
        }
    }
    return cells;
}

std::vector<std::string> extractRefs(const std::string& expr) {
    std::set<std::string> refs;
    std::string upper = toUpper(expr);

    for (std::size_t i = 0; i < upper.size(); ++i) {
        std::size_t end = 0;
        if (!isCellRef(upper, i, end)) continue;

        std::string ref = upper.substr(i, end - i);
        bool partOfRange = false;
        if (end < upper.size() && upper[end] == ':') {
            std::size_t end2 = 0;
            if (isCellRef(upper, end + 1, end2)) {
                std::vector<std::string> range = expandRange(ref, upper.substr(end + 1, end2 - end - 1));
                for (std::size_t k = 0; k < range.size(); ++k) refs.insert(range[k]);
                i = end2 - 1;
                partOfRange = true;
            }
        }
        if (!partOfRange) {
            if (i > 0 && upper[i - 1] == ':') {
                // already handled as range end
            } else {
                refs.insert(ref);
            }
            i = end - 1;
        }
    }

    return std::vector<std::string>(refs.begin(), refs.end());
}

// Recursive descent expression evaluator after cell refs substituted
struct ExprEval {
    std::string s;
    std::size_t pos;
    std::string err;

    void skip() {
        while (pos < s.size() && std::isspace(static_cast<unsigned char>(s[pos]))) ++pos;
    }

    double parseExpr() {
        double v = parseTerm();
        skip();
        while (pos < s.size() && (s[pos] == '+' || s[pos] == '-')) {
            char op = s[pos++];
            double r = parseTerm();
            if (op == '+') v += r; else v -= r;
            skip();
        }
        return v;
    }

    double parseTerm() {
        double v = parseFactor();
        skip();
        while (pos < s.size() && (s[pos] == '*' || s[pos] == '/')) {
            char op = s[pos++];
            double r = parseFactor();
            if (op == '*') v *= r;
            else {
                if (r == 0.0) { err = "#DIV/0!"; return 0; }
                v /= r;
            }
            skip();
        }
        return v;
    }

    double parseFactor() {
        skip();
        if (pos < s.size() && s[pos] == '+') { ++pos; return parseFactor(); }
        if (pos < s.size() && s[pos] == '-') { ++pos; return -parseFactor(); }
        if (pos < s.size() && s[pos] == '(') {
            ++pos;
            double v = parseExpr();
            skip();
            if (pos < s.size() && s[pos] == ')') ++pos;
            else err = "#ERROR!";
            return v;
        }
        return parseNumber();
    }

    double parseNumber() {
        skip();
        std::size_t start = pos;
        while (pos < s.size() && (std::isdigit(static_cast<unsigned char>(s[pos])) || s[pos] == '.')) ++pos;
        if (start == pos) { err = "#VALUE!"; return 0; }
        return std::atof(s.substr(start, pos - start).c_str());
    }
};

std::string replaceFunc(const std::string& expr, const std::string& name,
                        double (*reduce)(const std::vector<double>&)) {
    std::string result;
    std::string upper = toUpper(expr);
    std::size_t i = 0;
    while (i < expr.size()) {
        if (upper.compare(i, name.size(), name) == 0 &&
            i + name.size() < expr.size() && expr[i + name.size()] == '(') {
            std::size_t j = i + name.size() + 1;
            int depth = 1;
            std::size_t start = j;
            while (j < expr.size() && depth > 0) {
                if (expr[j] == '(') ++depth;
                else if (expr[j] == ')') --depth;
                ++j;
            }
            std::string inner = expr.substr(start, j - start - 1);
            std::vector<double> nums;
            std::stringstream ss(inner);
            std::string part;
            while (std::getline(ss, part, ',')) {
                nums.push_back(std::atof(part.c_str()));
            }
            std::ostringstream oss;
            oss << reduce(nums);
            result += oss.str();
            i = j;
        } else {
            result += expr[i];
            ++i;
        }
    }
    return result;
}

double sumFn(const std::vector<double>& v) {
    double s = 0;
    for (std::size_t i = 0; i < v.size(); ++i) s += v[i];
    return s;
}

double avgFn(const std::vector<double>& v) {
    if (v.empty()) return 0;
    return sumFn(v) / static_cast<double>(v.size());
}

double minFn(const std::vector<double>& v) {
    if (v.empty()) return 0;
    double m = v[0];
    for (std::size_t i = 1; i < v.size(); ++i) if (v[i] < m) m = v[i];
    return m;
}

double maxFn(const std::vector<double>& v) {
    if (v.empty()) return 0;
    double m = v[0];
    for (std::size_t i = 1; i < v.size(); ++i) if (v[i] > m) m = v[i];
    return m;
}

} // namespace

ParsedFormula parseFormula(const std::string& raw) {
    ParsedFormula p;
    if (raw.empty() || raw[0] != '=') {
        p.isFormula = false;
        p.literal = raw;
        return p;
    }
    p.isFormula = true;
    p.expr = raw.substr(1);
    while (!p.expr.empty() && std::isspace(static_cast<unsigned char>(p.expr[0]))) {
        p.expr.erase(p.expr.begin());
    }
    p.refs = extractRefs(p.expr);
    return p;
}

double evaluateFormula(const std::string& expr,
                       double (*getValue)(const std::string&, void*),
                       void* ctx,
                       std::string& errorOut) {
    errorOut.clear();
    std::string upper = toUpper(expr);
    std::string substituted;
    substituted.reserve(expr.size() * 2);

    for (std::size_t i = 0; i < upper.size();) {
        std::size_t end = 0;
        if (isCellRef(upper, i, end)) {
            std::string ref = upper.substr(i, end - i);
            double val = getValue(ref, ctx);
            std::ostringstream oss;
            oss << val;
            substituted += oss.str();
            i = end;
        } else {
            substituted += expr[i];
            ++i;
        }
    }

    substituted = replaceFunc(substituted, "SUM", sumFn);
    substituted = replaceFunc(substituted, "AVG", avgFn);
    substituted = replaceFunc(substituted, "MIN", minFn);
    substituted = replaceFunc(substituted, "MAX", maxFn);

    ExprEval ev;
    ev.s = substituted;
    ev.pos = 0;
    double result = ev.parseExpr();
    if (!ev.err.empty()) {
        errorOut = ev.err;
        return 0;
    }
    if (result != result) { // NaN
        errorOut = "#VALUE!";
        return 0;
    }
    return result;
}
