#include "ai_project_context.h"

#include <QCryptographicHash>
#include <QJsonArray>
#include <QRegularExpression>
#include <QSet>
#include <QUuid>
#include <QXmlStreamReader>
#include <algorithm>
#include <cmath>

namespace Ui {
namespace {
constexpr int kChunkSize = 2800;
constexpr int kPageSize = 6;
QString decodeText(QString text)
{
    // Text-model CDATA is HTML escaped by the writer. Decode once, without
    // interpreting document text as HTML or loading any referenced resources.
    return text.replace("&lt;", "<").replace("&gt;", ">").replace("&quot;", "\"")
        .replace("&#39;", "'").replace("&apos;", "'").replace("&amp;", "&");
}
}

void AiProjectContext::resetDocuments(const QVector<AiContextDocument>& documents)
{
    m_order.clear();
    QSet<QString> keep;
    for (const auto& document : documents) {
        if (!document.available || keep.contains(document.id)) { continue; }
        keep.insert(document.id);
        m_order.append(document.id);
    }
    for (auto it = m_documents.begin(); it != m_documents.end();) {
        if (!keep.contains(it.key())) { it = m_documents.erase(it); m_dirty = true; }
        else { ++it; }
    }
}

void AiProjectContext::updateDocument(const AiContextDocument& input)
{
    const auto hash = QCryptographicHash::hash((input.name + '\n' + input.content).toUtf8(), QCryptographicHash::Sha256);
    if (m_documents.contains(input.id) && m_documents.value(input.id).hash == hash) { return; }
    Document document{ input.id, input.name, hash, {} };
    QString scene, sceneTitle, text;
    auto flush = [&] {
        const auto body = text.trimmed();
        for (int offset = 0; offset < body.size();) {
            const auto part = body.mid(offset, kChunkSize);
            const auto key = input.id + '\n' + scene + '\n' + QString::number(offset) + '\n' + part;
            const auto id = QString::fromLatin1(QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Sha256).toHex().left(24));
            document.chunks.append({ id, input.id, scene,
                sceneTitle.isEmpty() ? input.name : input.name + " / " + sceneTitle, part });
            if (offset + kChunkSize >= body.size()) { break; }
            offset += kChunkSize - 200; // Keep context around chunk boundaries.
        }
        text.clear();
    };
    if (input.content.trimmed().startsWith('<')) {
        QXmlStreamReader xml(input.content);
        struct Container { QString tag, scene, title; };
        QVector<Container> containers;
        QStringList elements;
        while (!xml.atEnd()) {
            xml.readNext();
            if (xml.isStartElement()) {
                const auto tag = xml.name().toString();
                elements.append(tag);
                const auto uuid = xml.attributes().value("uuid").toString();
                // Scene/chapter groups retain their stable model UUID for navigation.
                if (!uuid.isEmpty() && (tag == "scene" || tag.startsWith("chapter")
                                       || tag == "beat" || tag == "page" || tag == "panel")) {
                    flush();
                    containers.append({ tag, scene, sceneTitle });
                    scene = QUuid(uuid).toString();
                    sceneTitle.clear();
                }
                for (const auto& attribute : xml.attributes()) {
                    const auto name = attribute.name();
                    if (name == "value" || name == "name" || name == "description"
                        || name == "title" || name == "label" || name == "text") {
                        text += tag + '.' + name.toString() + ": " + attribute.value().toString() + '\n';
                    }
                }
            } else if (xml.isCharacters() && !xml.isWhitespace()) {
                // Preserve field names in information documents (age, biography,
                // story lines, etc.), as well as text in ordinary paragraphs.
                const auto value = (xml.isCDATA() ? decodeText(xml.text().toString())
                                                  : xml.text().toString()).trimmed();
                if (!value.isEmpty()) {
                    if (sceneTitle.isEmpty() && !scene.isEmpty()) { sceneTitle = value.left(140); }
                    const auto tag = elements.isEmpty() ? QString() : elements.last();
                    text += (tag == "value" || tag == "v" || tag.isEmpty() ? QString() : tag + ": ") + value + '\n';
                }
            } else if (xml.isEndElement()) {
                if (!containers.isEmpty() && containers.last().tag == xml.name()) {
                    flush();
                    const auto parent = containers.takeLast();
                    scene = parent.scene;
                    sceneTitle = parent.title;
                }
                if (!elements.isEmpty()) { elements.removeLast(); }
            }
        }
        if (xml.hasError()) {
            document.chunks.clear();
            text = input.content;
            scene.clear(); sceneTitle.clear();
        } else if (text.trimmed().isEmpty() && document.chunks.isEmpty()) {
            // Preserve metadata-only documents as a fallback too.
            text = input.content;
            scene.clear(); sceneTitle.clear();
        }
    } else { text = input.content; }
    flush();
    m_documents.insert(input.id, document);
    m_dirty = true;
}

