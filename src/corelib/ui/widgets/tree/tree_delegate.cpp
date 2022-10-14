#include "tree_delegate.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/color_picker/color_picker_popup.h>
#include <ui/widgets/combo_box/combo_box.h>
#include <ui/widgets/key_sequence_edit/key_sequence_edit.h>
#include <utils/helpers/text_helper.h>

#include <QPainter>
#include <QScopedValueRollback>


TreeDelegate::TreeDelegate(QObject* _parent)
    : QStyledItemDelegate(_parent)
{
}

void TreeDelegate::setAdditionalLeftMargin(qreal _margin)
{
    m_additionalLeftMargin = _margin;
}

void TreeDelegate::setHoverTrailingIcon(const QString _icon)
{
    m_hoverTrailingIcon = _icon;
}

void TreeDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option,
                         const QModelIndex& _index) const
{

    if (m_editorActiveFor == _index) {
        return;
    }

    //
    // Получим настройки стиля
    //
    QStyleOptionViewItem opt = _option;
    initStyleOption(&opt, _index);

    //
    // Рисуем ручками
    //
    _painter->setRenderHint(QPainter::Antialiasing, true);

    auto backgroundColor = _index.data(Qt::BackgroundRole).isValid()
        ? _index.data(Qt::BackgroundRole).value<QColor>()
        : opt.palette.color(QPalette::Base);
    auto textColor = _index.data(Qt::ForegroundRole).isValid()
        ? _index.data(Qt::ForegroundRole).value<QColor>()
        : opt.palette.color(QPalette::Text);
    const auto isLeftToRight = QLocale().textDirection() == Qt::LeftToRight;

    //
    // Рисуем
    //

    //
    // ... фон
    //
    const QRectF backgroundRect = opt.rect;
    if (opt.state.testFlag(QStyle::State_Selected)) {
        //
        // ... для выделенных элементов
        //
        backgroundColor = opt.palette.color(QPalette::Highlight);
        textColor = opt.palette.color(QPalette::HighlightedText);
    } else if (opt.state.testFlag(QStyle::State_MouseOver)) {
        //
        // ... для элементов на которые наведена мышь
        //
        backgroundColor = opt.palette.color(QPalette::AlternateBase);
    } else {
        //
        // ... для остальных элементов
        //
        textColor.setAlphaF(Ui::DesignSystem::inactiveTextOpacity());
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
                                               Ui::DesignSystem::treeOneLineItem().margins().left())
                                          + m_additionalLeftMargin,
                                      backgroundRect.top()),
                              QSizeF(Ui::DesignSystem::treeOneLineItem().iconSize().width(),
                                     backgroundRect.height()));
        } else {
            iconRect = QRectF(QPointF(backgroundRect.right() - m_additionalLeftMargin
                                          - Ui::DesignSystem::treeOneLineItem().iconSize().width(),
                                      backgroundRect.top()),
                              QSizeF(Ui::DesignSystem::treeOneLineItem().iconSize().width(),
                                     backgroundRect.height()));
        }

        if (_index.data(Qt::DecorationRole).type() == QVariant::String) {
            if (_index.data(Qt::DecorationPropertyRole).isValid()
                && _index.data(Qt::DecorationPropertyRole).value<QColor>().isValid()) {

                _painter->setPen(_index.data(Qt::DecorationPropertyRole).value<QColor>());
            }
            _painter->setFont(Ui::DesignSystem::font().iconsMid());
            _painter->drawText(iconRect, Qt::AlignCenter,
                               _index.data(Qt::DecorationRole).toString());
        } else {
            const auto pixmap = _index.data(Qt::DecorationRole).value<QPixmap>();
            _painter->drawPixmap(
                QPointF(iconRect.left() + (iconRect.width() - pixmap.width()) / 2.0,
                        iconRect.top() + (iconRect.height() - pixmap.height()) / 2.0),
                pixmap);
        }
    }

    //
    // ... текст
    //
    _painter->setPen(textColor);
    _painter->setFont(Ui::DesignSystem::font().subtitle2());
    QRectF textRect;
    if (isLeftToRight) {
        const qreal textLeft = iconRect.isValid()
            ? iconRect.right() + Ui::DesignSystem::treeOneLineItem().spacing()
            : backgroundRect.left() + Ui::DesignSystem::treeOneLineItem().margins().left()
                + m_additionalLeftMargin;
        textRect = QRectF(QPointF(textLeft, backgroundRect.top()),
                          QSizeF(backgroundRect.right() - textLeft
                                     - Ui::DesignSystem::treeOneLineItem().margins().right(),
                                 backgroundRect.height()));
        if (!m_hoverTrailingIcon.isEmpty() && opt.state.testFlag(QStyle::State_MouseOver)) {
            textRect = textRect.adjusted(
                0, 0, -1 * Ui::DesignSystem::treeOneLineItem().iconSize().width(), 0);
        }
    } else {
        const qreal textRight = iconRect.isValid()
            ? iconRect.left() - Ui::DesignSystem::treeOneLineItem().spacing()
            : backgroundRect.right() - Ui::DesignSystem::treeOneLineItem().margins().right()
                - m_additionalLeftMargin;
        const auto textLeft
            = backgroundRect.left() + Ui::DesignSystem::treeOneLineItem().margins().left();
        textRect = QRectF(QPointF(textLeft, backgroundRect.top()),
                          QSizeF(textRight - textLeft, backgroundRect.height()));
        if (!m_hoverTrailingIcon.isEmpty() && opt.state.testFlag(QStyle::State_MouseOver)) {
            textRect = textRect.adjusted(Ui::DesignSystem::treeOneLineItem().iconSize().width(), 0,
                                         0, 0);
        }
    }
    const auto text = _painter->fontMetrics().elidedText(_index.data().toString(), Qt::ElideRight,
                                                         static_cast<int>(textRect.width()));
    _painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);

    //
    // Рисуем декорацию
    //
    if (!m_hoverTrailingIcon.isEmpty() && opt.state.testFlag(QStyle::State_MouseOver)) {
        auto iconRect = QRectF(
            QPointF(isLeftToRight ? (backgroundRect.right()
                                     - Ui::DesignSystem::treeOneLineItem().iconSize().width()
                                     - Ui::DesignSystem::treeOneLineItem().margins().right())
                                  : Ui::DesignSystem::treeOneLineItem().margins().left(),
                    backgroundRect.top()),
            QSizeF(Ui::DesignSystem::treeOneLineItem().iconSize().width(),
                   backgroundRect.height()));
        _painter->setFont(Ui::DesignSystem::font().iconsMid());
        _painter->drawText(iconRect, Qt::AlignCenter, m_hoverTrailingIcon);
    }
}

