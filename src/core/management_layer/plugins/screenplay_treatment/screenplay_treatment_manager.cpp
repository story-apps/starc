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
    explicit Implementation(ScreenplayTreatmentManager* _q);

    /**
     * @brief Создать представление
     */
    Ui::ScreenplayTreatmentView* createView(BusinessLayer::AbstractModel* _model);

    /**
     * @brief Связать заданную модель и представление
     */
    void setModelForView(BusinessLayer::AbstractModel* _model, Ui::ScreenplayTreatmentView* _view);

    /**
     * @brief Получить модель связанную с заданным представлением
     */
    QPointer<BusinessLayer::ScreenplayTextModel> modelForView(
        Ui::ScreenplayTreatmentView* _view) const;

    /**
     * @brief Работа с параметрами отображения представления
     */
    void loadModelAndViewSettings(BusinessLayer::AbstractModel* _model,
                                  Ui::ScreenplayTreatmentView* _view);
    void saveModelAndViewSettings(BusinessLayer::AbstractModel* _model,
                                  Ui::ScreenplayTreatmentView* _view);


    ScreenplayTreatmentManager* q = nullptr;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::ScreenplayTreatmentView* view = nullptr;
    Ui::ScreenplayTreatmentView* secondaryView = nullptr;

    /**
     * @brief Все созданные представления с моделями, которые в них отображаются
     */
    struct ViewAndModel {
        QPointer<Ui::ScreenplayTreatmentView> view;
        QPointer<BusinessLayer::ScreenplayTextModel> model;
    };
    QVector<ViewAndModel> allViews;
};

ScreenplayTreatmentManager::Implementation::Implementation(ScreenplayTreatmentManager* _q)
    : q(_q)
{
}

Ui::ScreenplayTreatmentView* ScreenplayTreatmentManager::Implementation::createView(
    BusinessLayer::AbstractModel* _model)
{
    auto view = new Ui::ScreenplayTreatmentView;
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
    connect(view, &Ui::ScreenplayTreatmentView::addBookmarkRequested, q, [showBookmarkDialog] {
        showBookmarkDialog(Ui::BookmarkDialog::DialogType::CreateNew);
    });
    connect(view, &Ui::ScreenplayTreatmentView::editBookmarkRequested, q,
            [showBookmarkDialog] { showBookmarkDialog(Ui::BookmarkDialog::DialogType::Edit); });
    connect(view, &Ui::ScreenplayTreatmentView::createBookmarkRequested, q,
            [this, view](const QString& _text, const QColor& _color) {
                auto item = modelForView(view)->itemForIndex(view->currentModelIndex());
                if (item->type() != BusinessLayer::TextModelItemType::Text) {
                    return;
                }

                auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
                textItem->setBookmark({ _color, _text });
                modelForView(view)->updateItem(textItem);
            });
    connect(view, &Ui::ScreenplayTreatmentView::changeBookmarkRequested, q,
            [this, view](const QModelIndex& _index, const QString& _text, const QColor& _color) {
                auto item = modelForView(view)->itemForIndex(_index);
                if (item->type() != BusinessLayer::TextModelItemType::Text) {
                    return;
                }

                auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
                textItem->setBookmark({ _color, _text });
                modelForView(view)->updateItem(textItem);
            });
    connect(view, &Ui::ScreenplayTreatmentView::removeBookmarkRequested, q, [this, view] {
        auto item = modelForView(view)->itemForIndex(view->currentModelIndex());
        if (item->type() != BusinessLayer::TextModelItemType::Text) {
            return;
        }

        auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
        textItem->clearBookmark();
        modelForView(view)->updateItem(textItem);
    });
    //
    connect(view, &Ui::ScreenplayTreatmentView::rephraseTextRequested, q,
            &ScreenplayTreatmentManager::rephraseTextRequested);
    connect(view, &Ui::ScreenplayTreatmentView::expandTextRequested, q,
            &ScreenplayTreatmentManager::expandTextRequested);
    connect(view, &Ui::ScreenplayTreatmentView::shortenTextRequested, q,
            &ScreenplayTreatmentManager::shortenTextRequested);
    connect(view, &Ui::ScreenplayTreatmentView::insertTextRequested, q,
            &ScreenplayTreatmentManager::insertTextRequested);
    connect(view, &Ui::ScreenplayTreatmentView::summarizeTextRequested, q,
            &ScreenplayTreatmentManager::summarizeTextRequested);
    connect(view, &Ui::ScreenplayTreatmentView::translateTextRequested, q,
            &ScreenplayTreatmentManager::translateTextRequested);
    connect(view, &Ui::ScreenplayTreatmentView::generateTextRequested, q,
            [this](const QString& _text) { emit q->generateTextRequested({}, _text, {}); });
    connect(view, &Ui::ScreenplayTreatmentView::buyCreditsRequested, q,
            &ScreenplayTreatmentManager::buyCreditsRequested);

    return view;
}

