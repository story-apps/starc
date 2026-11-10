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

Command syntax is available through `/help` and inline composer hints. For example:

- `/rephrase friendly | An excerpt` replies in chat; `/rephrase friendly` uses the current document.
- `/expand`, `/shorten`, `/summarize` accept an excerpt or default to the current document.
- `/insert preceding text | following text` writes a transition in chat.
- `/translate French | An excerpt` inserts a translated excerpt; `/translate French` confirms replacement of the entire document.
- `/generate --end A scene` writes at the end; `--begin` and `--cursor` are also supported.
- `/synopsis 20`, `/novel`, `/script` invoke the existing document conversion tools where supported.
- `/character` and `/mindmap` use their respective editors. Character flags are `--personal`, `--physique`, `--life`, `--attitude`, `--biography`, `--image`.
