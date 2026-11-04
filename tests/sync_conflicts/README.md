# Online/offline synchronization conflicts

Build corelib first, then run from the repository root (Qt 5 or 6):

```sh
mkdir -p work/sync-conflicts-tests
cd work/sync-conflicts-tests
qmake ../../tests/sync_conflicts/sync_conflicts.pro
make -j2
QT_QPA_PLATFORM=offscreen ./tst_sync_conflicts
```

The tests exercise exact patch application, three-way XML merging, and the actual
AbstractModel merge/reset/save path. They cover independent edits, overlapping
replacements, identical edits, insertions, deletion versus editing, Unicode,
malformed XML/patches, undo history reconstruction, rebased upload patches,
idempotent retries, and atomic failures. No cloud account is required.

The Linux project integration target additionally links `coreplugin`. After building
corelib and core, run it in a separate build directory:

```sh
mkdir -p work/project-sync-tests
cd work/project-sync-tests
qmake ../../tests/sync_conflicts/project_sync_conflicts.pro
make -j2
QT_QPA_PLATFORM=offscreen ./tst_project_sync_conflicts
```

It uses temporary project databases and verifies a single conflicting edit,
conflicts in existing drafts, an unrelated selected document, recovery write
failure, and the rebased upload history. It also exercises the comparison action
and reopens the database to check that the offline version remains recoverable.
Editor plugins are optional for these storage/manager checks; absent plugins may
produce editor-activation warnings.

Manual integration checks with a cloud project:

1. Edit different paragraphs offline and online, then reconnect. Both edits should
   appear and an automatic-merge notice should explain the result.
2. Change the same text on both sides, including just **one** offline edit. The
   online version stays current; an offline draft is created on the affected
   document even if another document is selected. The notice opens that draft or
   compares it directly with the online version. Closing the notice keeps both.
3. Repeat with existing drafts and with an offline edit to a draft. Recovery drafts
   must belong to the document, never to a nested draft or the currently selected
   unrelated document. Check that the newly created draft is the one opened.
4. Reopen the project and verify the offline draft remains available. XML recovery
   copies of both versions also live in `conflicts/` beside the project file.
   Documents without a text editor use this recovery folder instead of draft UI.
5. Make the recovery directory unwritable and trigger a conflict. The local model
   and pending history must remain intact, and synchronization must report the
   save failure instead of marking the document synchronized.
6. Trigger several conflicts, then close/switch projects. Notices must appear one
   at a time and must not open documents belonging to the previous project.
