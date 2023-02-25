#include "novel_text_manager.h"

#include "novel_text_view.h"

#include <business_layer/model/novel/novel_dictionaries_model.h>
#include <business_layer/model/novel/text/novel_text_model.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>
#include <ui/modules/bookmarks/bookmark_dialog.h>
#include <utils/logging.h>

#include <QApplication>
#include <QFileDialog>
#include <QStringListModel>


namespace ManagementLayer {

namespace {

const QString kSettingsKey = "novel-text";
QString cursorPositionFor(Domain::DocumentObject* _item)
{
    return QString("%1/%2/last-cursor").arg(kSettingsKey, _item->uuid().toString());
}
QString verticalScrollFor(Domain::DocumentObject* _item)
{
    return QString("%1/%2/vertical-scroll").arg(kSettingsKey, _item->uuid().toString());
}

} // namespace

class NovelTextManager::Implementation
{
public:
    explicit Implementation(NovelTextManager* _q);

    /**
     * @brief Создать представление
     */
    Ui::NovelTextView* createView(BusinessLayer::AbstractModel* _model);

    /**
     * @brief Связать заданную модель и представление
     */
    void setModelForView(BusinessLayer::AbstractModel* _model, Ui::NovelTextView* _view);


    /**
     * @brief Получить модель связанную с заданным представлением
     */
    QPointer<BusinessLayer::NovelTextModel> modelForView(Ui::NovelTextView* _view) const;


    /**
     * @brief Работа с параметрами отображения представления
     */
    void loadModelAndViewSettings(BusinessLayer::AbstractModel* _model, Ui::NovelTextView* _view);
    void saveModelAndViewSettings(BusinessLayer::AbstractModel* _model, Ui::NovelTextView* _view);

    /**
     * @brief Обновить переводы
     */
    void updateTranslations();


    NovelTextManager* q = nullptr;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::NovelTextView* view = nullptr;
    Ui::NovelTextView* secondaryView = nullptr;

    /**
     * @brief Все созданные представления с моделями, которые в них отображаются
     */
    struct ViewAndModel {
        QPointer<Ui::NovelTextView> view;
        QPointer<BusinessLayer::NovelTextModel> model;
    };
    QVector<ViewAndModel> allViews;
};

NovelTextManager::Implementation::Implementation(NovelTextManager* _q)
    : q(_q)
{
}

Ui::NovelTextView* NovelTextManager::Implementation::createView(
    BusinessLayer::AbstractModel* _model)
{
    Log::info("Create novel text view for model");
    auto view = new Ui::NovelTextView;
    view->installEventFilter(q);
    setModelForView(_model, view);

    connect(view, &Ui::NovelTextView::currentModelIndexChanged, q,
            &NovelTextManager::currentModelIndexChanged);
    //
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
    connect(view, &Ui::NovelTextView::addBookmarkRequested, q, [showBookmarkDialog] {
        showBookmarkDialog(Ui::BookmarkDialog::DialogType::CreateNew);
    });
    connect(view, &Ui::NovelTextView::editBookmarkRequested, q,
            [showBookmarkDialog] { showBookmarkDialog(Ui::BookmarkDialog::DialogType::Edit); });
    connect(view, &Ui::NovelTextView::createBookmarkRequested, q,
            [this, view](const QString& _text, const QColor& _color) {
                auto item = modelForView(view)->itemForIndex(view->currentModelIndex());
                if (item->type() != BusinessLayer::TextModelItemType::Text) {
                    return;
                }

                auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
                textItem->setBookmark({ _color, _text });
                modelForView(view)->updateItem(textItem);
            });
    connect(view, &Ui::NovelTextView::changeBookmarkRequested, q,
            [this, view](const QModelIndex& _index, const QString& _text, const QColor& _color) {
                auto item = modelForView(view)->itemForIndex(_index);
                if (item->type() != BusinessLayer::TextModelItemType::Text) {
                    return;
                }

                auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
                textItem->setBookmark({ _color, _text });
                modelForView(view)->updateItem(textItem);
            });
    connect(view, &Ui::NovelTextView::removeBookmarkRequested, q, [this, view] {
        auto item = modelForView(view)->itemForIndex(view->currentModelIndex());
        if (item->type() != BusinessLayer::TextModelItemType::Text) {
            return;
        }

        auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
        textItem->clearBookmark();
        modelForView(view)->updateItem(textItem);
    });
    //
    connect(view, &Ui::NovelTextView::generateTextRequested, q, [this](const QString& _text) {
        emit q->generateTextRequested(
            tr("Generate novel text"),
            tr("Start prompt from something like \"Write a novel about ...\", or \"Write a "
               "short novel about ...\""),
            _text, {});
    });


    updateTranslations();

    Log::info("Novel text view created");

    return view;
}

void NovelTextManager::Implementation::setModelForView(BusinessLayer::AbstractModel* _model,
                                                       Ui::NovelTextView* _view)
{
    Log::info("Set model for view");

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
        _view->disconnect(allViews[viewIndex].model->dictionariesModel());
        allViews[viewIndex].model->disconnect(_view);
        allViews[viewIndex].model->dictionariesModel()->disconnect(_view);
    }

    //
    // Определяем новую модель
    //
    auto model = qobject_cast<BusinessLayer::NovelTextModel*>(_model);
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
    }

    Log::info("Model for view set");
}

