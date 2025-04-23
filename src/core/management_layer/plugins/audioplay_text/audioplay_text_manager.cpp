#include "audioplay_text_manager.h"

#include "audioplay_text_view.h"

#include <business_layer/model/audioplay/text/audioplay_text_model.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>
#include <ui/modules/bookmarks/bookmark_dialog.h>

#include <QApplication>
#include <QFileDialog>


namespace ManagementLayer {

namespace {
const QLatin1String kSettingsKey("audioplay-text");
QString cursorPositionFor(Domain::DocumentObject* _item)
{
    return QString("%1/%2/last-cursor").arg(kSettingsKey, _item->uuid().toString());
}
QString verticalScrollFor(Domain::DocumentObject* _item)
{
    return QString("%1/%2/vertical-scroll").arg(kSettingsKey, _item->uuid().toString());
}
} // namespace

class AudioplayTextManager::Implementation
{
public:
    explicit Implementation(AudioplayTextManager* _q);

    /**
     * @brief Создать представление
     */
    Ui::AudioplayTextView* createView(BusinessLayer::AbstractModel* _model);

    /**
     * @brief Связать заданную модель и представление
     */
    void setModelForView(BusinessLayer::AbstractModel* _model, Ui::AudioplayTextView* _view);

    /**
     * @brief Получить модель связанную с заданным представлением
     */
    QPointer<BusinessLayer::AudioplayTextModel> modelForView(Ui::AudioplayTextView* _view);

    /**
     * @brief Работа с параметрами отображения представления
     */
    void loadModelAndViewSettings(BusinessLayer::AbstractModel* _model,
                                  Ui::AudioplayTextView* _view);
    void saveModelAndViewSettings(BusinessLayer::AbstractModel* _model,
                                  Ui::AudioplayTextView* _view);


    AudioplayTextManager* q = nullptr;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::AudioplayTextView* view = nullptr;
    Ui::AudioplayTextView* secondaryView = nullptr;

