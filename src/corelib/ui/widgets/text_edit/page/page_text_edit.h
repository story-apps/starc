#pragma once

#include <QtGui/qpagesize.h>
#include <QtGui/qtextcursor.h>
#include <QtGui/qtextdocument.h>
#include <QtGui/qtextformat.h>
#include <QtGui/qtextoption.h>
#include <QtWidgets/qabstractscrollarea.h>

#include <corelib_global.h>


QT_BEGIN_NAMESPACE

class ContextMenu;
class PageTextEditPrivate;
class QMenu;
class QMimeData;
class QPagedPaintDevice;
class QRegularExpression;
class QStyleSheet;
class QTextDocument;

class CORE_LIBRARY_EXPORT PageTextEdit : public QAbstractScrollArea
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(PageTextEdit)
    Q_PROPERTY(AutoFormatting autoFormatting READ autoFormatting WRITE setAutoFormatting)
    Q_PROPERTY(bool tabChangesFocus READ tabChangesFocus WRITE setTabChangesFocus)
    Q_PROPERTY(QString documentTitle READ documentTitle WRITE setDocumentTitle)
    Q_PROPERTY(bool undoRedoEnabled READ isUndoRedoEnabled WRITE setUndoRedoEnabled)
    Q_PROPERTY(LineWrapMode lineWrapMode READ lineWrapMode WRITE setLineWrapMode)
    QDOC_PROPERTY(QTextOption::WrapMode wordWrapMode READ wordWrapMode WRITE setWordWrapMode)
    Q_PROPERTY(int lineWrapColumnOrWidth READ lineWrapColumnOrWidth WRITE setLineWrapColumnOrWidth)
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly)
#if QT_CONFIG(textmarkdownreader) && QT_CONFIG(textmarkdownwriter)
    Q_PROPERTY(QString markdown READ toMarkdown WRITE setMarkdown NOTIFY textChanged)
#endif
#ifndef QT_NO_TEXTHTMLPARSER
    Q_PROPERTY(QString html READ toHtml WRITE setHtml NOTIFY textChanged USER true)
#endif
    Q_PROPERTY(QString plainText READ toPlainText WRITE setPlainText DESIGNABLE false)
    Q_PROPERTY(bool overwriteMode READ overwriteMode WRITE setOverwriteMode)
#if QT_DEPRECATED_SINCE(5, 10)
    Q_PROPERTY(int tabStopWidth READ tabStopWidth WRITE setTabStopWidth)
#endif
    Q_PROPERTY(qreal tabStopDistance READ tabStopDistance WRITE setTabStopDistance)
    Q_PROPERTY(bool acceptRichText READ acceptRichText WRITE setAcceptRichText)
    Q_PROPERTY(int cursorWidth READ cursorWidth WRITE setCursorWidth)
    Q_PROPERTY(Qt::TextInteractionFlags textInteractionFlags READ textInteractionFlags WRITE
                   setTextInteractionFlags)
    Q_PROPERTY(QTextDocument* document READ document WRITE setDocument DESIGNABLE false)
    Q_PROPERTY(QString placeholderText READ placeholderText WRITE setPlaceholderText)
public:
    enum LineWrapMode { NoWrap, WidgetWidth, FixedPixelWidth, FixedColumnWidth };
    Q_ENUM(LineWrapMode)

    enum AutoFormattingFlag { AutoNone = 0, AutoBulletList = 0x00000001, AutoAll = 0xffffffff };

    Q_DECLARE_FLAGS(AutoFormatting, AutoFormattingFlag)
    Q_FLAG(AutoFormatting)

    /**
     * @brief Расширим список параметров форматов блока
     */
    enum TextBlockProperty {
        //
        // Запрет на позиционирование курсора в данном блоке [bool]
        //
        PropertyDontShowCursor = QTextFormat::UserProperty + 10
    };

    explicit PageTextEdit(QWidget* parent = nullptr);
    explicit PageTextEdit(const QString& text, QWidget* parent = nullptr);
    ~PageTextEdit() override;

    void setDocument(QTextDocument* document);
    QTextDocument* document() const;

    void setPlaceholderText(const QString& placeholderText);
    QString placeholderText() const;

    void setTextCursor(const QTextCursor& cursor);
    void setTextCursorForced(const QTextCursor& cursor);
    QTextCursor textCursor() const;

    bool isReadOnly() const;
    void setReadOnly(bool ro);

    void setTextInteractionFlags(Qt::TextInteractionFlags flags);
    Qt::TextInteractionFlags textInteractionFlags() const;

    qreal fontPointSize() const;
    QString fontFamily() const;
    int fontWeight() const;
    bool fontUnderline() const;
    bool fontItalic() const;
    QColor textColor() const;
    QColor textBackgroundColor() const;
    QFont currentFont() const;
    Qt::Alignment alignment() const;

    void mergeCurrentCharFormat(const QTextCharFormat& modifier);

    void setCurrentCharFormat(const QTextCharFormat& format);
    QTextCharFormat currentCharFormat() const;

    AutoFormatting autoFormatting() const;
    void setAutoFormatting(AutoFormatting features);

    bool tabChangesFocus() const;
    void setTabChangesFocus(bool b);

    inline void setDocumentTitle(const QString& title)
    {
        document()->setMetaInformation(QTextDocument::DocumentTitle, title);
    }
    inline QString documentTitle() const
    {
        return document()->metaInformation(QTextDocument::DocumentTitle);
    }

    inline bool isUndoRedoEnabled() const
    {
        return document()->isUndoRedoEnabled();
    }
    inline void setUndoRedoEnabled(bool enable)
    {
        document()->setUndoRedoEnabled(enable);
    }

    LineWrapMode lineWrapMode() const;
    void setLineWrapMode(LineWrapMode mode);

    int lineWrapColumnOrWidth() const;
    void setLineWrapColumnOrWidth(int w);

    QTextOption::WrapMode wordWrapMode() const;
    void setWordWrapMode(QTextOption::WrapMode policy);

    bool find(const QString& exp, QTextDocument::FindFlags options = QTextDocument::FindFlags());
    bool find(const QRegularExpression& exp,
              QTextDocument::FindFlags options = QTextDocument::FindFlags());

    QString toPlainText() const;
