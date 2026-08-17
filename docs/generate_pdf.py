#!/usr/bin/env python3
"""Build the printable Story Architect Codex handbook from its Markdown source."""

from __future__ import annotations

import html
import re
from pathlib import Path

from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER, TA_LEFT
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.lib.units import mm
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.pdfbase import pdfmetrics
from reportlab.platypus import (
    HRFlowable,
    Image,
    KeepTogether,
    ListFlowable,
    ListItem,
    PageBreak,
    Paragraph,
    SimpleDocTemplate,
    Spacer,
    Table,
    TableStyle,
)


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "docs" / "Story_Architect_Codex_Guide.md"
OUTPUT = ROOT / "output" / "pdf" / "Story_Architect_Codex_Guide.pdf"
LOGO = ROOT / "img" / "starc.png"

NAVY = colors.HexColor("#18212F")
INK = colors.HexColor("#202938")
MUTED = colors.HexColor("#667085")
BLUE = colors.HexColor("#168EF5")
PALE_BLUE = colors.HexColor("#EAF4FF")
PALE_GREY = colors.HexColor("#F4F6F8")
RULE = colors.HexColor("#D8DEE8")
WHITE = colors.white


def choose_fonts() -> tuple[str, str, str]:
    """Use a modern local font if available, otherwise built-in Helvetica."""
    candidates = [
        (
            Path("/System/Library/Fonts/Supplemental/Arial.ttf"),
            Path("/System/Library/Fonts/Supplemental/Arial Bold.ttf"),
            Path("/System/Library/Fonts/Supplemental/Arial Italic.ttf"),
        ),
        (
            Path("/Library/Fonts/Arial.ttf"),
            Path("/Library/Fonts/Arial Bold.ttf"),
            Path("/Library/Fonts/Arial Italic.ttf"),
        ),
    ]
    for regular, bold, italic in candidates:
        if regular.exists() and bold.exists() and italic.exists():
            pdfmetrics.registerFont(TTFont("GuideSans", str(regular)))
            pdfmetrics.registerFont(TTFont("GuideSans-Bold", str(bold)))
            pdfmetrics.registerFont(TTFont("GuideSans-Italic", str(italic)))
            pdfmetrics.registerFontFamily(
                "GuideSans",
                normal="GuideSans",
                bold="GuideSans-Bold",
                italic="GuideSans-Italic",
                boldItalic="GuideSans-Bold",
            )
            return "GuideSans", "GuideSans-Bold", "GuideSans-Italic"
    return "Helvetica", "Helvetica-Bold", "Helvetica-Oblique"


FONT, FONT_BOLD, FONT_ITALIC = choose_fonts()


def ascii_pdf_text(value: str) -> str:
    """Normalize punctuation to reliable PDF glyphs and ASCII hyphens."""
    replacements = {
        "\u2010": "-",
        "\u2011": "-",
        "\u2012": "-",
        "\u2013": "-",
        "\u2014": "-",
        "\u2212": "-",
        "\u2018": "'",
        "\u2019": "'",
        "\u201c": '"',
        "\u201d": '"',
        "\u2026": "...",
        "\u00a0": " ",
        "\u2192": "->",
        "\u2193": "v",
        "\u2022": "*",
    }
    for source, target in replacements.items():
        value = value.replace(source, target)
    return value


def inline_markup(value: str) -> str:
    value = ascii_pdf_text(value.strip())
    escaped = html.escape(value)
    escaped = re.sub(
        r"\[([^\]]+)\]\(([^)]+)\)",
        r'<link href="\2" color="#168EF5">\1</link>',
        escaped,
    )
    escaped = re.sub(r"\*\*([^*]+)\*\*", r"<b>\1</b>", escaped)
    escaped = re.sub(
        r"`([^`]+)`",
        r'<font name="Courier" color="#0F5E9C" backColor="#EAF4FF">\1</font>',
        escaped,
    )
    return escaped


