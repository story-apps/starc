# AI assistant chat tests

`ai_chat.pro` tests the local SQLite conversations without the cloud service or UI.
It covers reopening the database, isolation by project and account, current document
replacement, unsaved content, history, delayed replies, deletion, renaming and schema migration,
tool routing and stale document protection, errors and coexistence
with writing statistics in `sessions.db`.

`ai_chat_view.pro` links against a built corelib and tests keyboard submission,
multiline input, safe rendering, explicit insertion, read-only documents and chat management.
The assistant uses the existing `ChatMessagesView`, `ChatMessage` and `User` widgets
and types. The tests also check Markdown formatting, partial text selection, command dispatch,
translation confirmation, scrolling long replies and system context messages.
The compact command menu opens a configuration card inside the shared chat view.
Tests cover synopsis length selection, the explicit Start step, cancellation when
the document changes, draft restoration, and clearing history without restoring late replies.

Build each target in its own directory with `qmake /path/to/ai_chat.pro` (or
`ai_chat_view.pro`), followed by `make` and the resulting test executable.
For a headless UI test set `QT_QPA_PLATFORM=offscreen`.
Optionally set `AI_CHAT_SCREENSHOT` to save an interface preview.

The proprietary cloud submodule is required to test actual AI responses. Free-form chat
uses the existing `aiGenerateText` request and `textGenerated` response. Slash commands
use the dedicated existing tool signals. Rephrasing and similar tools reply in chat;
generation and translation apply to the originating document while it remains active.
Full-document translation additionally requires that its content has not changed.
Results always remain in their originating conversation unless it was deleted.
The timeout leaves this untagged transport unavailable until its outstanding reply
arrives or the connection is re-established.

Commands can be chosen from the menu next to the composer; configuration does not
send a request until Start is pressed. Complete slash commands also remain available
for keyboard users. Command syntax is available through `/help` and inline composer hints. For example:

- `/rephrase friendly | An excerpt` replies in chat; `/rephrase friendly` uses the current document.
- `/expand`, `/shorten`, `/summarize` accept an excerpt or default to the current document.
- `/insert preceding text | following text` writes a transition in chat.
- `/translate French | An excerpt` inserts a translated excerpt; `/translate French` confirms replacement of the entire document.
- `/generate --end A scene` writes at the end; `--begin` and `--cursor` are also supported.
- `/synopsis 20`, `/novel`, `/script` invoke the existing document conversion tools where supported.
- `/character` and `/mindmap` use their respective editors. Character flags are `--personal`, `--physique`, `--life`, `--attitude`, `--biography`, `--image`.

## Project context and sources

Each chat remembers its context choice: the open document, the entire project, or
chat history only. Chat-only mode does not invoke document snapshots, project providers
or source retrieval, and does not attach current document metadata. Earlier discussion
is retained, including facts previously discussed about documents. Changing the open
document in this mode does not add context-change notices. Explicit writing commands
still target the open document; the selector controls automatic context for free chat.
Persisted scope values remain compatible: Document = 0, Project = 1, None = 2.
Project context walks the live structure, taking the main document UUID (the latest
working draft), never `item->drafts()` (archived drafts), comparisons or the recycle
bin. Unsaved edits in loaded models are included. Text, information cards, groups,
series story lines and presentation/gallery descriptions are searchable; binary
assets, external links, derived statistics and duplicate text views are excluded.
Writing commands continue to target the open document regardless of research scope.

The project is **not replaced with a summary**. A local in-memory index keeps all
extracted text in overlapping 2,800-character chunks, with document and scene/chapter
UUIDs. Initial evidence and additional `search`, `read` and `catalog` requests are
paginated. Searches cover the entire corpus, including documents outside the first
catalog page. The model can change search terms and read original excerpts, retaining
compact research notes as earlier evidence pages leave the prompt. This avoids losing
late-episode facts through truncation. Search is lexical with prefix matching for
inflections, not an embedding-based semantic index; the model must reformulate queries
or page through documents when needed. No claim of exhaustive review is warranted
merely because a query returned no matches.

