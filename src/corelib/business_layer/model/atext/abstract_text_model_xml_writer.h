#pragma once

#include <QScopedPointer>

namespace BusinessLayer {

class AbstractTextModelItem;
class AbstractTextModelTextItem;

namespace xml {

/**
 * @brief Класс для формирования xml данных текстового документа
 * @note Основное предназначение - склеивать разорванные текстовые блоки перед записью
 */
class AbstractTextModelXmlWriter
{
public:
    explicit AbstractTextModelXmlWriter(bool _addHeader = false);
    ~AbstractTextModelXmlWriter();

    void operator+=(const char* _data);
    void operator+=(const QByteArray& _data);
    void operator+=(const QString& _data);
    void operator+=(AbstractTextModelItem* _item);
    struct TextItemData {
        AbstractTextModelTextItem* item = nullptr;
        int fromPosition = 0;
        int toPosition = 0;
    };
    void operator+=(const TextItemData& _data);

    QByteArray data() const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace xml
} // namespace BusinessLayer