def make_styles():
    base = getSampleStyleSheet()
    styles = {
        "body": ParagraphStyle(
            "GuideBody",
            parent=base["BodyText"],
            fontName=FONT,
            fontSize=9.4,
            leading=13.8,
            textColor=INK,
            spaceAfter=6,
            allowWidows=0,
            allowOrphans=0,
        ),
        "small": ParagraphStyle(
            "GuideSmall",
            parent=base["BodyText"],
            fontName=FONT,
            fontSize=7.6,
            leading=10.4,
            textColor=MUTED,
        ),
        "h2": ParagraphStyle(
            "GuideH2",
            parent=base["Heading1"],
            fontName=FONT_BOLD,
            fontSize=18,
            leading=22,
            textColor=NAVY,
            spaceBefore=16,
            spaceAfter=8,
            keepWithNext=True,
        ),
        "h3": ParagraphStyle(
            "GuideH3",
            parent=base["Heading2"],
            fontName=FONT_BOLD,
            fontSize=12.5,
            leading=16,
            textColor=BLUE,
            spaceBefore=11,
            spaceAfter=5,
            keepWithNext=True,
        ),
        "quote": ParagraphStyle(
            "GuideQuote",
            parent=base["BodyText"],
            fontName=FONT_ITALIC,
            fontSize=9.2,
            leading=13.2,
            leftIndent=8 * mm,
            rightIndent=5 * mm,
            borderColor=BLUE,
            borderWidth=0,
            borderPadding=8,
            backColor=PALE_BLUE,
            textColor=INK,
            spaceBefore=5,
            spaceAfter=9,
        ),
        "code": ParagraphStyle(
            "GuideCode",
            parent=base["Code"],
            fontName="Courier",
            fontSize=7.6,
            leading=10.2,
            leftIndent=5 * mm,
            rightIndent=5 * mm,
            borderPadding=8,
            backColor=PALE_GREY,
            textColor=colors.HexColor("#263649"),
            spaceBefore=4,
            spaceAfter=8,
        ),
        "table_header": ParagraphStyle(
            "GuideTableHeader",
            parent=base["BodyText"],
            fontName=FONT_BOLD,
            fontSize=7.8,
            leading=10,
            textColor=WHITE,
        ),
        "table_cell": ParagraphStyle(
            "GuideTableCell",
            parent=base["BodyText"],
            fontName=FONT,
            fontSize=7.7,
            leading=10.2,
            textColor=INK,
        ),
        "cover_title": ParagraphStyle(
            "CoverTitle",
            parent=base["Title"],
            fontName=FONT_BOLD,
            fontSize=29,
            leading=34,
            alignment=TA_CENTER,
            textColor=WHITE,
        ),
        "cover_subtitle": ParagraphStyle(
            "CoverSubtitle",
            parent=base["BodyText"],
            fontName=FONT,
            fontSize=14,
            leading=19,
            alignment=TA_CENTER,
            textColor=colors.HexColor("#DDEBFA"),
        ),
        "cover_meta": ParagraphStyle(
            "CoverMeta",
            parent=base["BodyText"],
            fontName=FONT,
            fontSize=9,
            leading=14,
            alignment=TA_CENTER,
            textColor=MUTED,
        ),
        "section_card": ParagraphStyle(
            "SectionCard",
            parent=base["BodyText"],
            fontName=FONT_BOLD,
            fontSize=9,
            leading=12,
            textColor=NAVY,
            alignment=TA_LEFT,
        ),
    }
    return styles


STYLES = make_styles()


def parse_table(lines: list[str]) -> Table:
    rows = []
    for row_index, line in enumerate(lines):
        cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
        if row_index == 1 and all(re.fullmatch(r":?-{3,}:?", cell) for cell in cells):
            continue
        style = STYLES["table_header"] if row_index == 0 else STYLES["table_cell"]
        rows.append([Paragraph(inline_markup(cell), style) for cell in cells])
    column_count = max(len(row) for row in rows)
    available = A4[0] - 38 * mm
    widths = [available / column_count] * column_count
    if column_count == 2:
        widths = [available * 0.31, available * 0.69]
    elif column_count == 3:
        widths = [available * 0.24, available * 0.20, available * 0.56]
    table = Table(rows, colWidths=widths, repeatRows=1, hAlign="LEFT")
    table.setStyle(
        TableStyle(
            [
                ("BACKGROUND", (0, 0), (-1, 0), NAVY),
                ("VALIGN", (0, 0), (-1, -1), "TOP"),
                ("GRID", (0, 0), (-1, -1), 0.35, RULE),
                ("ROWBACKGROUNDS", (0, 1), (-1, -1), [WHITE, PALE_GREY]),
                ("LEFTPADDING", (0, 0), (-1, -1), 6),
                ("RIGHTPADDING", (0, 0), (-1, -1), 6),
                ("TOPPADDING", (0, 0), (-1, -1), 5),
                ("BOTTOMPADDING", (0, 0), (-1, -1), 5),
            ]
        )
    )
    return table


