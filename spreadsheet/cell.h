#pragma once

#include "common.h"
#include "formula.h"

#include <optional>
#include <functional>
#include <unordered_set>

class Sheet;
class Cell : public CellInterface {
public:
    explicit Cell(Sheet& sheet);
    ~Cell();

    void Set(const std::string& text);
    void Clear();

    CellInterface::Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;
    std::vector<Position> GetDependentCells() const {
        return std::vector<Position>(dependent_cells_.begin(), dependent_cells_.end());
    }

    bool CheckCyclicDependencies(const Cell* start_cell_ptr, const Position& end_pos) const;
    void InvalidateCache();
    bool IsCacheValid() const;


    void AddDependentCell(const Position& pos) {
        dependent_cells_.insert(pos);
    }
private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;

    std::unique_ptr<Impl> impl_;
    Sheet& sheet_;

    class PositionHasher {
    public:
        size_t operator()(const Position& pos) const {
            return static_cast<size_t>(hasher_(pos.row * 13 + pos.col * 31));
        }
    private:
        std::hash<int> hasher_;
    };

    std::unordered_set<Position, PositionHasher> dependent_cells_;
};