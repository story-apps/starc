#pragma once

#include <ui/widgets/text_edit/base/base_text_edit.h>

namespace Domain {
struct CursorInfo;
}


namespace Ui {

/**
 * @brief Базовый класс текстовых редакторов сценария
 */
class CORE_LIBRARY_EXPORT ScriptTextEdit : public BaseTextEdit
{
    Q_OBJECT

public:
    explicit ScriptTextEdit(QWidget* _parent = nullptr);
    ~ScriptTextEdit() override;

    /**
     * @brief Показывать ли автодополнения в пустых блоках
     */
    bool showSuggestionsInEmptyBlocks() const;
    void setShowSuggestionsInEmptyBlocks(bool _show);

    /**
     * @brief Своя реализация установки курсора
     */
    void setTextCursorAndKeepScrollBars(const QTextCursor& _cursor);

    /**
     * @brief Задать курсоры соавторов
     */
    void setCollaboratorsCursors(const QVector<Domain::CursorInfo>& _cursors);

protected:
    /**
     * @brief Обновить положение курсоров соавторов
     */
    void updateCollaboratorsCursors(int _position, int _charsRemoved, int _charsAdded);

    /**
     * @brief Нарисовать курсоры соавторов
     */
    void paintCollaboratorsCursors(QPainter& _painter, const QUuid& _documentUuid,
                                   const QTextBlock& _topBlock,
                                   const QTextBlock& _bottomBlock) const;

    /**
     * @brief Отслеживаем движение мыши, чтобы всегда иметь под рукой её последнюю координату
     * @note QCursor::pos() нам не подходит, т.к. текстовые редакторы находятся внутри
     *       ScalableWrapper и координаты мыши могут несколько искажаться
     */
    void mouseMoveEvent(QMouseEvent* _event) override;

    /**
     * @brief Дополнительная функция для обработки нажатий самим редактором
     * @return Обработано ли событие
     * @note В наследниках необходимо вызывать вручную при необходимости
     */
    bool keyPressEventReimpl(QKeyEvent* _event) override;

    /**
     * @brief Обрабатываем специфичные ситуации для редактора сценария
     */
    bool updateEnteredText(const QKeyEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
