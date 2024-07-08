#include "fast_format_widget.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/check_box/check_box.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>
#include <utils/tools/run_once.h>

#include <QAbstractItemModel>
#include <QPainter>
#include <QPointer>
#include <QSettings>
#include <QVBoxLayout>


namespace Ui {

namespace {
const char* kButtonTypeKey = "button-type";
const char* kIsButtonCurrentTypeKey = "is-button-current-type";
const char* kShowShortcutsKey = "fast-format-widget/show-shortcuts";
} // namespace

/**
 * @brief Кнопка панели инструментов
 */
class FormatButton : public Button
{
public:
    explicit FormatButton(QWidget* _parent);

    /**
     * @brief Задать текст горячей кнопки
     */
    void setShortcut(const QString& _shortcut);

    /**
     * @brief Переопределяем минимальный размер, чтобы панель выглядела опрятно
     */
    QSize sizeHint() const override;

protected:
    /**
     * @brief Переопределяем, чтобы нарисовать текст шортката
     */
    void paintEvent(QPaintEvent* _event) override;

private:
    /**
     * @brief Текст шортката
     */
    QString m_shortcut;
};

FormatButton::FormatButton(QWidget* _parent)
    : Button(_parent)
{
    setFocusPolicy(Qt::NoFocus);
    setFlat(true);
}

void FormatButton::setShortcut(const QString& _shortcut)
{
    if (m_shortcut == _shortcut) {
        return;
    }

    m_shortcut = _shortcut;
    updateGeometry();
    update();
}

QSize FormatButton::sizeHint() const
{
    const auto shortcutWidth
        = TextHelper::fineTextWidthF(m_shortcut, Ui::DesignSystem::font().overline());
    return Button::sizeHint() + QSize(shortcutWidth + DesignSystem::button().spacing(), 0);
}

void FormatButton::paintEvent(QPaintEvent* _event)
{
    Button::paintEvent(_event);

    //
    // Рисуем шорткат
    //
    QPainter painter(this);
    //
    // ... настроим цвет текста, как в самой кнопке
    //
    painter.setPen(
        isEnabled() ? textColor()
                    : ColorHelper::transparent(textColor(), DesignSystem::disabledTextOpacity()));
    //
    // ... собственно добавим текст шортката
    //
    const QRectF shortcutRect
        = contentsRect().marginsRemoved(DesignSystem::button().margins().toMargins());
    painter.setFont(DesignSystem::font().overline());
    painter.drawText(shortcutRect, Qt::AlignVCenter | Qt::AlignRight, m_shortcut);
}


// ****


class FastFormatWidget::Implementation
{
public:
    explicit Implementation(FastFormatWidget* _q);

    /**
     * @brief Обновить кнопки в соответствии с текущей моделью стилей
     */
    void updateButtons();


    FastFormatWidget* q = nullptr;

    /**
     * @brief Модель типов форматов блоков
     */
    QPointer<QAbstractItemModel> model;

    /**
     * @brief Список кнопок
     */
    QVector<FormatButton*> buttons;

    /**
     * @brief Показывать ли шорткаты на кнопках с форматами
     */
    CheckBox* showShortcuts = nullptr;

