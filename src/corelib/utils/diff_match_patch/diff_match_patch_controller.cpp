#include "diff_match_patch_controller.h"

#include "diff_match_patch.h"

#include <utils/shugar.h>


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
    for (const auto& tag : _tags) {
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
    const QString newXmlPlain = d->xmlToPlain(d->applyPatchXml(_xml, _patch));

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


    return { { d->plainToXml(oldXmlForUpdate).toUtf8(),
               static_cast<int>(d->plainToXml(oldXmlPlain.left(oldStartPosForXmlPlain)).length()) },
             { d->plainToXml(newXmlForUpdate).toUtf8(),
               static_cast<int>(
                   d->plainToXml(newXmlPlain.left(newStartPosForXmlPlain)).length()) } };
}

QVector<QPair<DiffMatchPatchController::Change, DiffMatchPatchController::Change>>
DiffMatchPatchController::changedXmlList(const QString& _xml, const QString& _patch) const
{
    //
    // Применим патчи
    //
    const QString oldXmlPlain = d->xmlToPlain(_xml);
    const QString newXmlPlain = d->xmlToPlain(d->applyPatchXml(_xml, _patch));

    //
    // Формируем новый патч, он будет содержать корректные данные,
    // для текста сценария текущего пользователя
    //
    const QString newPatch = makePatch(oldXmlPlain, newXmlPlain);
    if (newPatch.isEmpty()) {
        return {};
    }

    QVector<QPair<Change, Change>> result;

    //
    // Разберём патчи на список
    //
    diff_match_patch dmp;
    QList<Patch> patches = dmp.patch_fromText(newPatch);

    //
    // Рассчитаем метрики для формирования xml для обновления
    //
    int patchesDelta = 0;
    for (const Patch& patch : patches) {
        int oldStartPos = patch.start1 - patchesDelta;
        int oldEndPos = patch.start1 + patch.length1 - patchesDelta;
        int oldDistance = patch.length2 - patch.length1;
        int newStartPos = patch.start2;
        int newEndPos = patch.start2 + patch.length2;
        for (const auto& diff : patch.diffs) {
            if (diff.operation != EQUAL) {
                break;
            }

            oldStartPos += diff.text.length();
            newStartPos += diff.text.length();
        }
        for (const auto& diff : reversed(patch.diffs)) {
            if (diff.operation != EQUAL) {
                break;
            }

            oldEndPos -= diff.text.length();
            newEndPos -= diff.text.length();
        }
        patchesDelta += patch.length2 - patch.length1;

        //
        // Отнимаем один символ, т.к. в патче указан индекс символа начиная с 1
        //
        oldStartPos = std::max(0, oldStartPos - 1);
        oldEndPos = std::max(0, oldEndPos - 1);
        newStartPos = std::max(0, newStartPos - 1);
        newEndPos = std::max(0, newEndPos - 1);

        //
        // Определим кусок xml из текущего документа для обновления
        //
        int oldStartPosForXmlPlain = oldStartPos;
        int oldEndPosForXml = oldEndPos;
        if (oldStartPosForXmlPlain > 0 || oldEndPosForXml > 0) {
            for (; oldStartPosForXmlPlain > 0; --oldStartPosForXmlPlain) {
                //
                // Идём до открывающего тега
                //
                if (isOpenTag(d->tagsMap.key(oldXmlPlain.at(oldStartPosForXmlPlain)))) {
                    break;
                }
            }
            for (; oldEndPosForXml < oldXmlPlain.length(); ++oldEndPosForXml) {
                //
                // Идём до закрывающего тэга, он находится в конце строки
                //
                if (isCloseTag(d->tagsMap.key(oldXmlPlain.at(oldEndPosForXml)))) {
                    ++oldEndPosForXml;
                    break;
                }
            }
        }
        const QString oldXmlPlainForUpdate
            = oldXmlPlain.mid(oldStartPosForXmlPlain, oldEndPosForXml - oldStartPosForXmlPlain);

        //
        // Определим кусок из нового документа для обновления
        //
        int newStartPosForXmlPlain = newStartPos;
        int newEndPosForXml = newEndPos;
        if (newStartPosForXmlPlain > 0 || newEndPosForXml > 0) {
            for (; newStartPosForXmlPlain > 0; --newStartPosForXmlPlain) {
                //
                // Идём до открывающего тега
                //
                if (isOpenTag(d->tagsMap.key(newXmlPlain.at(newStartPosForXmlPlain)))) {
                    break;
                }
            }
            for (; newEndPosForXml < newXmlPlain.length(); ++newEndPosForXml) {
                //
                // Идём до закрывающего тэга, он находится в конце строки
                //
                if (isCloseTag(d->tagsMap.key(newXmlPlain.at(newEndPosForXml)))) {
                    ++newEndPosForXml;
                    break;
                }
            }
        }
        const QString newXmlPlainForUpdate
            = newXmlPlain.mid(newStartPosForXmlPlain, newEndPosForXml - newStartPosForXmlPlain);

        QPair<Change, Change> change = {
            { d->plainToXml(oldXmlPlainForUpdate).toUtf8(),
              static_cast<int>(d->plainToXml(oldXmlPlain.left(oldStartPosForXmlPlain)).length()) },
            { d->plainToXml(newXmlPlainForUpdate).toUtf8(),
              static_cast<int>(d->plainToXml(newXmlPlain.left(newStartPosForXmlPlain)).length()) }
        };
        //
        // Если текст нового изменения такой же как и у предыдущего
        //
        if (!result.isEmpty() && change.first.xml == result.constLast().first.xml
            && change.second.xml == result.constLast().second.xml) {
            //
            // ... если они начинаются в одном месте
            //
            if (change.first.from == result.constLast().first.from
                && change.second.from == result.constLast().second.from) {
                //
                // ... то пропускаем такое изменение, т.к. это дубликат
                //
                qDebug("skip similar");
            }
            //
            // ... а если в разных местах
            //
            else {
                //
                // ... то сохраняем, как самодостаточное изменение
                //
                result.append(change);
                qDebug("append similar");
            }
        }
        //
        // Если текст нового изменения содержит в себе текст предыдущего изменения
        //
        else if (!result.isEmpty() && change.first.xml.contains(result.constLast().first.xml)
                 && change.first.from <= result.constLast().first.from
                 && change.second.xml.contains(result.constLast().second.xml)
                 && change.second.from <= result.constLast().second.from) {
            //
            // ... заменяем старое изменение новым
            //
            result.last() = change;
            qDebug("replace previous");
        }
        //
        // Если текст нового изменения входит в текст предыдущего изменения
        //
        else if (!result.isEmpty() && result.constLast().first.xml.contains(change.first.xml)
                 && result.constLast().first.from <= change.first.from
                 && result.constLast().second.xml.contains(change.second.xml)
                 && result.constLast().second.from <= change.second.from) {
            //
            // ... то пропускаем такое изменение, т.к. оно по-сути уже есть в списке результатов
            //
            qDebug("skip as contained in previous");
        }
        //
        // Во всех остальны случаях
        //
        else {
            //
            // ... добавляем изменение в список результатов
            //
            result.append(change);
            qDebug("append new");
        }

        qDebug("\n\n\n");
    }

    return result;
}

int DiffMatchPatchController::changeEndPosition(const QString& _before, const QString& _after) const
{
    //
    // Обработаем крайние случаи, если один из текстов пуст
    //
    if (_before.isEmpty()) {
        return _after.length() - 1;
    } else if (_after.isEmpty()) {
        return 0;
    }

    //
    // А если оба текста не пусты определим позицию по дифу
    //
    diff_match_patch dmp;
    const auto patches = dmp.patch_make(_before, _after);
    if (patches.isEmpty()) {
        return _after.length();
    }

    const auto& patch = patches.constLast();
    if (patch.diffs.isEmpty()) {
        return _after.length();
    }

    int position = patch.start2;
    for (const auto& diff : std::as_const(patch.diffs)) {
        switch (diff.operation) {
        case EQUAL: {
            if (diff == patch.diffs.constLast()) {
                break;
            }
            Q_FALLTHROUGH();
        }
        case INSERT: {
            position += diff.text.length();
            break;
        }

        default: {
            break;
        }
        }
    }
    return position;
}
