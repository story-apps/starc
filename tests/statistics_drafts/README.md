# Statistics for document drafts

QtTest regression tests using the real project model facade, document storage,
text models and statistics reports. Build corelib first with the normal qmake
build. Private statistics UI plugins are not required for these tests.

From the repository root, with Qt's qmake and libraries available:

```sh
mkdir -p work/statistics-drafts
cd work/statistics-drafts
qmake ../../tests/statistics_drafts/statistics_drafts.pro
make -j2
QT_QPA_PLATFORM=offscreen ./tst_statistics_drafts
```

The data-driven cases cover screenplay, novel, audio play, stage play and comic
book documents. They verify that current text and saved drafts produce different
word counts, that reports can coexist without changing each other's source, and
that switching back reuses the correct model. They also cover renamed and locked
drafts, invalid draft identifiers, drafts whose content arrives later, deletion
and project cleanup.

To also exercise the actual statistics UI plugins, initialize `src/cloud` and
the five statistics submodules, build the plugins, then enable the optional
integration checks:

```sh
qmake ../../tests/statistics_drafts/statistics_drafts.pro CONFIG+=statistics_plugins
make -j2
QT_QPA_PLATFORM=offscreen ./tst_statistics_drafts
```

These checks load the built plugins, show their primary and secondary widgets,
switch between drafts and back to the current text, and verify that each table
is bound to the correct report. They also exercise report/plot navigation for
screenplays and audio plays, and disconnect the views before model cleanup.

With the private statistics plugins installed, also check in the application:

1. Create two drafts with different text, open Statistics and switch the draft
   tabs. Check both the summary and reports selected in the navigator.
2. Leave Statistics and reopen it; verify that the selected draft is restored.
   Add a newer draft in the text editor and reopen Statistics; the previous
   selection must still refer to the same draft.
3. Delete the selected draft in the text editor and reopen Statistics; it must
   use the current draft. Rename drafts and verify the tab names and colors.
4. Open statistics in two panes with different drafts and check that switching
   one pane does not change the other pane's results.
5. Switch drafts in a remote project while a draft is still loading; verify that
   its report appears when its content arrives. Exit full screen and check that
   the draft tabs remain available.
