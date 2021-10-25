#include "abstract_model.h"

#include "abstract_image_wrapper.h"

#include <domain/document_object.h>
#include <utils/diff_match_patch/diff_match_patch_controller.h>
#include <utils/tools/debouncer.h>

#include <QScopedValueRollback>


namespace BusinessLayer {

class AbstractModel::Implementation
{
public:
    explicit Implementation(const QVector<QString>& _tags);


    /**
     * @brief Документ на основе которого строится модель
     */
    Domain::DocumentObject* document = nullptr;

    /**
     * @brief Загрузчик фотографий
     */
    AbstractImageWrapper* image = nullptr;

    /**
     * @brief Контроллер для формирования патчей изменений документа
     */
    DiffMatchPatchController dmpController;

    /**
     * @brief Выполняется ли отмена последнего действия, или повтор отмены
     */
    bool isUndoInProgress = false;
    bool isRedoInProgress = false;

    /**
     * @brief Список отменённых действий, которые можно повторить
     */
    struct Change {
        QByteArray undo;
        QByteArray redo;
    };
    QVector<Change> undoedChanges;

    /**
     * @brief Шаг в истории изменений, который будет отменён как последняя правка
     */
    int undoStep = 0;

    /**
     * @brief Дебаунсер на изменение контента документа
     */
    Debouncer updateDocumentContentDebouncer;
};

AbstractModel::Implementation::Implementation(const QVector<QString>& _tags)
    : dmpController(_tags)
    , updateDocumentContentDebouncer(300)
{
}


// ****


AbstractModel::AbstractModel(const QVector<QString>& _tags, QObject* _parent)
    : QAbstractItemModel(_parent)
    , d(new Implementation(_tags))
{
    connect(&d->updateDocumentContentDebouncer, &Debouncer::gotWork, this,
            &AbstractModel::saveChanges);

    connect(this, &AbstractModel::modelReset, this, &AbstractModel::updateDocumentContent);
    connect(this, &AbstractModel::rowsInserted, this, &AbstractModel::updateDocumentContent);
    connect(this, &AbstractModel::rowsRemoved, this, &AbstractModel::updateDocumentContent);
    connect(this, &AbstractModel::rowsMoved, this, &AbstractModel::updateDocumentContent);
    connect(this, &AbstractModel::dataChanged, this, &AbstractModel::updateDocumentContent);
}

AbstractModel::~AbstractModel() = default;

Domain::DocumentObject* AbstractModel::document() const
{
    return d->document;
}

void AbstractModel::setDocument(Domain::DocumentObject* _document)
{
    if (d->document == _document) {
        return;
    }

    d->document = _document;

    initDocument();
}

QString AbstractModel::documentName() const
{
    return {};
}

void AbstractModel::setDocumentName(const QString& _name)
{
    Q_UNUSED(_name)
}

QColor AbstractModel::documentColor() const
{
    return {};
}

void AbstractModel::setDocumentColor(const QColor& _color)
{
    Q_UNUSED(_color)
}

void AbstractModel::setImageWrapper(AbstractImageWrapper* _image)
{
    d->image = _image;
}

void AbstractModel::clear()
{
    d->updateDocumentContentDebouncer.abortWork();
    d->document = nullptr;

    clearDocument();
}

void AbstractModel::saveChanges()
{
    if (d->document == nullptr) {
        return;
    }

    const auto content = toXml();
    //
    const QByteArray undoPatch = d->dmpController.makePatch(content, d->document->content());
    if (undoPatch.isEmpty()) {
        return;
    }
    //
    const QByteArray redoPatch = d->dmpController.makePatch(d->document->content(), content);
    if (redoPatch.isEmpty()) {
        return;
    }

    //
    // Уведомляем о смене контента только, если контент уже был установлен,
    // если содержимое документа было пустым (как правило это кейс после создания документа),
    // то отменять и нечего, поэтому игнорируем такие изменения
    //
    const auto needToNotifyAboutContentChanged = !d->document->content().isEmpty();
    d->document->setContent(content);
    if (needToNotifyAboutContentChanged) {
        emit contentsChanged(undoPatch, redoPatch);
    }
}

void AbstractModel::undo()
{

    //
    // Перед отменой последнего действия нужно сохранить все несохранённые изменения
    //
    saveChanges();

    //
    // И после этого уже можно отменять
    //
    emit undoRequested(d->undoStep);
}

void AbstractModel::undoChange(const QByteArray& _undo, const QByteArray& _redo)
{
    QScopedValueRollback isUndoInProgressRollback(d->isUndoInProgress, true);

    applyPatch(_undo);
    saveChanges();

    d->undoedChanges.append({ _undo, _redo });
    d->undoStep += 2;
}

void AbstractModel::redo()
{
    if (d->undoedChanges.isEmpty()) {
        return;
    }

    QScopedValueRollback isRedoInProgressRollback(d->isRedoInProgress, true);

    const auto change = d->undoedChanges.takeLast();
    applyPatch(change.redo);
    saveChanges();
}

QModelIndex AbstractModel::index(int _row, int _column, const QModelIndex& _parent) const
{
    Q_UNUSED(_row)
    Q_UNUSED(_column)
    Q_UNUSED(_parent)

    return {};
}

QModelIndex AbstractModel::parent(const QModelIndex& _child) const
{
    Q_UNUSED(_child)

    return {};
}

int AbstractModel::columnCount(const QModelIndex& _parent) const
{
    Q_UNUSED(_parent)

    return {};
}

int AbstractModel::rowCount(const QModelIndex& _parent) const
{
    Q_UNUSED(_parent)

    return {};
}

QVariant AbstractModel::data(const QModelIndex& _index, int _role) const
{
    Q_UNUSED(_index)
    Q_UNUSED(_role)

    return {};
}

void AbstractModel::applyPatch(const QByteArray& _patch)
{
    const auto newContent = d->dmpController.applyPatch(toXml(), _patch);

    clearDocument();
    document()->setContent(newContent);
    initDocument();
}

AbstractImageWrapper* AbstractModel::imageWrapper() const
{
    Q_ASSERT(d->image);
    return d->image;
}

const DiffMatchPatchController& AbstractModel::dmpController() const
{
    return d->dmpController;
}

void AbstractModel::updateDocumentContent()
{
    //
    // В режиме отмены/повтора последнего действия сохранение изменений будет делаться вручную
    //
    if (d->isUndoInProgress || d->isRedoInProgress) {
        return;
    }

    //
    // Если произошло изменение в самой модели, то очищаем список изменений доступных для повтора
    //
    d->undoedChanges.clear();
    d->undoStep = 0;

    //
    // Запускаем таймер сохранения изменений
    //
    d->updateDocumentContentDebouncer.orderWork();
}

} // namespace BusinessLayer
