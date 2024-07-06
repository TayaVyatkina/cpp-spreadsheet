#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>


void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position!");
    }
    if (static_cast<int>(sheet_.size()) < (pos.row + 1)) {
        sheet_.reserve(pos.row + 1);
        sheet_.resize(pos.row + 1);
    }
    if (static_cast<int>(sheet_.at(pos.row).size()) < (pos.col + 1)) {
        sheet_.at(pos.row).reserve(pos.col + 1);
        sheet_.at(pos.row).resize(pos.col + 1);
    }
    auto cell = GetCell(pos);

    if (cell) {
        std::string old_text = cell->GetText();
        InvalidateCell(pos);
        DeleteDependencies(pos);
        dynamic_cast<Cell*>(cell)->Clear();
        dynamic_cast<Cell*>(cell)->Set(text);
        if (dynamic_cast<Cell*>(cell)->CheckCyclicDependencies(dynamic_cast<Cell*>(cell), pos)) {
            dynamic_cast<Cell*>(cell)->Set(std::move(old_text));
            throw CircularDependencyException("Cycle dependency!");
        }
        for (const auto& ref_cell : dynamic_cast<Cell*>(cell)->GetReferencedCells()) {
            AddDependentCell(ref_cell, pos);
        }
    }
    else {
        auto new_cell = std::make_unique<Cell>(*this);
        new_cell->Set(text);

        if (new_cell.get()->CheckCyclicDependencies(new_cell.get(), pos)) {
            throw CircularDependencyException("Cycle dependency!");
        }
        for (const auto& ref_cell : new_cell.get()->GetReferencedCells()) {
            AddDependentCell(ref_cell, pos);
        }
        sheet_.at(pos.row).at(pos.col) = std::move(new_cell);
    }
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position!");
    }

    if (IsCell(pos)) {
        if (sheet_.at(pos.row).at(pos.col)) {
            return sheet_.at(pos.row).at(pos.col).get();
        }
    }
    return nullptr;
}

CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position!");
    }

    if (IsCell(pos)) {
        if (sheet_.at(pos.row).at(pos.col)) {
            return sheet_.at(pos.row).at(pos.col).get();
        }
    }
    return nullptr;
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position!");
    }
    if (IsCell(pos)) {
        sheet_.at(pos.row).at(pos.col).reset();
    }
}

Size Sheet::GetPrintableSize() const {
    Size res;
    for (int row = 0; row < static_cast<int>(sheet_.size()); ++row) {
        for (int col = static_cast<int>(sheet_[row].size() - 1); col >= 0; --col) {
            if (sheet_[row][col]) {
                if (!sheet_[row][col]->GetText().empty()) {
                    res.rows = std::max(res.rows, row + 1);
                    res.cols = std::max(res.cols, col + 1);
                    break;
                }
            }
        }
    }
    return res;
}

void Sheet::PrintValues(std::ostream& output) const {
    Size size = GetPrintableSize();
    for (int x = 0; x < size.rows; ++x) {
        bool is_need_separator = false;
        for (int y = 0; y < size.cols; ++y) {
            if (is_need_separator) {
                output << '\t';
            }
            is_need_separator = true;
            if ((y < static_cast<int>(sheet_.at(x).size())) && sheet_.at(x).at(y)) {
                auto value = sheet_.at(x).at(y)->GetValue();
                if (std::holds_alternative<std::string>(value)) {
                    output << std::get<std::string>(value);
                }
                if (std::holds_alternative<double>(value)) {
                    output << std::get<double>(value);
                }
                if (std::holds_alternative<FormulaError>(value)) {
                    output << std::get<FormulaError>(value);
                }
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    Size size = GetPrintableSize();
    for (int x = 0; x < size.rows; ++x) {
        bool need_separator = false;
        for (int y = 0; y < size.cols; ++y) {
            if (need_separator) {
                output << '\t';
            }
            need_separator = true;
            if ((y < static_cast<int>(sheet_.at(x).size())) && sheet_.at(x).at(y)) {
                output << sheet_.at(x).at(y)->GetText();
            }
        }
        output << '\n';
    }
}

void Sheet::InvalidateCell(const Position& pos) {
    for (const auto& dependent_cell : GetDependentCells(pos)) {
        auto cell = GetCell(dependent_cell);
        dynamic_cast<Cell*>(cell)->InvalidateCache();
        InvalidateCell(dependent_cell);
    }
}

void Sheet::AddDependentCell(const Position& main_cell, const Position& dependent_cell) {
    cells_dependencies_[main_cell].insert(dependent_cell);
}

const std::set<Position> Sheet::GetDependentCells(const Position& pos) {
    if (cells_dependencies_.count(pos) != 0) {
        return cells_dependencies_.at(pos);
    }
    return {};
}

void Sheet::DeleteDependencies(const Position& pos) {
    cells_dependencies_.erase(pos);
}

bool Sheet::IsCell(Position pos) const {
    return (pos.row < static_cast<int>(sheet_.size())) && (pos.col < static_cast<int>(sheet_.at(pos.row).size()));
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
