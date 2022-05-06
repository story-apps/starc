#include "screenplay_treatment_manager.h"

#include "screenplay_treatment_view.h"

#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>
#include <ui/modules/bookmarks/bookmark_dialog.h>

#include <QApplication>
#include <QFileDialog>


namespace ManagementLayer {

namespace {
const QString kSettingsKey = "screenplay-treatment";
QString cursorPositionFor(Domain::DocumentObject* _item)
{
    return QString("%1/%2/last-cursor").arg(kSettingsKey, _item->uuid().toString());
}
QString verticalScrollFor(Domain::DocumentObject* _item)
{
    return QString("%1/%2/vertical-scroll").arg(kSettingsKey, _item->uuid().toString());
}
} // namespace

class ScreenplayTreatmentManager::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Создать представление
     */
    Ui::ScreenplayTreatmentView* createView();

    /**
     * @brief Работа с параметрами отображения представления
     */
    void loadViewSettings();
    void saveViewSettings();

    /**
     * @brief Работа с параметрами отображения текущей модели
     */
    void loadModelSettings();
    void saveModelSettings();


    /**
     * @brief Текущая модель представления основного окна
     */
    QPointer<BusinessLayer::ScreenplayTextModel> model;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::ScreenplayTreatmentView* view = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::ScreenplayTreatmentView*> allViews;
};

ScreenplayTreatmentManager::Implementation::Implementation()
{
    view = createView();
    loadViewSettings();
}

Ui::ScreenplayTreatmentView* ScreenplayTreatmentManager::Implementation::createView()
{
    allViews.append(new Ui::ScreenplayTreatmentView);
    return allViews.last();
}

void ScreenplayTreatmentManager::Implementation::loadViewSettings()
{
    view->loadViewSettings();
}

void ScreenplayTreatmentManager::Implementation::saveViewSettings()
{
    view->saveViewSettings();
}

void ScreenplayTreatmentManager::Implementation::loadModelSettings()
{
    const auto cursorPosition = settingsValue(cursorPositionFor(model->document()), 0).toInt();
    view->setCursorPosition(cursorPosition);
    const auto verticalScroll = settingsValue(verticalScrollFor(model->document()), 0).toInt();
    view->setverticalScroll(verticalScroll);
}

void ScreenplayTreatmentManager::Implementation::saveModelSettings()
{
    setSettingsValue(cursorPositionFor(model->document()), view->cursorPosition());
    setSettingsValue(verticalScrollFor(model->document()), view->verticalScroll());
}


// ****


ScreenplayTreatmentManager::ScreenplayTreatmentManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
    connect(d->view, &Ui::ScreenplayTreatmentView::currentModelIndexChanged, this,
            &ScreenplayTreatmentManager::currentModelIndexChanged);
    auto showBookmarkDialog = [this](Ui::BookmarkDialog::DialogType _type) {
        auto item = d->model->itemForIndex(d->view->currentModelIndex());
        if (item->type() != BusinessLayer::TextModelItemType::Text) {
            return;
        }

        auto dialog = new Ui::BookmarkDialog(d->view->topLevelWidget());
        dialog->setDialogType(_type);
        if (_type == Ui::BookmarkDialog::DialogType::Edit) {
            const auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
            dialog->setBookmarkName(textItem->bookmark()->name);
            dialog->setBookmarkColor(textItem->bookmark()->color);
        }
        connect(dialog, &Ui::BookmarkDialog::savePressed, this, [this, item, dialog] {
            auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
            textItem->setBookmark({ dialog->bookmarkColor(), dialog->bookmarkName() });
            d->model->updateItem(textItem);

            dialog->hideDialog();
        });
        connect(dialog, &Ui::BookmarkDialog::disappeared, dialog, &Ui::BookmarkDialog::deleteLater);

        //
        // Отображаем диалог
        //
        dialog->showDialog();
    };
    connect(
        d->view, &Ui::ScreenplayTreatmentView::addBookmarkRequested, this,
        [showBookmarkDialog] { showBookmarkDialog(Ui::BookmarkDialog::DialogType::CreateNew); });
    connect(d->view, &Ui::ScreenplayTreatmentView::editBookmarkRequested, this,
            [showBookmarkDialog] { showBookmarkDialog(Ui::BookmarkDialog::DialogType::Edit); });
    connect(d->view, &Ui::ScreenplayTreatmentView::createBookmarkRequested, this,
            [this](const QString& _text, const QColor& _color) {
                auto item = d->model->itemForIndex(d->view->currentModelIndex());
                if (item->type() != BusinessLayer::TextModelItemType::Text) {
                    return;
                }

                auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
                textItem->setBookmark({ _color, _text });
                d->model->updateItem(textItem);
            });
    connect(d->view, &Ui::ScreenplayTreatmentView::changeBookmarkRequested, this,
            [this](const QModelIndex& _index, const QString& _text, const QColor& _color) {
                auto item = d->model->itemForIndex(_index);
                if (item->type() != BusinessLayer::TextModelItemType::Text) {
                    return;
                }

                auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
                textItem->setBookmark({ _color, _text });
                d->model->updateItem(textItem);
            });
    connect(d->view, &Ui::ScreenplayTreatmentView::removeBookmarkRequested, this, [this] {
        auto item = d->model->itemForIndex(d->view->currentModelIndex());
        if (item->type() != BusinessLayer::TextModelItemType::Text) {
            return;
        }

        auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
        textItem->clearBookmark();
        d->model->updateItem(textItem);
    });
}

