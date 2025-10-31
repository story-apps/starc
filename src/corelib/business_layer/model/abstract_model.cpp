#include "abstract_model.h"

#include "abstract_image_wrapper.h"
#include "abstract_model_item.h"

#include <domain/document_object.h>
#include <utils/diff_match_patch/diff_match_patch_controller.h>
#include <utils/tools/debouncer.h>

#include <QApplication>
#include <QDomDocument>
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
     * @brief Счётчик транзакций операции сброса модели
     */
    int resetModelTransationsCounter = 0;

    /**
     * @brief Загрузчик фотографий
     */
    AbstractImageWrapper* imageWrapper = nullptr;

    /**
     * @brief Загрузчик сырых данный
     */
    AbstractRawDataWrapper* rawDataWrapper = nullptr;

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
     * @brief Выполняется ли в данный момент наложение изменений
     */
    bool isChangesApplyingInProgress = false;

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
     * @brief Позиция текста после отмены/повтора последнего действия
     */
    ChangeCursor undoRedoPosition;

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

    if (d->document) {
        initDocument();
    } else {
        clearDocument();
    }
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

void AbstractModel::setImageWrapper(AbstractImageWrapper* _imageWrapper)
{
    d->imageWrapper = _imageWrapper;

    if (d->imageWrapper != nullptr) {
        initImageWrapper();
    }
}

void AbstractModel::setRawDataWrapper(AbstractRawDataWrapper* _wrapper)
{
    d->rawDataWrapper = _wrapper;

    if (d->rawDataWrapper != nullptr) {
        initRawDataWrapper();
    }
}

void AbstractModel::clear()
{
    d->updateDocumentContentDebouncer.abortWork();
    d->document = nullptr;

    clearDocument();
}

void AbstractModel::reassignContent()
{
    if (d->document == nullptr) {
        return;
    }

    d->document->setContent(toXml());
}

