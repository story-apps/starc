#include "diff_match_patch_controller.h"

#include "diff_match_patch.h"


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
    // Используем зарезервированную секцию кодов юникода U+E000–U+F8FF, для генерации служебних кодов
    //
    uint characterIndex = 0xE000;

    //
    // Добавить заданный тэг в карту служебных символов
    //
    auto addTag = [this, &characterIndex] (const QString& _tag) {
        tagsMap.insert("<" + _tag + ">", QChar(characterIndex++));
        tagsMap.insert("</" + _tag + ">", QChar(characterIndex++));
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

QString DiffMatchPatchController::Implementation::makePatchPlain(const QString& _plain1, const QString& _plain2)
{
    diff_match_patch dmp;
    return dmp.patch_toText(dmp.patch_make(_plain1, _plain2));
}

QString DiffMatchPatchController::Implementation::makePatchXml(const QString& _xml1, const QString& _xml2)
{
    return
            plainToXml(
                makePatchPlain(
                    xmlToPlain(_xml1),
                    xmlToPlain(_xml2)
                    )
                );
}

QString DiffMatchPatchController::Implementation::applyPatchPlain(const QString& _plain, const QString& _patch)
{
    diff_match_patch dmp;
    QList<Patch> patches = dmp.patch_fromText(_patch);
    return dmp.patch_apply(patches, _plain).first;
}

QString DiffMatchPatchController::Implementation::applyPatchXml(const QString& _xml, const QString& _patch)
{
    return
            plainToXml(
                applyPatchPlain(
                    xmlToPlain(_xml),
                    xmlToPlain(_patch)
                    )
                );
}


// ****


DiffMatchPatchController::DiffMatchPatchController(const QVector<QString>& _tags)
    : d(new Implementation(_tags))
{
}

DiffMatchPatchController::~DiffMatchPatchController() = default;

QString DiffMatchPatchController::makePatch(const QString& _lhs, const QString& _rhs)
{
    return d->makePatchXml(_lhs, _rhs);
}

QString DiffMatchPatchController::applyPatch(const QString& _content, const QString& _patch)
{
    return d->applyPatchXml(_content, _patch);
}
