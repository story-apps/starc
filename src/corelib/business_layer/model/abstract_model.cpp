#include "abstract_model.h"

#include "abstract_image_wrapper.h"

#include <domain/document_object.h>

#include <utils/diff_match_patch/diff_match_patch_controller.h>
#include <utils/tools/debouncer.h>

#include <QScopedValueRollback>


namespace BusinessLayer
{

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
     * @brief Дебаунсер на изменение контента документа
     */
    Debouncer updateDocumentContentDebouncer;
};

AbstractModel::Implementation::Implementation(const QVector<QString>& _tags)
    : dmpController(_tags),
      updateDocumentContentDebouncer(300)
{
}


// ****


AbstractModel::AbstractModel(const QVector<QString>& _tags, QObject* _parent)
    : QAbstractItemModel(_parent),
      d(new Implementation(_tags))
{
    connect(&d->updateDocumentContentDebouncer, &Debouncer::gotWork, this, &AbstractModel::saveChanges);

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

void AbstractModel::setDocumentName(const QString& _name)
{
    Q_UNUSED(_name);
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

    d->document->setContent(content);

    emit contentsChanged(undoPatch, redoPatch);
}

void AbstractModel::undo()
{
    emit undoRequested(d->undoedChanges.size());
}

void AbstractModel::undoChange(const QByteArray& _undo, const QByteArray& _redo)
{
    QScopedValueRollback isUndoInProgressRollback(d->isUndoInProgress, true);

    //
    // TODO: Проверить, а можно ли вообще отменить это изменение
    //

    applyPatch(_undo);
    d->undoedChanges.append({_undo, _redo});

    //
    // Говорим, что контент изменился с заданными правками, при этом тут меняется порядок изменений
    //
    emit contentsChanged(_redo, _undo);
}

void AbstractModel::redo()
{
    if (d->undoedChanges.isEmpty()) {
        return;
    }

    QScopedValueRollback isRedoInProgressRollback(d->isRedoInProgress, true);

    const auto change = d->undoedChanges.takeLast();
    applyPatch(change.redo);

    //
    // Говорим, что контент изменился с заданными правками
    //
    emit contentsChanged(change.undo, change.redo);

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
    return d->image;
}

const DiffMatchPatchController& AbstractModel::dmpController() const
{
    return d->dmpController;
}

void AbstractModel::updateDocumentContent()
{
    if (!d->isUndoInProgress && !d->isRedoInProgress) {
        //
        // Если произошло изменение в самой модели, то очищаем список изменений доступных для повтора
        //
        d->undoedChanges.clear();
    }

    d->updateDocumentContentDebouncer.orderWork();
}

} // namespace BusinessLayer
