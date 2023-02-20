#pragma once

#include "text_template.h"

#include <corelib_global.h>

class QTextBlock;
class QXmlStreamAttributes;


namespace BusinessLayer {

/**
 * @brief Класс шаблона сценария
 */
class CORE_LIBRARY_EXPORT ComicBookTemplate : public TextTemplate
{
    class Implementation;

public:
    ComicBookTemplate();
    explicit ComicBookTemplate(const QString& _fromFile);

    /**
     * @brief Название
     */
    QString name() const override;

    /**
     * @brief Можно ли объединять таблицы в данном шаблоне
     */
    bool canMergeParagraph() const override;
};

} // namespace BusinessLayer