QSize TreeDelegate::sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const
{
    Q_UNUSED(_option)
    Q_UNUSED(_index)

    return QSizeF(Ui::DesignSystem::treeOneLineItem().margins().left()
                      + TextHelper::fineTextWidthF(_index.data().toString(),
                                                   Ui::DesignSystem::font().subtitle2())
                      + Ui::DesignSystem::treeOneLineItem().margins().right()
                      // ... для последнего столбца, добавляем дополнительный отступ от края
                      + (_index.column() == _index.model()->columnCount() - 1
                             ? Ui::DesignSystem::layout().px24()
                             : 0.0),
                  Ui::DesignSystem::treeOneLineItem().height())
        .toSize();
}

void TreeDelegate::setEditorData(QWidget* _editor, const QModelIndex& _index) const
{
    m_editorActiveFor = _index;

    QStyledItemDelegate::setEditorData(_editor, _index);
}

void TreeDelegate::updateEditorGeometry(QWidget* _editor, const QStyleOptionViewItem& _option,
                                        const QModelIndex& _index) const
{
    Q_UNUSED(_index);

    _editor->setGeometry(_option.rect);
}

void TreeDelegate::destroyEditor(QWidget* editor, const QModelIndex& index) const
{
    QStyledItemDelegate::destroyEditor(editor, index);

    m_editorActiveFor = {};
}


// ****


KeySequenceDelegate::KeySequenceDelegate(QObject* _parent)
    : TreeDelegate(_parent)
{
}

QWidget* KeySequenceDelegate::createEditor(QWidget* _parent, const QStyleOptionViewItem& _option,
                                           const QModelIndex& _index) const
{
    Q_UNUSED(_option);
    Q_UNUSED(_index);

    auto editor = new KeySequenceEdit(_parent);
    editor->setBackgroundColor(_parent->palette().text().color());
    editor->setTextColor(_parent->palette().text().color());
    editor->setDefaultMarginsEnabled(false);
    editor->setLabel(tr("Press shortcut"));

    return editor;
}

void KeySequenceDelegate::setEditorData(QWidget* _editor, const QModelIndex& _index) const
{
    TreeDelegate::setEditorData(_editor, _index);

    const QString value = _index.model()->data(_index, Qt::EditRole).toString();

    auto keySequenceEdit = qobject_cast<KeySequenceEdit*>(_editor);
    keySequenceEdit->setKeySequence(QKeySequence(value));
}

void KeySequenceDelegate::setModelData(QWidget* _editor, QAbstractItemModel* _model,
                                       const QModelIndex& _index) const
{
    auto keySequenceEdit = qobject_cast<KeySequenceEdit*>(_editor);
    const QString value = keySequenceEdit->keySequence().toString(QKeySequence::NativeText);
    _model->setData(_index, value, Qt::EditRole);
}


// ****


TextFieldItemDelegate::TextFieldItemDelegate(QObject* _parent)
    : TreeDelegate(_parent)
{
}

