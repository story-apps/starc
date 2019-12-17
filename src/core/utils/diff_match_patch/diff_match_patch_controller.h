#pragma once

#include <QScopedPointer>


/**
 * @brief Управляющий классом сравнения данных документов
 */
class DiffMatchPatchController final
{
public:
    explicit DiffMatchPatchController(const QVector<QString>& _tags);
    ~DiffMatchPatchController();

    /**
     * @brief Сформировать патч
     */
    QString makePatch(const QString& _lhs, const QString& _rhs);

    /**
     * @brief Применить патч
     */
    QString applyPatch(const QString& _content, const QString& _patch);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};










#include <QString>
#include <QPair>

/**
 * @brief Класс со вспомогательными функциями для сравнения xml-текстов
 */
class DiffMatchPatchHelper
{
public:
    /**
     * @brief Сформировать патч между двумя простыми текстами
     */
    static QString makePatch(const QString& _text1, const QString& _text2);

    /**
     * @brief Сформировать патч между двумя xml-текстами
     */
    static QString makePatchXml(const QString& _xml1, const QString& _xml2);

    /**
     * @brief Применить патч для простого текста
     */
    static QString applyPatch(const QString& _text, const QString& _patch);

    /**
     * @brief Применить патч для xml-текста
     */
    static QString applyPatchXml(const QString& _xml, const QString& _patch);

    /**
     * @brief Изменение xml
     */
    class ChangeXml {
    public:
        /**
         * @brief Сам xml
         */
        QString xml;

        /**
         * @brief Позиция изменения
         */
        int plainPos = -1;

        /**
         * @brief Длина текста
         */
        int plainLength = -1;

        ChangeXml() = default;
        ChangeXml(const QString& _xml, const int _pos, const int _length = -1);

        /**
         * @brief Валидно ли изменение
         */
        bool isValid() const;
    };

    /**
     * @brief Определить куски xml из документов, которые затрагивает данное изменение
     * @return Пара: 1) текст, который был изменён; 2) текст замены
     */
    static QPair<ChangeXml, ChangeXml> changedXml(const QString& _xml, const QString& _patch, bool _checkXml = false);

    /**
     * @brief Определить куски xml из документов, которые затрагивает данное изменение
     * @return Вектор пар <текст, который был изменён | текст замены>
     */
    static QVector<QPair<ChangeXml, ChangeXml>> changedXmlList(const QString& _xml, const QString& _patch);

private:
    /**
     * @brief Преобразовать xml в плоский текст, заменяя тэги спецсимволами
     */
    static QString xmlToPlain(const QString& _xml);

    /**
     * @brief Преобразовать плоский текст в xml, заменяя спецсимволы на тэги
     */
    static QString plainToXml(const QString& _plain);

    /**
     * @brief Добавить тэг и его закрывающий аналог в карту соответствий
     */
    static void addTag(const QString& _tag, QHash<QString, QString>& _tagsMap, int& _index);

    /**
     * @brief Карта соответствий xml-тэгов и спецсимволов
     */
    static const QHash<QString,QString>& tagsMap();
};
