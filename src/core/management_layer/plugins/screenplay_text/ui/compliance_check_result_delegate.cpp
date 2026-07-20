#include "compliance_check_result_delegate.h"

#include <business_layer/compliance/compliance_checker.h>
#include <business_layer/compliance_check_result_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_folder_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_scene_item.h>
#include <ui/design_system/design_system.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/eighths_helper.h>
#include <utils/helpers/text_helper.h>
#include <utils/helpers/time_helper.h>

#include <QAbstractItemView>
#include <QPainter>


namespace Ui {

class ComplianceCheckResultDelegate::Implementation
{
public:
    /**
     * @brief Нарисовать хронометраж
     */
    QRectF paintItemDuration(QPainter* _painter, const QStyleOptionViewItem& _option,
                             const QString& _duration) const;

    /**
     * @brief Нарисовать элемент
     */
    void paintRule(QPainter* _painter, const QStyleOptionViewItem& _option,
                   const QModelIndex& _index) const;
    void paintItem(QPainter* _painter, const QStyleOptionViewItem& _option,
                   const QModelIndex& _index) const;
    void paintScene(QPainter* _painter, const QStyleOptionViewItem& _option,
                    const QModelIndex& _index) const;

    /**
     * @brief Идеальный размер для элемент
     */
    QSize ruleSizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const;
    QSize itemSizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const;
    QSize sceneSizeHint(const QStyleOptionViewItem& _option) const;


    bool useEighths = false;
};

QRectF ComplianceCheckResultDelegate::Implementation::paintItemDuration(
    QPainter* _painter, const QStyleOptionViewItem& _option, const QString& _duration) const
{
    using namespace BusinessLayer;

    _painter->setFont(DesignSystem::font().subtitle2());

    const auto durationText = QString("%1").arg(_duration);
    const qreal durationWidth = TextHelper::fineTextWidthF(durationText, _painter->font());

    const QRectF backgroundRect = _option.rect;
    const QRectF durationRect(
        QPointF(QLocale().textDirection() == Qt::LeftToRight
                    ? (backgroundRect.right() - durationWidth
                       - DesignSystem::treeOneLineItem().margins().right())
                    : backgroundRect.left() + DesignSystem::treeOneLineItem().margins().left(),
                backgroundRect.top() + DesignSystem::treeOneLineItem().margins().top()),
        QSizeF(durationWidth, DesignSystem::treeOneLineItem().contentHeight()));
    _painter->drawText(durationRect, Qt::AlignLeft | Qt::AlignVCenter, durationText);

    return durationRect;
}

void ComplianceCheckResultDelegate::Implementation::paintRule(QPainter* _painter,
                                                              const QStyleOptionViewItem& _option,
                                                              const QModelIndex& _index) const
{
    using namespace BusinessLayer;

    auto backgroundColor = _option.palette.color(QPalette::Base);
    auto textColor = _option.palette.color(QPalette::Text);
    const auto isLeftToRight = QLocale().textDirection() == Qt::LeftToRight;

    //
    // Рисуем
    //

    //
    // ... фон
    //
    const QRectF backgroundRect = _option.rect;
    if (_option.state.testFlag(QStyle::State_Selected)) {
        //
        // ... для выделенных элементов
        //
        backgroundColor = _option.palette.color(QPalette::Highlight);
        textColor = _option.palette.color(QPalette::HighlightedText);
    } else if (_option.state.testFlag(QStyle::State_MouseOver)) {
        //
        // ... для элементов на которые наведена мышь
        //
        backgroundColor = _option.palette.color(QPalette::AlternateBase);
    } else {
        //
        // ... для остальных элементов
        //
        textColor.setAlphaF(DesignSystem::inactiveTextOpacity());
    }
    _painter->fillRect(backgroundRect, backgroundColor);

    //
    // ... иконка
    //
    QColor statusColor;
    switch (static_cast<ComplianceCheckResultStatus>(
        _index.data(ComplianceCheckResultModelItemDataRole::RuleStatusRole).toInt())) {
    case ComplianceCheckResultStatus::Passed: {
        statusColor = Ui::DesignSystem::color().success();
        break;
    }
    case ComplianceCheckResultStatus::Warning: {
        statusColor = Ui::DesignSystem::color().warning();
        break;
    }
    default: {
        statusColor = Ui::DesignSystem::color().error();
        break;
    }
    }
    _painter->setPen(statusColor);
    //
    QRectF iconRect;
    if (_index.data(Qt::DecorationRole).isValid()) {
        if (isLeftToRight) {
            iconRect = QRectF(QPointF(std::max(backgroundRect.left(),
                                               DesignSystem::treeOneLineItem().margins().left()),
                                      backgroundRect.top()),
                              QSizeF(DesignSystem::treeOneLineItem().iconSize().width(),
                                     DesignSystem::treeOneLineItem().height()));
        } else {
            iconRect = QRectF(
                QPointF(backgroundRect.right() - DesignSystem::treeOneLineItem().iconSize().width(),
                        backgroundRect.top()),
                QSizeF(DesignSystem::treeOneLineItem().iconSize().width(),
                       DesignSystem::treeOneLineItem().height()));
        }
        _painter->setFont(DesignSystem::font().iconsMid());
        _painter->drawText(iconRect, Qt::AlignLeft | Qt::AlignVCenter,
                           _index.data(Qt::DecorationRole).toString());
    }

    //
    // ... заголовок правила
    //
    _painter->setPen(textColor);
    _painter->setFont(DesignSystem::font().subtitle2());
    qreal titleLeft = 0.0;
    qreal titleWidth = 0.0;
    if (isLeftToRight) {
        titleLeft = iconRect.right() + DesignSystem::treeOneLineItem().spacing();
        titleWidth = backgroundRect.right() - DesignSystem::treeOneLineItem().margins().right()
            - titleLeft;
    } else {
        titleLeft = backgroundRect.left() + DesignSystem::treeOneLineItem().margins().left();
        titleWidth = iconRect.left() - titleLeft;
    }
    const auto ruleTitle
        = _index.data(ComplianceCheckResultModelItemDataRole::TitleRole).toString();
    const QRectF titleRect(
        QPointF(titleLeft,
                backgroundRect.top() + DesignSystem::treeOneLineItem().margins().top()
                    + DesignSystem::layout().px4()),
        QSizeF(titleWidth, TextHelper::heightForWidth(ruleTitle, _painter->font(), titleWidth)));
    _painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, ruleTitle);

