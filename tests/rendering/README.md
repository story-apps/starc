# Rendering regressions and benchmarks

Build `corelib` and the screenplay structure plugin with the normal qmake build first.
This target uses the actual widgets and the plugin's `ScreenplayTextStructureDelegate`.
On Linux, from the repository root:

```sh
mkdir -p work/rendering-tests
cd work/rendering-tests
qmake ../../tests/rendering/rendering.pro
make -j2
LD_LIBRARY_PATH="../../src/_build:../../src/_build/libs:../../src/_build/plugins${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
    QT_QPA_PLATFORM=offscreen ./tst_rendering -median 5
```

The checks cover:

- Text elision against the pre-optimization layout algorithm, including line breaks,
  whitespace, Cyrillic, Arabic, CJK, emoji, narrow/fractional widths, font sizes,
  layout directions and translated drawing rectangles.
- Shadow cache invalidation for source shape/alpha, margins, fractional blur radius
  and color; rounded-shadow pixel equivalence; cache eviction and insufficient space.
- Ripple repaint scope, a second click during animation, and cleanup after scrolling.
- Preview reuse, image replacement, resizing, cache eviction, and pointer movement
  within an unchanged navigation region.

`compareCardReference` is optional and skips unless `STARC_RENDER_REFERENCE_DIR`
is set. To compare pixels against an old build, keep a copy of the pre-change
`libcorelib.so` in a separate directory before rebuilding. Compile this test against
the new library, then run **only** `compareCardReference` with that old directory
first in `LD_LIBRARY_PATH`, plus `STARC_WRITE_REFERENCE=1` and
`STARC_RENDER_REFERENCE_DIR=/absolute/path/to/reference-images`. Run the same test
with the new library and without `STARC_WRITE_REFERENCE` to compare. Distinct
sizes for opaque and translucent cases isolate rasterization from the old shadow
cache's alpha-mask collision, which is covered separately by the shadow tests.

For before/after timings, select only `benchmarkSceneDelegate benchmarkCard
benchmarkButton benchmarkPreview -median 5`, switching the library directory in
`LD_LIBRARY_PATH`. Run sequentially on an otherwise idle system, with the same Qt
version, fonts, platform plugin and build configuration. These are warm repaint
benchmarks, not whole-application FPS or measurements of a cold cache:

- One 320 × 120 scene row with a 4,000-character description and two preview lines.
- A 700 × 500 card.
- A 240 × 64 contained button with a shadow.
- A 2,688 × 1,800 image displayed inside a 1,000 × 700 preview.

Also run `tests/remote_text_changes` after changing page decorations; it covers
cursor, selection and viewport preservation in continuous/paginated editors.

The text cache budgets approximately 1 MiB for keys/results/entry overhead and
bypasses very large strings and non-GUI threads. Font-database changes clear it.
Image caches share Qt's existing `QPixmapCache` byte budget (10 MiB by default);
the application-wide limit is not increased. Evicted or oversized images are
rendered normally, without retaining unbounded per-widget bitmaps.

## Recorded comparison

Linux, Qt 5.15.15, release build, offscreen platform, median of five measurements.
Old/new library runs were sequential, with other test processes stopped. These
numbers describe this environment and the warm workloads above, not a target FPS.

| Repaint | Before (ms) | After (ms) |
| --- | ---: | ---: |
| Scene delegate | 8.0 | 1.4 |
| Card | 0.69 | 0.43 |
| Contained button | 0.35 | 0.053 |
| Image preview | 6.43 | 0.65 |

Validation: corelib built successfully; rendering QtTest reported 13 passed,
0 failed, 0 skipped with the optional pixel reference enabled; remote text changes
reported 42 passed, 0 failed. Qt 6 and other operating systems were not exercised.
