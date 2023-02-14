#pragma once

#include <QObject>

#include <corelib_global.h>

class QTextDocument;


namespace BusinessLayer {

class TextModelItem;

/**
 * @brief Класс корректирующий текст документа
 */
class CORE_LIBRARY_EXPORT AbstractTextCorrector : public QObject
{
    Q_OBJECT

public:
    explicit AbstractTextCorrector(QTextDocument* _document);
    ~AbstractTextCorrector() override;

    /**
     * @brief Документ для корректировки
     */
    QTextDocument* document() const;

    /**
     * @brief Задать идентификатор шаблона, с которым работает корректор
     */
    void setTemplateId(const QString& _templateId);
    QString templateId() const;

    /**
     * @brief Установить необходимость корректировать текст блоков имён персонажей
     */
    virtual void setCorrectionOptions(const QStringList& _options) = 0;

    /**
     * @brief Задавть верхнеуровневый видимый элемент
     */
    void setVisibleTopLevelItem(TextModelItem* _item);
    TextModelItem* visibleTopLevelItem() const;

    /**
     * @brief Очистить все сохранённые параметры
     */
    void clear();

    /**
     * @brief Подготовиться к корректировке
     */
    void planCorrection(int _position, int _charsRemoved, int _charsAdded);

    /**
     * @brief Выполнить корректировку для документа с заданным хэшем
     */
    void makePlannedCorrection(const QByteArray& _contentHash);

protected:
    /**
     * @brief Реализация очистки, которая будет выполняться внутри корректора
     */
    virtual void clearImpl() = 0;

    /**
     * @brief Выполнить все корректировки
     */
    virtual void makeCorrections(int _position = -1, int _charsChanged = 0) = 0;

    /**
     * @brief Выполнить "мягкие" корректировки
     */
    virtual void makeSoftCorrections(int _postion = -1, int _charsChanged = 0) = 0;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