    /**
     * @brief Все созданные представления с моделями, которые в них отображаются
     */
    struct ViewAndModel {
        QPointer<Ui::AudioplayTextView> view;
        QPointer<BusinessLayer::AudioplayTextModel> model;
    };
    QVector<ViewAndModel> allViews;
};

AudioplayTextManager::Implementation::Implementation(AudioplayTextManager* _q)
    : q(_q)
{
}

Ui::AudioplayTextView* AudioplayTextManager::Implementation::createView(
    BusinessLayer::AbstractModel* _model)
{
    auto view = new Ui::AudioplayTextView;
    setModelForView(_model, view);

    auto showBookmarkDialog = [this, view](Ui::BookmarkDialog::DialogType _type) {
        auto item = modelForView(view)->itemForIndex(view->currentModelIndex());
        if (item->type() != BusinessLayer::TextModelItemType::Text) {
            return;
        }

        auto dialog = new Ui::BookmarkDialog(view->topLevelWidget());
        dialog->setDialogType(_type);
        if (_type == Ui::BookmarkDialog::DialogType::Edit) {
            const auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
            dialog->setBookmarkName(textItem->bookmark()->name);
            dialog->setBookmarkColor(textItem->bookmark()->color);
        }
        connect(dialog, &Ui::BookmarkDialog::savePressed, q, [this, view, item, dialog] {
            auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
            textItem->setBookmark({ dialog->bookmarkColor(), dialog->bookmarkName() });
            modelForView(view)->updateItem(textItem);

            dialog->hideDialog();
        });
        connect(dialog, &Ui::BookmarkDialog::disappeared, dialog, &Ui::BookmarkDialog::deleteLater);

        //
        // Отображаем диалог
        //
        dialog->showDialog();
    };
    connect(view, &Ui::AudioplayTextView::addBookmarkRequested, q, [showBookmarkDialog] {
        showBookmarkDialog(Ui::BookmarkDialog::DialogType::CreateNew);
    });
    connect(view, &Ui::AudioplayTextView::editBookmarkRequested, q,
            [showBookmarkDialog] { showBookmarkDialog(Ui::BookmarkDialog::DialogType::Edit); });
    connect(view, &Ui::AudioplayTextView::createBookmarkRequested, q,
            [this, view](const QString& _text, const QColor& _color) {
                auto model = modelForView(view);
                auto item = model->itemForIndex(view->currentModelIndex());
                if (item->type() != BusinessLayer::TextModelItemType::Text) {
                    return;
                }

                auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
                textItem->setBookmark({ _color, _text });
                model->updateItem(textItem);
            });
    connect(view, &Ui::AudioplayTextView::changeBookmarkRequested, q,
            [this, view](const QModelIndex& _index, const QString& _text, const QColor& _color) {
                auto model = modelForView(view);
                auto item = model->itemForIndex(_index);
                if (item->type() != BusinessLayer::TextModelItemType::Text) {
                    return;
                }

                auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
                textItem->setBookmark({ _color, _text });
                model->updateItem(textItem);
            });
    connect(view, &Ui::AudioplayTextView::removeBookmarkRequested, q, [this, view] {
        auto model = modelForView(view);
        auto item = model->itemForIndex(view->currentModelIndex());
        if (item->type() != BusinessLayer::TextModelItemType::Text) {
            return;
        }

        auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
        textItem->clearBookmark();
        model->updateItem(textItem);
    });
    //
    connect(view, &Ui::AudioplayTextView::rephraseTextRequested, q,
            &AudioplayTextManager::rephraseTextRequested);
    connect(view, &Ui::AudioplayTextView::expandTextRequested, q,
            &AudioplayTextManager::expandTextRequested);
    connect(view, &Ui::AudioplayTextView::shortenTextRequested, q,
            &AudioplayTextManager::shortenTextRequested);
    connect(view, &Ui::AudioplayTextView::insertTextRequested, q,
            &AudioplayTextManager::insertTextRequested);
    connect(view, &Ui::AudioplayTextView::summarizeTextRequested, q,
            &AudioplayTextManager::summarizeTextRequested);
    connect(view, &Ui::AudioplayTextView::translateTextRequested, q,
            &AudioplayTextManager::translateTextRequested);
    connect(view, &Ui::AudioplayTextView::translateDocumentRequested, q,
            [this, view](const QString& _languageCode) {
                const auto model = modelForView(view);
                QVector<QString> groups;
                QString group;
                std::function<void(const QModelIndex&)> findGroups;
                findGroups
                    = [&findGroups, model, &groups, &group](const QModelIndex& _parentItemIndex) {
                          for (int row = 0; row < model->rowCount(_parentItemIndex); ++row) {
                              const auto itemIndex = model->index(row, 0, _parentItemIndex);
                              const auto item = model->itemForIndex(itemIndex);
                              switch (item->type()) {
                              case BusinessLayer::TextModelItemType::Folder: {
                                  findGroups(itemIndex);
                                  break;
                              }

                              case BusinessLayer::TextModelItemType::Group: {
                                  if (!group.isEmpty()) {
                                      groups.append(group);
                                      group.clear();
                                  }

                                  findGroups(itemIndex);
                                  break;
                              }

                              case BusinessLayer::TextModelItemType::Text: {
                                  const auto textItem
                                      = static_cast<const BusinessLayer::TextModelTextItem*>(item);
                                  if (!textItem->text().isEmpty()) {
                                      if (!group.isEmpty()) {
                                          group.append("\n");
                                      }
                                      group.append(textItem->text());
                                  }
                                  break;
                              }

                              default: {
                                  break;
                              }
                              }
                          }
                      };
                findGroups({});
                if (!group.isEmpty()) {
                    groups.append(group);
                }
                emit q->translateDocumentRequested(groups, _languageCode,
                                                   Domain::DocumentObjectType::AudioplayText);
            });
    connect(view, &Ui::AudioplayTextView::generateTextRequested, q, [this](const QString& _text) {
        emit q->generateTextRequested({}, _text, ". Write result in fountain format.");
    });
    connect(view, &Ui::AudioplayTextView::buyCreditsRequested, q,
            &AudioplayTextManager::buyCreditsRequested);

    return view;
}

void AudioplayTextManager::Implementation::setModelForView(BusinessLayer::AbstractModel* _model,
                                                           Ui::AudioplayTextView* _view)
{
    constexpr int invalidIndex = -1;
    int viewIndex = invalidIndex;
    for (int index = 0; index < allViews.size(); ++index) {
        if (allViews[index].view == _view) {
            if (allViews[index].model == _model) {
                return;
            }

            viewIndex = index;
            break;
        }
    }

    //
    // Если модель была задана
    //
    if (viewIndex != invalidIndex && allViews[viewIndex].model != nullptr) {
        //
        // ... сохраняем параметры
        //
        saveModelAndViewSettings(allViews[viewIndex].model, _view);
        //
        // ... разрываем соединения
        //
        _view->disconnect(allViews[viewIndex].model);
        allViews[viewIndex].model->disconnect(_view);
    }

    //
    // Определяем новую модель
    //
    auto model = qobject_cast<BusinessLayer::AudioplayTextModel*>(_model);
    _view->setModel(model);

    //
    // Обновляем связь представления с моделью
    //
    if (viewIndex != invalidIndex) {
        allViews[viewIndex].model = model;
    }
    //
    // Или сохраняем связь представления с моделью
    //
    else {
        allViews.append({ _view, model });
    }

    //
    // Если новая модель задана
    //
    if (model != nullptr) {
        //
        // ... загрузим параметры
        //
        loadModelAndViewSettings(model, _view);
        //
        // ... настраиваем соединения
        //
    }
}

QPointer<BusinessLayer::AudioplayTextModel> AudioplayTextManager::Implementation::modelForView(
    Ui::AudioplayTextView* _view)
{
    for (auto& viewAndModel : allViews) {
        if (viewAndModel.view == _view) {
            return viewAndModel.model;
        }
    }
    return {};
}

void AudioplayTextManager::Implementation::loadModelAndViewSettings(
    BusinessLayer::AbstractModel* _model, Ui::AudioplayTextView* _view)
{
    const auto cursorPosition = settingsValue(cursorPositionFor(_model->document()), 0).toInt();
    _view->setCursorPosition(cursorPosition);
    const auto verticalScroll = settingsValue(verticalScrollFor(_model->document()), 0).toInt();
    _view->setverticalScroll(verticalScroll);

    _view->loadViewSettings();
}

void AudioplayTextManager::Implementation::saveModelAndViewSettings(
    BusinessLayer::AbstractModel* _model, Ui::AudioplayTextView* _view)
{
    setSettingsValue(cursorPositionFor(_model->document()), _view->cursorPosition());
    setSettingsValue(verticalScrollFor(_model->document()), _view->verticalScroll());

    _view->saveViewSettings();
}


// ****


AudioplayTextManager::AudioplayTextManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation(this))
{
}