    //
    // ... подзаголовок правила
    //
    const auto subtitleText
        = _index.data(ComplianceCheckResultModelItemDataRole::SubtitleRole).toString();
    if (!subtitleText.isEmpty()) {
        _painter->setFont(DesignSystem::font().body2());
        const auto subtitleWidth = titleRect.right() - iconRect.left();
        const QRectF subtitleRect(
            QPointF(iconRect.left(),
                    titleRect.bottom() + DesignSystem::treeOneLineItem().spacing()),
            QSizeF(subtitleWidth,
                   TextHelper::heightForWidth(ruleTitle, _painter->font(), subtitleWidth)));
        _painter->drawText(subtitleRect, Qt::TextWordWrap, subtitleText);
    }
}

void ComplianceCheckResultDelegate::Implementation::paintItem(QPainter* _painter,
                                                              const QStyleOptionViewItem& _option,
                                                              const QModelIndex& _index) const
{
    using namespace BusinessLayer;

    auto backgroundColor = _option.palette.color(QPalette::Base);
    auto textColor = _option.palette.color(QPalette::Text);
    const auto isLeftToRight = QLocale().textDirection() == Qt::LeftToRight;

    //
    // Рисуем
    //

    //
    // ... фон
    //
    const QRectF backgroundRect = _option.rect;
    if (_option.state.testFlag(QStyle::State_Selected)) {
        //
        // ... для выделенных элементов
        //
        backgroundColor = _option.palette.color(QPalette::Highlight);
        textColor = _option.palette.color(QPalette::HighlightedText);
    } else if (_option.state.testFlag(QStyle::State_MouseOver)) {
        //
        // ... для элементов на которые наведена мышь
        //
        backgroundColor = _option.palette.color(QPalette::AlternateBase);
    } else {
        //
        // ... для остальных элементов
        //
        textColor.setAlphaF(DesignSystem::inactiveTextOpacity());
    }
    _painter->fillRect(backgroundRect, backgroundColor);

    //
    // ... иконка
    //
    _painter->setPen(textColor);
    QRectF iconRect;
    if (_index.data(Qt::DecorationRole).isValid()) {
        if (isLeftToRight) {
            iconRect = QRectF(QPointF(std::max(backgroundRect.left(),
                                               DesignSystem::treeOneLineItem().margins().left()),
                                      backgroundRect.top()),
                              QSizeF(DesignSystem::treeOneLineItem().iconSize().width(),
                                     DesignSystem::treeOneLineItem().height()));
        } else {
            iconRect = QRectF(
                QPointF(backgroundRect.right() - DesignSystem::treeOneLineItem().iconSize().width(),
                        backgroundRect.top()),
                QSizeF(DesignSystem::treeOneLineItem().iconSize().width(),
                       DesignSystem::treeOneLineItem().height()));
        }
        _painter->setFont(DesignSystem::font().iconsMid());
        _painter->drawText(iconRect, Qt::AlignLeft | Qt::AlignVCenter,
                           _index.data(Qt::DecorationRole).toString());
    }

    //
    // ... заголовок элемента
    //
    _painter->setFont(DesignSystem::font().subtitle2());
    qreal headingLeft = 0.0;
    qreal headingWidth = 0.0;
    if (isLeftToRight) {
        headingLeft = iconRect.right() + DesignSystem::treeOneLineItem().spacing();
        headingWidth = backgroundRect.right() - DesignSystem::treeOneLineItem().margins().right()
            - headingLeft - DesignSystem::treeOneLineItem().spacing();
    } else {
        headingLeft = backgroundRect.left() + DesignSystem::treeOneLineItem().margins().left();
        headingWidth = iconRect.left() - headingLeft;
    }
    const QRectF titleRect(
        QPointF(headingLeft,
                backgroundRect.top() + DesignSystem::treeOneLineItem().margins().top()),
        QSizeF(headingWidth, DesignSystem::layout().px24()));
    auto ruleTitle = _index.data(ComplianceCheckResultModelItemDataRole::TitleRole).toString();
    ruleTitle = _painter->fontMetrics().elidedText(ruleTitle, Qt::ElideRight,
                                                   static_cast<int>(titleRect.width()));
    _painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, ruleTitle);

    //
    // ... подзаголовок элемента
    //
    auto subtitleText
        = _index.data(ComplianceCheckResultModelItemDataRole::SubtitleRole).toString();
    if (!subtitleText.isEmpty()) {
        QRectF textRect;
        _painter->setFont(DesignSystem::font().body2());
        textRect = QRectF(
            QPointF(iconRect.left(), titleRect.bottom() + DesignSystem::compactLayout().px8()),
            QSizeF(titleRect.right() - iconRect.left(),
                   TextHelper::fineLineSpacing(_painter->font())));
        subtitleText = TextHelper::elidedText(subtitleText, DesignSystem::font().body2(), textRect);
        _painter->drawText(textRect, Qt::TextWordWrap, subtitleText);
    }
}

