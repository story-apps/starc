#pragma once

#include <QScopedPointer>

namespace BusinessLayer {

class ComicBookTextModelItem;
class ComicBookTextModelTextItem;

namespace xml {

/**
 * @brief Класс для формирования xml данных документа текста сценария
 * @note Основное предназначение - склеивать разорванные текстовые блоки перед записью
 */
class ComicBookTextModelXmlWriter
{
public:
    explicit ComicBookTextModelXmlWriter(bool _addHeader = false);
    ~ComicBookTextModelXmlWriter();

    void operator+=(const char* _data);
    void operator+=(const QByteArray& _data);
    void operator+=(const QString& _data);
    void operator+=(ComicBookTextModelItem* _item);
    struct TextItemData {
        ComicBookTextModelTextItem* item = nullptr;
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
