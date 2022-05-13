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
 * @brief Класс шаблона пьесы
 */
class CORE_LIBRARY_EXPORT StageplayTemplate : public TextTemplate
{
    class Implementation;

public:
    StageplayTemplate();
    explicit StageplayTemplate(const QString& _fromFile);

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
