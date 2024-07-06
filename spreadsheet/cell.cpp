#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <cmath>

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
        return "";
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
    FormulaImpl(SheetInterface& sheet, std::string formula)
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
    SheetInterface& sheet_;
    std::unique_ptr<FormulaInterface> formula_;
    std::optional<CellInterface::Value> cache_;
};

Cell::Cell(SheetInterface& sheet)
    : sheet_(sheet)
{}

Cell::~Cell()
{
    if (impl_) {
        impl_.reset(nullptr);
    }
}

void Cell::Set(const std::string& text) {
    if (text.empty()) {
        impl_ = std::make_unique<EmptyImpl>();
        return;
    }
    if (text[0] != FORMULA_SIGN || (text[0] == FORMULA_SIGN && text.size() == 1)) {
        impl_ = std::make_unique<TextImpl>(text);
        return;
    }
    try {
        impl_ = std::make_unique<FormulaImpl>(sheet_, std::string{ text.begin() + 1, text.end() });
    }
    catch (...) {
        throw FormulaException("Parsing error!");
    }
}

void Cell::Clear() {
    impl_ = std::make_unique<EmptyImpl>();
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

bool Cell::CheckCyclicDependencies(const Cell* cur_cell_ptr, const Position& end) const {
    for (const auto& cell : GetReferencedCells()) {
        if (cell == end) {
            return true;
        }
        const Cell* ref_cell_ptr = dynamic_cast<const Cell*>(sheet_.GetCell(cell));
        if (!cur_cell_ptr) {
            sheet_.SetCell(cell, "");
            ref_cell_ptr = dynamic_cast<const Cell*>(sheet_.GetCell(cell));
        }
        if (cur_cell_ptr == ref_cell_ptr) {
            return true;
        }
        if (ref_cell_ptr->CheckCyclicDependencies(cur_cell_ptr, end)) {
            return true;
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

