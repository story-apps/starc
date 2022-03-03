#include "title_page_edit_toolbar.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/card/card_popup_with_tree.h>
#include <utils/helpers/measurement_helper.h>
#include <utils/helpers/text_helper.h>

#include <QAction>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QStringListModel>


namespace Ui {

class TitlePageEditToolbar::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Показать попап
     */
    void showPopup(TitlePageEditToolbar* _parent, QAction* _forAction);


    QAction* undoAction = nullptr;
    QAction* redoAction = nullptr;
    QAction* textFontAction = nullptr;
    QAction* textFontSizeAction = nullptr;
    QAction* restoreTitlePageAction = nullptr;

    QStringListModel fontsModel;
    QStringListModel fontSizesModel;

    CardPopupWithTree* popup = nullptr;
};

TitlePageEditToolbar::Implementation::Implementation(QWidget* _parent)
    : undoAction(new QAction)
    , redoAction(new QAction)
    , textFontAction(new QAction)
    , textFontSizeAction(new QAction)
    , restoreTitlePageAction(new QAction)
    , popup(new CardPopupWithTree(_parent))
{
    undoAction->setIconText(u8"\U000f054c");
    redoAction->setIconText(u8"\U000f044e");
    textFontAction->setIconText(u8"\U000f035d");
    textFontSizeAction->setIconText(u8"\U000f035d");
    restoreTitlePageAction->setIconText(u8"\U000F099B");

    fontsModel.setStringList(QFontDatabase().families());
    fontSizesModel.setStringList(
        { "8", "9", "10", "11", "12", "14", "18", "24", "30", "36", "48", "60", "72", "96" });
}

void TitlePageEditToolbar::Implementation::showPopup(TitlePageEditToolbar* _parent,
                                                     QAction* _forAction)
{
    const auto width = Ui::DesignSystem::floatingToolBar().spacing() * 2
        + _parent->actionCustomWidth(_forAction);

    const auto left = QPoint(
        Ui::DesignSystem::floatingToolBar().shadowMargins().left()
            + Ui::DesignSystem::floatingToolBar().margins().left()
            + Ui::DesignSystem::floatingToolBar().iconSize().width() * 2
            //
            // Тут костылик, чтобы понимать, что нужно дополнительное смещение для попапа с размером
            // шрифта
            //
            + (_forAction == textFontSizeAction ? Ui::DesignSystem::floatingToolBar().spacing()
                       + _parent->actionCustomWidth(textFontAction)
                                                : 0)
            //
            + Ui::DesignSystem::floatingToolBar().spacing()
            - Ui::DesignSystem::card().shadowMargins().left(),
        _parent->rect().bottom() - Ui::DesignSystem::floatingToolBar().shadowMargins().bottom());
    const auto position = _parent->mapToGlobal(left)
        + QPointF(Ui::DesignSystem::textField().margins().left(),
                  -Ui::DesignSystem::textField().margins().bottom());

    popup->showPopup(position.toPoint(), _parent->height(), width, 9);
}


// ****


