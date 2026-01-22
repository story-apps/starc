/***********************************************************************
 *
 * Copyright (C) 2013, 2014 Graeme Gott <graeme@gottcode.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ***********************************************************************/

#include "docx_reader.h"

#include "format_helpers.h"
#include "qtzip/QtZipReader"

#include <QTextDocument>
#include <QXmlStreamAttributes>

namespace {
qreal pixelsFromTwips(qint32 _twips)
{
    qreal inches = _twips / 1440.0;
    qreal pixels = inches * 96.0;
    return pixels;
}

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
static bool readBool(QStringView value)
#else
static bool readBool(const QStringRef& value)
#endif
{
    // ECMA-376, ISO/IEC 29500 strict
    if (value.isEmpty()) {
        return true;
    } else if (value == QLatin1String("false")) {
        return false;
    } else if (value == QLatin1String("true")) {
        return true;
    } else if (value == QLatin1String("0")) {
        return false;
    } else if (value == QLatin1String("1")) {
        return true;
        // ECMA-376 1st edition, ECMA-376 2nd edition transitional, ISO/IEC 29500 transitional
    } else if (value == QLatin1String("off")) {
        return false;
    } else if (value == QLatin1String("on")) {
        return true;
        // Invalid, just guess
    } else {
        return true;
    }
}
} // namespace

//-----------------------------------------------------------------------------

void DocxReader::Comment::insertIfReady(const QTextCursor& _cursor) const
{
    if (start_position != -1 && end_position != -1 && start_position < end_position
        && !text.isEmpty()) {
        QTextCursor commentCursor(_cursor);
        commentCursor.setPosition(start_position);
        commentCursor.setPosition(end_position, QTextCursor::KeepAnchor);
        QTextCharFormat format = commentCursor.charFormat();
        //
        format.setProperty(Docx::IsComment, true);
        //
        // Проверяем, не добавлен ли ещё этот комментарий
        //
        QStringList comments = format.property(Docx::Comments).toStringList();
        if (!comments.contains(text)) {
            comments.append(text);
            format.setProperty(Docx::Comments, comments);
            //
            QStringList authors = format.property(Docx::CommentsAuthors).toStringList();
            authors.append(author);
            format.setProperty(Docx::CommentsAuthors, authors);
            //
            QStringList dates = format.property(Docx::CommentsDates).toStringList();
            dates.append(date);
            format.setProperty(Docx::CommentsDates, dates);
            //
            // Цвет настраивается по первому автору
            //
            format.setBackground(Docx::commentColor(authors.first()));
            format.setForeground(Qt::black);
            //
            commentCursor.mergeCharFormat(format);
        }
    }
}

//-----------------------------------------------------------------------------

DocxReader::DocxReader()
    : m_in_block(false)
{
    m_xml.setNamespaceProcessing(false);
}

//-----------------------------------------------------------------------------

bool DocxReader::canRead(QIODevice* device)
{
    return QtZipReader::canRead(device);
}

//-----------------------------------------------------------------------------

