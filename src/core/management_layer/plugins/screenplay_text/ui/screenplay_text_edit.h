#pragma once

#include <ui/widgets/text_edit/base/base_text_edit.h>

namespace BusinessLayer {
    class CharactersModel;
    class LocationsModel;
    class ScreenplayDictionariesModel;
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
     * @brief Поставить курсор в позицию элемента с заданным индексом модели сценария
     */
    void setCurrentModelIndex(const QModelIndex& _index);

signals:
    /**
     * @brief Сменился текущий элемент модели (сместился курсор)
     */
    void currentModelIndexChanged(const QModelIndex& _index);

    /**
     * @brief Запрос на отмену последнего действия
     */
    void undoRequest();

    /**
     * @brief Запрос на повтор последнего действия
     */
    void redoRequest();

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

private:
    /**
     * @brief Очистить текущий блок от установленного в нём типа
     */
    void cleanParagraphType();

    /**
     * @brief Применить заданный тип к текущему блоку редактора
     * @param Тип блока
     */
    void applyParagraphType(BusinessLayer::ScreenplayParagraphType _type);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
