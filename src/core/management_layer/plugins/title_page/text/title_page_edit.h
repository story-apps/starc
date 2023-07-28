#pragma once

#include <ui/modules/script_text_edit/script_text_edit.h>

namespace BusinessLayer {
class TitlePageModel;
class TextTemplate;
enum class TextParagraphType;
} // namespace BusinessLayer


namespace Ui {

/**
 * @brief Текстовый редактор
 */
class TitlePageEdit : public ScriptTextEdit
{
    Q_OBJECT

public:
    explicit TitlePageEdit(QWidget* _parent = nullptr);
    ~TitlePageEdit() override;

    /**
     * @brief Задать модель текста сценария
     */
    void initWithModel(BusinessLayer::TitlePageModel* _model);

    /**
     * @brief Перенастроить редактор в соответствии с текущей моделью
     * @note Например в случае смены шаблона оформления сценария
     */
    void reinit();

    /**
     * @brief Текущий используемый шаблон оформления
     */
    const BusinessLayer::TextTemplate& textTemplate() const;


    /**
     * @brief Отменить последнее изменение
     */
    void undo();

    /**
     * @brief Повторить последнее отменённое изменение
     */
    void redo();

    /**
     * @brief Добавить список действующих лиц
     */
    void addCastList();

    /**
     * @brief Восстановить дефолтную титульную страницу из шаблона
     */
    void restoreFromTemplate();

    /**
     * @brief Вставить новый блок
     * @param Тип блока
     */
    void addParagraph(BusinessLayer::TextParagraphType _type);

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
    void addReviewMark(const QColor& _textColor, const QColor& _backgroundColor,
                       const QString& _comment);

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
    bool keyPressEventReimpl(QKeyEvent* _event) override;

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
