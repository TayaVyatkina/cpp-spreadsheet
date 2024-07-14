#include "cell.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <cmath>
#include <memory>

class Cell::Impl {
public:
    virtual ~Impl() = default;
    virtual CellInterface::Value GetValue() const = 0;
    virtual std::string GetText() const = 0;

    virtual std::vector<Position> GetReferencedCells() const = 0;
    virtual void InvalidateCache() {
        return;
    }
    virtual bool IsCached() const {
        return true;
    }
};

class Cell::EmptyImpl : public Impl
{
public:
    EmptyImpl() = default;

    CellInterface::Value GetValue() const override {
        return 0.;
    }

    std::string GetText() const override {
        return std::string();
    }

    std::vector<Position> GetReferencedCells() const override {
        return {};
    }

};

class Cell::TextImpl : public Impl {
public:
    explicit TextImpl(std::string text)
        : value_(std::move(text)) {
        if (value_[0] == ESCAPE_SIGN) {
            is_escaped_ = true;
        }
    }

    CellInterface::Value GetValue() const override {
        if (is_escaped_) {
            return value_.substr(1, value_.size() - 1);
        }
        else {
            return value_;
        }
    }

    std::string GetText() const override {
        return value_;
    }

    std::vector<Position> GetReferencedCells() const override {
        return {};
    }

private:
    std::string value_;
    bool is_escaped_ = false;
};


class Cell::FormulaImpl : public Impl {
public:
    FormulaImpl(Sheet& sheet, std::string formula)
        : sheet_(sheet), formula_(ParseFormula(formula))
    {}

    CellInterface::Value GetValue() const override {
        if (!cache_) {
            FormulaInterface::Value result = formula_->Evaluate(sheet_);
            if (std::holds_alternative<double>(result)) {
                if (std::isfinite(std::get<double>(result))) {
                    return std::get<double>(result);
                }
                else {
                    return FormulaError(FormulaError::Category::Arithmetic);
                }
            }
            return std::get<FormulaError>(result);
        }
        return *cache_;
    }

    std::string GetText() const override {
        return { FORMULA_SIGN + formula_->GetExpression() };
    }

    std::vector<Position> GetReferencedCells() const override {
        return formula_.get()->GetReferencedCells();
    }

    void InvalidateCache() override {
        cache_.reset();
    }
    bool IsCached() const override {
        return cache_.has_value();
    }
private:
    Sheet& sheet_;
    std::unique_ptr<FormulaInterface> formula_;
    std::optional<CellInterface::Value> cache_;
};

Cell::Cell(Sheet& sheet)
    : impl_(std::make_unique<EmptyImpl>()),
    sheet_(sheet)
{}

Cell::~Cell() {
    if (impl_) {
        impl_.reset(nullptr);
    }
}

void Cell::Set(const std::string& text, Position pos) {
    std::unique_ptr<Impl> new_impl;
   
    if (text.empty()) {
        new_impl = std::make_unique<EmptyImpl>();
    }
    else if (text[0] != FORMULA_SIGN || (text[0] == FORMULA_SIGN && text.size() == 1)) {
        new_impl = std::make_unique<TextImpl>(text);
    }
    else {
        try {
            new_impl = std::make_unique<FormulaImpl>(sheet_, std::string{ text.begin() + 1, text.end() });
        }
        catch (...) {
            throw FormulaException("Parsing error!");
        }
    } 
    
    const auto cur_ref_cells = new_impl->GetReferencedCells();
    if (!cur_ref_cells.empty()) {
        // save all visited cells from all function calls for this Cell::Set iteration
        std::unordered_set<const Cell*, PositionHasher> visited;
        if (CheckCyclicDependencies(cur_ref_cells, pos, visited)) {
            throw CircularDependencyException("Circular dependency!");
        }      
    }  
    std::unique_ptr<Impl> old_impl = std::move(impl_);
    impl_ = std::move(new_impl);

    //For each ref cell, clear the reference from dependent_cells about the current cell
    for (const auto& pos_of_old_ref_cell : old_impl.get()->GetReferencedCells()) {
        Cell* refrenced = sheet_.GetConcreteCell(pos_of_old_ref_cell);
        refrenced->dependent_cells_.erase(this);
    }
    //set new reference in dependent_cells_ of referenced cells
    for (const auto& pos_of_new_ref_cell : impl_->GetReferencedCells()) {

        Cell* refrenced = sheet_.GetConcreteCell(pos_of_new_ref_cell);

        if (!refrenced) {
            sheet_.SetCell(pos_of_new_ref_cell, std::string());
            refrenced = sheet_.GetConcreteCell(pos_of_new_ref_cell);
        }
        refrenced->AddDependentCell(this);
    }
    // recursive invalidate cache in dependent cells
    InvalidateCacheInDependentCells(dependent_cells_);
}
void Cell::InvalidateCacheInDependentCells(const std::unordered_set<Cell*, PositionHasher>& dependent_cells) {
    for (const auto& refrenced : dependent_cells) {
        if (refrenced->IsCacheValid()) {
            refrenced->InvalidateCache();
            //The cache needs to be cleared for all cells that in any way depend on this one
            refrenced->InvalidateCacheInDependentCells(refrenced->GetDependentCells());
        }       
    }
}

void Cell::Clear(const Position& pos) {
    this->Set(std::string(), pos);
}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_.get()->GetReferencedCells();
}

std::unordered_set<Cell*, Cell::PositionHasher> Cell::GetDependentCells() const {
    return dependent_cells_;
}
void Cell::AddDependentCell(Cell* pos) {
    dependent_cells_.insert(pos);
}
bool Cell::CheckCyclicDependencies(const std::vector<Position>& referenced_cells, const Position& end, std::unordered_set<const Cell*, PositionHasher>& visited) const {

    for (const auto& cell : referenced_cells) {
        if (cell == end) {
            return true;
        }
        const Cell* ref_cell_ptr = sheet_.GetConcreteCell(cell);
        if (!visited.count(ref_cell_ptr)) {
            visited.insert(ref_cell_ptr);

            if (!ref_cell_ptr) {
                sheet_.SetCell(cell, std::string());
                ref_cell_ptr = sheet_.GetConcreteCell(cell);
            }
            
            if (ref_cell_ptr->CheckCyclicDependencies(ref_cell_ptr->GetReferencedCells(), end, visited)) {
                return true;
            }
        }
    }
    return false;
}

void Cell::InvalidateCache() {
    impl_->InvalidateCache();
}

bool Cell::IsCacheValid() const {
    return impl_->IsCached();
}