void ComplianceCheckResultDelegate::Implementation::paintScene(QPainter* _painter,
                                                               const QStyleOptionViewItem& _option,
                                                               const QModelIndex& _index) const
{
    using namespace BusinessLayer;

    auto backgroundColor = _option.palette.color(QPalette::Base);
    auto textColor = _option.palette.color(QPalette::Text);
    const auto isLeftToRight = QLocale().textDirection() == Qt::LeftToRight;

    //
    // Рисуем
    //

    //
    // ... фон
    //
    const QRectF backgroundRect = _option.rect;
    if (_option.state.testFlag(QStyle::State_Selected)) {
        //
        // ... для выделенных элементов
        //
        backgroundColor = _option.palette.color(QPalette::Highlight);
        textColor = _option.palette.color(QPalette::HighlightedText);
    } else if (_option.state.testFlag(QStyle::State_MouseOver)) {
        //
        // ... для элементов на которые наведена мышь
        //
        backgroundColor = _option.palette.color(QPalette::AlternateBase);
    } else {
        //
        // ... для остальных элементов
        //
        textColor.setAlphaF(DesignSystem::inactiveTextOpacity());
    }
    _painter->fillRect(backgroundRect, backgroundColor);

    //
    // ... иконка
    //
    _painter->setPen(textColor);
    QRectF iconRect;
    if (_index.data(Qt::DecorationRole).isValid()) {
        if (isLeftToRight) {
            iconRect = QRectF(QPointF(std::max(backgroundRect.left(),
                                               DesignSystem::treeOneLineItem().margins().left()),
                                      backgroundRect.top()),
                              QSizeF(DesignSystem::treeOneLineItem().iconSize().width(),
                                     DesignSystem::treeOneLineItem().height()));
        } else {
            iconRect = QRectF(
                QPointF(backgroundRect.right() - DesignSystem::treeOneLineItem().iconSize().width(),
                        backgroundRect.top()),
                QSizeF(DesignSystem::treeOneLineItem().iconSize().width(),
                       DesignSystem::treeOneLineItem().height()));
        }
        _painter->setFont(DesignSystem::font().iconsMid());
        _painter->drawText(iconRect, Qt::AlignLeft | Qt::AlignVCenter,
                           _index.data(Qt::DecorationRole).toString());
    }

    //
    // ... хронометраж
    //
    QString durationText;
    if (useEighths) {
        const auto duration
            = _index.data(ComplianceCheckResultModelItemDataRole::SceneEighthsRole).toReal();
        durationText = EighthsHelper::toStringWithPostfix(duration);
    } else {
        const std::chrono::seconds duration{
            _index.data(ComplianceCheckResultModelItemDataRole::SceneDurationRole).toInt()
        };
        durationText = TimeHelper::toString(duration);
    }
    const auto durationRect = paintItemDuration(_painter, _option, durationText);

    //
    // ... заголовок сцены
    //
    _painter->setFont(DesignSystem::font().subtitle2());
    qreal headingLeft = 0.0;
    qreal headingWidth = 0.0;
    if (isLeftToRight) {
        headingLeft = iconRect.right() + DesignSystem::treeOneLineItem().spacing();
        headingWidth
            = durationRect.left() - headingLeft - DesignSystem::treeOneLineItem().spacing();
    } else {
        headingLeft = durationRect.right() + DesignSystem::treeOneLineItem().spacing();
        headingWidth = iconRect.left() - headingLeft - DesignSystem::treeOneLineItem().spacing();
    }
    const QRectF headingRect(
        QPointF(headingLeft,
                backgroundRect.top() + DesignSystem::treeOneLineItem().margins().top()),
        QSizeF(headingWidth, DesignSystem::layout().px24()));
    auto sceneHeading
        = _index.data(ComplianceCheckResultModelItemDataRole::SceneNumberRole).toString() + " "
        + TextHelper::smartToUpper(
              _index.data(ComplianceCheckResultModelItemDataRole::SceneHeadingRole).toString());
    sceneHeading = _painter->fontMetrics().elidedText(sceneHeading, Qt::ElideRight,
                                                      static_cast<int>(headingRect.width()));
    _painter->drawText(headingRect, Qt::AlignLeft | Qt::AlignVCenter, sceneHeading);
}

