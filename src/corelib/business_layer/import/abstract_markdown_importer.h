#pragma once

#include <QScopedPointer>
#include <QtContainerFwd>

#include <corelib_global.h>

class QRegularExpression;
class QLatin1String;
class QXmlStreamWriter;

namespace BusinessLayer {

/**
 * @brief Абстрактный импортер из markdown и fountain
 */
class CORE_LIBRARY_EXPORT AbstractMarkdownImporter
{
public:
    AbstractMarkdownImporter(const QMap<QString, QLatin1String>& _selectionTypes,
                             const QRegularExpression& _selectionTypeChecker,
                             const QString& _capturedGroup);
    virtual ~AbstractMarkdownImporter();

protected:
    /**
     * @brief Сдвинуть расположение типов выделения текста, которые уже собраны
     */
    void movePreviousTypes(int _position, int _offset) const;

    /**
     * @brief Собрать типы выделения текста
     */
    void collectSelectionTypes(QString& _paragraphText) const;

    /**
     * @brief Записать информацию о типах выделения текста
     */
    void writeSelectionTypes(QXmlStreamWriter& _writer) const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
