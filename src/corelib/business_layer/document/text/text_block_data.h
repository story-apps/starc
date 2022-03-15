#pragma once

#include <QTextBlockUserData>

#include <corelib_global.h>


namespace BusinessLayer {

class TextModelItem;

/**
 * @brief Данные параграфа текста содержащие указатель на соответствующий элемент модели
 */
class CORE_LIBRARY_EXPORT TextBlockData : public QTextBlockUserData
{
public:
    explicit TextBlockData(BusinessLayer::TextModelItem* _item);
    explicit TextBlockData(const TextBlockData* _other);

    BusinessLayer::TextModelItem* item() const;

private:
    BusinessLayer::TextModelItem* m_item = nullptr;
};

} // namespace BusinessLayer
