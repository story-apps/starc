#include "abstract_text_corrector.h"

#include <business_layer/model/text/text_model_item.h>
#include <utils/logging.h>

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
     * @brief Верхнеуровневый видимый элемент модели
     */
    TextModelItem* visibleTopLevelItem = nullptr;

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
        int characterCount = 0;
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

void AbstractTextCorrector::setVisibleTopLevelItem(TextModelItem* _item)
{
    if (d->visibleTopLevelItem == _item) {
        return;
    }

    //
    // Убираем иконку с элемента, котрый раньше был изолирован
    //
    if (d->visibleTopLevelItem != nullptr) {
        d->visibleTopLevelItem->setCustomIcon({});
    }

    //
    // Сохраним новый элемент
    //
    d->visibleTopLevelItem = _item;

    //
    // Применяем к новому элементу иконку изолированности
    //
    if (d->visibleTopLevelItem != nullptr) {
        d->visibleTopLevelItem->setCustomIcon(u8"\U000F0EFF");
    }

    makeCorrections(-1, -1);
}

TextModelItem* AbstractTextCorrector::visibleTopLevelItem() const
{
    return d->visibleTopLevelItem;
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
        && d->lastContent.characterCount == d->document->characterCount()) {
        makeSoftCorrections();
        return;
    }

    makeCorrections(d->plannedCorrection.position, d->plannedCorrection.lenght);

    d->lastContent.hash = _contentHash;
    d->lastContent.characterCount = d->document->characterCount();

    d->plannedCorrection = {};
}

} // namespace BusinessLayer