QStringList AiProjectContext::terms(const QString& text)
{
    static const QRegularExpression words("[\\p{L}\\p{N}]+", QRegularExpression::UseUnicodePropertiesOption);
    static const QSet<QString> stop{ "the", "and", "for", "with", "what", "this", "that", "from", "about",
        "как", "что", "это", "или", "для", "его", "она", "они", "кто", "где", "все", "про", "есть", "мне", "расскажи", "проекте" };
    QSet<QString> found;
    auto it = words.globalMatch(text.toCaseFolded().replace(QChar(0x451), QChar(0x435)));
    while (it.hasNext()) {
        const auto word = it.next().captured();
        if ((word.size() < 2 && !word.at(0).isDigit()) || stop.contains(word)) { continue; }
        found.insert(word);
        // Prefix terms provide inexpensive inflection recall for Russian and other
        // languages; exact terms are retained and independently ranked.
        if (word.size() > 5) { found.insert(word.left(5) + '*'); }
    }
    return found.values();
}

void AiProjectContext::prepareIndex() { buildIndex(); }

void AiProjectContext::buildIndex()
{
    if (!m_dirty) { return; }
    m_chunks.clear(); m_postings.clear(); m_sources.clear();
    for (const auto& id : m_order) {
        for (const auto& chunk : m_documents.value(id).chunks) {
            const int index = m_chunks.size();
            m_chunks.append(chunk);
            m_sources.insert(chunk.id, chunkJson(chunk));
            for (const auto& term : terms(chunk.title + '\n' + chunk.text)) { m_postings[term].append(index); }
        }
    }
    m_dirty = false;
}

QJsonObject AiProjectContext::chunkJson(const Chunk& chunk) const
{
    return { { "source", chunk.id }, { "document", chunk.document }, { "scene", chunk.scene },
             { "title", chunk.title }, { "text", chunk.text } };
}

QJsonObject AiProjectContext::catalog(int offset) const
{
    offset = qMax(0, offset);
    QJsonArray documents;
    for (int i = offset; i < qMin(offset + 40, m_order.size()); ++i) {
        const auto& document = m_documents[m_order.at(i)];
        documents.append(QJsonObject{ { "document", document.id }, { "title", document.name.left(220) },
                                      { "chunks", document.chunks.size() } });
    }
    return { { "documents", documents }, { "total_documents", m_order.size() },
             { "next_offset", offset + documents.size() < m_order.size() ? offset + documents.size() : -1 } };
}

QString AiProjectContext::resolveDocumentId(const QString& id) const
{
    if (m_documents.contains(id)) { return id; }
    // Recover a missing outer brace or case differences, but never fuzzy-match a
    // different UUID or silently substitute the current document for an unknown ID.
    auto value = id.trimmed();
    if (value.startsWith('{')) { value.remove(0, 1); }
    if (value.endsWith('}')) { value.chop(1); }
    const auto uuid = QUuid(value);
    if (!uuid.isNull() && m_documents.contains(uuid.toString())) { return uuid.toString(); }
    return {};
}

QJsonObject AiProjectContext::search(const QString& query, int offset, const QString& document)
{
    buildIndex();
    const auto documentId = resolveDocumentId(document);
    if (!document.isEmpty() && documentId.isEmpty()) {
        return { { "error", "Unknown document. Copy a document ID from the catalog." } };
    }
    QHash<int, double> scores;
    for (const auto& term : terms(query)) {
        const auto positions = m_postings.value(term);
        const double weight = std::log(1.0 + double(m_chunks.size()) / (1 + positions.size()))
            * (term.endsWith('*') ? 0.45 : 1.0);
        for (const auto pos : positions) {
            if (!document.isEmpty() && m_chunks.at(pos).document != documentId) { continue; }
            scores[pos] += weight;
        }
    }
    auto indices = scores.keys();
    std::sort(indices.begin(), indices.end(), [&](int lhs, int rhs) {
        return scores.value(lhs) == scores.value(rhs) ? lhs < rhs : scores.value(lhs) > scores.value(rhs);
    });
    QJsonArray results;
    offset = qMax(0, offset);
    for (int i = offset; i < qMin(offset + kPageSize, indices.size()); ++i) { results.append(chunkJson(m_chunks.at(indices.at(i)))); }
    return { { "results", results }, { "total_matches", indices.size() },
             { "next_offset", offset + results.size() < indices.size() ? offset + results.size() : -1 } };
}

QJsonObject AiProjectContext::read(const QString& id, int offset) const
{
    const auto documentId = resolveDocumentId(id);
    if (documentId.isEmpty()) { return { { "error", "Unknown document. Use catalog to find a document ID." } }; }
    const auto& document = m_documents[documentId];
    offset = qMax(0, offset);
    QJsonArray results;
    for (int i = offset; i < qMin(offset + kPageSize, document.chunks.size()); ++i) { results.append(chunkJson(document.chunks.at(i))); }
    return { { "results", results }, { "total_chunks", document.chunks.size() },
             { "next_offset", offset + results.size() < document.chunks.size() ? offset + results.size() : -1 } };
}

QJsonObject AiProjectContext::source(const QString& id) const { return m_sources.value(id); }
int AiProjectContext::documentCount() const { return m_order.size(); }
int AiProjectContext::chunkCount() const
{
    int total = 0;
    for (const auto& document : m_documents) { total += document.chunks.size(); }
    return total;
}

} // namespace Ui
