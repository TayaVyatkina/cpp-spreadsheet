#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
#include <set>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError err) {
    return output << err.ToString();
}

namespace {

class Formula : public FormulaInterface {
public:
// Реализуйте следующие методы:
    explicit Formula(std::string expression) 
        : ast_(ParseFormulaAST(std::move(expression))), 
          referenced_cells_(ast_.GetCells().begin(), ast_.GetCells().end())
    {}

    Value Evaluate(const SheetInterface& sheet) const override {
        try {
            return ast_.Execute(
                [&sheet](const Position& pos) {
                    // no cell
                    if (sheet.GetCell(pos) == nullptr)  {
                        return 0.;
                    }

                    auto value = sheet.GetCell(pos)->GetValue();

                    // as number
                    if (std::holds_alternative<double>(value)) {
                        return std::get<double>(value);
                    }
                    // as text
                    else if (std::holds_alternative<std::string>(value)) {
                        try {
                            std::string tmp = std::get<std::string>(value);
                            if (std::all_of(tmp.cbegin(), tmp.cend(), 
                                [](char ch) {
                                         return (std::isdigit(ch) || ch == '.');
                                })) {

                                return std::stod(tmp);
                            }
                            else {
                                throw FormulaError(FormulaError::Category::Value);
                               //return 0.;
                            }
                        }
                        catch (...)
                        {
                            throw FormulaError(FormulaError::Category::Value);
                        }
                    }
                    //CellInterface::FormulaError
                    else {
                        throw std::get<FormulaError>(value);
                    }
                    //return 0.;
                }
            );
        }
        catch (const FormulaError& err) {
            return err;
        }
    }

    std::string GetExpression() const override {
        std::stringstream ss;
        ast_.PrintFormula(ss);
        return ss.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        std::vector<Position> result;

        std::set<Position> unique_ref_cells(referenced_cells_.begin(), referenced_cells_.end());
        for (const auto& cell : unique_ref_cells)
        {
            result.push_back(cell);
        }
        return result;
    }

private:
    FormulaAST ast_;
    std::vector<Position> referenced_cells_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try {
        return std::make_unique<Formula>(std::move(expression));
    }
    catch (const std::exception&)
    {
        throw FormulaException("Formula parse error");
    }
}