QSize ComplianceCheckResultDelegate::Implementation::ruleSizeHint(
    const QStyleOptionViewItem& _option, const QModelIndex& _index) const
{
    using namespace BusinessLayer;

    //
    // Ширина
    //
    int width = _option.widget->width();
    if (const QAbstractItemView* view = qobject_cast<const QAbstractItemView*>(_option.widget)) {
        width = view->viewport()->width();
    }
    width -= DesignSystem::tree().indicatorWidth()
        + DesignSystem::treeOneLineItem().margins().right();

    //
    // Считаем высоту
    //
    const auto title = _index.data(ComplianceCheckResultModelItemDataRole::TitleRole).toString();
    const auto titleWidth = width - DesignSystem::treeOneLineItem().iconSize().width()
        - DesignSystem::treeOneLineItem().spacing();
    int height = DesignSystem::treeOneLineItem().margins().top() + DesignSystem::layout().px4()
        + TextHelper::heightForWidth(title, DesignSystem::font().subtitle2(), titleWidth);
    //
    // ... если есть подзаголовок, добавляем высоты
    //
    const auto subtitle
        = _index.data(ComplianceCheckResultModelItemDataRole::SubtitleRole).toString();
    if (!subtitle.isEmpty()) {
        const auto subtitleWidth = width;
        height += DesignSystem::treeOneLineItem().spacing()
            + TextHelper::heightForWidth(subtitle, DesignSystem::font().body2(), subtitleWidth);
    }
    height += DesignSystem::treeOneLineItem().margins().bottom();

    return { width, height };
}

