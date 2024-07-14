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

    void Set(const std::string& text, Position pos);
    void Clear(const Position&);

    CellInterface::Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;
    
    bool operator==(const Cell* other) {
        return this == other;
    }
    class PositionHasher {
    public:
        size_t operator()(const Cell* cell) const {
            if (cell == nullptr) {
                return 0;
            }
            return static_cast<size_t>(hasher_(cell->GetText()));
        }
    private:
        std::hash<std::string> hasher_;

    };
   
    std::unordered_set<Cell*, PositionHasher> GetDependentCells() const;  
    void InvalidateCache();
    bool IsCacheValid() const;
    
    void AddDependentCell(Cell* pos);
    
private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;
    std::unique_ptr<Impl> impl_;
    Sheet& sheet_;
    std::unordered_set<Cell*, PositionHasher> dependent_cells_;  
    bool CheckCyclicDependencies(const std::vector<Position>&, const Position&, std::unordered_set<const Cell*, PositionHasher>& visited) const;
    void InvalidateCacheInDependentCells(const std::unordered_set<Cell*, PositionHasher>& dependent_cells);
};