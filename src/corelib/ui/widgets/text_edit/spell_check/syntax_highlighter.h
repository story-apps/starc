#pragma once

#include <corelib_global.h>

#include <QtCore/qglobal.h>
#include <QObject>
#include <QTextObject>
#include <QTextDocument>
#include <QTextCursor>
#include <QPointer>

class QTextDocument;
class SyntaxHighlighterPrivate;
class QTextCharFormat;
class QFont;
class QColor;
class QTextBlockUserData;


class CORE_LIBRARY_EXPORT SyntaxHighlighter : public QObject
{
    Q_OBJECT

    friend class SyntaxHighlighterPrivate;

public:
    explicit SyntaxHighlighter(QObject *parent);
    explicit SyntaxHighlighter(QTextDocument *parent);
    virtual ~SyntaxHighlighter();

    void setDocument(QTextDocument *doc);
    QTextDocument *document() const;

public Q_SLOTS:
    void rehighlight();
    void rehighlightBlock(const QTextBlock &block);

    bool isChanged() const;
    void setChanged(bool _changed);

protected:
    virtual void highlightBlock(const QString &text) = 0;

    void setFormat(int start, int count, const QTextCharFormat &format);
    void setFormat(int start, int count, const QColor &color);
    void setFormat(int start, int count, const QFont &font);
    QTextCharFormat format(int pos) const;

    int previousBlockState() const;
    int currentBlockState() const;
    void setCurrentBlockState(int newState);

    void setCurrentBlockUserData(QTextBlockUserData *data);
    QTextBlockUserData *currentBlockUserData() const;

    QTextBlock currentBlock() const;

private:
    Q_DISABLE_COPY(SyntaxHighlighter)
    Q_PRIVATE_SLOT(d, void _q_reformatBlocks(int from, int charsRemoved, int charsAdded))
    Q_PRIVATE_SLOT(d, void _q_delayedRehighlight())

    SyntaxHighlighterPrivate* d;
};


class SyntaxHighlighterPrivate : public QObject
{
    Q_OBJECT

    friend class SyntaxHighlighter;

public:
    inline SyntaxHighlighterPrivate(QObject* _parent, SyntaxHighlighter* _q)
        : QObject(_parent), q(_q), rehighlightPending(false), inReformatBlocks(false)
    {}

    SyntaxHighlighter* q;

    QPointer<QTextDocument> doc;
    void reformatBlocks(int from, int charsRemoved, int charsAdded);
    void reformatBlock(const QTextBlock &block);

    inline void rehighlight(QTextCursor &cursor, QTextCursor::MoveOperation operation) {
        inReformatBlocks = true;
        cursor.beginEditBlock();
        int from = cursor.position();
        cursor.movePosition(operation);
        reformatBlocks(from, 0, cursor.position() - from);
        cursor.endEditBlock();
        inReformatBlocks = false;
    }

    void applyFormatChanges();
    QVector<QTextCharFormat> formatChanges;
    QTextBlock currentBlock;
    bool rehighlightPending;
    bool inReformatBlocks;

public slots:
    void _q_reformatBlocks(int from, int charsRemoved, int charsAdded);

    inline void _q_delayedRehighlight() {
        if (!rehighlightPending)
            return;
        rehighlightPending = false;
        q->rehighlight();
    }

//
// Доработки
//
public:
    /**
     * @brief Изменялся ли документ с момента последней проверки
     */
    bool isDocumentChangedFormLastEdit = false;
};
