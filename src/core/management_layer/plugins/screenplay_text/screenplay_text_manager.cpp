#include "screenplay_text_manager.h"

#include "screenplay_text_view.h"

#include <business_layer/model/screenplay/screenplay_dictionaries_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>
#include <ui/dictionaries_view.h>
#include <ui/modules/bookmarks/bookmark_dialog.h>

#include <QApplication>
#include <QFileDialog>
#include <QStringListModel>


namespace ManagementLayer {

namespace {

const int kSceneIntrosIndex = 0;
const int kSceneTimesIndex = 1;
const int kCharacterExtensionsIndex = 2;
const int kTransitionIndex = 3;

const QString kSettingsKey = "screenplay-text";
QString cursorPositionFor(Domain::DocumentObject* _item)
{
    return QString("%1/%2/last-cursor").arg(kSettingsKey, _item->uuid().toString());
}

} // namespace

class ScreenplayTextManager::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Создать представление
     */
    Ui::ScreenplayTextView* createView();

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
     * @brief Обновить переводы
     */
    void updateTranslations();

    /**
     * @brief Обновить список элементов словаря в редакторе словарей
     */
    void updateDictionaryItemsList(int _dictionaryType);


    /**
     * @brief Текущая модель представления основного окна
     */
    QPointer<BusinessLayer::ScreenplayTextModel> model;

    /**
     * @brief Модель типов справочников
     */
    QStringListModel* dictionariesTypesModel = nullptr;

    /**
     * @brief Элементы выбранного пользователем справочника
     */
    QStringListModel* dictionaryItemsModel = nullptr;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::ScreenplayTextView* view = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::ScreenplayTextView*> allViews;
};

ScreenplayTextManager::Implementation::Implementation()
{
    view = createView();
    dictionariesTypesModel = new QStringListModel(view);
    view->dictionariesView()->setTypes(dictionariesTypesModel);
    dictionaryItemsModel = new QStringListModel(view);
    view->dictionariesView()->setDictionaryItems(dictionaryItemsModel);

    loadViewSettings();
}

Ui::ScreenplayTextView* ScreenplayTextManager::Implementation::createView()
{
    auto newView = new Ui::ScreenplayTextView;
    allViews.append(newView);

    return newView;
}

void ScreenplayTextManager::Implementation::loadViewSettings()
{
    view->loadViewSettings();
}

void ScreenplayTextManager::Implementation::saveViewSettings()
{
    view->saveViewSettings();
}

void ScreenplayTextManager::Implementation::loadModelSettings()
{
    const auto cursorPosition = settingsValue(cursorPositionFor(model->document()), 0).toInt();
    view->setCursorPosition(cursorPosition);
}

void ScreenplayTextManager::Implementation::saveModelSettings()
{
    setSettingsValue(cursorPositionFor(model->document()), view->cursorPosition());
}

void ScreenplayTextManager::Implementation::updateTranslations()
{
    dictionariesTypesModel->setStringList({
        tr("Scene intro"),
        tr("Scene time"),
        tr("Character extension"),
        tr("Transition"),
    });
}

void ScreenplayTextManager::Implementation::updateDictionaryItemsList(int _dictionaryType)
{
    if (model == nullptr) {
        return;
    }

    switch (_dictionaryType) {
    case kSceneIntrosIndex: {
        dictionaryItemsModel->setStringList(model->dictionariesModel()->sceneIntros().toList());
        break;
    }

    case kSceneTimesIndex: {
        dictionaryItemsModel->setStringList(model->dictionariesModel()->sceneTimes().toList());
        break;
    }

    case kCharacterExtensionsIndex: {
        dictionaryItemsModel->setStringList(
            model->dictionariesModel()->characterExtensions().toList());
        break;
    }

    case kTransitionIndex: {
        dictionaryItemsModel->setStringList(model->dictionariesModel()->transitions().toList());
        break;
    }

    default: {
        dictionaryItemsModel->setStringList({});
        break;
    }
    }
}


// ****


