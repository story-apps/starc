#pragma once

#include "text_model_item.h"

#include <QString>

#include <chrono>
#include <optional>

class QColor;
class QDateTime;
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
        /**
         * @brief Порядковый/кастомынй номер
         * @note При блокировке номер незаблокированной группы после последней заблокированной
         */
        QString value;

        /**
         * @brief Текстовое представление номера в соответствии с шаблоном
         */
        QString text;

        /**
         * @brief Является ли номер переопредённым пользователем
         */
        bool isCustom = false;

        /**
         * @brief Необходимо ли захватывать номер группы, если пользователь задал другой номер
         */
        bool isEatNumber = false;

        /**
         * @brief Зафиксирован ли номер сцены
         */
        bool isLocked = false;

        /**
         * @brief Номер предыдущей зафиксированной группы, добавляться к нему будет номер текущей
         */
        QString followNumber = {};

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
        GroupStartDateTimeRole,
        GroupUserRole,
    };

public:
    explicit TextModelGroupItem(const TextModel* _model);
    ~TextModelGroupItem() override;

    /**
     * @brief Подтип элемента
     */
    int subtype() const override final;

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

    /**
     * @brief Номер группы
     */
    std::optional<Number> number() const;
    void resetNumber();
    bool setNumber(int _number, const QString& _followNumber = {});
    void setCustomNumber(const QString& _customNumber, bool _isEatNumber);
    void lockNumber();
    void prepareNumberText(const QString& _template);

    /**
     * @brief Цвет группы
     */
    QColor color() const;
    void setColor(const QColor& _color);

    /**
     * @brief Название группы
     */
    QString title() const;
    void setTitle(const QString& _title);

    /**
     * @brief День сценария
     */
    QString storyDay() const;
    void setStoryDay(const QString& _storyDay);

    /**
     * @brief Дата начала события
     */
    QDateTime startDateTime() const;
    void setStartDateTime(const QDateTime& _startDateTime);

    /**
     * @brief Штамп группы
     */
    QString stamp() const;
    void setStamp(const QString& _stamp);

    /**
     * @brief Тэги группы
     */
    QVector<QPair<QString, QColor>> tags() const;
    void setTags(const QVector<QPair<QString, QColor>>& _tags);

    /**
     * @brief Заголовок группы
     */
    QString heading() const;

    /**
     * @brief Текст группы
     */
    QString text() const;

    /**
     * @brief Длина текста группы
     */
    int length() const;

    /**
     * @brief Количество заметок по тексту в группе
     */
    int inlineNotesSize() const;

    /**
     * @brief Количество редакторских заметок в группе
     */
    int reviewMarksSize() const;

    /**
     * @brief Пуста ли группа
     */
    bool isEmpty() const;

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
     * @brief Сохранить заданный заголовок группы
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

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
