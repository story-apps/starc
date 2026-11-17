# Temporary draft comparisons

QtTest regressions for the in-memory comparison model and the actual simple-text
editor inside `ProjectView`. Build corelib and the simple-text plugin first.

From the repository root, with Qt's qmake and libraries available:

```sh
mkdir -p work/draft-comparison-tests
cd work/draft-comparison-tests
qmake ../../tests/draft_comparison/draft_comparison.pro
make -j2
LD_LIBRARY_PATH="../../src/_build:../../src/_build/libs:../../src/_build/plugins${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
    QT_QPA_PLATFORM=offscreen ./tst_draft_comparison
```

The tests check that comparisons do not enter the project's model cache or alter
its structure/source content, show additions and removals in the comments model,
remain read-only after permissions are updated, and restore the original editor's
cursor and scroll position. They also cover Back, replacement by another
comparison, navigation to another document, independent split panes, and deletion
of the temporary editor/model while the outgoing animation is still running.
