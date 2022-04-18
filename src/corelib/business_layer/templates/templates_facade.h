#pragma once

#include <QScopedPointer>
#include <QString>

#include <corelib_global.h>

class QStandardItemModel;


namespace BusinessLayer {

class TextModel;
class TextTemplate;
class AudioplayTemplate;
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
    static QStandardItemModel* audioplayTemplates();

    /**
     * @brief Получить шаблон используемый для заданной модели
     */
    static const TextTemplate& textTemplate(const TextModel* _model);

    /**
     * @brief Получить шаблон в соответствии с заданным идентификатором
     * @note Если id не задан, возвращается стандартный шаблон
     */
    static const SimpleTextTemplate& simpleTextTemplate(const QString& _templateId = {});
    static const ScreenplayTemplate& screenplayTemplate(const QString& _templateId = {});
    static const TextTemplate& screenplayTitlePageTemplate(const QString& _templateId = {});
    static const ComicBookTemplate& comicBookTemplate(const QString& _templateId = {});
    static const TextTemplate& comicBookTitlePageTemplate(const QString& _templateId = {});
    static const AudioplayTemplate& audioplayTemplate(const QString& _templateId = {});
    static const TextTemplate& audioplayTitlePageTemplate(const QString& _templateId = {});

    /**
     * @brief Задать стандартный шаблон
     */
    static void setDefaultSimpleTextTemplate(const QString& _templateId);
    static void setDefaultScreenplayTemplate(const QString& _templateId);
    static void setDefaultComicBookTemplate(const QString& _templateId);
    static void setDefaultAudioplayTemplate(const QString& _templateId);

    /**
     * @brief Сохранить стиль в библиотеке шаблонов
     */
    static void saveSimpleTextTemplate(const SimpleTextTemplate& _template);
    static void saveScreenplayTemplate(const ScreenplayTemplate& _template);
    static void saveComicBookTemplate(const ComicBookTemplate& _template);
    static void saveAudioplayTemplate(const AudioplayTemplate& _template);

    /**
     * @brief Удалить шаблон по заданному имены
     */
    static void removeSimpleTextTemplate(const QString& _templateId);
    static void removeScreenplayTemplate(const QString& _templateId);
    static void removeComicBookTemplate(const QString& _templateId);
    static void removeAudioplayTemplate(const QString& _templateId);

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
