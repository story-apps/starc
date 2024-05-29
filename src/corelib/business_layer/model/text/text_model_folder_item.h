#pragma once

#include "text_model_item.h"

#include <Qt>
#include <QtContainerFwd>

class QColor;
class QXmlStreamReader;


namespace BusinessLayer {

enum class TextFolderType;

/**
 * @brief Класс элементов папок модели текста
 */
class CORE_LIBRARY_EXPORT TextModelFolderItem : public TextModelItem
{
public:
    /**
     * @brief Роли данных из модели
     */
    enum {
        FolderHeadingRole = Qt::UserRole + 1,
        FolderColorRole,
        FolderDescriptionRole,
        FolderStampRole,
        FolderUserRole,
    };

public:
    explicit TextModelFolderItem(const TextModel* _model, TextFolderType _type);
    ~TextModelFolderItem() override;

    /**
     * @brief Подтип элемента
     */
    int subtype() const override final;

    /**
     * @brief Тип группы
     */
    const TextFolderType& folderType() const;
    void setFolderType(TextFolderType _type);

    /**
     * @brief Цвет папки
     */
    QColor color() const;
    void setColor(const QColor& _color);

    /**
     * @brief Штамп папки
     */
    QString description() const;
    void setDescription(const QString& _description);

    /**
     * @brief Штамп папки
     */
    QString stamp() const;
    void setStamp(const QString& _stamp);

    /**
     * @brief Название папки
     */
    QString heading() const;

    /**
     * @brief Определяем интерфейс получения данных папки
     */
    QVariant data(int _role) const override;

    /**
     * @brief Считать контент из заданного ридера
     */
    void readContent(QXmlStreamReader& _contentReader) override final;

    /**
     * @brief Определяем интерфейс для получения XML блока
     */
    QByteArray toXml() const override;
    QByteArray toXml(TextModelItem* _from, int _fromPosition, TextModelItem* _to, int _toPosition,
                     bool _clearUuid) const;
    QByteArray xmlHeader(bool _clearUuid = false) const;

    /**
     * @brief Скопировать контент с заданного элемента
     */
    void copyFrom(TextModelItem* _item) override;

    /**
     * @brief Проверить равен ли текущий элемент заданному
     */
    bool isEqual(TextModelItem* _item) const override;

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

protected:
    /**
     * @brief Задать заголовок папки
     */
    void setHeading(const QString& _heading);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
