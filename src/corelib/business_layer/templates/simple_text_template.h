#pragma once

#include "text_template.h"

#include <corelib_global.h>

class QTextBlock;
class QXmlStreamAttributes;


namespace BusinessLayer {

/**
 * @brief Класс шаблона сценария
 */
class CORE_LIBRARY_EXPORT SimpleTextTemplate : public TextTemplate
{
    class Implementation;

public:
    SimpleTextTemplate();
    explicit SimpleTextTemplate(const QString& _fromFile);

    /**
     * @brief Название
     */
    QString name() const override;
};

} // namespace BusinessLayer
