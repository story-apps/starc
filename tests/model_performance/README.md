# Business model performance regressions

Build `corelib` first using the normal qmake build, then run from the repository root:

```sh
mkdir -p work/model-performance
cd work/model-performance
qmake ../../tests/model_performance/model_performance.pro
make -j2
LD_LIBRARY_PATH="../../src/_build:../../src/_build/libs${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
    QT_QPA_PLATFORM=offscreen ./tst_model_performance
```

The suite checks:

- Child positions after randomized batch inserts, removals, moves and sorting,
  including duplicates, drafts and temporary reparenting during patch application.
- Word-count compatibility with the original regular expression for every UTF-16
  code unit, multilingual text, punctuation, whitespace and malformed surrogates.
- Paragraph counters and combined chronometry in Page, Characters and Sophocles modes.
- XML round-tripping of nested folders, scenes and beats, including aggregate text
  and counters.
- A single counter/ancestor update per text edit, no counter update for metadata,
  and continued propagation of review marks to scene summaries.
- Text replacement, removal, merging, copying, XML loading and page-correction
  transitions in screenplay, audioplay, novel, comic book, stageplay and plain text.
- Forced chronometry refresh after changing timing settings, and range removal
  preserving reparented items, destruction order and cached rows.

QtTest benchmarks cover lookup of 20,000 sibling rows, word counting, forced paragraph
counter updates and loading a scene containing 1,000 paragraphs. The chronometry
benchmark directly compares separate and shared layouts within the same process.
Additional benchmarks cover paragraph alignment changes, editing one paragraph in
a 1,000-paragraph scene, and moving 10,000 of 20,000 siblings as a batch.
For more stable measurements, run with `-median 5` on an otherwise idle machine.
Timings are diagnostic; there are no hardware-dependent pass/fail thresholds.

Also run `tests/remote_text_changes` after modifying the common model tree or XML
loading: it exercises real editors, cloud patches, model resets and cursor restoration.

## Second optimization pass: sample measurements

Measured on Linux with Qt 5.15.15 / GCC 14.2, an optimized build and `-median 5`.
The baseline is the first optimization pass rebased onto master `0b91bfa2`
(commit `42fd912c`), not the original application before both passes.

| Benchmark | Before | After |
| --- | ---: | ---: |
| Paragraph alignment change | 0.33 ms | 0.015 ms |
| Edit in a 1,000-paragraph scene | 0.12 ms | 0.065 ms |
| Move 10,000 of 20,000 siblings | 9.7 ms | 0.091 ms |

These are individual model operations, not end-to-end UI latency. The regression
test also verifies the deterministic change: a text edit recalculates paragraph
counters and parent aggregates once, and metadata edits do not recalculate counters.
