#include "stageplay_text_fast_format_widget.h"

#include "stageplay_text_edit.h"

#include <business_layer/templates/stageplay_template.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>
#include <utils/tools/run_once.h>

#include <QAbstractItemModel>
#include <QPainter>
#include <QPointer>
#include <QVBoxLayout>


namespace Ui {

namespace {
const char* kButtonTypeKey = "button-type";
const char* kIsButtonCurrentTypeKey = "is-button-current-type";
} // namespace


class StageplayTextFastFormatWidget::Implementation
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
    QPointer<QAbstractItemModel> model;

    /**
     * @brief Список кнопок
     */
    QList<Button*> buttons;
};

StageplayTextFastFormatWidget::Implementation::Implementation(QWidget* _parent)
{
    //
    // Создаём столько кнопок, сколько может быть стилей
    //
    for (int index = 0; index < 12; ++index) {
        buttons.append(new Button(_parent));
    }
}

void StageplayTextFastFormatWidget::Implementation::updateButtons()
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
        button->setFlat(true);
        button->setText(itemModelIndex.data(Qt::DisplayRole).toString());
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


StageplayTextFastFormatWidget::StageplayTextFastFormatWidget(QWidget* _parent)
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

StageplayTextFastFormatWidget::~StageplayTextFastFormatWidget() = default;

void StageplayTextFastFormatWidget::setParagraphTypesModel(QAbstractItemModel* _model)
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

void StageplayTextFastFormatWidget::setCurrentParagraphType(const QModelIndex& _index)
{
    for (auto button : d->buttons) {
        const bool isCurrentType = button->property(kButtonTypeKey).toModelIndex() == _index;
        button->setProperty(kIsButtonCurrentTypeKey, isCurrentType);
        button->setTextColor(isCurrentType ? DesignSystem::color().secondary()
                                           : DesignSystem::color().onPrimary());
    }
}

void StageplayTextFastFormatWidget::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().primary());

    layout()->setSpacing(Ui::DesignSystem::layout().px8());
    layout()->setContentsMargins(DesignSystem::layout().px16(), DesignSystem::layout().px16(),
                                 DesignSystem::layout().px16(), DesignSystem::layout().px16());
    for (auto button : d->buttons) {
        button->setBackgroundColor(ColorHelper::nearby(DesignSystem::color().primary()));
        button->setTextColor(button->property(kIsButtonCurrentTypeKey).toBool()
                                 ? DesignSystem::color().secondary()
                                 : DesignSystem::color().onPrimary());
    }
}

} // namespace Ui