QSize ComplianceCheckResultDelegate::Implementation::itemSizeHint(
    const QStyleOptionViewItem& _option, const QModelIndex& _index) const
{
    return ruleSizeHint(_option, _index);
}

QSize ComplianceCheckResultDelegate::Implementation::sceneSizeHint(
    const QStyleOptionViewItem& _option) const
{
    using namespace BusinessLayer;

    //
    // Ширина
    //
    int width = _option.widget->width();
    if (const QAbstractItemView* view = qobject_cast<const QAbstractItemView*>(_option.widget)) {
        width = view->viewport()->width();
    }
    width -= DesignSystem::layout().px8() + DesignSystem::layout().px16()
        + DesignSystem::layout().px16();

    //
    // Считаем высоту
    //
    const int height = DesignSystem::treeOneLineItem().height();

    return { width, height };
}


// ****


ComplianceCheckResultDelegate::ComplianceCheckResultDelegate(QObject* _parent)
    : QStyledItemDelegate(_parent)
    , d(new Implementation)
{
}

ComplianceCheckResultDelegate::~ComplianceCheckResultDelegate() = default;

void ComplianceCheckResultDelegate::setUseEighths(bool _use)
{
    d->useEighths = _use;
}

void ComplianceCheckResultDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option,
                                          const QModelIndex& _index) const
{
    //
    // Получим настройки стиля
    //
    QStyleOptionViewItem opt = _option;
    initStyleOption(&opt, _index);

    //
    // Рисуем ручками
    //
    _painter->setRenderHint(QPainter::Antialiasing, true);

    //
    // Рисуем в зависимости от типа элемента
    //

    using namespace BusinessLayer;
    const auto typeValue = _index.data(ComplianceCheckResultModelItemDataRole::TypeRole);
    if (!typeValue.isValid()) {
        return;
    }

    const auto type = static_cast<ComplianceCheckResultModelItemType>(typeValue.toInt());
    switch (type) {
    case ComplianceCheckResultModelItemType::Rule: {
        d->paintRule(_painter, opt, _index);
        break;
    }

    case ComplianceCheckResultModelItemType::Item: {
        d->paintItem(_painter, opt, _index);
        break;
    }

    case ComplianceCheckResultModelItemType::Scene: {
        d->paintScene(_painter, opt, _index);
        break;
    }

    default:
        break;
    }
}

QSize ComplianceCheckResultDelegate::sizeHint(const QStyleOptionViewItem& _option,
                                              const QModelIndex& _index) const
{
    if (_option.widget == nullptr) {
        return QStyledItemDelegate::sizeHint(_option, _index);
    }

    using namespace BusinessLayer;
    const auto typeValue = _index.data(ComplianceCheckResultModelItemDataRole::TypeRole);
    if (!typeValue.isValid()) {
        return QStyledItemDelegate::sizeHint(_option, _index);
    }

    const auto type = static_cast<ComplianceCheckResultModelItemType>(typeValue.toInt());
    switch (type) {
    case ComplianceCheckResultModelItemType::Rule: {
        return d->ruleSizeHint(_option, _index);
    }

    case ComplianceCheckResultModelItemType::Item: {
        return d->itemSizeHint(_option, _index);
    }

    case ComplianceCheckResultModelItemType::Scene: {
        return d->sceneSizeHint(_option);
    }

    default: {
        return {};
    }
    }
}

} // namespace Ui
