# Qt document metrics

Build corelib first, then build this QtTest target with the same Qt version:

```sh
mkdir -p work/document-metrics
cd work/document-metrics
qmake ../../tests/document_metrics/document_metrics.pro
make -j2
QT_QPA_PLATFORM=offscreen ./tst_document_metrics
```

If required, add `src/_build` and `src/_build/libs` to the runtime library path.
The tests load the actual bundled fonts and templates. They cover:

- Physical units, fractional template and inline font sizes through serialization.
- Replacing a PageTextEdit document without reverting to viewport metrics.
- The international A4/Courier New sample from the LibreOffice comparison: the
  first line ends in “От”, using a real PageTextEdit with template margins.
- Whole button labels compared against an independent, unelided Qt rendering.
- Fractional page height, scrollbar extent and cursor page numbers after scrolling.
- Identical document raster output on paint devices with different logical DPI.
- Agreement between document line breaks and the standalone measurement helpers.
- Decoration font sizes on paint devices with 72, 96, 144 and 192 DPI.
- DOCX style and direct paragraph spacing, including half-point font sizes.
- The actual screenplay PDF exporter, with headers and footers.
- Identical serialized line geometry and page counts in child processes with
  `QT_FONT_DPI` 72/96/100/120/144/192 and `QT_SCALE_FACTOR` 1/1.25/1.5/2 varied
  independently as well as together, and unchanged layout when drawing at
  75/100/150/200% zoom. These documents include real page margins. The corpus
  includes Courier Prime, Courier New, Arial, Tamil, Hebrew and Arabic templates
  and mixed-script text.

For a PDF suitable for independent inspection, run:

```sh
STARC_METRICS_PDF=/absolute/path/document.pdf QT_QPA_PLATFORM=offscreen \
  ./tst_document_metrics pdfExport
```

## Layout contract

Document coordinates are logical pixels at 96 DPI, with fractional point sizes
stored in templates/model formatting. Application entry points must set
`Qt::AA_Use96Dpi` before creating QApplication or any fonts: QTextDocument scales
frame margins, fixed line heights and tab stops against Qt's default DPI even
with an explicit layout paint device. Display scaling/device-pixel ratios remain
enabled. Page dimensions and positions stay fractional until a widget/scrollbar
requires integers; rounding each page separately accumulates drift. The document,
independent QTextLayout measurements and document font metrics share a fixed paint device; screens only
affect painting. UI measurements use their native font metrics, separately from
`documentTextWidthF`/`documentLineSpacing`. Button sizing and elision must use the
same metrics as their painter, with sufficient whole pixels for the label. Font
selection still uses Qt's font database and fallback mechanisms.
This does not promise identical glyph rasterization or identical layout across
different Qt/font versions or against arbitrary Word/LibreOffice versions.
Run this target on Windows and macOS as well before releasing; the Linux DPI
matrix is not a substitute for testing their native font engines.

PDF uses the same logical coordinates without fitting text to an integer pixel
page size. Its text remains vector text. DOCX exports the editor's fixed baseline
spacing as `exact`, including direct paragraph overrides. This deliberately favors
initial visual fidelity: editing the DOCX to use larger fonts may require adjusting
its exact line spacing in the receiving office editor. DOCX cannot preserve every
line/page break across other layout engines.

Existing project/template formats remain readable. Their layout is recalculated
with the new metrics, so existing documents may change page counts. The old
screen-dependent "advanced handling" preference is no longer consulted or shown;
it is not migrated to a new layout mode. Fractional inline font sizes written by
this version require this version (older readers accepted integer sizes only).
