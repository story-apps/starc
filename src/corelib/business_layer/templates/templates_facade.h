#pragma once

#include <QScopedPointer>
#include <QString>

#include <corelib_global.h>

class QStandardItemModel;


namespace BusinessLayer {

class ComicBookTemplate;
class ScreenplayTemplate;
class SimpleTextTemplate;

/**
 * @brief Фасад доступа к шаблонам текстовых редакторов
 */
class CORE_LIBRARY_EXPORT TemplatesFacade
{
public:
    /**
     * @brief Индекс для сохранения в модели информации о идентификаторе шаблона
     */
    static const int kTemplateIdRole = Qt::UserRole + 1;

    /**
     * @brief Получить список шаблонов
     */
    static QStandardItemModel* simpleTextTemplates();
    static QStandardItemModel* screenplayTemplates();
    static QStandardItemModel* comicBookTemplates();

    /**
     * @brief Получить шаблон в соответствии с заданным идентификатором
     * @note Если id не задан, возвращается стандартный шаблон
     */
    static const BusinessLayer::SimpleTextTemplate& simpleTextTemplate(const QString& _templateId
                                                                       = {});
    static const BusinessLayer::ScreenplayTemplate& screenplayTemplate(const QString& _templateId
                                                                       = {});
    static const BusinessLayer::SimpleTextTemplate& screenplayTitlePageTemplate(
        const QString& _templateId = {});
    static const BusinessLayer::ComicBookTemplate& comicBookTemplate(const QString& _templateId
                                                                     = {});

    /**
     * @brief Задать стандартный шаблон
     */
    static void setDefaultSimpleTextTemplate(const QString& _templateId);
    static void setDefaultScreenplayTemplate(const QString& _templateId);
    static void setDefaultComicBookTemplate(const QString& _templateId);

    /**
     * @brief Сохранить стиль в библиотеке шаблонов
     */
    static void saveScreenplayTemplate(const ScreenplayTemplate& _template);

    /**
     * @brief Удалить шаблон по заданному имены
     */
    static void removeScreenplayTemplate(const QString& _templateId);

    /**
     * @brief Обновить переводы
     */
    static void updateTranslations();

public:
    virtual ~TemplatesFacade();

private:
    TemplatesFacade();
    static TemplatesFacade& instance();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
