#include "screenplay_text_fast_format_widget.h"

#include "screenplay_text_edit.h"

#include <business_layer/templates/screenplay_template.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>
#include <utils/tools/run_once.h>

#include <QAbstractItemModel>
#include <QPainter>
#include <QVBoxLayout>


namespace Ui {

namespace {
const char* kButtonTypeKey = "button-type";
const char* kIsButtonCurrentTypeKey = "is-button-current-type";
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


class ScreenplayTextFastFormatWidget::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Обновить кнопки в соответствии с текущей моделью стилей
     */
    void updateButtons();


    /**
     * @brief Модель типов форматов блоков
     */
    QAbstractItemModel* model = nullptr;

    /**
     * @brief Список кнопок
     */
    QList<FormatButton*> buttons;
};

ScreenplayTextFastFormatWidget::Implementation::Implementation(QWidget* _parent)
{
    //
    // Создаём столько кнопок, сколько может быть стилей
    //
    for (int index = 0; index < 12; ++index) {
        buttons.append(new FormatButton(_parent));
    }
}

void ScreenplayTextFastFormatWidget::Implementation::updateButtons()
{
    const auto canRun = RunOnce::tryRun(Q_FUNC_INFO);
    if (!canRun) {
        return;
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
        button->setShortcut(itemModelIndex.data(Qt::WhatsThisRole).toString());
        button->setProperty(kButtonTypeKey, itemModelIndex);
    }
    //
    // Остальные скрываем
    //
    for (; itemIndex < buttons.count(); ++itemIndex) {
        buttons.at(itemIndex)->setVisible(false);
    }


    auto w = buttons.first()->parentWidget();
    w->setMinimumSize(w->sizeHint());
}


// ****


ScreenplayTextFastFormatWidget::ScreenplayTextFastFormatWidget(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    for (auto button : d->buttons) {
        connect(button, &Button::clicked, this, [this, button] {
            emit paragraphTypeChanged(button->property(kButtonTypeKey).toModelIndex());
        });
    }

    auto layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins({});
    for (auto button : d->buttons) {
        layout->addWidget(button);
    }
    layout->addStretch();

    designSystemChangeEvent(nullptr);
}

ScreenplayTextFastFormatWidget::~ScreenplayTextFastFormatWidget() = default;

void ScreenplayTextFastFormatWidget::setParagraphTypesModel(QAbstractItemModel* _model)
{
    if (d->model == _model) {
        return;
    }

    if (d->model != nullptr) {
        d->model->disconnect(this);
    }

    d->model = _model;

    connect(d->model, &QAbstractItemModel::dataChanged, this, [this] { d->updateButtons(); });
    connect(d->model, &QAbstractItemModel::rowsRemoved, this, [this] { d->updateButtons(); });
    connect(d->model, &QAbstractItemModel::rowsInserted, this, [this] { d->updateButtons(); });
    connect(d->model, &QAbstractItemModel::modelReset, this, [this] { d->updateButtons(); });
}

void ScreenplayTextFastFormatWidget::setCurrentParagraphType(const QModelIndex& _index)
{
    for (auto button : d->buttons) {
        const bool isCurrentType = button->property(kButtonTypeKey).toModelIndex() == _index;
        button->setProperty(kIsButtonCurrentTypeKey, isCurrentType);
        button->setTextColor(isCurrentType ? DesignSystem::color().secondary()
                                           : DesignSystem::color().onPrimary());
    }
}

void ScreenplayTextFastFormatWidget::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().primary());

    layout()->setContentsMargins(DesignSystem::layout().px8(), DesignSystem::layout().px8(),
                                 DesignSystem::layout().px8(), DesignSystem::layout().px8());
    for (auto button : d->buttons) {
        button->setBackgroundColor(ColorHelper::nearby(DesignSystem::color().primary()));
        button->setTextColor(button->property(kIsButtonCurrentTypeKey).toBool()
                                 ? DesignSystem::color().secondary()
                                 : DesignSystem::color().onPrimary());
    }
}

} // namespace Ui
