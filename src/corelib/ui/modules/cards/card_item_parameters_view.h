#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui {

/**
 * @brief Типы элементов для отображения
 */
enum class CardItemType {
    Undefined,
    Folder,
    Scene,
    Beat,
};

/**
 * @brief Панель параметров карточки
 */
class CORE_LIBRARY_EXPORT CardItemParametersView : public Widget
{
    Q_OBJECT

public:
    explicit CardItemParametersView(QWidget* _parent = nullptr);
    ~CardItemParametersView() override;

    /**
     * @brief Тип элемента, для которого будут отображаться параметры
     */
    void setItemType(CardItemType _type);
    CardItemType itemType() const;

    /**
     * @brief Цвет
     */
    void setColor(const QColor& _color);
    Q_SIGNAL void colorChanged(const QColor& _color);

    /**
     * @brief Название
     */
    void setTitle(const QString& _title);
    Q_SIGNAL void titleChanged(const QString& _title);

    /**
     * @brief Заголовок
     */
    void setHeading(const QString& _heading);
    Q_SIGNAL void headingChanged(const QString& _heading);

    /**
     * @brief Описание папки
     */
    void setDescription(const QString& _description);
    Q_SIGNAL void descriptionChanged(const QString& _description);

    /**
     * @brief Биты
     */
    void setBeats(const QVector<QString>& _beats);
    Q_SIGNAL void beatAdded(int _addedBeatIndex);
    Q_SIGNAL void beatChanged(int _beatIndex, const QString& _beatText);
    Q_SIGNAL void beatRemoved(int _removedBeatIndex);

    /**
     * @brief Сценарный день
     */
    void setStoryDay(const QString& _storyDay, const QVector<QString>& _allStoryDays);
    Q_SIGNAL void storyDayChanged(const QString& _storyDay);

    /**
     * @brief Дата и время начала события
     */
    void setStartDateTimeVisible(bool _visible);
    void setStartDateTime(const QDateTime& _startDateTime);
    Q_SIGNAL void startDateTimeChanged(const QDateTime& _startDateTime);

    /**
     * @brief Штамп
     */
    void setStampVisible(bool _visible);
    void setStamp(const QString& _stamp);
    Q_SIGNAL void stampChanged(const QString& _stamp);

    /**
     * @brief Номер
     */
    void setNumberingVisible(bool _visible);
    void setNumber(const QString& _number, bool _isCustom, bool _isEatNumber, bool _isLocked);
    Q_SIGNAL void numberChanged(const QString& _number, bool _isCustom, bool _isEatNumber);

    /**
     * @brief Тэги
     */
    void setTagsVisible(bool _visible);
    void setTags(const QVector<QPair<QString, QColor>>& _tags,
                 const QVector<QPair<QString, QColor>>& _allTags);
    Q_SIGNAL void tagsChanged(const QVector<QPair<QString, QColor>>& _tags);

    /**
     * @brief Настроить возможность редактирования контента
     */
    void setReadOnly(bool _readOnly);

protected:
    /**
     * @brief Следим за нажатием кнопок в битах
     */
    bool eventFilter(QObject* _watched, QEvent* _event) override;

    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем виджет при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