TitlePageEditToolbar::TitlePageEditToolbar(QWidget* _parent)
    : FloatingToolBar(_parent)
    , d(new Implementation(this))
{
    addAction(d->undoAction);
    connect(d->undoAction, &QAction::triggered, this, &TitlePageEditToolbar::undoPressed);

    addAction(d->redoAction);
    connect(d->redoAction, &QAction::triggered, this, &TitlePageEditToolbar::redoPressed);

    //
    // FIXME: дефолтный шрифт из шаблона
    //
    const QFont defaultFont;
    d->textFontAction->setText(defaultFont.family());
    addAction(d->textFontAction);
    d->textFontSizeAction->setText(QString::number(defaultFont.pointSize()));
    addAction(d->textFontSizeAction);
    auto activatePopup = [this](QAction* _action, QStringListModel* _model) {
        QSignalBlocker blocker(d->popup);
        d->popup->setContentModel(_model);
        {
            //                QSignalBlocker signalBlocker(this);
            //                const int currentItemRow =
            //                _model->stringList().indexOf(_action->text());
            //                d->popupContent->setCurrentIndex(_model->index(currentItemRow,
            //                0));
        }
        _action->setIconText(u8"\U000f0360");
        d->showPopup(this, _action);
    };
    connect(d->textFontAction, &QAction::triggered, this,
            [this, activatePopup] { activatePopup(d->textFontAction, &d->fontsModel); });
    connect(d->textFontSizeAction, &QAction::triggered, this,
            [this, activatePopup] { activatePopup(d->textFontSizeAction, &d->fontSizesModel); });

    connect(
        d->popup, &CardPopupWithTree::currentIndexChanged, this, [this](const QModelIndex& _index) {
            if (d->popup->contentModel() == &d->fontsModel) {
                d->textFontAction->setText(_index.data().toString());
            } else {
                d->textFontSizeAction->setText(_index.data().toString());
            }
            update();

            QFont newFont(d->textFontAction->text());
            newFont.setPixelSize(MeasurementHelper::ptToPx(d->textFontSizeAction->text().toInt()));
            emit fontChanged(newFont);
        });
    connect(d->popup, &CardPopupWithTree::disappeared, this, [this] {
        d->textFontAction->setIconText(u8"\U000f035d");
        d->textFontSizeAction->setIconText(u8"\U000f035d");
    });


    addAction(d->restoreTitlePageAction);
    connect(d->restoreTitlePageAction, &QAction::triggered, this,
            &TitlePageEditToolbar::restoreTitlePagePressed);

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

TitlePageEditToolbar::~TitlePageEditToolbar() = default;

void TitlePageEditToolbar::setCurrentFont(const QFont& _font)
{
    //
    // Не вызываем сигнал о смене типа шрифта, т.к. это сделал не пользователь
    //
    QSignalBlocker blocker(this);

    d->textFontAction->setText(_font.family());
    d->textFontSizeAction->setText(QString::number(MeasurementHelper::pxToPt(_font.pixelSize())));
}

void TitlePageEditToolbar::updateTranslations()
{
    d->undoAction->setToolTip(
        tr("Undo last action")
        + QString(" (%1)").arg(
            QKeySequence(QKeySequence::Undo).toString(QKeySequence::NativeText)));
    d->redoAction->setToolTip(
        tr("Redo last action")
        + QString(" (%1)").arg(
            QKeySequence(QKeySequence::Redo).toString(QKeySequence::NativeText)));
    d->textFontAction->setToolTip(tr("Current text font family"));
    d->textFontSizeAction->setToolTip(tr("Current text font size"));
    d->restoreTitlePageAction->setToolTip(tr("Restore default title page"));
}

void TitlePageEditToolbar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    FloatingToolBar::designSystemChangeEvent(_event);

    auto findMaxWidthFor = [](const QStringList& _list) {
        int maxFontNameWidth = 0;
        for (const auto& fontName : _list) {
            maxFontNameWidth = std::max(
                maxFontNameWidth,
                TextHelper::fineTextWidth(fontName, Ui::DesignSystem::font().subtitle2()));
        }
        return maxFontNameWidth;
    };

    setActionCustomWidth(
        d->textFontAction,
        static_cast<int>(Ui::DesignSystem::treeOneLineItem().margins().left())
            + findMaxWidthFor(d->fontsModel.stringList())
            + static_cast<int>(Ui::DesignSystem::treeOneLineItem().margins().right()));
    setActionCustomWidth(
        d->textFontSizeAction,
        static_cast<int>(Ui::DesignSystem::treeOneLineItem().margins().left())
            + findMaxWidthFor(d->fontSizesModel.stringList())
            + static_cast<int>(Ui::DesignSystem::treeOneLineItem().margins().right()));

    d->popup->setBackgroundColor(Ui::DesignSystem::color().background());
    d->popup->setTextColor(Ui::DesignSystem::color().onBackground());

    resize(sizeHint());
}

} // namespace Ui
