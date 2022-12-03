#include "abstract_text_corrector.h"

#include <QDebug>
#include <QTextDocument>


namespace BusinessLayer {

class AbstractTextCorrector::Implementation
{
public:
    explicit Implementation(QTextDocument* _document);


    /**
     * @brief Документ который будем корректировать
     */
    QTextDocument* document = nullptr;

    /**
     * @brief Шаблон оформления сценария
     */
    QString templateId;

    /**
     * @brief Запланированная корректировка
     */
    struct {
        bool isValid = false;
        int position = 0;
        int lenght = 0;
        int end() const
        {
            return position + lenght;
        }
    } plannedCorrection;

    /**
     * @brief Информация о последнем скорректированном документе
     */
    struct {
        QByteArray hash = {};
        int blockCount = 0;
    } lastContent;
};

AbstractTextCorrector::Implementation::Implementation(QTextDocument* _document)
    : document(_document)
{
}


// ****


AbstractTextCorrector::AbstractTextCorrector(QTextDocument* _document)
    : QObject(_document)
    , d(new Implementation(_document))
{
    Q_ASSERT_X(d->document, Q_FUNC_INFO, "Document couldn't be a nullptr");
}

AbstractTextCorrector::~AbstractTextCorrector() = default;

QTextDocument* AbstractTextCorrector::document() const
{
    return d->document;
}

void AbstractTextCorrector::setTemplateId(const QString& _templateId)
{
    if (d->templateId == _templateId) {
        return;
    }

    d->templateId = _templateId;
    makeCorrections();
}

QString AbstractTextCorrector::templateId() const
{
    return d->templateId;
}

void AbstractTextCorrector::planCorrection(int _position, int _charsRemoved, int _charsAdded)
{
    //
    // Если корректировка ещё не была запланирована, то просто заполняем информацию
    // об изменённой части текстового документ
    //
    if (!d->plannedCorrection.isValid) {
        d->plannedCorrection = { true, _position, std::max(_charsRemoved, _charsAdded) };
    }
    //
    // А если уже была запланирована, то расширим выделение
    //
    else if (d->plannedCorrection.position >= _position) {
        const auto newPosition = _position;
        const auto newLenght = std::max(_charsRemoved, _charsAdded);
        if (newPosition < d->plannedCorrection.position) {
            d->plannedCorrection.lenght += d->plannedCorrection.position - newPosition;
            d->plannedCorrection.position = newPosition;
        }
        const auto newEnd = newPosition + newLenght;
        if (newEnd > d->plannedCorrection.end()) {
            d->plannedCorrection.lenght = newEnd - d->plannedCorrection.position;
        }
    }
}

void AbstractTextCorrector::makePlannedCorrection(const QByteArray& _contentHash)
{
    if (!d->plannedCorrection.isValid) {
        return;
    }

    if (d->lastContent.hash == _contentHash
        && d->lastContent.blockCount == d->document->blockCount()) {
        makeSoftCorrections(d->plannedCorrection.position, d->plannedCorrection.lenght);
        return;
    }

    qDebug() << d->document << d->lastContent.hash << d->lastContent.blockCount;

    makeCorrections(d->plannedCorrection.position, d->plannedCorrection.lenght);

    d->lastContent.hash = _contentHash;
    d->lastContent.blockCount = d->document->blockCount();

    d->plannedCorrection = {};
}

} // namespace BusinessLayer
