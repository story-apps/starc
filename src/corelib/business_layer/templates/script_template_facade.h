#pragma once

class QString;


namespace BusinessLayer
{

/**
 * @brief Фасад доступа к шаблонам сценария
 */
class ScriptTemplateFacade
{
//public:
//    /**
//     * @brief Получить список шаблонов
//     */
//    static QStandardItemModel* templatesList();

//    /**
//     * @brief Проверить существование шаблона с заданным именем
//     */
//    static bool containsTemplate(const QString& _templateName);

//    /**
//     * @brief Получить шаблон в соответствии с заданным именем
//     * Если имя не задано, возвращается стандартный шаблон
//     */
//    static ScenarioTemplate getTemplate(const QString& _templateName = QString());

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

//private:
//    ScenarioTemplateFacade();
//    static ScenarioTemplateFacade* s_instance;
//    static void init();

//private:
//    /**
//     * @brief Шаблон по умолчанию
//     */
//    ScenarioTemplate m_defaultTemplate;

//    /**
//     * @brief Шаблоны сценариев
//     */
//    QMap<QString, ScenarioTemplate> m_templates;

//    /**
//     * @brief Модель шаблонов
//     */
//    QStandardItemModel* m_templatesModel;
};

} // namespace BusinssLayer
