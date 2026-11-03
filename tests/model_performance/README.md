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

QtTest benchmarks cover lookup of 20,000 sibling rows, word counting, forced paragraph
counter updates and loading a scene containing 1,000 paragraphs. The chronometry
benchmark directly compares separate and shared layouts within the same process.
For more stable measurements, run with `-median 5` on an otherwise idle machine.
Timings are diagnostic; there are no hardware-dependent pass/fail thresholds.

Also run `tests/remote_text_changes` after modifying the common model tree or XML
loading: it exercises real editors, cloud patches, model resets and cursor restoration.