#ifndef QT_NO_TEXTHTMLPARSER
    QString toHtml() const;
#endif
#if QT_CONFIG(textmarkdownwriter)
    QString toMarkdown(QTextDocument::MarkdownFeatures features
                       = QTextDocument::MarkdownDialectGitHub) const;
#endif

    /**
     * @brief Своя реализация проверки виден ли курсор на экране
     */
    void ensureCursorVisible();
    void ensureCursorVisible(const QTextCursor& _cursor, bool _animate = true);

    Q_INVOKABLE virtual QVariant loadResource(int type, const QUrl& name);
#ifndef QT_NO_CONTEXTMENU
    QMenu* createStandardContextMenu();
    QMenu* createStandardContextMenu(const QPoint& position);
#endif

    QTextCursor cursorForPosition(const QPoint& pos) const;
    QTextCursor cursorForPositionReimpl(const QPoint& pos) const;
    QRect cursorRect(const QTextCursor& cursor) const;
    QRect cursorRect() const;

    QString anchorAt(const QPoint& pos) const;

    bool overwriteMode() const;
    void setOverwriteMode(bool overwrite);

#if QT_DEPRECATED_SINCE(5, 10)
    QT_DEPRECATED int tabStopWidth() const;
    QT_DEPRECATED void setTabStopWidth(int width);
#endif

    qreal tabStopDistance() const;
    void setTabStopDistance(qreal distance);

    int cursorWidth() const;
    void setCursorWidth(int width);

    bool acceptRichText() const;
    void setAcceptRichText(bool accept);

    struct ExtraSelection {
        QTextCursor cursor;
        QTextCharFormat format;
    };
    void setExtraSelections(const QList<ExtraSelection>& selections);
    QList<ExtraSelection> extraSelections() const;

    void moveCursor(QTextCursor::MoveOperation operation,
                    QTextCursor::MoveMode mode = QTextCursor::MoveAnchor);

    bool canPaste() const;

    void print(QPagedPaintDevice* printer) const;

    QVariant inputMethodQuery(Qt::InputMethodQuery property) const override;
    Q_INVOKABLE QVariant inputMethodQuery(Qt::InputMethodQuery query, QVariant argument) const;

public Q_SLOTS:
    void setFontPointSize(qreal s);
    void setFontFamily(const QString& fontFamily);
    void setFontWeight(int w);
    void setFontUnderline(bool b);
    void setFontItalic(bool b);
    void setTextColor(const QColor& c);
    void setTextBackgroundColor(const QColor& c);
    void setCurrentFont(const QFont& f);
    void setAlignment(Qt::Alignment a);

    void setPlainText(const QString& text);
#ifndef QT_NO_TEXTHTMLPARSER
    void setHtml(const QString& text);
#endif
#if QT_CONFIG(textmarkdownreader)
    void setMarkdown(const QString& markdown);
#endif
    void setText(const QString& text);

#ifndef QT_NO_CLIPBOARD
    void cut();
    void copy();
    void paste();
#endif

    void undo();
    void redo();

    virtual void prepareToClear();
    void clear();
    void selectAll();

    void insertPlainText(const QString& text);
#ifndef QT_NO_TEXTHTMLPARSER
    void insertHtml(const QString& text);
#endif // QT_NO_TEXTHTMLPARSER

    void append(const QString& text);

    void scrollToAnchor(const QString& name);

    void zoomIn(int range = 1);
    void zoomOut(int range = 1);

Q_SIGNALS:
    void textChanged();
    void undoAvailable(bool b);
    void redoAvailable(bool b);
    void currentCharFormatChanged(const QTextCharFormat& format);
    void copyAvailable(bool b);
    void selectionChanged();
    void cursorPositionChanged();