void AbstractModel::saveChanges()
{
    //
    // Если документ не задан, либо был наполнен данными при инициилизации, то ничего не сохраняем
    //
    if (d->document == nullptr || d->document->content().isNull()) {
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

ChangeCursor AbstractModel::undo()
{
    //
    // Перед отменой последнего действия нужно сохранить все несохранённые изменения
    //
    saveChanges();

    //
    // И после этого уже можно отменять
    //
    emit undoRequested(d->undoStep);

    return d->undoRedoPosition;
}

void AbstractModel::undoChange(const QByteArray& _undo, const QByteArray& _redo)
{
    QScopedValueRollback isUndoInProgressRollback(d->isUndoInProgress, true);

    d->undoRedoPosition = applyPatch(_undo);
    saveChanges();

    d->undoedChanges.append({ _undo, _redo });
    d->undoStep += 2;
}

bool AbstractModel::isUndoInProcess() const
{
    return d->isUndoInProgress;
}

ChangeCursor AbstractModel::redo()
{
    if (d->undoedChanges.isEmpty()) {
        return {};
    }

    QScopedValueRollback isRedoInProgressRollback(d->isRedoInProgress, true);

    const auto change = d->undoedChanges.takeLast();
    d->undoRedoPosition = applyPatch(change.redo);
    saveChanges();

    return d->undoRedoPosition;
}

bool AbstractModel::isRedoInProcess() const
{
    return d->isRedoInProgress;
}

bool AbstractModel::mergeDocumentChanges(const QByteArray _content,
                                         const QVector<QByteArray>& _patches)
{
    if (_content.isEmpty() && _patches.isEmpty()) {
        return false;
    }

    auto newContent = _content.isEmpty() ? toXml() : _content;
    for (const auto& patch : _patches) {
        auto patchedContent = d->dmpController.applyPatch(newContent, patch);

        //
        // Если патч не принёс успеха, значит ошибка в наложении изменений
        //
        if (patchedContent.size() == newContent.size() && patchedContent == newContent) {
            return false;
        }

        newContent.swap(patchedContent);
    }

    //
    // Если все эти изменения уже есть у нас локально, не нужно их заново применять
    // TODO: оптимизировать это место, сравнивать огромные строки думаю, что довольно ресурсоёмко
    //
    if (!_content.isEmpty() && toXml() == newContent) {
        return true;
    }

    //
    // Если применение патча, привело к поломке xml, то нахуй такой патч
    //
    {
        QDomDocument document;
        QString error;
        int line = 0;
        int column = 0;
        const auto isXmlFine = document.setContent(newContent, &error, &line, &column);
        if (!isXmlFine) {
            qWarning("***********");
            qWarning("Problem with patch applying happened");
            qWarning("Current document xml is (toXml()):");
            qWarning(qUtf8Printable(toXml()));
            qWarning("\n\n\n\n");
            qWarning("Input content is (_content):");
            qWarning(qUtf8Printable(_content));
            qWarning("\n\n\n\n");
            for (int i = 0; i < _patches.size(); ++i) {
                qWarning(QString("Input patch number %1 is (_patches[%1]):").arg(i).toUtf8());
                qWarning(qUtf8Printable(QByteArray::fromPercentEncoding(_patches[i])));
                qWarning("\n\n\n\n");
            }
            qWarning("New content is (newContent):");
            qWarning(qUtf8Printable(newContent));
            return false;
        }
    }

    beginResetModelTransaction();
    clearDocument();
    document()->setContent(newContent);
    initDocument();
    endResetModelTransaction();
    return true;
}

void AbstractModel::applyDocumentChanges(const QVector<QByteArray>& _patches)
{
    QScopedValueRollback isChangesApplyingInProgressRollback(d->isChangesApplyingInProgress, true);

    emit changesAboutToBeApplied();

    //
    // Накладываем изменения
    //
    ChangeCursor lastChangeCursor;
    for (const auto& patch : _patches) {
        lastChangeCursor = applyPatch(patch);
    }

    //
    // Чтобы изменения не продуцировали патч, применим новый контент с изменениями к документу
    //
    reassignContent();

    emit changesApplied(lastChangeCursor);
}

QPair<QByteArray, QByteArray> AbstractModel::adoptDocumentChanges(const QByteArray& _content)
{
    const auto content = toXml();
    const auto undoPatch = d->dmpController.makePatch(content, _content);
    const auto redoPatch = d->dmpController.makePatch(_content, content);
    return { undoPatch, redoPatch };
}

QVector<QPair<QByteArray, QByteArray>> AbstractModel::adoptDocumentChanges(
    const QVector<QByteArray>& _patches)
{
    QVector<QPair<QByteArray, QByteArray>> adoptedPatches;
    auto content = toXml();
    for (const auto& patch : _patches) {
        auto patchedContent = d->dmpController.applyPatch(content, patch);

        //
        // Если патч не принёс успеха, значит ошибка в наложении изменений
        //
        if (patchedContent.size() == content.size() && patchedContent == content) {
            return {};
        }

        //
        // Формируем корректные патчи для отмены и повтора последнего действия
        //
        const QByteArray undoPatch = d->dmpController.makePatch(content, patchedContent);
        if (undoPatch.isEmpty()) {
            return {};
        }
        const QByteArray redoPatch = d->dmpController.makePatch(patchedContent, content);
        if (redoPatch.isEmpty()) {
            return {};
        }

        //
        // Сохраним патчи
        //
        adoptedPatches.append({ undoPatch, redoPatch });

        //
        // Сохраним текущее состояние документа, чтобы продолжать накладывать следующие патчи
        //
        content.swap(patchedContent);
    }

    return adoptedPatches;
}

bool AbstractModel::isChangesApplyingInProcess() const
{
    return d->isChangesApplyingInProgress;
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

void AbstractModel::beginChangeRows()
{
    emit rowsAboutToBeChanged();
}

void AbstractModel::endChangeRows()
{
    emit rowsChanged();
}

void AbstractModel::beginResetModelTransaction()
{
    if (d->resetModelTransationsCounter == 0) {
        beginResetModel();
    }

    ++d->resetModelTransationsCounter;
}

void AbstractModel::endResetModelTransaction()
{
    --d->resetModelTransationsCounter;

    if (d->resetModelTransationsCounter == 0) {
        endResetModel();
    }
}

void AbstractModel::initImageWrapper()
{
}

void AbstractModel::initRawDataWrapper()
{
}

ChangeCursor AbstractModel::applyPatch(const QByteArray& _patch)
{
    const auto newContent = d->dmpController.applyPatch(toXml(), _patch);

    clearDocument();
    document()->setContent(newContent);
    initDocument();

    return {};
}

AbstractImageWrapper* AbstractModel::imageWrapper() const
{
    Q_ASSERT(d->imageWrapper);
    return d->imageWrapper;
}

AbstractRawDataWrapper* AbstractModel::rawDataWrapper() const
{
    Q_ASSERT(d->rawDataWrapper);
    return d->rawDataWrapper;
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
    if (d->isUndoInProgress || d->isRedoInProgress || d->isChangesApplyingInProgress) {
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
