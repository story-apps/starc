#pragma once

#include <QScopedPointer>
#include <QString>

class QStandardItemModel;


namespace BusinessLayer
{

class ScreenplayTemplate;

/**
 * @brief Фасад доступа к шаблонам сценария
 */
class ScreenplayTemplateFacade
{
public:
    /**
     * @brief Получить список шаблонов
     */
    static QStandardItemModel* templatesList();

//    /**
//     * @brief Проверить существование шаблона с заданным именем
//     */
//    static bool containsTemplate(const QString& _templateName);

    /**
     * @brief Получить шаблон в соответствии с заданным именем
     * Если имя не задано, возвращается стандартный шаблон
     */
    static const ScreenplayTemplate& getTemplate(const QString& _templateName = {});

//    /**
//     * @brief Сохранить стиль в библиотеке шаблонов
//     */
//    /** @{ */
//    static void saveTemplate(const ScenarioTemplate& _template);
//    static bool saveTemplate(const QString& _templateFilePath);
//    /** @} */

//    /**
//     * @brief Удалить шаблон по заданному имены
//     */
//    static void removeTemplate(const QString& _templateName);

//    /**
//     * @brief Обновить цвета блоков текста для всех шаблонов
//     */
//    static void updateTemplatesColors();

public:
    ~ScreenplayTemplateFacade();

private:
    ScreenplayTemplateFacade();
    static ScreenplayTemplateFacade& instance();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinssLayer
