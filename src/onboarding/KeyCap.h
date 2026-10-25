#pragma once

#include <string>

#include "widgets/View.h"

// Figma keycap (1902:7075): a key with a hard 6px drop shadow.
class KeyCap : public View
{
public:
    // Arrow and punctuation keys, shown in pairs, are `compact`, with narrower padding.
    KeyCap(std::wstring label, bool compact);

    Size preferredSize() const override;
    void paint(Canvas &canvas) override;

private:
    std::wstring m_label;
    bool m_compact;
};
