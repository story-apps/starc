#pragma once

#include <ui/widgets/text_edit/completer/completer_text_edit.h>

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
class ScreenplayTextEdit : public CompleterTextEdit
{
public:
    explicit ScreenplayTextEdit(QWidget* _parent = nullptr);
    ~ScreenplayTextEdit() override;

    /**
     * @brief Задать модель текста сценария
     */
    void setModel(BusinessLayer::ScreenplayTextModel* _model);

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
    void addScenarioBlock(BusinessLayer::ScreenplayParagraphType _blockType);

    /**
     * @brief Установить вид текущего блока
     * @param Тип блока
     */
    void changeScenarioBlockType(BusinessLayer::ScreenplayParagraphType _blockType, bool _forced = false);

    /**
     * @brief Установить заданный тип блока для всех выделенных блоков
     */
    void changeScenarioBlockTypeForSelection(BusinessLayer::ScreenplayParagraphType _blockType);

    /**
     * @brief Применить тип блока ко всему тексту в блоке
     * @param Тип для применения
     */
    void applyScenarioTypeToBlockText(BusinessLayer::ScreenplayParagraphType _blockType);

    /**
     * @brief Получить вид блока в котором находится курсор
     */
    BusinessLayer::ScreenplayParagraphType scenarioBlockType() const;

    /**
     * @brief Своя реализация установки курсора
     */
    void setTextCursorReimpl(const QTextCursor& _cursor);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