    /**
     * @brief Компоновщик
     */
    QVBoxLayout* buttonsLayout = nullptr;
};

FastFormatWidget::Implementation::Implementation(FastFormatWidget* _q)
    : q(_q)
    , showShortcuts(new CheckBox(_q))
    , buttonsLayout(new QVBoxLayout)
{
    buttonsLayout->setSpacing(0);
    buttonsLayout->setContentsMargins({});

    showShortcuts->setChecked(QSettings().value(kShowShortcutsKey, false).toBool());
}

void FastFormatWidget::Implementation::updateButtons()
{
    const auto canRun = RunOnce::tryRun(Q_FUNC_INFO);
    if (!canRun) {
        return;
    }

    //
    // Создаём необходимое количество кнопок
    //
    for (int index = 0; index < model->rowCount() - buttons.size(); ++index) {
        auto button = new FormatButton(q);
        button->setBackgroundColor(ColorHelper::nearby(DesignSystem::color().primary()));
        button->setTextColor(DesignSystem::color().onPrimary());
        connect(button, &FormatButton::clicked, q, [this, button] {
            emit q->paragraphTypeChanged(button->property(kButtonTypeKey).toModelIndex());
        });
        buttons.append(button);

        //
        // Вставляем в лейаут перед чекбоксом отображения шоркатов
        //
        buttonsLayout->addWidget(button);
    }

    //
    // Настраиваем видимые кнопки
    //
    int itemIndex = 0;
    for (; itemIndex < model->rowCount(); ++itemIndex) {
        const auto itemModelIndex = model->index(itemIndex, 0);
        auto button = buttons.at(itemIndex);
        button->setVisible(true);
        button->setText(itemModelIndex.data(Qt::DisplayRole).toString());
        button->setShortcut(
            showShortcuts->isChecked() ? itemModelIndex.data(Qt::ToolTipRole).toString() : "");
        button->setProperty(kButtonTypeKey, itemModelIndex);
    }
    //
    // Остальные скрываем
    //
    for (; itemIndex < buttons.count(); ++itemIndex) {
        buttons.at(itemIndex)->setVisible(false);
    }

    //
    // Настроим минимальный размер панели быстрого форматирования
    // NOTE: делаем это отложенно, чтобы лейаут успел пересчитать идеальные размеры кнопок
    //
    QMetaObject::invokeMethod(
        q, [this] { q->setMinimumSize(q->sizeHint()); }, Qt::QueuedConnection);
}


// ****


FastFormatWidget::FastFormatWidget(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    auto layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins({});
    layout->addLayout(d->buttonsLayout);
    layout->addWidget(d->showShortcuts);
    layout->addStretch();

    connect(d->showShortcuts, &CheckBox::checkedChanged, this, [this](bool _checked) {
        d->updateButtons();
        QSettings().setValue(kShowShortcutsKey, _checked);
    });
}

FastFormatWidget::~FastFormatWidget() = default;

void FastFormatWidget::setParagraphTypesModel(QAbstractItemModel* _model)
{
    if (d->model == _model) {
        return;
    }

    if (d->model != nullptr) {
        d->model->disconnect(this);
    }

    d->model = _model;
    d->updateButtons();

    connect(d->model, &QAbstractItemModel::dataChanged, this, [this] { d->updateButtons(); });
    connect(d->model, &QAbstractItemModel::rowsRemoved, this, [this] { d->updateButtons(); });
    connect(d->model, &QAbstractItemModel::rowsInserted, this, [this] { d->updateButtons(); });
    connect(d->model, &QAbstractItemModel::modelReset, this, [this] { d->updateButtons(); });
}

void FastFormatWidget::setCurrentParagraphType(const QModelIndex& _index)
{
    for (auto button : std::as_const(d->buttons)) {
        const bool isCurrentType = button->property(kButtonTypeKey).toModelIndex() == _index;
        button->setProperty(kIsButtonCurrentTypeKey, isCurrentType);
        button->setTextColor(isCurrentType ? DesignSystem::color().accent()
                                           : DesignSystem::color().onPrimary());
    }
}

void FastFormatWidget::updateTranslations()
{
    d->showShortcuts->setText(tr("Show shortcuts"));
}

void FastFormatWidget::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().primary());

    d->buttonsLayout->setSpacing(Ui::DesignSystem::compactLayout().px16());
    d->buttonsLayout->setContentsMargins(
        DesignSystem::layout().px24(), DesignSystem::layout().px24(), DesignSystem::layout().px24(),
        DesignSystem::compactLayout().px12());
    for (auto button : std::as_const(d->buttons)) {
        button->setBackgroundColor(ColorHelper::nearby(DesignSystem::color().primary()));
        button->setTextColor(button->property(kIsButtonCurrentTypeKey).toBool()
                                 ? DesignSystem::color().accent()
                                 : DesignSystem::color().onPrimary());
    }
    d->showShortcuts->setBackgroundColor(DesignSystem::color().primary());
    d->showShortcuts->setTextColor(DesignSystem::color().onPrimary());
}

} // namespace Ui