def parse_markdown(text: str):
    lines = text.splitlines()
    flow = []
    index = 0
    in_code = False
    code_lines: list[str] = []
    paragraph: list[str] = []

    def flush_paragraph():
        if paragraph:
            joined = " ".join(part.strip() for part in paragraph)
            flow.append(Paragraph(inline_markup(joined), STYLES["body"]))
            paragraph.clear()

    while index < len(lines):
        line = lines[index]
        stripped = line.strip()
        if stripped.startswith("```"):
            flush_paragraph()
            if in_code:
                escaped = html.escape(ascii_pdf_text("\n".join(code_lines)))
                flow.append(Paragraph(escaped.replace("\n", "<br/>"), STYLES["code"]))
                code_lines.clear()
                in_code = False
            else:
                in_code = True
            index += 1
            continue
        if in_code:
            code_lines.append(line)
            index += 1
            continue
        if index < 7 and (stripped.startswith("# ") or stripped.startswith("## User")):
            index += 1
            continue
        if index < 12 and (stripped.startswith("Documentation edition:")
                           or stripped.startswith("Application base:")
                           or stripped.startswith("Status:")):
            index += 1
            continue
        if not stripped:
            flush_paragraph()
            index += 1
            continue
        if stripped.startswith("## "):
            flush_paragraph()
            flow.append(Paragraph(inline_markup(stripped[3:]), STYLES["h2"]))
            flow.append(HRFlowable(width="100%", thickness=0.7, color=RULE, spaceAfter=5))
            index += 1
            continue
        if stripped.startswith("### "):
            flush_paragraph()
            flow.append(Paragraph(inline_markup(stripped[4:]), STYLES["h3"]))
            index += 1
            continue
        if stripped.startswith("> "):
            flush_paragraph()
            flow.append(Paragraph(inline_markup(stripped[2:]), STYLES["quote"]))
            index += 1
            continue
        if stripped.startswith("|") and index + 1 < len(lines) and lines[index + 1].strip().startswith("|"):
            flush_paragraph()
            table_lines = []
            while index < len(lines) and lines[index].strip().startswith("|"):
                table_lines.append(lines[index])
                index += 1
            flow.append(parse_table(table_lines))
            flow.append(Spacer(1, 7))
            continue
        bullet_match = re.match(r"^[-*] (.+)$", stripped)
        number_match = re.match(r"^(\d+)\. (.+)$", stripped)
        if bullet_match or number_match:
            flush_paragraph()
            items = []
            ordered = bool(number_match)
            while index < len(lines):
                candidate = lines[index].strip()
                match = re.match(r"^\d+\. (.+)$", candidate) if ordered else re.match(r"^[-*] (.+)$", candidate)
                if not match:
                    break
                items.append(
                    ListItem(
                        Paragraph(inline_markup(match.group(1)), STYLES["body"]),
                        leftIndent=5 * mm,
                    )
                )
                index += 1
            list_options = {
                "bulletType": "1" if ordered else "bullet",
                "start": "1" if ordered else "-",
                "leftIndent": 7 * mm,
                "bulletFontName": FONT_BOLD,
                "bulletFontSize": 8,
                "bulletColor": BLUE,
                "spaceAfter": 5,
            }
            flow.append(ListFlowable(items, **list_options))
            continue
        paragraph.append(stripped)
        index += 1
    flush_paragraph()
    return flow


