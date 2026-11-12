#include "ai_project_context.h"

#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMap>
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
    const auto hash = QCryptographicHash::hash((input.name + '\n' + input.storyTitle + '\n' + input.content).toUtf8(), QCryptographicHash::Sha256);
    if (m_documents.contains(input.id) && m_documents.value(input.id).hash == hash) { return; }
    Document document{ input.id, input.name, input.storyTitle, hash, {} };
    QString scene, sceneTitle, text, sceneNumber;
    auto flush = [&] {
        const auto body = text.trimmed();
        for (int offset = 0; offset < body.size();) {
            const auto part = body.mid(offset, kChunkSize);
            const auto key = input.id + '\n' + scene + '\n' + QString::number(offset) + '\n' + part;
            const auto id = QString::fromLatin1(QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Sha256).toHex().left(24));
            document.chunks.append({ id, input.id, scene,
                sceneTitle.isEmpty() ? input.name : input.name + " / " + sceneTitle, part, sceneNumber });
            if (offset + kChunkSize >= body.size()) { break; }
            offset += kChunkSize - 200; // Keep context around chunk boundaries.
        }
        text.clear();
    };
    if (input.content.trimmed().startsWith('<')) {
        QXmlStreamReader xml(input.content);
        struct Container { QString tag, scene, title, number; };
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
                    containers.append({ tag, scene, sceneTitle, sceneNumber });
                    scene = QUuid(uuid).toString();
                    sceneTitle.clear();
                    sceneNumber.clear();
                }
                if (tag == "number" && !scene.isEmpty()) {
                    sceneNumber = xml.attributes().value("value").toString();
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
                    sceneNumber = parent.number;
                }
                if (!elements.isEmpty()) { elements.removeLast(); }
            }
        }
        if (xml.hasError()) {
            document.chunks.clear();
            text = input.content;
            scene.clear(); sceneTitle.clear(); sceneNumber.clear();
        } else if (text.trimmed().isEmpty() && document.chunks.isEmpty()) {
            // Preserve metadata-only documents as a fallback too.
            text = input.content;
            scene.clear(); sceneTitle.clear(); sceneNumber.clear();
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
        if (word.size() >= 5) { found.insert(word.left(4) + '*'); }
        else if (word.size() >= 3 && QStringLiteral("аяуыие").contains(word.back())) {
            found.insert(word.left(word.size() - 1) + '*');
        }
    }
    return found.values();
}

void AiProjectContext::prepareIndex() { buildIndex(); }

void AiProjectContext::buildIndex()
{
    if (!m_dirty) { return; }
    m_chunks.clear(); m_postings.clear(); m_titlePostings.clear(); m_sources.clear();
    for (const auto& id : m_order) {
        for (const auto& chunk : m_documents.value(id).chunks) {
            const int index = m_chunks.size();
            m_chunks.append(chunk);
            m_sources.insert(chunk.id, chunkJson(chunk));
            for (const auto& term : terms(chunk.text)) { m_postings[term].append(index); }
            for (const auto& term : terms(chunk.title)) { m_titlePostings[term].append(index); }
        }
    }
    m_dirty = false;
}

