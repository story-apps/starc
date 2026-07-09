#pragma once

#include <QObject>

#include <corelib_global.h>

class QTextBlock;
class QTextCharFormat;
class QTextDocument;
class SpellChecker;


/**
 * @brief Класс подсвечивающий слова не прошедшие проверку правописания
 */
class CORE_LIBRARY_EXPORT SpellCheckHighlighter : public QObject
{
    Q_OBJECT

    friend class SpellCheckHighlighterPrivate;

public:
    explicit SpellCheckHighlighter(QTextDocument* _parent, const SpellChecker& _checker);
    ~SpellCheckHighlighter() override;

    /**
     * @brief Установить документ для подсветки орфографии
     */
    void setDocument(QTextDocument* _document);
    QTextDocument* document() const;

    /**
     * @brief Перепроверить орфографию в заданном диапазоне абзацев
     */
    void rehighlightBlock(const QTextBlock& _block);
    void rehighlightBlocks(const QTextBlock& _firstBlock, const QTextBlock& _lastBlock);

    /**
     * @brief Включить/выключить проверку орфографии
     */
    void setUseSpellChecker(bool _use);
    bool useSpellChecker() const;

    /**
     * @brief Задать позицию курсора
     */
    void setCursorPosition(int position);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
