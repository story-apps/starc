#pragma once

#include <ui/widgets/text_edit/base/base_text_edit.h>

namespace BusinessLayer {
    class CharactersModel;
    class LocationsModel;
    class ScreenplayDictionariesModel;
    class ScreenplayTextCursor;
    class ScreenplayTextModel;
    enum class ScreenplayParagraphType;
}


namespace Ui
{

/**
 * @brief Текстовый редактор сценария
 */
class ScreenplayTextEdit : public BaseTextEdit
{
    Q_OBJECT

public:
    explicit ScreenplayTextEdit(QWidget* _parent = nullptr);
    ~ScreenplayTextEdit() override;

    /**
     * @brief Включить отображение номеров сцен
     */
    void setShowSceneNumber(bool _show, bool _onLeft, bool _onRight);

    /**
     * @brief Включить отображение номеров реплик
     */
    void setShowDialogueNumber(bool _show);

    /**
     * @brief Задать модель текста сценария
     */
    void initWithModel(BusinessLayer::ScreenplayTextModel* _model);

    /**
     * @brief Получить модель справочников сценария
     */
    BusinessLayer::ScreenplayDictionariesModel* dictionaries() const;

    /**
     * @brief Получить модель персонажей
     */
    BusinessLayer::CharactersModel* characters() const;

    /**
     * @brief Получить модель локаций
     */
    BusinessLayer::LocationsModel* locations() const;

    /**
     * @brief Отменить последнее изменение
     */
    void undo();

    /**
     * @brief Повторить последнее отменённое изменение
     */
    void redo();

    /**
     * @brief Вставить новый блок
     * @param Тип блока
     */
    void addParagraph(BusinessLayer::ScreenplayParagraphType _type);

    /**
     * @brief Установить тип текущего блока
     * @param Тип блока
     */
    void setCurrentParagraphType(BusinessLayer::ScreenplayParagraphType _type);

    /**
     * @brief Получить тип блока в котором находится курсор
     */
    BusinessLayer::ScreenplayParagraphType currentParagraphType() const;

    /**
     * @brief Своя реализация установки курсора
     */
    void setTextCursorReimpl(const QTextCursor& _cursor);

    /**
     * @brief Получить индекс элемента модели в текущей позиции курсора
     */
    QModelIndex currentModelIndex() const;

    /**
     * @brief Поставить курсор в позицию элемента с заданным индексом модели сценария
     */
    void setCurrentModelIndex(const QModelIndex& _index);

    /**
     * @brief Получить позицию заданного элемента модели
     */
    int positionForModelIndex(const QModelIndex& _index);

    /**
     * @brief Добавить редакторскую заметку для текущего выделения
     */
    void addReviewMark(const QColor& _textColor, const QColor& _backgroundColor, const QString& _comment);

signals:
    /**
     * @brief Изменён тип абзаца
     */
    void paragraphTypeChanged();

protected:
    /**
     * @brief Нажатия многих клавиш обрабатываются вручную
     */
    void keyPressEvent(QKeyEvent* _event) override;

    /**
     * @brief Обрабатываем специфичные ситуации для редактора сценария
     */
    /** @{ */
    bool keyPressEventReimpl(QKeyEvent* _event) override;
    bool updateEnteredText(const QString& _eventText) override;
    /** @} */

    /**
     * @brief Реализуем отрисовку дополнительных элементов
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Переопределяем для добавления в меню собственных пунктов
     */
    ContextMenu* createContextMenu(const QPoint& _position, QWidget* _parent = nullptr) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