QJsonObject AiProjectContext::chunkJson(const Chunk& chunk) const
{
    QJsonObject result{ { "source", chunk.id }, { "document", chunk.document }, { "scene", chunk.scene },
                        { "title", chunk.title }, { "text", chunk.text } };
    if (!chunk.number.isEmpty()) { result.insert("number", chunk.number); }
    return result;
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

QJsonArray AiProjectContext::stories() const
{
    QJsonArray result;
    // Match the episode's own name, not a parent project, draft or scene number.
    static const QRegularExpression before(R"((\d+)\s*(?:[-–]?\s*[яй])?\s*(?:серия|серии|эпизод)\b)",
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::UseUnicodePropertiesOption);
    static const QRegularExpression after(R"((?:серия|эпизод|episode)\s*#?\s*(\d+)\b)",
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::UseUnicodePropertiesOption);
    static const QRegularExpression special(QStringLiteral("спешл|спецвыпуск|рождеств|special|christmas"),
        QRegularExpression::CaseInsensitiveOption);
    for (const auto& id : m_order) {
        const auto& doc = m_documents[id];
        if (doc.storyTitle.isEmpty()) { continue; }
        auto match = before.match(doc.storyTitle);
        if (!match.hasMatch()) { match = after.match(doc.storyTitle); }
        const bool isSpecial = special.match(doc.storyTitle).hasMatch();
        QJsonObject item{ { "document", id }, { "title", doc.name },
                         { "chunks", doc.chunks.size() }, { "special", isSpecial } };
        if (match.hasMatch() && !isSpecial) { item.insert("episode", match.captured(1).toInt()); }
        result.append(item);
    }
    return result;
}

QJsonArray AiProjectContext::synopsisDocuments(const QString& question) const
{
    const auto text = question.toCaseFolded();
    if (!text.contains(QStringLiteral("синопсис")) && !text.contains("synops")
        && !text.contains("summar")) { return {}; }
    static const QRegularExpression range(
        QStringLiteral(R"((?:сери[йияю]|episodes?)\s*(\d+)\s*[-–—]\s*(\d+))"));
    static const QRegularExpression last(
        QStringLiteral(R"((?:последн[\p{L}]*|last)\s+(\d+|одной|двух|тр[её]х|четыр[её]х|пяти|шести|семи|восьми|девяти|десяти|двенадцати|five|eight|ten)\s+(?:сери|episodes?))"),
        QRegularExpression::UseUnicodePropertiesOption);
    // Combined/negative requests involving specials can change the requested
    // total. Let the model select explicit IDs instead of guessing that count.
    const bool mentionsSpecial = text.contains(QStringLiteral("спешл")) || text.contains(QStringLiteral("рождеств"))
        || text.contains("special") || text.contains("christmas");
    static const QRegularExpression additionalSpecial(
        QStringLiteral(R"(\b(?:и|плюс|and|plus)\s+(?:рождеств[\p{L}]*\s+)?(?:спешл|рождеств|special|christmas))"),
        QRegularExpression::UseUnicodePropertiesOption);
    if (mentionsSpecial && !additionalSpecial.match(text).hasMatch()) { return {}; }
    const auto interval = range.match(text);
    const auto tail = last.match(text);
    if (!interval.hasMatch() && !tail.hasMatch()) { return {}; }
    QMap<int, QJsonObject> episodes;
    for (const auto& value : stories()) {
        const auto story = value.toObject();
        if (!story.contains("episode")) { continue; }
        const auto number = story.value("episode").toInt();
        // Multiple seasons with repeated numbers require explicit disambiguation.
        if (episodes.contains(number)) { return {}; }
        episodes.insert(number, story);
    }
    if (episodes.isEmpty()) { return {}; }
    QJsonArray result;
    if (interval.hasMatch()) {
        const int first = interval.captured(1).toInt(), end = interval.captured(2).toInt();
        if (end < first || end - first >= 100) { return {}; }
        for (int number = first; number <= end; ++number) {
            result.append(episodes.value(number, QJsonObject{ { "episode", number },
                { "title", QStringLiteral("Episode %1").arg(number) }, { "missing", true } }));
        }
    } else {
        const QHash<QString, int> numbers{ { "одной", 1 }, { "двух", 2 }, { "трех", 3 }, { "трёх", 3 },
            { "четырех", 4 }, { "четырёх", 4 }, { "пяти", 5 }, { "шести", 6 }, { "семи", 7 },
            { "восьми", 8 }, { "девяти", 9 }, { "десяти", 10 }, { "двенадцати", 12 },
            { "five", 5 }, { "eight", 8 }, { "ten", 10 } };
        const auto countText = tail.captured(1);
        const int count = numbers.value(countText, countText.toInt());
        if (count < 1 || count > 100) { return {}; }
        auto it = episodes.cbegin();
        std::advance(it, qMax(0, episodes.size() - count));
        for (; it != episodes.cend(); ++it) { result.append(it.value()); }
    }
    // An explicitly requested special is additional to the numbered episodes.
    if (mentionsSpecial) {
        for (const auto& item : stories()) {
            if (item.toObject().value("special").toBool()) { result.append(item); }
        }
    }
    return result;
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
    // Titles help locate documents but must not drown out matching dialogue.
    for (const auto& term : terms(query)) {
        const auto positions = m_titlePostings.value(term);
        const double weight = 0.2 * std::log(1.0 + double(m_chunks.size()) / (1 + positions.size()));
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

QJsonObject AiProjectContext::read(const QString& id, int offset, int characterBudget) const
{
    const auto documentId = resolveDocumentId(id);
    if (documentId.isEmpty()) { return { { "error", "Unknown document. Use catalog to find a document ID." } }; }
    const auto& document = m_documents[documentId];
    offset = qMax(0, offset);
    QJsonArray results;
    // Synopsis passes use a character budget rather than six fixed chunks, so
    // short establishing scenes do not each cause another model round trip.
    int characters = 0;
    const int end = characterBudget > 0 ? document.chunks.size() : qMin(offset + kPageSize, document.chunks.size());
    for (int i = offset; i < end; ++i) {
        const auto chunk = chunkJson(document.chunks.at(i));
        const int size = QString::fromUtf8(QJsonDocument(chunk).toJson(QJsonDocument::Compact)).size() + 1;
        if (characterBudget > 0 && !results.isEmpty() && characters + size > characterBudget) { break; }
        results.append(chunk);
        characters += size;
    }
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
