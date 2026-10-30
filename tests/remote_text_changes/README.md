# Remote screenplay changes

QtTest regression tests using the actual `ScreenplayTextEdit`, `ScreenplayTextModel`,
and cloud patch encoder. Build corelib and the screenplay text plugin first using
the repository's normal qmake build. The test target links those built libraries.

On Linux, from the repository root, with Qt's qmake and libraries available:

```sh
mkdir -p work/remote-text-tests
cd work/remote-text-tests
qmake ../../tests/remote_text_changes/remote_text_changes.pro
make -j2
LD_LIBRARY_PATH="../../src/_build:../../src/_build/libs:../../src/_build/plugins${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
    QT_QPA_PLATFORM=offscreen ./tst_remote_text_changes
```

The data-driven cases cover continuous and paginated layout, growth/shrinkage above
the cursor, changes before/after the cursor within a paragraph, disjoint edits,
batches, both selection directions, scrolling away from the cursor, inserted and
deleted paragraphs, replacement of the current text, Unicode, and large patches
that reset the model (including subsequent patches and an active selection).
They check the applied model content, cursor/selection and viewport position both
immediately and after queued layout/correction events.

With the original implementation, growth above the cursor changes its viewport
coordinate, batches restore an obsolete absolute position, and full model resets
lose the cursor's location. The tests exercise `applyDocumentChanges()` directly;
no cloud account or network connection is needed.

The full-view regression also constructs `ScreenplayTextView` and its scalable
wrapper. It inserts an empty paragraph and changes distant formatting to trigger
a full model reset while the cursor is at the end of a paragraph near the bottom
of the viewport. It checks the position after queued view callbacks at 75%, 100%
and 150% zoom, with page-break correction enabled and disabled. This catches a
second restoration of the obsolete absolute position by the view, which tests of
the text widget alone cannot exercise.
