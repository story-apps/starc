#pragma once

#include <corelib_global.h>

#include <QScopedPointer>
#include <QString>

class QStandardItemModel;


namespace BusinessLayer
{

class ScreenplayTemplate;

/**
 * @brief Фасад доступа к шаблонам сценария
 */
class CORE_LIBRARY_EXPORT ScreenplayTemplateFacade
{
public:
    /**
     * @brief Индекс для сохранения в модели информации о идентификаторе шаблона
     */
    static const int kTemplateIdRole = Qt::UserRole + 1;

    /**
     * @brief Получить список шаблонов
     */
    static QStandardItemModel* templates();

//    /**
//     * @brief Проверить существование шаблона с заданным именем
//     */
//    static bool containsTemplate(const QString& _templateName);

    /**
     * @brief Получить шаблон в соответствии с заданным идентификатором
     * @note Если id не задан, возвращается стандартный шаблон
     */
    static const ScreenplayTemplate& getTemplate(const QString& _templateId = {});

    /**
     * @brief Задать стандартный шаблон
     */
    static void setDefaultTemplate(const QString& _templateId);

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

    /**
     * @brief Обновить переводы
     */
    static void updateTranslations();

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
