#pragma once

#include "text_template.h"

#include <corelib_global.h>

class QTextBlock;
class QXmlStreamAttributes;


namespace BusinessLayer {

/**
 * @brief Класс шаблона
 */
class CORE_LIBRARY_EXPORT NovelTemplate : public TextTemplate
{
    class Implementation;

public:
    NovelTemplate();
    explicit NovelTemplate(const QString& _fromFile);

    /**
     * @brief Название
     */
    QString name() const override;
};

} // namespace BusinessLayer