ScreenplayTextManager::ScreenplayTextManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
    d->view->installEventFilter(this);

    connect(d->view, &Ui::ScreenplayTextView::currentModelIndexChanged, this,
            &ScreenplayTextManager::currentModelIndexChanged);
    //
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
    connect(d->view, &Ui::ScreenplayTextView::addBookmarkRequested, this, [showBookmarkDialog] {
        showBookmarkDialog(Ui::BookmarkDialog::DialogType::CreateNew);
    });
    connect(d->view, &Ui::ScreenplayTextView::editBookmarkRequested, this,
            [showBookmarkDialog] { showBookmarkDialog(Ui::BookmarkDialog::DialogType::Edit); });
    connect(d->view, &Ui::ScreenplayTextView::createBookmarkRequested, this,
            [this](const QString& _text, const QColor& _color) {
                auto item = d->model->itemForIndex(d->view->currentModelIndex());
                if (item->type() != BusinessLayer::TextModelItemType::Text) {
                    return;
                }

                auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
                textItem->setBookmark({ _color, _text });
                d->model->updateItem(textItem);
            });
    connect(d->view, &Ui::ScreenplayTextView::changeBookmarkRequested, this,
            [this](const QModelIndex& _index, const QString& _text, const QColor& _color) {
                auto item = d->model->itemForIndex(_index);
                if (item->type() != BusinessLayer::TextModelItemType::Text) {
                    return;
                }

                auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
                textItem->setBookmark({ _color, _text });
                d->model->updateItem(textItem);
            });
    connect(d->view, &Ui::ScreenplayTextView::removeBookmarkRequested, this, [this] {
        auto item = d->model->itemForIndex(d->view->currentModelIndex());
        if (item->type() != BusinessLayer::TextModelItemType::Text) {
            return;
        }

        auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
        textItem->clearBookmark();
        d->model->updateItem(textItem);
    });
    //
    connect(d->view->dictionariesView(), &Ui::DictionariesView::typeChanged, this,
            [this](const QModelIndex& _index) { d->updateDictionaryItemsList(_index.row()); });
    connect(d->view->dictionariesView(), &Ui::DictionariesView::addItemRequested, this,
            [this](const QModelIndex& _typeIndex) {
                if (d->model == nullptr) {
                    return;
                }

                switch (_typeIndex.row()) {
                case kSceneIntrosIndex: {
                    d->model->dictionariesModel()->addSceneIntro("");
                    break;
                }

                case kSceneTimesIndex: {
                    d->model->dictionariesModel()->addSceneTime("");
                    break;
                }

                case kCharacterExtensionsIndex: {
                    d->model->dictionariesModel()->addCharacterExtension("");
                    break;
                }

                case kTransitionIndex: {
                    d->model->dictionariesModel()->addTransition("");
                    break;
                }
                }

                d->view->dictionariesView()->editLastItem();
            });
    connect(
        d->view->dictionariesView(), &Ui::DictionariesView::editItemRequested, this,
        [this](const QModelIndex& _typeIndex, const QModelIndex& _itemIndex, const QString& _item) {
            if (d->model == nullptr) {
                return;
            }

            switch (_typeIndex.row()) {
            case kSceneIntrosIndex: {
                d->model->dictionariesModel()->setSceneIntro(_itemIndex.row(), _item);
                break;
            }

            case kSceneTimesIndex: {
                d->model->dictionariesModel()->setSceneTime(_itemIndex.row(), _item);
                break;
            }

            case kCharacterExtensionsIndex: {
                d->model->dictionariesModel()->setCharacterExtension(_itemIndex.row(), _item);
                break;
            }

            case kTransitionIndex: {
                d->model->dictionariesModel()->setTransition(_itemIndex.row(), _item);
                break;
            }
            }
        });
    connect(d->view->dictionariesView(), &Ui::DictionariesView::removeItemRequested, this,
            [this](const QModelIndex& _typeIndex, const QModelIndex& _itemIndex) {
                if (d->model == nullptr) {
                    return;
                }

                switch (_typeIndex.row()) {
                case kSceneIntrosIndex: {
                    d->model->dictionariesModel()->removeSceneIntro(_itemIndex.row());
                    break;
                }

                case kSceneTimesIndex: {
                    d->model->dictionariesModel()->removeSceneTime(_itemIndex.row());
                    break;
                }

                case kCharacterExtensionsIndex: {
                    d->model->dictionariesModel()->removeCharacterExtension(_itemIndex.row());
                    break;
                }

                case kTransitionIndex: {
                    d->model->dictionariesModel()->removeTransition(_itemIndex.row());
                    break;
                }
                }
            });


    d->updateTranslations();
}