protected:
    virtual bool event(QEvent* e) override;
    virtual void timerEvent(QTimerEvent* e) override;
    virtual void keyPressEvent(QKeyEvent* e) override;
    virtual void keyReleaseEvent(QKeyEvent* e) override;
    virtual void resizeEvent(QResizeEvent* e) override;
    virtual void paintEvent(QPaintEvent* e) override;
    virtual void mousePressEvent(QMouseEvent* e) override;
    virtual void mouseMoveEvent(QMouseEvent* e) override;
    virtual void mouseReleaseEvent(QMouseEvent* e) override;
    virtual void mouseDoubleClickEvent(QMouseEvent* e) override;
    virtual bool focusNextPrevChild(bool next) override;
#ifndef QT_NO_CONTEXTMENU
    virtual void contextMenuEvent(QContextMenuEvent* e) override;
#endif
#if QT_CONFIG(draganddrop)
    virtual void dragEnterEvent(QDragEnterEvent* e) override;
    virtual void dragLeaveEvent(QDragLeaveEvent* e) override;
    virtual void dragMoveEvent(QDragMoveEvent* e) override;
    virtual void dropEvent(QDropEvent* e) override;
#endif
    virtual void focusInEvent(QFocusEvent* e) override;
    virtual void focusOutEvent(QFocusEvent* e) override;
    virtual void showEvent(QShowEvent*) override;
    virtual void changeEvent(QEvent* e) override;
#if QT_CONFIG(wheelevent)
    virtual void wheelEvent(QWheelEvent* e) override;
#endif

    virtual QMimeData* createMimeDataFromSelection() const;
    virtual bool canInsertFromMimeData(const QMimeData* source) const;
    virtual void insertFromMimeData(const QMimeData* source);

    virtual void inputMethodEvent(QInputMethodEvent*) override;

    PageTextEdit(PageTextEditPrivate& dd, QWidget* parent);

    virtual void scrollContentsBy(int dx, int dy) override;
    virtual void doSetTextCursor(const QTextCursor& cursor);

    void zoomInF(float range);

private:
    Q_DISABLE_COPY(PageTextEdit)
    Q_PRIVATE_SLOT(d_func(), void _q_repaintContents(const QRectF& r))
    Q_PRIVATE_SLOT(d_func(), void _q_currentCharFormatChanged(const QTextCharFormat&))
    Q_PRIVATE_SLOT(d_func(), void _q_adjustScrollbars())
    Q_PRIVATE_SLOT(d_func(), void _q_ensureVisible(const QRectF&))
    Q_PRIVATE_SLOT(d_func(), void _q_cursorPositionChanged())
#if QT_CONFIG(cursor)
    Q_PRIVATE_SLOT(d_func(), void _q_hoveredBlockWithMarkerChanged(const QTextBlock&))
#endif
    friend class PageTextEditControl;
    friend class QTextDocument;
    friend class QWidgetTextControl;

    //
    // Дополнения, необходимые для того, чтобы превратить простой QTextEdit в постраничный редактор
    //

public:
    /**
     * @brief Установить доступность выделения текста мышью
     */
    void setTextSelectionEnabled(bool _enabled);

    /**
     * @brief Установить формат страницы
     */
    void setPageFormat(QPageSize::PageSizeId _pageFormat);

    /**
     * @brief Настроить поля страницы
     */
    void setPageMarginsMm(const QMarginsF& _margins);
    void setPageMarginsPx(const QMarginsF& _margins);

    /**
     * @brief Задать отступ между страницами, пикселей
     */
    void setPageSpacing(qreal _spacing);

    /**
     * @brief Режим отображения текста
     */
    bool usePageMode() const;
    void setUsePageMode(bool _use);

    /**
     * @brief Получить номер страницы курсора
     */
    int cursorPage(const QTextCursor& _cursor);

    /**
     * @brief Установить значение необходимости добавления дополнительной прокрутки снизу
     */
    void setAddSpaceToBottom(bool _addSpace);

    /**
     * @brief Установить значение необходимости отображения номеров страниц
     */
    void setShowPageNumbers(bool _show);
    void setShowPageNumberAtFirstPage(bool _show);

    /**
     * @brief Установить место отображения номеров страниц
     */
    void setPageNumbersAlignment(Qt::Alignment _align);

    /**
     * @brief Задать колонтитулы
     */
    void setHeader(const QString& _header);
    void setFooter(const QString& _footer);

    /**
     * @brief Установить необходимость подсвечивания текущей строки
     */
    void setHighlightCurrentLine(bool _highlight);

    /**
     * @brief Установить необходимость фокусировки на текущем абзаце
     */
    bool isFocusCurrentParagraph() const;
    void setFocusCurrentParagraph(bool _focus);

    /**
     * @brief Использовать режим прокрутки как в печатной машинке
     */
    void setUseTypewriterScrolling(bool _use);

    /**
     * @brief Остановить анимацию вертикального скрола
     */
    void stopVerticalScrollAnimation();

    /**
     * @brief Создать контекстное меню в заданной позиции
     */
    virtual ContextMenu* createContextMenu(const QPoint& _position, QWidget* _parent = nullptr);

protected:
    /**
     * @brief Установить область обрезки так, чтобы вырезалось всё, что выходит на поля страницы
     */
    void clipPageDecorationRegions(QPainter* _painter);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(PageTextEdit::AutoFormatting)

QT_END_NAMESPACE