ScreenplayTreatmentManager::~ScreenplayTreatmentManager() = default;

QObject* ScreenplayTreatmentManager::asQObject()
{
    return this;
}

void ScreenplayTreatmentManager::setModel(BusinessLayer::AbstractModel* _model)
{
    if (d->model == _model) {
        return;
    }

    //
    // Если модель была задана
    //
    if (d->model != nullptr) {
        //
        // ... сохраняем её параметры
        //
        d->saveModelSettings();
        //
        // ... разрываем соединения
        //
        d->view->disconnect(d->model);
    }

    //
    // Определяем новую модель
    //
    d->model = qobject_cast<BusinessLayer::ScreenplayTextModel*>(_model);
    d->view->setModel(d->model);

    //
    // Если новая модель задана
    //
    if (d->model != nullptr) {
        //
        // ... загрузим параметры
        //
        d->loadModelSettings();
        //
        // ... настраиваем соединения
        //
        //        d->view->setName(d->model->name());
        //        d->view->setText(d->model->text());

        //        connect(d->model, &BusinessLayer::ScreenplayTextModel::nameChanged,
        //                d->view, &Ui::ScreenplayTextView::setName);
        //        connect(d->model, &BusinessLayer::ScreenplayTextModel::textChanged,
        //                d->view, &Ui::ScreenplayTextView::setText);
        //        //
        //        connect(d->view, &Ui::ScreenplayTextView::nameChanged,
        //                d->model, &BusinessLayer::ScreenplayTextModel::setName);
        //        connect(d->view, &Ui::ScreenplayTextView::textChanged,
        //                d->model, &BusinessLayer::ScreenplayTextModel::setText);
    }
}

Ui::IDocumentView* ScreenplayTreatmentManager::view()
{
    return d->view;
}

Ui::IDocumentView* ScreenplayTreatmentManager::createView()
{
    return d->createView();
}

void ScreenplayTreatmentManager::reconfigure(const QStringList& _changedSettingsKeys)
{
    d->view->reconfigure(_changedSettingsKeys);
}

void ScreenplayTreatmentManager::bind(IDocumentManager* _manager)
{
    Q_ASSERT(_manager);

    const auto isConnectedFirstTime
        = connect(_manager->asQObject(), SIGNAL(currentModelIndexChanged(const QModelIndex&)), this,
                  SLOT(setCurrentModelIndex(const QModelIndex&)), Qt::UniqueConnection);

    //
    // Ставим в очередь событие нотификацию о смене текущей сцены,
    // чтобы навигатор отобразил её при первом открытии
    //
    if (isConnectedFirstTime) {
        QMetaObject::invokeMethod(
            this, [this] { emit currentModelIndexChanged(d->view->currentModelIndex()); },
            Qt::QueuedConnection);
    }
}

void ScreenplayTreatmentManager::saveSettings()
{
    d->saveViewSettings();

    if (d->model != nullptr) {
        d->saveModelSettings();
    }
}

void ScreenplayTreatmentManager::setCurrentModelIndex(const QModelIndex& _index)
{
    QSignalBlocker blocker(this);

    if (!_index.isValid()) {
        return;
    }

    d->view->setCurrentModelIndex(_index);
    d->view->setFocus();
}

} // namespace ManagementLayer