void ScreenplayTreatmentManager::Implementation::setModelForView(
    BusinessLayer::AbstractModel* _model, Ui::ScreenplayTreatmentView* _view)
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
    auto model = qobject_cast<BusinessLayer::ScreenplayTextModel*>(_model);
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

QPointer<BusinessLayer::ScreenplayTextModel> ScreenplayTreatmentManager::Implementation::
    modelForView(Ui::ScreenplayTreatmentView* _view) const
{
    for (auto& viewAndModel : allViews) {
        if (viewAndModel.view == _view) {
            return viewAndModel.model;
        }
    }
    return {};
}

void ScreenplayTreatmentManager::Implementation::loadModelAndViewSettings(
    BusinessLayer::AbstractModel* _model, Ui::ScreenplayTreatmentView* _view)
{
    const auto cursorPosition = settingsValue(cursorPositionFor(_model->document()), 0).toInt();
    _view->setCursorPosition(cursorPosition);
    const auto verticalScroll = settingsValue(verticalScrollFor(_model->document()), 0).toInt();
    _view->setverticalScroll(verticalScroll);

    _view->loadViewSettings();
}

void ScreenplayTreatmentManager::Implementation::saveModelAndViewSettings(
    BusinessLayer::AbstractModel* _model, Ui::ScreenplayTreatmentView* _view)
{
    setSettingsValue(cursorPositionFor(_model->document()), _view->cursorPosition());
    setSettingsValue(verticalScrollFor(_model->document()), _view->verticalScroll());

    _view->saveViewSettings();
}


// ****


ScreenplayTreatmentManager::ScreenplayTreatmentManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation(this))
{
}

ScreenplayTreatmentManager::~ScreenplayTreatmentManager() = default;

QObject* ScreenplayTreatmentManager::asQObject()
{
    return this;
}

Ui::IDocumentView* ScreenplayTreatmentManager::view()
{
    return d->view;
}

Ui::IDocumentView* ScreenplayTreatmentManager::view(BusinessLayer::AbstractModel* _model)
{
    if (d->view == nullptr) {
        d->view = d->createView(_model);

        //
        // Наружу даём сигналы только от первичного представления, только оно может
        // взаимодействовать с навигатором документа
        //
        connect(d->view, &Ui::ScreenplayTreatmentView::currentModelIndexChanged, this,
                &ScreenplayTreatmentManager::currentModelIndexChanged);
    } else {
        d->setModelForView(_model, d->view);
    }

    return d->view;
}

Ui::IDocumentView* ScreenplayTreatmentManager::secondaryView()
{
    return d->secondaryView;
}

Ui::IDocumentView* ScreenplayTreatmentManager::secondaryView(BusinessLayer::AbstractModel* _model)
{
    if (d->secondaryView == nullptr) {
        d->secondaryView = d->createView(_model);
    } else {
        d->setModelForView(_model, d->secondaryView);
    }

    return d->secondaryView;
}

Ui::IDocumentView* ScreenplayTreatmentManager::createView(BusinessLayer::AbstractModel* _model)
{
    return d->createView(_model);
}

void ScreenplayTreatmentManager::resetModels()
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        d->setModelForView(nullptr, viewAndModel.view);
    }
}

void ScreenplayTreatmentManager::reconfigure(const QStringList& _changedSettingsKeys)
{
    for (const auto& view : std::as_const(d->allViews)) {
        if (!view.view.isNull()) {
            view.view->reconfigure(_changedSettingsKeys);
        }
    }
}

void ScreenplayTreatmentManager::bind(IDocumentManager* _manager)
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

void ScreenplayTreatmentManager::saveSettings()
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.model.isNull() || viewAndModel.view.isNull()) {
            continue;
        }

        d->saveModelAndViewSettings(viewAndModel.model, viewAndModel.view);
    }
}

void ScreenplayTreatmentManager::setEditingMode(DocumentEditingMode _mode)
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        viewAndModel.view->setEditingMode(_mode);
    }
}

void ScreenplayTreatmentManager::setAvailableCredits(int _credits)
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        viewAndModel.view->setAvailableCredits(_credits);
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
