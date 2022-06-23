#include "simple_text_manager.h"

#include "simple_text_view.h"

#include <business_layer/model/simple_text/simple_text_model.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>
#include <ui/modules/bookmarks/bookmark_dialog.h>

#include <QApplication>
#include <QFileDialog>


namespace ManagementLayer {

namespace {
const QString kSettingsKey = "simple-text";
QString cursorPositionFor(Domain::DocumentObject* _item)
{
    return QString("%1/%2/last-cursor").arg(kSettingsKey, _item->uuid().toString());
}
QString verticalScrollFor(Domain::DocumentObject* _item)
{
    return QString("%1/%2/vertical-scroll").arg(kSettingsKey, _item->uuid().toString());
}
} // namespace

class SimpleTextManager::Implementation
{
public:
    explicit Implementation(SimpleTextManager* _q);

    /**
     * @brief Создать представление
     */
    Ui::SimpleTextView* createView(BusinessLayer::AbstractModel* _model);

    /**
     * @brief Связать заданную модель и представление
     */
    void setModelForView(BusinessLayer::AbstractModel* _model, Ui::SimpleTextView* _view);

    /**
     * @brief Получить модель связанную с заданным представлением
     */
    QPointer<BusinessLayer::SimpleTextModel> modelForView(Ui::SimpleTextView* _view) const;

    /**
     * @brief Работа с параметрами отображения представления
     */
    void loadModelAndViewSettings(BusinessLayer::AbstractModel* _model, Ui::SimpleTextView* _view);
    void saveModelAndViewSettings(BusinessLayer::AbstractModel* _model, Ui::SimpleTextView* _view);


    SimpleTextManager* q = nullptr;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::SimpleTextView* view = nullptr;
    Ui::SimpleTextView* secondaryView = nullptr;

    /**
     * @brief Все созданные представления с моделями, которые в них отображаются
     */
    struct ViewAndModel {
        Ui::SimpleTextView* view = nullptr;
        QPointer<BusinessLayer::SimpleTextModel> model;
    };
    QVector<ViewAndModel> allViews;
};

SimpleTextManager::Implementation::Implementation(SimpleTextManager* _q)
    : q(_q)
{
}

Ui::SimpleTextView* SimpleTextManager::Implementation::createView(
    BusinessLayer::AbstractModel* _model)
{
    auto view = new Ui::SimpleTextView;
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
    connect(view, &Ui::SimpleTextView::addBookmarkRequested, q, [showBookmarkDialog] {
        showBookmarkDialog(Ui::BookmarkDialog::DialogType::CreateNew);
    });
    connect(view, &Ui::SimpleTextView::editBookmarkRequested, q,
            [showBookmarkDialog] { showBookmarkDialog(Ui::BookmarkDialog::DialogType::Edit); });
    connect(view, &Ui::SimpleTextView::createBookmarkRequested, q,
            [this, view](const QString& _text, const QColor& _color) {
                auto item = modelForView(view)->itemForIndex(view->currentModelIndex());
                if (item->type() != BusinessLayer::TextModelItemType::Text) {
                    return;
                }

                auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
                textItem->setBookmark({ _color, _text });
                modelForView(view)->updateItem(textItem);
            });
    connect(view, &Ui::SimpleTextView::changeBookmarkRequested, q,
            [this, view](const QModelIndex& _index, const QString& _text, const QColor& _color) {
                auto item = modelForView(view)->itemForIndex(_index);
                if (item->type() != BusinessLayer::TextModelItemType::Text) {
                    return;
                }

                auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
                textItem->setBookmark({ _color, _text });
                modelForView(view)->updateItem(textItem);
            });
    connect(view, &Ui::SimpleTextView::removeBookmarkRequested, q, [this, view] {
        auto item = modelForView(view)->itemForIndex(view->currentModelIndex());
        if (item->type() != BusinessLayer::TextModelItemType::Text) {
            return;
        }

        auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
        textItem->clearBookmark();
        modelForView(view)->updateItem(textItem);
    });

    return view;
}

void SimpleTextManager::Implementation::setModelForView(BusinessLayer::AbstractModel* _model,
                                                        Ui::SimpleTextView* _view)
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
    }

    //
    // Определяем новую модель
    //
    auto model = qobject_cast<BusinessLayer::SimpleTextModel*>(_model);
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

