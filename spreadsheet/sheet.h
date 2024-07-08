#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <map>
#include <set>

class Sheet : public SheetInterface {
public:
    ~Sheet() = default;

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;

    Cell* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintTexts(std::ostream& output) const override;
    void PrintValues(std::ostream& output) const override;

    void InvalidateCell(const Position& pos);

private:
    std::vector<std::vector<std::unique_ptr<Cell>>> sheet_ = {};
    bool IsCell(Position pos) const;
};
