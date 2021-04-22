#pragma once

#include <QScopedPointer>

namespace BusinessLayer {

class ScreenplayTextModelItem;
class ScreenplayTextModelTextItem;

namespace xml {

class ScreenplayTextModelXmlWriter
{
public:
    explicit ScreenplayTextModelXmlWriter(bool _addHeader = false);
    ~ScreenplayTextModelXmlWriter();

    void operator+=(const char* _data);
    void operator+=(const QByteArray& _data);
    void operator+=(const QString& _data);
    void operator+=(ScreenplayTextModelItem* _data);
    struct TextItemData {
        ScreenplayTextModelTextItem* item = nullptr;
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
