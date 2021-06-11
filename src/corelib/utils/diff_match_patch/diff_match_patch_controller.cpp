#include "diff_match_patch_controller.h"

#include "diff_match_patch.h"


namespace {

/**
 * @brief Является ли строка тэгом
 */
static bool isTag(const QString& _tag)
{
    return !_tag.isEmpty() && _tag.startsWith(QLatin1String("<"))
        && _tag.endsWith(QLatin1String(">"));
}

/**
 * @brief Является ли тэг открывающим
 */
static bool isOpenTag(const QString& _tag)
{
    return isTag(_tag) && !_tag.contains(QLatin1String("/"));
}

/**
 * @brief Является ли тэг закрывающим
 */
static bool isCloseTag(const QString& _tag)
{
    return isTag(_tag) && _tag.contains(QLatin1String("/"));
}

} // namespace


class DiffMatchPatchController::Implementation
{
public:
    explicit Implementation(const QVector<QString>& _tags);

    /**
     * @brief Преобразовать xml в плоский текст, заменяя тэги спецсимволами
     */
    QString xmlToPlain(const QString& _xml);

    /**
     * @brief Преобразовать плоский текст в xml, заменяя спецсимволы на тэги
     */
    QString plainToXml(const QString& _plain);

    /**
     * @brief Сформировать патч между двумя простыми текстами
     */
    QString makePatchPlain(const QString& _plain1, const QString& _plain2);

    /**
     * @brief Сформировать патч между двумя xml-текстами
     */
    QString makePatchXml(const QString& _xml1, const QString& _xml2);

    /**
     * @brief Применить патч для простого текста
     */
    QString applyPatchPlain(const QString& _plain, const QString& _patch);

    /**
     * @brief Применить патч для xml-текста
     */
    QString applyPatchXml(const QString& _xml, const QString& _patch);


    QHash<QString, QChar> tagsMap;
};

DiffMatchPatchController::Implementation::Implementation(const QVector<QString>& _tags)
{
    //
    // Используем зарезервированную секцию кодов юникода U+E000–U+F8FF,
    // для генерации служебных символов для карты тэгов
    //
    uint characterIndex = 0xE000;
    auto nextCharacter = [&characterIndex] { return QChar(characterIndex++); };

    //
    // Добавить заданный тэг в карту служебных символов
    //
    auto addTag = [this, nextCharacter](const QString& _tag) {
        tagsMap.insert("<" + _tag + ">", nextCharacter());
        tagsMap.insert("</" + _tag + ">", nextCharacter());
    };
    for (auto tag : _tags) {
        addTag(tag);
    }
}

QString DiffMatchPatchController::Implementation::xmlToPlain(const QString& _xml)
{
    QString plain = _xml;
    for (auto iter = tagsMap.begin(); iter != tagsMap.end(); ++iter) {
        plain.replace(iter.key(), iter.value());
    }
    return plain;
}

QString DiffMatchPatchController::Implementation::plainToXml(const QString& _plain)
{
    QString xml = _plain;
    for (auto iter = tagsMap.begin(); iter != tagsMap.end(); ++iter) {
        xml.replace(iter.value(), iter.key());
    }
    return xml;
}

QString DiffMatchPatchController::Implementation::makePatchPlain(const QString& _plain1,
                                                                 const QString& _plain2)
{
    diff_match_patch dmp;
    return dmp.patch_toText(dmp.patch_make(_plain1, _plain2));
}

QString DiffMatchPatchController::Implementation::makePatchXml(const QString& _xml1,
                                                               const QString& _xml2)
{
    return plainToXml(makePatchPlain(xmlToPlain(_xml1), xmlToPlain(_xml2)));
}

QString DiffMatchPatchController::Implementation::applyPatchPlain(const QString& _plain,
                                                                  const QString& _patch)
{
    diff_match_patch dmp;
    QList<Patch> patches = dmp.patch_fromText(_patch);
    return dmp.patch_apply(patches, _plain).first;
}

QString DiffMatchPatchController::Implementation::applyPatchXml(const QString& _xml,
                                                                const QString& _patch)
{
    return plainToXml(applyPatchPlain(xmlToPlain(_xml), xmlToPlain(_patch)));
}


// ****


DiffMatchPatchController::DiffMatchPatchController(const QVector<QString>& _tags)
    : d(new Implementation(_tags))
{
}

DiffMatchPatchController::~DiffMatchPatchController() = default;

QByteArray DiffMatchPatchController::makePatch(const QString& _lhs, const QString& _rhs) const
{
    return d->makePatchXml(_lhs, _rhs).toUtf8();
}

