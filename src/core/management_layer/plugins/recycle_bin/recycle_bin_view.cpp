#include "recycle_bin_view.h"

#include <interfaces/management_layer/i_document_manager.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/label/label.h>
#include <utils/helpers/color_helper.h>

#include <QBoxLayout>


namespace Ui {

class RecycleBinView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Задать количество удалённых документов
     */
    void setDocumentsToRemove(int _size);

    /**
     * @brief Обновить состояние представления в зависимости от количества удалённых документов
     */
    void updateState();


    /**
     * @brief Количество файлов в корзине
     */
    int documentsToRemove = 0;

    IconsBigLabel* iconLabel = nullptr;
    H6Label* titleLabel = nullptr;
    Body1Label* subtitleLabel = nullptr;
    Button* emptyRecycleBinButton = nullptr;
};

RecycleBinView::Implementation::Implementation(QWidget* _parent)
    : iconLabel(new IconsBigLabel(_parent))
    , titleLabel(new H6Label(_parent))
    , subtitleLabel(new Body1Label(_parent))
    , emptyRecycleBinButton(new Button(_parent))
{
    iconLabel->setDecorationVisible(true);
    titleLabel->setAlignment(Qt::AlignCenter);
    subtitleLabel->setAlignment(Qt::AlignCenter);

    updateState();
}

void RecycleBinView::Implementation::setDocumentsToRemove(int _size)
{
    if (documentsToRemove == _size) {
        return;
    }

    documentsToRemove = _size;
    updateState();
}

void RecycleBinView::Implementation::updateState()
{
    if (documentsToRemove == 0) {
        iconLabel->setIcon(u8"\U000F06CC");
        titleLabel->setText(tr("Recycle bin is empty."));
        subtitleLabel->setText(tr("There are no documents in the recycle bin, but when they will "
                                  "be here, you'll be able to check them out."));
        emptyRecycleBinButton->hide();
    } else {
        iconLabel->setIcon(u8"\U000F01B4");
        titleLabel->setText(
            tr("There are %n document(s) in the recycle bin.", "", documentsToRemove));
        subtitleLabel->setText(tr("Select a document to see its contents."));
        emptyRecycleBinButton->setText(tr("Empty recycle bin"));
        emptyRecycleBinButton->show();
    }
}


// ****


RecycleBinView::RecycleBinView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    setFocusPolicy(Qt::StrongFocus);

    auto layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addStretch();
    layout->addWidget(d->iconLabel, 0, Qt::AlignHCenter);
    layout->addWidget(d->titleLabel, 0, Qt::AlignHCenter);
    layout->addWidget(d->subtitleLabel, 0, Qt::AlignHCenter);
    layout->addWidget(d->emptyRecycleBinButton, 0, Qt::AlignHCenter);
    layout->addStretch();
    setLayout(layout);


    connect(d->emptyRecycleBinButton, &Button::clicked, this,
            &RecycleBinView::emptyRecycleBinPressed);
}

RecycleBinView::~RecycleBinView() = default;

QWidget* RecycleBinView::asQWidget()
{
    return this;
}

void RecycleBinView::setEditingMode(ManagementLayer::DocumentEditingMode _mode)
{
    const auto enabled = _mode == ManagementLayer::DocumentEditingMode::Edit;
    d->emptyRecycleBinButton->setEnabled(enabled);
}

void RecycleBinView::setDocumentsToRemoveSize(int _size)
{
    d->setDocumentsToRemove(_size);
}

void RecycleBinView::updateTranslations()
{
    d->updateState();
}

void RecycleBinView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().surface());

    d->iconLabel->setBackgroundColor(Ui::DesignSystem::color().surface());
    d->iconLabel->setTextColor(Ui::DesignSystem::color().accent());
    d->titleLabel->setBackgroundColor(Ui::DesignSystem::color().surface());
    d->titleLabel->setTextColor(Ui::DesignSystem::color().onSurface());
    d->titleLabel->setContentsMargins(Ui::DesignSystem::layout().px24(),
                                      Ui::DesignSystem::compactLayout().px24(),
                                      Ui::DesignSystem::layout().px24(), 0);
    d->subtitleLabel->setBackgroundColor(Ui::DesignSystem::color().surface());
    d->subtitleLabel->setTextColor(ColorHelper::transparent(
        Ui::DesignSystem::color().onSurface(), Ui::DesignSystem::inactiveTextOpacity()));
    d->subtitleLabel->setContentsMargins(
        Ui::DesignSystem::layout().px(128), Ui::DesignSystem::compactLayout().px24(),
        Ui::DesignSystem::layout().px(128), Ui::DesignSystem::compactLayout().px24());
    d->emptyRecycleBinButton->setBackgroundColor(Ui::DesignSystem::color().accent());
    d->emptyRecycleBinButton->setTextColor(Ui::DesignSystem::color().accent());
}

} // namespace Ui