Transport includes at most 10,000 characters of recent conversation and a
6,000-character question. In current-document mode, a complete source bundle of up
to 80,000 serialized characters is pinned in every request (within a 96,000-character
prompt budget). This includes all scenes in order and their citation IDs, so an ordinary
episode can be summarized in one AI call without paging. Repeated reads do not duplicate
or evict that bundle. Raw XML is not additionally sent alongside the extracted text.

Larger documents and project searches use a 40-document catalog page and bounded
evidence pages (six excerpts per page, 48 KB of UTF-8 evidence). Research notes from
successive steps are accumulated, at most 3,000 characters per step, rather than replacing
earlier findings. Source coverage reports how many unique chunks of each document have
been supplied over the whole request, distinguishing a pinned full text from pages that
are now represented by research notes. An evicted page is not a missing document page;
notes remain a model-generated summary, not a lossless replacement for original text.
Full chat history stays in SQLite. Research has eight additional retrieval steps, at
most one protocol-repair request and at most one citation-repair request, using the
existing AI transport. Each step is an AI request and may consume the account's word
allowance. The eight-step limit still applies to paginated research; it does not limit
the number of scenes in a complete pinned document. If the budget is exhausted, the
assistant is asked to disclose remaining uncertainty; a further tool request is replaced
with a message asking the user to narrow the question. Exhaustive synthesis of a corpus
larger than these budgets requires additional passes; source coverage must not be
misrepresented as proof that all facts have been understood or retained.

Cloud documents are fetched in batches of up to four before the first AI request.
Missing data fails explicitly after a timeout, rather than being represented as the
whole project. Availability polling does not serialize loaded document content.
Snapshots are captured once all documents are present; unchanged parsed documents are
reused, parsing yields between documents, and inverted-index construction runs off the
GUI thread. Index content is local, transient and rebuilt from project documents;
chat/source metadata stays in `sessions.db` and is not synchronized across devices.

The model cites supplied IDs using `[[source:ID]]`; validated IDs become Markdown links.
The local source table persists document/scene targets across restarts. Link activation
checks the current chat, project and account. It opens the corresponding editor and
selects the scene/chapter; if that scene no longer exists, it opens the document with an
explanation. Deleted documents and unavailable editors produce a visible error. External
links do not trigger browsing or downloads from the shared chat widget.

Session tests cover a 90-episode corpus, late-scene discovery, catalog/read pagination,
metadata extraction, stale-index removal, cloud preparation/cancellation, persisted
scope and citations, source isolation, bounded retrieval, and response routing after a
chat switch. UI tests cover context selection, clicking citations and group navigation
inside folders, in addition to the existing selection and Markdown tests. Live AI
reasoning and cloud-download integration still require the proprietary cloud submodule.

Retrieval replies may contain concatenated JSON objects, fenced objects or arrays.
The session extracts complete commands with a string-aware parser, executes a bounded
batch, and deduplicates identical calls within that reply (ignoring research notes).
Each executed command counts against the eight-step budget; excess commands must be
requested again after the model reads the supplied results. The batch retains request
metadata and pagination offsets. A missing outer UUID brace is recoverable only when
the remaining UUID matches a document in the captured corpus; unknown IDs never fall
back to a different document. Malformed control output gets one format-repair attempt,
then a localized error. Control output and transport separators do not become assistant
messages, and legacy control replies already on disk are excluded from model history.
Tests reproduce concatenated commands with a malformed UUID, duplicate calls, arbitrary
separators, escaped quotes/braces, malformed JSON and exhaustion of the retrieval budget.

Coverage regression tests reproduce an episode of 45 chunks / approximately 48k text
characters, ensuring that its beginning, middle and ending remain available even after
repeated reads. A larger 54-chunk document exercises pagination and verifies that early
findings survive eviction and that complete coverage is recorded only after the final
page. These tests validate context assembly, not the quality of live model summaries.
