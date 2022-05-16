#include "stageplay_text_manager.h"

#include "stageplay_text_view.h"

#include <business_layer/model/stageplay/text/stageplay_text_model.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>
#include <ui/modules/bookmarks/bookmark_dialog.h>

#include <QApplication>
#include <QFileDialog>


namespace ManagementLayer {

namespace {
const QString kSettingsKey = "stageplay-text";
QString cursorPositionFor(Domain::DocumentObject* _item)
{
    return QString("%1/%2/last-cursor").arg(kSettingsKey, _item->uuid().toString());
}
QString verticalScrollFor(Domain::DocumentObject* _item)
{
    return QString("%1/%2/vertical-scroll").arg(kSettingsKey, _item->uuid().toString());
}
} // namespace

class StageplayTextManager::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Создать представление
     */
    Ui::StageplayTextView* createView();

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
    QPointer<BusinessLayer::StageplayTextModel> model;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::StageplayTextView* view = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::StageplayTextView*> allViews;
};

StageplayTextManager::Implementation::Implementation()
{
    view = createView();
    loadViewSettings();
}

Ui::StageplayTextView* StageplayTextManager::Implementation::createView()
{
    allViews.append(new Ui::StageplayTextView);
    return allViews.last();
}

void StageplayTextManager::Implementation::loadViewSettings()
{
    view->loadViewSettings();
}

void StageplayTextManager::Implementation::saveViewSettings()
{
    view->saveViewSettings();
}

void StageplayTextManager::Implementation::loadModelSettings()
{
    const auto cursorPosition = settingsValue(cursorPositionFor(model->document()), 0).toInt();
    view->setCursorPosition(cursorPosition);
    const auto verticalScroll = settingsValue(verticalScrollFor(model->document()), 0).toInt();
    view->setverticalScroll(verticalScroll);
}

void StageplayTextManager::Implementation::saveModelSettings()
{
    setSettingsValue(cursorPositionFor(model->document()), view->cursorPosition());
    setSettingsValue(verticalScrollFor(model->document()), view->verticalScroll());
}


// ****


StageplayTextManager::StageplayTextManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
    connect(d->view, &Ui::StageplayTextView::currentModelIndexChanged, this,
            &StageplayTextManager::currentModelIndexChanged);
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
    connect(d->view, &Ui::StageplayTextView::addBookmarkRequested, this, [showBookmarkDialog] {
        showBookmarkDialog(Ui::BookmarkDialog::DialogType::CreateNew);
    });
    connect(d->view, &Ui::StageplayTextView::editBookmarkRequested, this,
            [showBookmarkDialog] { showBookmarkDialog(Ui::BookmarkDialog::DialogType::Edit); });
    connect(d->view, &Ui::StageplayTextView::createBookmarkRequested, this,
            [this](const QString& _text, const QColor& _color) {
                auto item = d->model->itemForIndex(d->view->currentModelIndex());
                if (item->type() != BusinessLayer::TextModelItemType::Text) {
                    return;
                }

                auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
                textItem->setBookmark({ _color, _text });
                d->model->updateItem(textItem);
            });
    connect(d->view, &Ui::StageplayTextView::changeBookmarkRequested, this,
            [this](const QModelIndex& _index, const QString& _text, const QColor& _color) {
                auto item = d->model->itemForIndex(_index);
                if (item->type() != BusinessLayer::TextModelItemType::Text) {
                    return;
                }

                auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
                textItem->setBookmark({ _color, _text });
                d->model->updateItem(textItem);
            });
    connect(d->view, &Ui::StageplayTextView::removeBookmarkRequested, this, [this] {
        auto item = d->model->itemForIndex(d->view->currentModelIndex());
        if (item->type() != BusinessLayer::TextModelItemType::Text) {
            return;
        }

        auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
        textItem->clearBookmark();
        d->model->updateItem(textItem);
    });
}

StageplayTextManager::~StageplayTextManager() = default;

QObject* StageplayTextManager::asQObject()
{
    return this;
}

void StageplayTextManager::setModel(BusinessLayer::AbstractModel* _model)
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
    d->model = qobject_cast<BusinessLayer::StageplayTextModel*>(_model);
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
    }
}

Ui::IDocumentView* StageplayTextManager::view()
{
    return d->view;
}

Ui::IDocumentView* StageplayTextManager::createView()
{
    return d->createView();
}

void StageplayTextManager::reconfigure(const QStringList& _changedSettingsKeys)
{
    d->view->reconfigure(_changedSettingsKeys);
}

void StageplayTextManager::bind(IDocumentManager* _manager)
{
    Q_ASSERT(_manager);

    const auto isConnectedFirstTime
        = connect(_manager->asQObject(), SIGNAL(currentModelIndexChanged(QModelIndex)), this,
                  SLOT(setCurrentModelIndex(QModelIndex)), Qt::UniqueConnection);

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

void StageplayTextManager::saveSettings()
{
    d->saveViewSettings();

    if (d->model != nullptr) {
        d->saveModelSettings();
    }
}

void StageplayTextManager::setCurrentModelIndex(const QModelIndex& _index)
{
    QSignalBlocker blocker(this);

    if (!_index.isValid()) {
        return;
    }

    d->view->setCurrentModelIndex(_index);
    d->view->setFocus();
}

} // namespace ManagementLayer
