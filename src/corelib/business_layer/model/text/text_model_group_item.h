#pragma once

#include "text_model_item.h"

#include <QString>

#include <chrono>
#include <optional>

class QColor;
class QUuid;
class QXmlStreamReader;


namespace BusinessLayer {

enum class TextGroupType;

/**
 * @brief Класс элементов групп модели текста
 */
class CORE_LIBRARY_EXPORT TextModelGroupItem : public TextModelItem
{
public:
    /**
     * @brief Номер группы
     */
    struct Number {
        int value = 0;
        QString text;

        bool operator==(const Number& _other) const;
    };

    /**
     * @brief Роли данных из модели
     */
    enum {
        GroupTypeRole = Qt::UserRole + 1,
        GroupNumberRole,
        GroupHeadingRole,
        GroupTextRole,
        GroupColorRole,
        GroupInlineNotesSizeRole,
        GroupReviewMarksSizeRole,
        GroupUserRole,
    };

public:
    explicit TextModelGroupItem(const TextModel* _model);
    ~TextModelGroupItem() override;

    /**
     * @brief Тип группы
     */
    const TextGroupType& groupType() const;
    void setGroupType(TextGroupType _type);

    /**
     * @brief Идентификатор группы
     */
    QUuid uuid() const;

    /**
     * @brief Уровень вложенности группы
     */
    int level() const;
    void setLevel(int _level);

    /**
     * @brief Номер группы
     */
    std::optional<Number> number() const;
    bool setNumber(int _number, const QString& _prefix);

    /**
     * @brief Цвет группы
     */
    QColor color() const;
    void setColor(const QColor& _color);

    /**
     * @brief Заголовок группы
     */
    QString heading() const;

    /**
     * @brief Текст группы
     */
    QString text() const;

    /**
     * @brief Количество заметок по тексту в группе
     */
    int inlineNotesSize() const;

    /**
     * @brief Количество редакторских заметок в группе
     */
    int reviewMarksSize() const;

    /**
     * @brief Определяем интерфейс получения данных группы
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

protected:
    /**
     * @brief Задать заголовок группы
     */
    void setHeading(const QString& _heading);

    /**
     * @brief Задать текст группы
     */
    void setText(const QString& _text);

    /**
     * @brief Задать количество заметок по тексту в группе
     */
    void setInlineNotesSize(int _size);

    /**
     * @brief Задать количество заметок на полях в группе
     */
    void setReviewMarksSize(int _size);

    /**
     * @brief Считать кастомный контент и вернуть название тэга на котором стоит ридер
     */
    virtual QStringRef readCustomContent(QXmlStreamReader& _contentReader) = 0;

    /**
     * @brief Сформировать xml-блок с кастомными данными элемента
     */
    virtual QByteArray customContent() const = 0;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
