#pragma once

#include <business_layer/model/abstract_model_item.h>

#include <QtContainerFwd>

class QXmlStreamReader;
class QStringRef;


namespace BusinessLayer {

class TextModel;

/**
 * @brief Перечисление типов элементов модели текста
 */
enum class CORE_LIBRARY_EXPORT TextModelItemType {
    //
    // Элементы имеющие открывающий и закрывающий блоки (Акты и папки)
    //
    Folder,
    //
    // Элементы имеющие только открывающий блок (сцена, бит, страница, панель, глава)
    //
    Group,
    //
    // Текстовый элемент - параграф текста
    //
    Text,
    //
    // Разделитель страницы на таблицу
    //
    Splitter,
};


/**
 * @brief Базовый класс элемента модели текста
 */
class CORE_LIBRARY_EXPORT TextModelItem : public AbstractModelItem
{
public:
    TextModelItem(TextModelItemType _type, const TextModel* _model);
    ~TextModelItem() override;

    /**
     * @brief Получить тип элемента
     */
    const TextModelItemType& type() const;

    /**
     * @brief Подтип элемента
     */
    virtual int subtype() const;

    /**
     * @brief Кастомная иконка элемента
     */
    QString customIcon() const;
    void setCustomIcon(const QString& _icon);

    /**
     * @brief Получить модель, в которой находится данный элемент
     */
    const TextModel* model() const;

    /**
     * @brief Переопределяем интерфейс для возврата элемента собственного класса
     */
    TextModelItem* parent() const override;
    TextModelItem* childAt(int _index) const override;

    /**
     * @brief Определяем интерфейс получения данных элемента
     */
    QVariant data(int _role) const override;

    /**
     * @brief Количество слов
     */
    int wordsCount() const;
    void setWordsCount(int _count);

    /**
     * @brief Количество символов
     */
    QPair<int, int> charactersCount() const;
    void setCharactersCount(QPair<int, int> _count);

    /**
     * @brief Считать контент из заданного ридера
     */
    virtual void readContent(QXmlStreamReader& _contentReader) = 0;

    /**
     * @brief Сформировать xml блока
     */
    virtual QByteArray toXml() const = 0;

    /**
     * @brief Скопировать контент с заданного элемента
     */
    virtual void copyFrom(TextModelItem* _item) = 0;

    /**
     * @brief Проверить равен ли текущий элемент заданному
     */
    virtual bool isEqual(TextModelItem* _item) const = 0;

protected:
    /**
     * @brief Считать кастомный контент и вернуть название тэга на котором стоит ридер
     */
    virtual QStringRef readCustomContent(QXmlStreamReader& _contentReader);

    /**
     * @brief Сформировать xml-блок с кастомными данными элемента
     */
    virtual QByteArray customContent() const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
