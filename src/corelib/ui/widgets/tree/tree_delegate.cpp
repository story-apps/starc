#include "tree_delegate.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/combo_box/combo_box.h>
#include <ui/widgets/key_sequence_edit/key_sequence_edit.h>
#include <utils/helpers/text_helper.h>

#include <QPainter>


TreeDelegate::TreeDelegate(QObject* _parent)
    : QStyledItemDelegate(_parent)
{
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

    auto backgroundColor = opt.palette.color(QPalette::Base);
    auto textColor = opt.palette.color(QPalette::Text);

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
        if (_index.data(Qt::DecorationPropertyRole).isValid()
            && _index.data(Qt::DecorationPropertyRole).value<QColor>().isValid()) {

            _painter->setPen(_index.data(Qt::DecorationPropertyRole).value<QColor>());
        }

        iconRect = QRectF(QPointF(std::max(backgroundRect.left(),
                                           Ui::DesignSystem::treeOneLineItem().margins().left()),
                                  backgroundRect.top()),
                          QSizeF(Ui::DesignSystem::treeOneLineItem().iconSize().width(),
                                 backgroundRect.height()));
        _painter->setFont(Ui::DesignSystem::font().iconsMid());
        _painter->drawText(iconRect, Qt::AlignCenter, _index.data(Qt::DecorationRole).toString());
    }

    //
    // ... текст
    //
    _painter->setPen(textColor);
    _painter->setFont(Ui::DesignSystem::font().body1());
    const qreal textLeft = iconRect.isValid()
        ? iconRect.right() + Ui::DesignSystem::treeOneLineItem().spacing()
        : backgroundRect.left() + Ui::DesignSystem::treeOneLineItem().margins().left();
    const QRectF textRect(QPointF(textLeft, backgroundRect.top()),
                          QSizeF(backgroundRect.right() - textLeft
                                     - Ui::DesignSystem::treeOneLineItem().margins().right(),
                                 backgroundRect.height()));
    const auto text = _painter->fontMetrics().elidedText(_index.data().toString(), Qt::ElideRight,
                                                         static_cast<int>(textRect.width()));
    _painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);
}

QSize TreeDelegate::sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const
{
    Q_UNUSED(_option)
    Q_UNUSED(_index)

    return QSizeF(TextHelper::fineTextWidthF(_index.data().toString(),
                                             Ui::DesignSystem::font().body1()),
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

QWidget* KeySequenceDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                                           const QModelIndex& _index) const
{
    Q_UNUSED(option);
    Q_UNUSED(_index);

    auto editor = new KeySequenceEdit(parent);
    editor->setBackgroundColor(Ui::DesignSystem::color().background());
    editor->setTextColor(Ui::DesignSystem::color().onBackground());
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


ComboBoxItemDelegate::ComboBoxItemDelegate(QObject* _parent, QAbstractItemModel* _model)
    : TreeDelegate(_parent)
    , m_model(_model)
{
}

void ComboBoxItemDelegate::setLabel(const QString& _label)
{
    m_label = _label;
}

QWidget* ComboBoxItemDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                                            const QModelIndex& _index) const
{
    Q_UNUSED(option);
    Q_UNUSED(_index);

    auto editor = new ComboBox(parent);
    editor->setModel(m_model);
    editor->setBackgroundColor(Ui::DesignSystem::color().background());
    editor->setTextColor(Ui::DesignSystem::color().onBackground());
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
