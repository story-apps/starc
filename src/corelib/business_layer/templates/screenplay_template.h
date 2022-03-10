#pragma once

#include "text_template.h"

#include <QHash>
#include <QPageSize>
#include <QTextFormat>

#include <corelib_global.h>

class QTextBlock;
class QXmlStreamAttributes;


namespace BusinessLayer {

/**
 * @brief Класс шаблона сценария
 */
class CORE_LIBRARY_EXPORT ScreenplayTemplate : public TextTemplate
{
    class Implementation;

public:
    ScreenplayTemplate();
    explicit ScreenplayTemplate(const QString& _fromFile);

    /**
     * @brief Название
     */
    QString name() const override;
};

} // namespace BusinessLayer