ScreenplayTextManager::~ScreenplayTextManager() = default;

QObject* ScreenplayTextManager::asQObject()
{
    return this;
}

void ScreenplayTextManager::setModel(BusinessLayer::AbstractModel* _model)
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
        d->view->disconnect(d->model->dictionariesModel());
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
        d->updateDictionaryItemsList(kSceneIntrosIndex);
        //
        // ... настраиваем соединения
        //
        connect(d->model->dictionariesModel(),
                &BusinessLayer::ScreenplayDictionariesModel::sceneIntrosChanged, d->view, [this] {
                    if (d->view->dictionariesView()->currentTypeIndex().row()
                        == kSceneIntrosIndex) {
                        d->updateDictionaryItemsList(kSceneIntrosIndex);
                    }
                });
        connect(d->model->dictionariesModel(),
                &BusinessLayer::ScreenplayDictionariesModel::sceneTimesChanged, d->view, [this] {
                    if (d->view->dictionariesView()->currentTypeIndex().row() == kSceneTimesIndex) {
                        d->updateDictionaryItemsList(kSceneTimesIndex);
                    }
                });
        connect(d->model->dictionariesModel(),
                &BusinessLayer::ScreenplayDictionariesModel::charactersExtensionsChanged, d->view,
                [this] {
                    if (d->view->dictionariesView()->currentTypeIndex().row()
                        == kCharacterExtensionsIndex) {
                        d->updateDictionaryItemsList(kCharacterExtensionsIndex);
                    }
                });
        connect(d->model->dictionariesModel(),
                &BusinessLayer::ScreenplayDictionariesModel::transitionsChanged, d->view, [this] {
                    if (d->view->dictionariesView()->currentTypeIndex().row() == kTransitionIndex) {
                        d->updateDictionaryItemsList(kTransitionIndex);
                    }
                });
    }
}

Ui::IDocumentView* ScreenplayTextManager::view()
{
    return d->view;
}

Ui::IDocumentView* ScreenplayTextManager::createView()
{
    return d->createView();
}

void ScreenplayTextManager::reconfigure(const QStringList& _changedSettingsKeys)
{
    d->view->reconfigure(_changedSettingsKeys);
}

void ScreenplayTextManager::bind(IDocumentManager* _manager)
{
    Q_ASSERT(_manager);

    const auto isConnectedFirstTime
        = connect(_manager->asQObject(), SIGNAL(currentModelIndexChanged(QModelIndex)), this,
                  SLOT(setCurrentModelIndex(QModelIndex)), Qt::UniqueConnection);
    connect(_manager->asQObject(), SIGNAL(pasteBeatNameToEditorRequested(QString)), this,
            SLOT(pasteBeatNameToEditor(QString)), Qt::UniqueConnection);

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

void ScreenplayTextManager::saveSettings()
{
    d->saveViewSettings();

    if (d->model != nullptr) {
        d->saveModelSettings();
    }
}

bool ScreenplayTextManager::eventFilter(QObject* _watched, QEvent* _event)
{
    if (_event->type() == QEvent::LanguageChange && _watched == d->view) {
        d->updateTranslations();
    }

    return QObject::eventFilter(_watched, _event);
}

void ScreenplayTextManager::setCurrentModelIndex(const QModelIndex& _index)
{
    QSignalBlocker blocker(this);

    if (!_index.isValid()) {
        return;
    }

    d->view->setCurrentModelIndex(_index);
    d->view->setFocus();
}

void ScreenplayTextManager::pasteBeatNameToEditor(const QString& _name)
{
    d->view->insertText(_name);
}

} // namespace ManagementLayer