QPointer<BusinessLayer::NovelTextModel> NovelTextManager::Implementation::modelForView(
    Ui::NovelTextView* _view) const
{
    for (auto& viewAndModel : allViews) {
        if (viewAndModel.view == _view) {
            return viewAndModel.model;
        }
    }
    return {};
}

void NovelTextManager::Implementation::loadModelAndViewSettings(
    BusinessLayer::AbstractModel* _model, Ui::NovelTextView* _view)
{
    const auto cursorPosition = settingsValue(cursorPositionFor(_model->document()), 0).toInt();
    _view->setCursorPosition(cursorPosition);
    const auto verticalScroll = settingsValue(verticalScrollFor(_model->document()), 0).toInt();
    _view->setVerticalScroll(verticalScroll);

    _view->loadViewSettings();
}

void NovelTextManager::Implementation::saveModelAndViewSettings(
    BusinessLayer::AbstractModel* _model, Ui::NovelTextView* _view)
{
    setSettingsValue(cursorPositionFor(_model->document()), _view->cursorPosition());
    setSettingsValue(verticalScrollFor(_model->document()), _view->verticalScroll());

    _view->saveViewSettings();
}

void NovelTextManager::Implementation::updateTranslations()
{
}


// ****


NovelTextManager::NovelTextManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation(this))
{
    Log::info("Init novel text manager");
}

NovelTextManager::~NovelTextManager() = default;

QObject* NovelTextManager::asQObject()
{
    return this;
}

Ui::IDocumentView* NovelTextManager::view()
{
    return d->view;
}

Ui::IDocumentView* NovelTextManager::view(BusinessLayer::AbstractModel* _model)
{
    if (d->view == nullptr) {
        d->view = d->createView(_model);

        connect(d->view, &Ui::NovelTextView::currentModelIndexChanged, this,
                &NovelTextManager::viewCurrentModelIndexChanged);
    } else {
        d->setModelForView(_model, d->view);
    }

    return d->view;
}

Ui::IDocumentView* NovelTextManager::secondaryView()
{
    return d->secondaryView;
}

Ui::IDocumentView* NovelTextManager::secondaryView(BusinessLayer::AbstractModel* _model)
{
    if (d->secondaryView == nullptr) {
        d->secondaryView = d->createView(_model);
    } else {
        d->setModelForView(_model, d->secondaryView);
    }

    return d->secondaryView;
}

Ui::IDocumentView* NovelTextManager::createView(BusinessLayer::AbstractModel* _model)
{
    return d->createView(_model);
}

void NovelTextManager::resetModels()
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        d->setModelForView(nullptr, viewAndModel.view);
    }
}

void NovelTextManager::reconfigure(const QStringList& _changedSettingsKeys)
{
    for (const auto& view : std::as_const(d->allViews)) {
        if (!view.view.isNull()) {
            view.view->reconfigure(_changedSettingsKeys);
        }
    }
}

void NovelTextManager::bind(IDocumentManager* _manager)
{
    Q_ASSERT(_manager);
    if (_manager == nullptr || _manager == this) {
        return;
    }

    //
    // Т.к. навигатор соединяется только с главным инстансом редактора, проверяем создан ли он
    //
    if (_manager->isNavigationManager()) {
        const auto isConnectedFirstTime
            = connect(_manager->asQObject(), SIGNAL(currentModelIndexChanged(QModelIndex)), this,
                      SLOT(setViewCurrentModelIndex(QModelIndex)), Qt::UniqueConnection);

        //
        // Ставим в очередь событие нотификацию о смене текущей сцены,
        // чтобы навигатор отобразил её при первом открытии
        //
        if (isConnectedFirstTime && d->view != nullptr) {
            QMetaObject::invokeMethod(
                this, [this] { emit viewCurrentModelIndexChanged(d->view->currentModelIndex()); },
                Qt::QueuedConnection);
        }
    }
    //
    // Между собой можно соединить любые менеджеры редакторов
    //
    else {
        connect(_manager->asQObject(), SIGNAL(currentModelIndexChanged(QModelIndex)), this,
                SLOT(setCurrentModelIndex(QModelIndex)), Qt::UniqueConnection);
    }
}

void NovelTextManager::saveSettings()
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.model.isNull() || viewAndModel.view.isNull()) {
            continue;
        }

        d->saveModelAndViewSettings(viewAndModel.model, viewAndModel.view);
    }
}

void NovelTextManager::setEditingMode(DocumentEditingMode _mode)
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        viewAndModel.view->setEditingMode(_mode);
    }
}

bool NovelTextManager::eventFilter(QObject* _watched, QEvent* _event)
{
    if (_event->type() == QEvent::LanguageChange && _watched == d->view) {
        d->updateTranslations();
    }

    return QObject::eventFilter(_watched, _event);
}

void NovelTextManager::setViewCurrentModelIndex(const QModelIndex& _index)
{
    QSignalBlocker blocker(this);

    if (!_index.isValid() || d->view == nullptr) {
        return;
    }

    d->view->setCurrentModelIndex(_index);
    d->view->setFocus();
}

void NovelTextManager::setCurrentModelIndex(const QModelIndex& _index)
{
    QSignalBlocker blocker(this);

    if (!_index.isValid()) {
        return;
    }

    for (const auto& viewAndModel : std::as_const(d->allViews)) {
        if (!viewAndModel.view.isNull()) {
            viewAndModel.view->setCurrentModelIndex(_index);
        }
    }
}

} // namespace ManagementLayer