QByteArray DiffMatchPatchController::applyPatch(const QByteArray& _content,
                                                const QByteArray& _patch) const
{
    return d->applyPatchXml(_content, _patch).toUtf8();
}

QPair<DiffMatchPatchController::Change, DiffMatchPatchController::Change> DiffMatchPatchController::
    changedXml(const QString& _xml, const QString& _patch) const
{
    //
    // Применим патчи
    //
    const QString oldXmlPlain = d->xmlToPlain(_xml);
    const QString newXml = d->applyPatchXml(_xml, _patch);
    const QString newXmlPlain = d->xmlToPlain(newXml);

    //
    // Формируем новый патч, он будет содержать корректные данные,
    // для текста сценария текущего пользователя
    //
    const QString newPatch = makePatch(oldXmlPlain, newXmlPlain);
    if (newPatch.isEmpty()) {
        return {};
    }

    //
    // Разберём патчи на список
    //
    diff_match_patch dmp;
    const QList<Patch> patches = dmp.patch_fromText(newPatch);

    //
    // Рассчитаем метрики для формирования xml для обновления
    //
    int oldStartPos = -1;
    int oldEndPos = -1;
    int oldDistance = 0;
    int newStartPos = -1;
    int newEndPos = -1;
    for (const Patch& patch : patches) {
        //
        // ... для старого
        //
        if (oldStartPos == -1 || patch.start1 < oldStartPos) {
            oldStartPos = patch.start1;
        }
        if (oldEndPos == -1 || oldEndPos < (patch.start1 + patch.length1 - oldDistance)) {
            oldEndPos = patch.start1 + patch.length1 - oldDistance;
        }
        oldDistance += patch.length2 - patch.length1;
        //
        // ... для нового
        //
        if (newStartPos == -1 || patch.start2 < newStartPos) {
            newStartPos = patch.start2;
        }
        if (newEndPos == -1 || newEndPos < (patch.start2 + patch.length2)) {
            newEndPos = patch.start2 + patch.length2;
        }
    }
    //
    // Для случая, когда текста остаётся ровно столько же, сколько и было
    //
    if (oldDistance == 0) {
        oldEndPos = newEndPos;
    }
    //
    // Отнимает один символ, т.к. в патче указан индекс символа начиная с 1
    //
    oldEndPos -= 1;
    newEndPos -= 1;


    //
    // Определим кусок xml из текущего документа для обновления
    //
    int oldStartPosForXmlPlain = oldStartPos;
    for (; oldStartPosForXmlPlain > 0; --oldStartPosForXmlPlain) {
        //
        // Идём до открывающего тега
        //
        if (isOpenTag(d->tagsMap.key(oldXmlPlain.at(oldStartPosForXmlPlain)))) {
            break;
        }
    }
    int oldEndPosForXml = oldEndPos;
    for (; oldEndPosForXml < oldXmlPlain.length(); ++oldEndPosForXml) {
        //
        // Идём до закрывающего тэга, он находится в конце строки
        //
        if (isCloseTag(d->tagsMap.key(oldXmlPlain.at(oldEndPosForXml)))) {
            ++oldEndPosForXml;
            break;
        }
    }
    const QString oldXmlForUpdate
        = oldXmlPlain.mid(oldStartPosForXmlPlain, oldEndPosForXml - oldStartPosForXmlPlain);


    //
    // Определим кусок из нового документа для обновления
    //
    int newStartPosForXmlPlain = newStartPos;
    for (; newStartPosForXmlPlain > 0; --newStartPosForXmlPlain) {
        //
        // Идём до открывающего тега
        //
        if (isOpenTag(d->tagsMap.key(newXmlPlain.at(newStartPosForXmlPlain)))) {
            break;
        }
    }
    int newEndPosForXml = newEndPos;
    for (; newEndPosForXml < newXmlPlain.length(); ++newEndPosForXml) {
        //
        // Идём до закрывающего тэга, он находится в конце строки
        //
        if (isCloseTag(d->tagsMap.key(newXmlPlain.at(newEndPosForXml)))) {
            ++newEndPosForXml;
            break;
        }
    }
    const QString newXmlForUpdate
        = newXmlPlain.mid(newStartPosForXmlPlain, newEndPosForXml - newStartPosForXmlPlain);


    return { { d->plainToXml(oldXmlForUpdate),
               d->plainToXml(oldXmlPlain.left(oldStartPosForXmlPlain)).length() },
             { d->plainToXml(newXmlForUpdate),
               d->plainToXml(newXmlPlain.left(newStartPosForXmlPlain)).length() } };
}
