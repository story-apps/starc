# AI assistant chat tests

`ai_chat.pro` tests the local SQLite conversations without the cloud service or UI.
It covers reopening the database, isolation by project and account, current document
replacement, unsaved content, history, delayed replies, deletion, errors and coexistence
with writing statistics in `sessions.db`.

`ai_chat_view.pro` links against a built corelib and tests keyboard submission,
multiline input, safe rendering, explicit insertion, read-only documents and chat management.

Build each target in its own directory with `qmake /path/to/ai_chat.pro` (or
`ai_chat_view.pro`), followed by `make` and the resulting test executable.
For a headless UI test set `QT_QPA_PLATFORM=offscreen`.
Optionally set `AI_CHAT_SCREENSHOT` to save an interface preview.

The proprietary cloud submodule is required to test actual AI responses. The chat
uses the existing `aiGenerateText` request and `textGenerated` response. Chat results
are consumed before the editor's generation handler, and are never inserted automatically.
The timeout leaves this untagged transport unavailable until its outstanding reply
arrives or the connection is re-established.
