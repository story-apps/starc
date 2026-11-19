# Rendering performance, second round

This target exercises the real `TextField`, `ImageCropper`, and `ImagesList`
widgets. Its comparison base is the first-round commit `4a410022`, not the
original unoptimized application.

Build corelib with the normal qmake build, then, from the repository root:

```sh
mkdir -p work/rendering-round-2
cd work/rendering-round-2
qmake ../../tests/rendering_round_2/rendering_round_2.pro
make -j2
LD_LIBRARY_PATH="../../src/_build:../../src/_build/libs${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
    QT_QPA_PLATFORM=offscreen ./tst_rendering_round_2 -median 5
```

For before/after pixel comparisons, first preserve the first-round `libcorelib.so`
in a separate directory. Run the following cases with that directory first in
`LD_LIBRARY_PATH`, `STARC_WRITE_REFERENCE=1`, and
`STARC_RENDER_REFERENCE_DIR=/absolute/path/to/reference-images`:

```text
cropperReferenceAndInvalidation textFieldReference focusedDescendantReference galleryReference
```

Then run the complete suite against the new library with the same reference
directory and without `STARC_WRITE_REFERENCE`. Tests needing baseline images skip
if that directory is not specified. References cover:

- Cropper image replacement, resizing, fixed proportions, the actual crop result,
  a high-DPI source, null images, cache eviction, and an insufficient cache budget.
- Empty/multiline/Unicode/password fields, clearing a field, and focus in both an
  embedded child widget and an owned tool window.
- Galleries with fractional thumbnail sizes, disjoint paint regions, changed
  image size, row wrapping, and read-only mode.

Additional event tests check that hovering repaints only the affected gallery
cells, pointer movement within a settled cell schedules no repaint, the add-image
button still updates, and dragging still emits the correct reordered image IDs.

The benchmarks use warm rendering and the same offscreen Qt platform in both
builds. Run the old and new processes sequentially, without other tests/builds:

```text
benchmarkCropper benchmarkLongTextField benchmarkGalleryPartialUpdate -median 5
```

Workloads are a 2688 × 1800 image in a 1000 × 700 cropper, a 600 × 300 text field
with 4000 paragraphs, and an 80 × 80 update region in a gallery of 300 thumbnails.
The cropper reuses Qt's existing bounded pixmap cache; its global limit is not
increased. Gallery layout/reordering and text layout are unchanged.

## Recorded comparison

Linux, Qt 5.15.15, release, offscreen, median of five measurements. The baseline
and updated test processes ran sequentially without concurrent builds or suites.
These are times for the specified warm repaint operations, not application FPS.

| Workload | First round (ms) | Second round (ms) |
| --- | ---: | ---: |
| Cropper | 0.55 | 0.20 |
| Long text field | 0.75 | 0.47 |
| Partial gallery update | 1.1 | 0.041 |

The second-round suite reported 17 passed, 0 failed, 0 skipped with baseline
references enabled. The first-round suite also passed all 13 cases (its benchmarks
were run once as smoke checks, not used for timing comparisons). Corelib built
successfully. Qt 6, other operating systems,
and performance on physical low-end user hardware were not measured.