void DocxReader::readData(QIODevice* device)
{
    m_in_block = m_cursor.document()->blockCount();
    m_current_style.block_format = m_cursor.blockFormat();

    // Open archive
    QtZipReader zip(device);

    // Read archive
    if (zip.isReadable()) {
        const QString files[]
            = { QString::fromLatin1("word/styles.xml"), QString::fromLatin1("word/comments.xml"),
                QString::fromLatin1("word/document.xml") };
        for (int i = 0; i < 3; ++i) {
            QByteArray data = zip.fileData(files[i]);
            if (data.isEmpty()) {
                continue;
            }
            m_xml.addData(data);
            readContent();
            if (m_xml.hasError()) {
                m_error = m_xml.errorString();
                break;
            }
            m_xml.clear();
        }
    } else {
        m_error = tr("Unable to open archive.");
    }

    // Close archive
    zip.close();

    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

//-----------------------------------------------------------------------------

void DocxReader::readContent()
{
    m_xml.readNextStartElement();
    if (m_xml.qualifiedName() == QLatin1String("w:styles")) {
        readStyles();
    } else if (m_xml.qualifiedName() == QLatin1String("w:comments")) {
        readComments();
    } else if (m_xml.qualifiedName() == QLatin1String("w:document")) {
        readDocument();
    }
}

//-----------------------------------------------------------------------------

void DocxReader::readStyles()
{
    if (!m_xml.readNextStartElement()) {
        return;
    }

    // Read document defaults
    if (m_xml.qualifiedName() == QLatin1String("w:docDefaults")) {
        while (m_xml.readNextStartElement()) {
            if (m_xml.qualifiedName() == QLatin1String("w:rPrDefault")) {
                if (m_xml.readNextStartElement()) {
                    if (m_xml.qualifiedName() == QLatin1String("w:rPr")) {
                        readRunProperties(m_current_style);
                    } else {
                        m_xml.skipCurrentElement();
                    }
                }
            } else if (m_xml.qualifiedName() == QLatin1String("w:pPrDefault")) {
                if (m_xml.readNextStartElement()) {
                    if (m_xml.qualifiedName() == QLatin1String("w:pPr")) {
                        readParagraphProperties(m_current_style);
                    } else {
                        m_xml.skipCurrentElement();
                    }
                }
            } else {
                m_xml.skipCurrentElement();
            }
        }
        m_xml.skipCurrentElement();
    }

    // Read styles
    QHash<Style::Type, QString> default_style;
    QHash<QString, QStringList> style_tree;

    do {
        if (m_xml.qualifiedName() == QLatin1String("w:style")) {
            Style style;

            // Find style type
            auto type = m_xml.attributes().value("w:type");
            if (type == QLatin1String("paragraph")) {
                style.type = Style::Paragraph;
            } else if (type == QLatin1String("character")) {
                style.type = Style::Character;
            } else {
                m_xml.skipCurrentElement();
                continue;
            }

            // Find style ID
            QString style_id = m_xml.attributes().value(QLatin1String("w:styleId")).toString();
            if (m_styles.contains(style_id)) {
                m_xml.skipCurrentElement();
                continue;
            }

            // Add style ID to tree
            if (!style_tree.contains(style_id)) {
                style_tree.insert(style_id, QStringList());
            }

            // Determine if this is the default style
            if (m_xml.attributes().hasAttribute("w:default")
                && readBool(m_xml.attributes().value("w:default"))) {
                default_style[style.type] = style_id;
            }

            // Read style contents
            while (m_xml.readNextStartElement()) {
                if (m_xml.qualifiedName() == QLatin1String("w:name")) {
                    QString name = m_xml.attributes().value("w:val").toString();
                    if (name.startsWith("Head")) {
                        int heading = qBound(1, name.at(name.length() - 1).digitValue(), 6);
                        style.block_format.setProperty(QTextFormat::UserProperty, heading);
                    }
                    m_xml.skipCurrentElement();
                } else if (m_xml.qualifiedName() == QLatin1String("w:basedOn")) {
                    QString parent_style_id = m_xml.attributes().value("w:val").toString();
                    if (m_styles.contains(parent_style_id)
                        && (style.type == m_styles[parent_style_id].type)) {
                        Style newstyle = m_styles[parent_style_id];
                        newstyle.block_format.merge(style.block_format);
                        newstyle.char_format.merge(style.char_format);
                        style = newstyle;
                    }
                    style_tree[parent_style_id] += style_id;
                    m_xml.skipCurrentElement();
                } else if ((style.type == Style::Paragraph)
                           && (m_xml.qualifiedName() == QLatin1String("w:pPr"))) {
                    readParagraphProperties(style, false);
                } else if (m_xml.qualifiedName() == QLatin1String("w:rPr")) {
                    readRunProperties(style, false);
                } else {
                    m_xml.skipCurrentElement();
                }
            }

            // Add to style list
            m_styles.insert(style_id, style);

            // Recursively apply style to children
            QStringList children = style_tree.value(style_id);
            while (!children.isEmpty()) {
                QString child_id = children.takeFirst();

                Style newstyle = style;
                Style& childstyle = m_styles[child_id];
                newstyle.merge(childstyle);
                childstyle = newstyle;

                children.append(style_tree.value(child_id));
            }
        } else if (m_xml.tokenType() != QXmlStreamReader::EndElement) {
            m_xml.skipCurrentElement();
        }
    } while (m_xml.readNextStartElement());

    // Apply default style
    m_current_style.block_format.merge(
        m_styles.value(default_style.value(Style::Paragraph)).block_format);
    m_current_style.char_format.merge(
        m_styles.value(default_style.value(Style::Character)).char_format);
}

//-----------------------------------------------------------------------------

void DocxReader::readComments()
{
    if (!m_xml.readNextStartElement()) {
        return;
    }

    // Read comments
    do {
        if (m_xml.qualifiedName() == QLatin1String("w:comment")) {
            Comment comment;

            // Find comment ID
            const QString comment_id = m_xml.attributes().value(QLatin1String("w:id")).toString();
            if (m_comments.contains(comment_id)) {
                m_xml.skipCurrentElement();
                continue;
            }

            // Read comment contents
            comment.author = m_xml.attributes().value(QLatin1String("w:author")).toString();
            comment.date = m_xml.attributes().value(QLatin1String("w:date")).toString();
            while (m_xml.readNextStartElement()) {
                if (m_xml.qualifiedName() == QLatin1String("w:p")) {
                    if (!comment.text.isEmpty()) {
                        comment.text.append("\n");
                    }
                    while (m_xml.readNextStartElement()) {
                        if (m_xml.qualifiedName() == QLatin1String("w:r")) {
                            while (m_xml.readNextStartElement()) {
                                if (m_xml.qualifiedName() == QLatin1String("w:t")) {
                                    comment.text.append(m_xml.readElementText());
                                } else {
                                    m_xml.skipCurrentElement();
                                }
                            }
                        } else {
                            m_xml.skipCurrentElement();
                        }
                    }
                } else {
                    m_xml.skipCurrentElement();
                }
            }

            // Add to comments list
            m_comments.insert(comment_id, comment);
        } else if (m_xml.tokenType() != QXmlStreamReader::EndElement) {
            m_xml.skipCurrentElement();
        }
    } while (m_xml.readNextStartElement());
}

//-----------------------------------------------------------------------------

void DocxReader::readDocument()
{
    m_cursor.beginEditBlock();
    while (m_xml.readNextStartElement()) {
        if (m_xml.qualifiedName() == QLatin1String("w:body")) {
            readBody();
        } else {
            m_xml.skipCurrentElement();
        }
    }
    m_cursor.endEditBlock();
}

//-----------------------------------------------------------------------------

void DocxReader::readBody()
{
    while (m_xml.readNextStartElement()) {
        if (m_xml.qualifiedName() == QLatin1String("w:p")) {
            readParagraph();
        } else if ((m_xml.qualifiedName() == QLatin1String("w:commentRangeStart"))
                   || (m_xml.qualifiedName() == QLatin1String("w:bookmarkStart"))) {
            m_current_comment.clear();
            m_current_comment.start_position = m_cursor.position();
            m_xml.skipCurrentElement();
        } else if ((m_xml.qualifiedName() == QLatin1String("w:commentRangeEnd"))
                   || (m_xml.qualifiedName() == QLatin1String("w:bookmarkEnd"))) {
            m_current_comment.end_position = m_cursor.position();
            m_current_comment.insertIfReady(m_cursor);

            m_xml.skipCurrentElement();
        } else {
            m_xml.skipCurrentElement();
        }
    }
}

//-----------------------------------------------------------------------------

void DocxReader::readParagraph()
{
    bool has_children = m_xml.readNextStartElement();

    // Style paragraph
    bool changedstate = false;
    if (has_children && (m_xml.qualifiedName() == QLatin1String("w:pPr"))) {
        changedstate = true;
        m_previous_styles.push(m_current_style);
        readParagraphProperties(m_current_style);
    }

    // Create paragraph
    if (!m_in_block) {
        m_cursor.insertBlock(m_current_style.block_format, m_current_style.char_format);
        m_in_block = true;
    } else {
        m_cursor.mergeBlockFormat(m_current_style.block_format);
        m_cursor.mergeBlockCharFormat(m_current_style.char_format);
    }

    // Read paragraph text
    if (has_children) {
        do {
            if (m_xml.qualifiedName() == QLatin1String("w:r")) {
                readRun();
            } else if ((m_xml.qualifiedName() == QLatin1String("w:commentRangeStart"))
                       || (m_xml.qualifiedName() == QLatin1String("w:bookmarkStart"))) {
                m_current_comment.clear();
                m_current_comment.start_position = m_cursor.position();
                m_xml.skipCurrentElement();
            } else if ((m_xml.qualifiedName() == QLatin1String("w:commentRangeEnd"))
                       || (m_xml.qualifiedName() == QLatin1String("w:bookmarkEnd"))) {
                m_current_comment.end_position = m_cursor.position();
                m_current_comment.insertIfReady(m_cursor);

                m_xml.skipCurrentElement();
            } else if (m_xml.tokenType() != QXmlStreamReader::EndElement) {
                m_xml.skipCurrentElement();
            }
        } while (m_xml.readNextStartElement());
    }
    m_in_block = false;

    // Reset paragraph styling
    if (changedstate) {
        m_current_style = m_previous_styles.pop();
    }

    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

//-----------------------------------------------------------------------------

void DocxReader::readParagraphProperties(Style& style, bool allowstyles)
{
    int left_indent = 0, right_indent = 0, text_indent = 0, indent = 0, top_indent = 0,
        bottom_indent = 0;
    while (m_xml.readNextStartElement()) {
        const QXmlStreamAttributes& attributes = m_xml.attributes();
        const auto value = attributes.value("w:val");
        if (m_xml.qualifiedName() == QLatin1String("w:jc")) {
            // ECMA-376 1st edition, ECMA-376 2nd edition transitional, ISO/IEC 29500 transitional
            if (value == QLatin1String("left")) {
                style.block_format.setAlignment(Qt::AlignLeft | Qt::AlignAbsolute);
            } else if (value == QLatin1String("right")) {
                style.block_format.setAlignment(Qt::AlignRight | Qt::AlignAbsolute);
                // ECMA-376, ISO/IEC 29500 strict
            } else if (value == QLatin1String("center")) {
                style.block_format.setAlignment(Qt::AlignCenter);
            } else if (value == QLatin1String("both")) {
                style.block_format.setAlignment(Qt::AlignJustify);
                // ECMA-376 2nd edition, ISO/IEC 29500 strict
            } else if (value == QLatin1String("start")) {
                style.block_format.setAlignment(Qt::AlignLeft);
            } else if (value == QLatin1String("end")) {
                style.block_format.setAlignment(Qt::AlignRight);
            }
        } else if (m_xml.qualifiedName() == QLatin1String("w:ind")) {
            // ECMA-376 1st edition, ECMA-376 2nd edition transitional, ISO/IEC 29500 transitional
            if (attributes.hasAttribute("w:left")) {
                left_indent = pixelsFromTwips(attributes.value("w:left").toString().toInt());
                style.block_format.setLeftMargin(left_indent);
            }
            if (attributes.hasAttribute("w:right")) {
                right_indent = pixelsFromTwips(attributes.value("w:right").toString().toInt());
                style.block_format.setRightMargin(right_indent);
            }
            if (attributes.hasAttribute("w:firstLine")) {
                text_indent = pixelsFromTwips(attributes.value("w:firstLine").toString().toInt());
                style.block_format.setTextIndent(text_indent);
            }
            if (attributes.hasAttribute("w:hanging")) {
                auto t = attributes.value("w:hanging").toString();
                indent = pixelsFromTwips(attributes.value("w:hanging").toString().toInt());
                if (indent) {
                    // В docx этот параметр хранится в инвертированном по отношению к Qt виде
                    style.block_format.setIndent(-1 * indent);
                    left_indent = right_indent = 0;
                }
            }
            // ECMA-376 2nd edition, ISO/IEC 29500 strict
            else if (attributes.hasAttribute("w:start")) {
                indent = pixelsFromTwips(attributes.value("w:start").toString().toInt());
                if (indent) {
                    style.block_format.setIndent(indent);
                    left_indent = right_indent = 0;
                }
            }
        } else if (m_xml.qualifiedName() == QLatin1String("w:spacing")) {
            // ECMA-376 1st edition, ECMA-376 2nd edition transitional, ISO/IEC 29500 transitional
            if (attributes.hasAttribute("w:before")) {
                top_indent = pixelsFromTwips(attributes.value("w:before").toString().toInt());
                style.block_format.setTopMargin(top_indent);
            }
            if (attributes.hasAttribute("w:after")) {
                bottom_indent = pixelsFromTwips(attributes.value("w:after").toString().toInt());
                style.block_format.setBottomMargin(bottom_indent);
            }
        } else if (m_xml.qualifiedName() == QLatin1String("w:textDirection")) {
            if (value == QLatin1String("rl")) {
                style.block_format.setLayoutDirection(Qt::RightToLeft);
            } else if (value == QLatin1String("lr")) {
                style.block_format.setLayoutDirection(Qt::LeftToRight);
            }
        } else if (m_xml.qualifiedName() == QLatin1String("w:outlineLvl")) {
            int heading = qBound(1, attributes.value("w:val").toString().toInt() + 1, 6);
            style.block_format.setProperty(QTextFormat::UserProperty, heading);
        } else if ((m_xml.qualifiedName() == QLatin1String("w:pStyle")) && allowstyles) {
            Style pstyle = m_styles.value(value.toString());
            pstyle.merge(style);
            style = pstyle;
        } else if (m_xml.qualifiedName() == QLatin1String("w:rPr")) {
            readRunProperties(style);
            continue;
        }

        m_xml.skipCurrentElement();
    }

    if (style.block_format.property(QTextFormat::UserProperty).toInt()) {
        style.char_format.setFontWeight(QFont::Normal);
    }
}

//-----------------------------------------------------------------------------

void DocxReader::readRun()
{
    if (m_xml.readNextStartElement()) {
        // Style text run
        bool changedstate = false;
        if (m_xml.qualifiedName() == QLatin1String("w:rPr")) {
            changedstate = true;
            m_previous_styles.push(m_current_style);
            readRunProperties(m_current_style);
        }

        // Read text run
        do {
            if (m_xml.qualifiedName() == QLatin1String("w:t")) {
                readText();
            } else if (m_xml.qualifiedName() == QLatin1String("w:tab")) {
                m_cursor.insertText(QChar(0x0009), m_current_style.char_format);
                m_xml.skipCurrentElement();
            } else if (m_xml.qualifiedName() == QLatin1String("w:br")) {
                m_cursor.insertText(QChar(0x2028), m_current_style.char_format);
                m_xml.skipCurrentElement();
            } else if (m_xml.qualifiedName() == QLatin1String("w:cr")) {
                m_cursor.insertText(QChar(0x2028), m_current_style.char_format);
                m_xml.skipCurrentElement();
            } else if (m_xml.qualifiedName() == QLatin1String("w:noBreakHyphen")) {
                m_cursor.insertText(QChar(0x2013), m_current_style.char_format);
                m_xml.skipCurrentElement();
            } else if (m_xml.qualifiedName() == QLatin1String("w:commentReference")) {
                const QString comment_id = m_xml.attributes().value("w:id").toString();
                m_current_comment.text = m_comments.value(comment_id).text;
                m_current_comment.author = m_comments.value(comment_id).author;
                m_current_comment.date = m_comments.value(comment_id).date;
                m_current_comment.insertIfReady(m_cursor);
                m_xml.skipCurrentElement();
            } else if (m_xml.tokenType() != QXmlStreamReader::EndElement) {
                m_xml.skipCurrentElement();
            }
        } while (m_xml.readNextStartElement());

        // Reset text run styling
        if (changedstate) {
            m_current_style = m_previous_styles.pop();
        }
    }
}

//-----------------------------------------------------------------------------

void DocxReader::readRunProperties(Style& style, bool allowstyles)
{
    while (m_xml.readNextStartElement()) {
        const auto value = m_xml.attributes().value("w:val");
        if ((m_xml.qualifiedName() == QLatin1String("w:b"))
            || (m_xml.qualifiedName() == QLatin1String("w:bCs"))) {
            style.char_format.setFontWeight(readBool(value) ? QFont::Bold : QFont::Normal);
        } else if ((m_xml.qualifiedName() == QLatin1String("w:i"))
                   || (m_xml.qualifiedName() == QLatin1String("w:iCs"))) {
            style.char_format.setFontItalic(readBool(value));
        } else if (m_xml.qualifiedName() == QLatin1String("w:u")) {
            if (value == QLatin1String("single")) {
                style.char_format.setFontUnderline(true);
            } else if (value == QLatin1String("none")) {
                style.char_format.setFontUnderline(false);
            } else if ((value == QLatin1String("dash"))
                       || (value == QLatin1String("dashDotDotHeavy"))
                       || (value == QLatin1String("dashDotHeavy"))
                       || (value == QLatin1String("dashedHeavy"))
                       || (value == QLatin1String("dashLong"))
                       || (value == QLatin1String("dashLongHeavy"))
                       || (value == QLatin1String("dotDash"))
                       || (value == QLatin1String("dotDotDash"))
                       || (value == QLatin1String("dotted"))
                       || (value == QLatin1String("dottedHeavy"))
                       || (value == QLatin1String("double")) || (value == QLatin1String("thick"))
                       || (value == QLatin1String("wave")) || (value == QLatin1String("wavyDouble"))
                       || (value == QLatin1String("wavyHeavy"))
                       || (value == QLatin1String("words"))) {
                style.char_format.setFontUnderline(true);
            }
        } else if (m_xml.qualifiedName() == QLatin1String("w:strike")) {
            style.char_format.setFontStrikeOut(readBool(value));
        } else if (m_xml.qualifiedName() == QLatin1String("w:vertAlign")) {
            if (value == QLatin1String("superscript")) {
                style.char_format.setVerticalAlignment(QTextCharFormat::AlignSuperScript);
            } else if (value == QLatin1String("subscript")) {
                style.char_format.setVerticalAlignment(QTextCharFormat::AlignSubScript);
            } else if (value == QLatin1String("baseline")) {
                style.char_format.setVerticalAlignment(QTextCharFormat::AlignNormal);
            }
        } else if (m_xml.qualifiedName() == QLatin1String("w:caps")) {
            if (readBool(value)) {
                style.char_format.setFontCapitalization(QFont::AllUppercase);
            }
        } else if ((m_xml.qualifiedName() == QLatin1String("w:rStyle")) && allowstyles) {
            Style rstyle = m_styles.value(value.toString());
            rstyle.merge(style);
            style = rstyle;
        }
        //
        // Заливка
        //
        else if (m_xml.qualifiedName() == QLatin1String("w:shd")) {
            const QColor color("#" + m_xml.attributes().value("w:fill").toString());
            //
            // Игнорируем белый
            //
            if (color.isValid() && color != Qt::white) {
                style.char_format.setProperty(Docx::IsBackground, true);
                style.char_format.setBackground(color);
                style.char_format.setForeground(Qt::black);
            }
        }
        //
        // Выделение маркером
        //
        else if (m_xml.qualifiedName() == QLatin1String("w:highlight")) {
            const QColor color(Docx::highlightColor(value.toString()));
            //
            // Игнорируем белый
            //
            if (color.isValid() && color != Qt::white) {
                style.char_format.setProperty(Docx::IsHighlight, true);
                style.char_format.setBackground(color);
                style.char_format.setForeground(Qt::black);
            }
        }
        //
        // Цвет текста
        //
        else if (m_xml.qualifiedName() == QLatin1String("w:color")) {
            const QColor color("#" + value.toString());
            //
            // Игнорируем все оттенки близкие к чёрному
            //
            if (color.isValid() && color.valueF() > 0.25) {
                style.char_format.setProperty(Docx::IsForeground, true);
                style.char_format.setForeground(color);
            }
        }

        m_xml.skipCurrentElement();
    }
}

//-----------------------------------------------------------------------------

void DocxReader::readText()
{
    //
    // Иногда попадаются файлы странно оформленными, они внутри могут содержать разный шлак, поэтому
    // будем читать, пока не дойдём до конца текстового блока, а всё лишнее просто игнорируем
    //
    QString text;
    do {
        const auto tokenType = m_xml.readNext();
        if (tokenType != QXmlStreamReader::Characters) {
            continue;
        }

        text += m_xml.text();
    } while (!m_xml.isEndElement() || m_xml.qualifiedName() != QLatin1String("w:t"));

    if (!text.isEmpty()) {
        //
        // NOTE: Почему-то при вставке текста в документ, даже если стиль содержит указание, что все
        //       символы должны быть в верхнем регистре, текст вставляется в документ всё равно в
        //       нижнем регистре, поэтому тут делаем это вручную
        //
        if (m_current_style.char_format.fontCapitalization() == QFont::AllUppercase) {
            text = text.toUpper();
        }
        m_cursor.insertText(text, m_current_style.char_format);
    }
}

//-----------------------------------------------------------------------------
