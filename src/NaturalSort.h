#pragma once

#include <QString>

namespace veylo {

int naturalCompare(const QString &left, const QString &right);

struct NaturalLess {
    bool operator()(const QString &left, const QString &right) const
    {
        return naturalCompare(left, right) < 0;
    }
};

} // namespace veylo