AudioplayTextManager::~AudioplayTextManager() = default;

QObject* AudioplayTextManager::asQObject()
{
    return this;
}

Ui::IDocumentView* AudioplayTextManager::view()
{
    return d->view;
}

Ui::IDocumentView* AudioplayTextManager::view(BusinessLayer::AbstractModel* _model)
{
    if (d->view == nullptr) {
        d->view = d->createView(_model);

        //
        // Наружу даём сигналы только от первичного представления, только оно может
        // взаимодействовать с навигатором документа
        //
        connect(d->view, &Ui::AudioplayTextView::currentModelIndexChanged, this,
                &AudioplayTextManager::currentModelIndexChanged);
    } else {
        d->setModelForView(_model, d->view);
    }

    return d->view;
}

Ui::IDocumentView* AudioplayTextManager::secondaryView()
{
    return d->secondaryView;
}

Ui::IDocumentView* AudioplayTextManager::secondaryView(BusinessLayer::AbstractModel* _model)
{
    if (d->secondaryView == nullptr) {
        d->secondaryView = d->createView(_model);
    } else {
        d->setModelForView(_model, d->secondaryView);
    }

    return d->secondaryView;
}

Ui::IDocumentView* AudioplayTextManager::createView(BusinessLayer::AbstractModel* _model)
{
    return d->createView(_model);
}

void AudioplayTextManager::resetModels()
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        d->setModelForView(nullptr, viewAndModel.view);
    }
}

void AudioplayTextManager::reconfigure(const QStringList& _changedSettingsKeys)
{
    for (const auto& view : std::as_const(d->allViews)) {
        if (!view.view.isNull()) {
            view.view->reconfigure(_changedSettingsKeys);
        }
    }
}

void AudioplayTextManager::bind(IDocumentManager* _manager)
{
    //
    // Т.к. навигатор соединяется только с главным инстансом редактора, проверяем создан ли он
    //
    if (d->view == nullptr) {
        return;
    }

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

void AudioplayTextManager::saveSettings()
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.model.isNull() || viewAndModel.view.isNull()) {
            continue;
        }

        d->saveModelAndViewSettings(viewAndModel.model, viewAndModel.view);
    }
}

void AudioplayTextManager::setEditingMode(DocumentEditingMode _mode)
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        viewAndModel.view->setEditingMode(_mode);
    }
}

void AudioplayTextManager::setAvailableCredits(int _credits)
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        viewAndModel.view->setAvailableCredits(_credits);
    }
}

void AudioplayTextManager::setCurrentModelIndex(const QModelIndex& _index)
{
    QSignalBlocker blocker(this);

    if (!_index.isValid()) {
        return;
    }

    d->view->setCurrentModelIndex(_index);
    d->view->setFocus();
}

} // namespace ManagementLayer
