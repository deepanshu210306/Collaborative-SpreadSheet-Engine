#pragma once
#include <string>
#include <vector>

struct ParsedFormula {
    bool isFormula;
    std::string expr;
    std::string literal;
    std::vector<std::string> refs;
};

ParsedFormula parseFormula(const std::string& raw);
double evaluateFormula(const std::string& expr,
                       double (*getValue)(const std::string&, void*),
                       void* ctx,
                       std::string& errorOut);