QPointer<BusinessLayer::SimpleTextModel> SimpleTextManager::Implementation::modelForView(
    Ui::SimpleTextView* _view) const
{
    for (auto& viewAndModel : allViews) {
        if (viewAndModel.view == _view) {
            return viewAndModel.model;
        }
    }
    return {};
}

void SimpleTextManager::Implementation::loadModelAndViewSettings(
    BusinessLayer::AbstractModel* _model, Ui::SimpleTextView* _view)
{
    const auto cursorPosition = settingsValue(cursorPositionFor(_model->document()), 0).toInt();
    _view->setCursorPosition(cursorPosition);
    const auto verticalScroll = settingsValue(verticalScrollFor(_model->document()), 0).toInt();
    _view->setverticalScroll(verticalScroll);

    _view->loadViewSettings();
}

void SimpleTextManager::Implementation::saveModelAndViewSettings(
    BusinessLayer::AbstractModel* _model, Ui::SimpleTextView* _view)
{
    setSettingsValue(cursorPositionFor(_model->document()), _view->cursorPosition());
    setSettingsValue(verticalScrollFor(_model->document()), _view->verticalScroll());

    _view->saveViewSettings();
}


// ****


SimpleTextManager::SimpleTextManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation(this))
{
}

SimpleTextManager::~SimpleTextManager() = default;

QObject* SimpleTextManager::asQObject()
{
    return this;
}

Ui::IDocumentView* SimpleTextManager::view()
{
    return d->view;
}

Ui::IDocumentView* SimpleTextManager::view(BusinessLayer::AbstractModel* _model)
{
    if (d->view == nullptr) {
        d->view = d->createView(_model);

        //
        // Наружу даём сигналы только от первичного представления, только оно может
        // взаимодействовать с навигатором документа
        //
        connect(d->view, &Ui::SimpleTextView::currentModelIndexChanged, this,
                &SimpleTextManager::currentModelIndexChanged);
    } else {
        d->setModelForView(_model, d->view);
    }

    return d->view;
}

Ui::IDocumentView* SimpleTextManager::secondaryView()
{
    return d->secondaryView;
}

Ui::IDocumentView* SimpleTextManager::secondaryView(BusinessLayer::AbstractModel* _model)
{
    if (d->secondaryView == nullptr) {
        d->secondaryView = d->createView(_model);
    } else {
        d->setModelForView(_model, d->secondaryView);
    }

    return d->secondaryView;
}

Ui::IDocumentView* SimpleTextManager::createView(BusinessLayer::AbstractModel* _model)
{
    return d->createView(_model);
}

void SimpleTextManager::resetModels()
{
    for (auto& viewAndModel : d->allViews) {
        d->setModelForView(nullptr, viewAndModel.view);
    }
}

void SimpleTextManager::reconfigure(const QStringList& _changedSettingsKeys)
{
    d->view->reconfigure(_changedSettingsKeys);
}

void SimpleTextManager::bind(IDocumentManager* _manager)
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
    // Ставим в очередь событие нотификацию о смене текущей главы,
    // чтобы навигатор отобразил её при первом открытии
    //
    if (isConnectedFirstTime) {
        QMetaObject::invokeMethod(
            this, [this] { emit currentModelIndexChanged(d->view->currentModelIndex()); },
            Qt::QueuedConnection);
    }
}

void SimpleTextManager::saveSettings()
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.model != nullptr && viewAndModel.view != nullptr) {
            d->saveModelAndViewSettings(viewAndModel.model, viewAndModel.view);
        }
    }
}

void SimpleTextManager::setCurrentModelIndex(const QModelIndex& _index)
{
    QSignalBlocker blocker(this);

    if (!_index.isValid()) {
        return;
    }

    d->view->setCurrentModelIndex(_index);
    d->view->setFocus();
}

} // namespace ManagementLayer
