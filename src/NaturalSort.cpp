#include "NaturalSort.h"

#include <algorithm>

namespace {

QString normalizedDigitRun(QStringView value)
{
    QString result;
    result.reserve(value.size());
    for (const QChar character : value) {
        const int digit = character.digitValue();
        result.append(QChar(u'0' + std::max(0, digit)));
    }
    return result;
}

int compareDigitRuns(QStringView left, QStringView right)
{
    const QString leftDigits = normalizedDigitRun(left);
    const QString rightDigits = normalizedDigitRun(right);

    qsizetype leftSignificant = 0;
    while (leftSignificant < leftDigits.size() - 1 && leftDigits.at(leftSignificant) == u'0') {
        ++leftSignificant;
    }
    qsizetype rightSignificant = 0;
    while (rightSignificant < rightDigits.size() - 1 && rightDigits.at(rightSignificant) == u'0') {
        ++rightSignificant;
    }

    const QStringView leftValue(leftDigits.constData() + leftSignificant,
                                leftDigits.size() - leftSignificant);
    const QStringView rightValue(rightDigits.constData() + rightSignificant,
                                 rightDigits.size() - rightSignificant);

    if (leftValue.size() != rightValue.size()) {
        return leftValue.size() < rightValue.size() ? -1 : 1;
    }
    const int valueComparison = leftValue.compare(rightValue, Qt::CaseSensitive);
    if (valueComparison != 0) {
        return valueComparison < 0 ? -1 : 1;
    }

    // For equal numeric values, fewer leading zeros sort first.
    if (left.size() != right.size()) {
        return left.size() < right.size() ? -1 : 1;
    }
    return 0;
}

} // namespace

namespace veylo {

int naturalCompare(const QString &left, const QString &right)
{
    const QString foldedLeft = left.toCaseFolded();
    const QString foldedRight = right.toCaseFolded();

    qsizetype leftIndex = 0;
    qsizetype rightIndex = 0;
    while (leftIndex < foldedLeft.size() && rightIndex < foldedRight.size()) {
        const bool leftIsDigit = foldedLeft.at(leftIndex).isDigit();
        const bool rightIsDigit = foldedRight.at(rightIndex).isDigit();

        if (leftIsDigit && rightIsDigit) {
            qsizetype leftEnd = leftIndex;
            while (leftEnd < foldedLeft.size() && foldedLeft.at(leftEnd).isDigit()) {
                ++leftEnd;
            }
            qsizetype rightEnd = rightIndex;
            while (rightEnd < foldedRight.size() && foldedRight.at(rightEnd).isDigit()) {
                ++rightEnd;
            }

            const int numberComparison = compareDigitRuns(
                QStringView(foldedLeft).sliced(leftIndex, leftEnd - leftIndex),
                QStringView(foldedRight).sliced(rightIndex, rightEnd - rightIndex));
            if (numberComparison != 0) {
                return numberComparison;
            }
            leftIndex = leftEnd;
            rightIndex = rightEnd;
            continue;
        }

        if (foldedLeft.at(leftIndex) != foldedRight.at(rightIndex)) {
            return foldedLeft.at(leftIndex) < foldedRight.at(rightIndex) ? -1 : 1;
        }
        ++leftIndex;
        ++rightIndex;
    }

    if (leftIndex != foldedLeft.size() || rightIndex != foldedRight.size()) {
        return leftIndex == foldedLeft.size() ? -1 : 1;
    }

    const int caseSensitiveTieBreak = QString::compare(left, right, Qt::CaseSensitive);
    if (caseSensitiveTieBreak != 0) {
        return caseSensitiveTieBreak < 0 ? -1 : 1;
    }
    return 0;
}

} // namespace veylo