def cover_story():
    result = [Spacer(1, 15 * mm)]
    if LOGO.exists():
        result.append(Image(str(LOGO), width=50 * mm, height=50 * mm))
        result.append(Spacer(1, 6 * mm))
    title_box = Table(
        [
            [Paragraph("STORY ARCHITECT CODEX", STYLES["cover_title"])],
            [Paragraph("User and Maintainer Guide", STYLES["cover_subtitle"])],
        ],
        colWidths=[155 * mm],
        rowHeights=[23 * mm, 16 * mm],
    )
    title_box.setStyle(
        TableStyle(
            [
                ("BACKGROUND", (0, 0), (-1, -1), NAVY),
                ("BOX", (0, 0), (-1, -1), 1.2, BLUE),
                ("VALIGN", (0, 0), (-1, -1), "MIDDLE"),
                ("LEFTPADDING", (0, 0), (-1, -1), 10),
                ("RIGHTPADDING", (0, 0), (-1, -1), 10),
            ]
        )
    )
    result.extend([title_box, Spacer(1, 9 * mm)])
    result.append(
        Paragraph(
            "A practical handbook for the local Codex-powered screenplay editor: "
            "chat, live story context, direct native edits, continuity review, memory, "
            "character safety, build, recovery, and release.",
            ParagraphStyle(
                "CoverDeck",
                parent=STYLES["body"],
                fontName=FONT,
                fontSize=12,
                leading=18,
                alignment=TA_CENTER,
                textColor=INK,
                leftIndent=16 * mm,
                rightIndent=16 * mm,
            ),
        )
    )
    result.append(Spacer(1, 12 * mm))
    cards = [
        "PROJECT-AWARE CHAT",
        "REVIEWED NATIVE EDITS",
        "CONTINUITY + MEMORY",
        "SAFE CHARACTER OPERATIONS",
    ]
    card_table = Table(
        [[Paragraph(card, STYLES["section_card"]) for card in cards[:2]],
         [Paragraph(card, STYLES["section_card"]) for card in cards[2:]]],
        colWidths=[75 * mm, 75 * mm],
        rowHeights=[16 * mm, 16 * mm],
    )
    card_table.setStyle(
        TableStyle(
            [
                ("BACKGROUND", (0, 0), (-1, -1), PALE_BLUE),
                ("GRID", (0, 0), (-1, -1), 1, WHITE),
                ("VALIGN", (0, 0), (-1, -1), "MIDDLE"),
                ("LEFTPADDING", (0, 0), (-1, -1), 9),
            ]
        )
    )
    result.extend([card_table, Spacer(1, 14 * mm)])
    result.append(
        Paragraph(
            "Documentation edition 2026-08-14<br/>"
            "Based on Story Architect beta 0.8.3<br/>"
            "Local experimental fork - GNU GPLv3",
            STYLES["cover_meta"],
        )
    )
    result.append(PageBreak())
    return result


def draw_cover(canvas, doc):
    canvas.saveState()
    width, height = A4
    canvas.setFillColor(colors.HexColor("#F8FAFC"))
    canvas.rect(0, 0, width, height, fill=1, stroke=0)
    canvas.setFillColor(BLUE)
    canvas.rect(0, height - 7 * mm, width, 7 * mm, fill=1, stroke=0)
    canvas.setFillColor(NAVY)
    canvas.rect(0, 0, width, 7 * mm, fill=1, stroke=0)
    canvas.restoreState()


def draw_page(canvas, doc):
    canvas.saveState()
    width, height = A4
    canvas.setStrokeColor(RULE)
    canvas.setLineWidth(0.5)
    canvas.line(19 * mm, height - 14 * mm, width - 19 * mm, height - 14 * mm)
    canvas.setFont(FONT_BOLD, 7.5)
    canvas.setFillColor(NAVY)
    canvas.drawString(19 * mm, height - 10.5 * mm, "STORY ARCHITECT CODEX")
    canvas.setFont(FONT, 7.2)
    canvas.setFillColor(MUTED)
    canvas.drawRightString(width - 19 * mm, height - 10.5 * mm, "USER AND MAINTAINER GUIDE")
    canvas.line(19 * mm, 13 * mm, width - 19 * mm, 13 * mm)
    canvas.drawString(19 * mm, 8.5 * mm, "Modified beta build - keep independent project backups")
    canvas.setFont(FONT_BOLD, 7.5)
    canvas.setFillColor(BLUE)
    canvas.drawRightString(width - 19 * mm, 8.5 * mm, f"{doc.page}")
    canvas.restoreState()


def build():
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    source_text = SOURCE.read_text(encoding="utf-8")
    document = SimpleDocTemplate(
        str(OUTPUT),
        pagesize=A4,
        rightMargin=19 * mm,
        leftMargin=19 * mm,
        topMargin=19 * mm,
        bottomMargin=18 * mm,
        title="Story Architect Codex - User and Maintainer Guide",
        author="Story Architect Codex project",
        subject="User, architecture, privacy, build, test, and recovery documentation",
        creator="ReportLab documentation generator",
    )
    story = cover_story()
    story.extend(parse_markdown(source_text))
    document.build(story, onFirstPage=draw_cover, onLaterPages=draw_page)
    print(OUTPUT)


if __name__ == "__main__":
    build()
