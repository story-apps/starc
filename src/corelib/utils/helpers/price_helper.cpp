#include "price_helper.h"

#include <QLocale>

namespace {
bool needAdoptToRubles()
{
    return QLocale().language() == QLocale::Russian;
}
} // namespace


qreal PriceHelper::adoptPrice(qreal _priceInUsd)
{
    if (needAdoptToRubles()) {
        return _priceInUsd * 90.0;
    }

    return _priceInUsd;
}

QString PriceHelper::adoptPriceLabel(const QString& _text)
{
    if (needAdoptToRubles()) {
        auto result = _text;
        result.replace("$", "₽");
        return result;
    }

    return _text;
}