void TextFieldItemDelegate::setLabel(const QString& _label)
{
    m_label = _label;
}

void TextFieldItemDelegate::setTrailingIconPickColor(bool _isPickColor)
{
    m_isTrailingIconPickColor = _isPickColor;
}

QWidget* TextFieldItemDelegate::createEditor(QWidget* _parent, const QStyleOptionViewItem& _option,
                                             const QModelIndex& _index) const
{
    Q_UNUSED(_option);
    Q_UNUSED(_index);

    auto editor = new TextField(_parent);
    editor->setBackgroundColor(_parent->palette().text().color());
    editor->setTextColor(_parent->palette().text().color());
    editor->setDefaultMarginsEnabled(false);
    editor->setCapitalizeWords(false);
    editor->setLabel(m_label);
    if (m_isTrailingIconPickColor) {
        editor->setTrailingIcon(u8"\U000f0766");
        auto colorPicker = new ColorPickerPopup(editor);
        colorPicker->setColorCanBeDeselected(true);
        colorPicker->setBackgroundColor(_parent->palette().base().color());
        colorPicker->setTextColor(_parent->palette().text().color());
        connect(editor, &TextField::trailingIconPressed, colorPicker, [editor, colorPicker] {
            QSignalBlocker signalBlocker(colorPicker);
            if (editor->trailingIconColor().isValid()) {
                colorPicker->setSelectedColor(editor->trailingIconColor());
            }
            colorPicker->showPopup(editor, Qt::AlignBottom | Qt::AlignRight);
        });
        connect(colorPicker, &ColorPickerPopup::selectedColorChanged, editor,
                [editor](const QColor& _color) {
                    editor->setTrailingIcon(_color.isValid() ? u8"\U000F0765" : u8"\U000f0766");
                    editor->setTrailingIconColor(_color);
                });
    }
    editor->setPlaceholderText(" ");
    return editor;
}

void TextFieldItemDelegate::setEditorData(QWidget* _editor, const QModelIndex& _index) const
{
    if (m_isInSetModelData) {
        return;
    }

    TreeDelegate::setEditorData(_editor, _index);

    auto editor = qobject_cast<TextField*>(_editor);
    const auto text = _index.model()->data(_index, Qt::EditRole).toString();
    editor->setText(text);
    //
    if (m_isTrailingIconPickColor) {
        const auto color = _index.model()->data(_index, Qt::DecorationPropertyRole).value<QColor>();
        editor->setTrailingIcon(color.isValid() ? u8"\U000F0765" : u8"\U000f0766");
        editor->setTrailingIconColor(color);
    }
}

void TextFieldItemDelegate::setModelData(QWidget* _editor, QAbstractItemModel* _model,
                                         const QModelIndex& _index) const
{
    QScopedValueRollback<bool> isInSetModelData(m_isInSetModelData, true);

    auto editor = qobject_cast<TextField*>(_editor);
    const auto text = editor->text();
    const auto color = editor->trailingIconColor();

    _model->setData(_index, text, Qt::EditRole);
    if (m_isTrailingIconPickColor) {
        _model->setData(_index, color, Qt::DecorationPropertyRole);
    }
}


// ****


ComboBoxItemDelegate::ComboBoxItemDelegate(QObject* _parent, QAbstractItemModel* _model)
    : TreeDelegate(_parent)
    , m_model(_model)
{
}

void ComboBoxItemDelegate::setLabel(const QString& _label)
{
    m_label = _label;
}

QWidget* ComboBoxItemDelegate::createEditor(QWidget* _parent, const QStyleOptionViewItem& _option,
                                            const QModelIndex& _index) const
{
    Q_UNUSED(_option);
    Q_UNUSED(_index);

    auto editor = new ComboBox(_parent);
    editor->setModel(m_model);
    editor->setBackgroundColor(_parent->palette().text().color());
    editor->setTextColor(_parent->palette().text().color());
    editor->setPopupBackgroundColor(Ui::DesignSystem::color().background());
    editor->setDefaultMarginsEnabled(false);
    editor->setUseContentsWidth(true);
    editor->setLabel(m_label);
    return editor;
}

void ComboBoxItemDelegate::setEditorData(QWidget* _editor, const QModelIndex& _index) const
{
    TreeDelegate::setEditorData(_editor, _index);

    const QString value = _index.model()->data(_index, Qt::EditRole).toString();

    auto comboBox = qobject_cast<ComboBox*>(_editor);
    comboBox->setCurrentText(value);
}

void ComboBoxItemDelegate::setModelData(QWidget* _editor, QAbstractItemModel* _model,
                                        const QModelIndex& _index) const
{
    auto comboBox = qobject_cast<ComboBox*>(_editor);
    const QString value = comboBox->currentText();
    _model->setData(_index, value, Qt::EditRole);
}
