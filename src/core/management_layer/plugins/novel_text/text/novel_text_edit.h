#pragma once

#include <ui/modules/script_text_edit/script_text_edit.h>

namespace BusinessLayer {
class NovelDictionariesModel;
class TextCursor;
class NovelTextModel;
class NovelTemplate;
enum class TextParagraphType;
} // namespace BusinessLayer

namespace Domain {
struct CursorInfo;
}


namespace Ui {

/**
 * @brief Текстовый редактор сценария
 */
class NovelTextEdit : public ScriptTextEdit
{
    Q_OBJECT

public:
    explicit NovelTextEdit(QWidget* _parent = nullptr);
    ~NovelTextEdit() override;

    /**
     * @brief Включить отображение номеров сцен
     */
    void setShowSceneNumber(bool _show, bool _onLeft, bool _onRight);

    /**
     * @brief Включить отображение номеров реплик
     */
    void setShowDialogueNumber(bool _show);

    /**
     * @brief Настроить необходимость корректировок
     */
    void setCorrectionOptions(bool _needToCorrectPageBreaks);

    /**
     * @brief Задать модель текста сценария
     */
    void initWithModel(BusinessLayer::NovelTextModel* _model);

    /**
     * @brief Перенастроить редактор в соответствии с текущей моделью
     * @note Например в случае смены шаблона оформления сценария
     */
    void reinit();

    /**
     * @brief Текущий используемый шаблон оформления
     */
    const BusinessLayer::NovelTemplate& novelTemplate() const;

    /**
     * @brief Получить модель справочников сценария
     */
    BusinessLayer::NovelDictionariesModel* dictionaries() const;

    /**
     * @brief Получить модель персонажей
     */
    QAbstractItemModel* characters() const;

    /**
     * @brief Создать персонажа с заданным именем
     */
    void createCharacter(const QString& _name);

    /**
     * @brief Получить модель локаций
     */
    QAbstractItemModel* locations() const;

    /**
     * @brief Создать локацию с заданным именем
     */
    void createLocation(const QString& _name);

    /**
     * @brief Отменить последнее изменение
     */
    void undo();

    /**
     * @brief Повторить последнее отменённое изменение
     */
    void redo();

    /**
     * @brief Задать видимость битов в документе
     */
    void setBeatsVisible(bool _visible);

    /**
     * @brief Вставить новый блок
     * @param Тип блока
     */
    void addParagraph(BusinessLayer::TextParagraphType _type);

    /**
     * @brief Установить тип текущего блока
     * @param Тип блока
     */
    void setCurrentParagraphType(BusinessLayer::TextParagraphType _type);

    /**
     * @brief Получить тип блока в котором находится курсор
     */
    BusinessLayer::TextParagraphType currentParagraphType() const;

    /**
     * @brief Получить индекс элемента модели в текущей позиции курсора
     */
    QModelIndex currentModelIndex() const;

    /**
     * @brief Поставить курсор в позицию элемента с заданным индексом модели сценария
     */
    void setCurrentModelIndex(const QModelIndex& _index);

    /**
     * @brief Задать индекс верхнеуровневого видимого элемента
     */
    void setVisibleTopLevelItemIndex(const QModelIndex& _index);

    /**
     * @brief Получить позицию заданного элемента модели
     */
    int positionForModelIndex(const QModelIndex& _index);

    /**
     * @brief Добавить редакторскую заметку для текущего выделения
     */
    void addReviewMark(const QColor& _textColor, const QColor& _backgroundColor,
                       const QString& _comment, bool _isRevision, bool _isAddition,
                       bool _isRemoval);

    /**
     * @brief Настроить автоматический режим добавления редакторских заметок
     */
    void setAutoReviewModeEnabled(bool _enabled);
    void setAutoReviewMode(const QColor& _textColor, const QColor& _backgroundColor,
                           bool _isRevision, bool _isTrackChanges);

signals:
    /**
     * @brief Изменён тип абзаца
     */
    void paragraphTypeChanged();

    /**
     * @brief Пользователь хочет добавить закладку
     */
    void addBookmarkRequested();

    /**
     * @brief Пользователь хочет изменить закладку
     */
    void editBookmarkRequested();

    /**
     * @brief Пользователь хочет удалить закладку
     */
    void removeBookmarkRequested();

    /**
     * @brief Пользователь хочет показать/скрыть список закладок
     */
    void showBookmarksRequested();

protected:
    /**
     * @brief Нажатия многих клавиш обрабатываются вручную
     */
    void keyPressEvent(QKeyEvent* _event) override;

    /**
     * @brief Обрабатываем специфичные ситуации для редактора сценария
     */
    bool keyPressEventReimpl(QKeyEvent* _event) override;

    /**
     * @brief Реализуем отрисовку дополнительных элементов
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Переопределяем для добавления в меню собственных пунктов
     */
    ContextMenu* createContextMenu(const QPoint& _position, QWidget* _parent = nullptr) override;

    /**
     * @brief Переопределяем работу с буфером обмена для использования собственного майм типа данных
     */
    /** @{ */
    bool canInsertFromMimeData(const QMimeData* _source) const override;
    QMimeData* createMimeDataFromSelection() const override;
    void insertFromMimeData(const QMimeData* _source) override;
    void dropEvent(QDropEvent* _event) override;
    /** @} */

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
