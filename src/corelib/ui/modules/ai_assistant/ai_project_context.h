#pragma once

#include <corelib_global.h>
#include <QHash>
#include <QJsonObject>
#include <QJsonArray>
#include <QVector>
#include <QString>

namespace Ui {

struct AiContextDocument {
    QString id;
    QString name;
    QString content;
    bool available = true;
    // Set by the project tree, never inferred from draft numbers or scene headings.
    QString storyTitle = {};
};

// A lossless local corpus with bounded, paginated views for the model. Extracts are
// selected from original text, rather than replacing the corpus with lossy summaries.
class CORE_LIBRARY_EXPORT AiProjectContext
{
public:
    void resetDocuments(const QVector<AiContextDocument>& _documents);
    void updateDocument(const AiContextDocument& _document);
    void prepareIndex();
    QJsonObject catalog(int _offset = 0) const;
    QJsonObject search(const QString& _query, int _offset = 0, const QString& _document = {});
    QJsonObject read(const QString& _document, int _offset = 0, int _characterBudget = 0) const;
    QJsonArray stories() const;
    QJsonArray synopsisDocuments(const QString& _question) const;
    QJsonObject source(const QString& _id) const;
    int documentCount() const;
    int chunkCount() const;

private:
    struct Chunk {
        QString id, document, scene, title, text, number;
    };
    struct Document {
        QString id, name, storyTitle;
        QByteArray hash;
        QVector<Chunk> chunks;
    };
    QString resolveDocumentId(const QString& _id) const;
    void buildIndex();
    QJsonObject chunkJson(const Chunk& _chunk) const;
    static QStringList terms(const QString& _text);
    QVector<QString> m_order;
    QHash<QString, Document> m_documents;
    QVector<Chunk> m_chunks;
    QHash<QString, QVector<int>> m_postings;
    QHash<QString, QVector<int>> m_titlePostings;
    QHash<QString, QJsonObject> m_sources;
    bool m_dirty = true;
};

} // namespace Ui
