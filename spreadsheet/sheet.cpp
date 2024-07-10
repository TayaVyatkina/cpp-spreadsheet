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

    auto cell = GetConcreteCell(pos);
    if (cell) {
        if (cell->GetText() == text) { return; }
        cell->Set(text, pos);
    }
    else {
        auto new_cell = std::make_unique<Cell>(*this);
        new_cell->Set(text, pos);
        sheet_.at(pos.row).at(pos.col) = std::move(new_cell);
    }
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

Cell* Sheet::GetConcreteCell(Position pos) const {
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
    if (IsCell(pos) && sheet_.at(pos.row).at(pos.col)) {
        if (sheet_.at(pos.row).at(pos.col).get()->GetDependentCells().empty()) {
            sheet_.at(pos.row).at(pos.col).reset();
        }
        else {
            // clear cache of dependent cells
            for (const auto& cell : sheet_.at(pos.row).at(pos.col).get()->GetDependentCells()) {
                cell->InvalidateCache();
            }
            // set EmptyImpl
            sheet_.at(pos.row).at(pos.col).get()->Clear();
        }
        
        
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


bool Sheet::IsCell(Position pos) const {
    return (pos.row < static_cast<int>(sheet_.size())) && (pos.col < static_cast<int>(sheet_.at(pos.row).size()));
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
