# Story Architect Codex

<p align="center">
  <img width="300" height="300" src="./img/starc.png" alt="Story Architect logo">
</p>

Story Architect Codex is a local, experimental fork of [Story Architect](https://github.com/story-apps/starc) that puts Codex inside the screenplay editor. It keeps the native screenplay, character, location, world, synopsis, treatment, and metadata tools, then adds a project-aware chat assistant that can propose and - only after review - apply structured changes directly to the story.

This is an independently modified beta build, not an official Story Apps release. Keep backups of important work.

## What is included

### A real story-aware chat

- Chat-style writer and Codex messages with scrolling, selection, and partial-text copying.
- `Enter` sends; `Shift+Enter` inserts a new line.
- Visible preparing, connecting, thinking, elapsed-time, and cancellation states.
- A separate conversation and long-term local memory for each screenplay document.
- `New Chat` clears the visible session while preserving older project memory for relevant recall.

### Live story context

Every assistant request receives a request-time snapshot of the current screenplay and its linked STARC data, including:

- logline and project metadata;
- synopsis and treatment;
- character profiles and relationships;
- locations and worlds;
- the full approved screenplay; and
- structured Story Memory, when available.

That means questions such as “What is this screenplay’s logline?” can be answered from the project instead of relying only on text pasted into chat.

### Two selectable story methods

Use the **Story method** menu at the top of the AI Assistant panel:

- **Story continuity (current)** uses the `edit-story` skill to protect canon, author voice, character knowledge, chronology, relationships, world rules, setups, and payoffs.
- **Eric Edson method** uses `eric-edson-story-skill` for Hero Goal sequences, three-act tent poles, Stunning Surprises, and character-function analysis.

The methods are intentionally selectable rather than silently mixed. The active method is saved locally.

### Writer’s Room mode

Enable **Writer’s Room suggestions** to let Codex write alongside you without taking over the editor. After a meaningful burst of writing and a quiet period, it can offer a short room note containing:

1. the strongest recent development;
2. one continuity or structural watchpoint; and
3. one concrete possibility for the next turn.

Notes are advisory only, wait roughly 45 seconds after meaningful activity, and have a five-minute cooldown.

### Direct, format-preserving edits

The assistant uses **STARC Action Protocol V3**: Codex must return one schema-valid action, and STARC validates the action before anything can change.

Supported actions include:

- insert screenplay material at the cursor, beginning, or end;
- replace or delete the request-time selection;
- clear the screenplay after explicit confirmation;
- update the logline, synopsis, or existing treatment paragraphs;
- create, update, remove, or merge native character records;
- create or update character relationships;
- refresh Story Memory; and
- answer questions or suggest ideas without changing the project.

Screenplay generation uses Fountain as the interchange layer and imports it into native STARC paragraph types. Scene headings, action, character cues, parentheticals, dialogue, shots, and transitions remain structured. Codex is instructed to use complete headings such as `INT. KITCHEN - NIGHT`, never labels such as `# Scene 1`.

### Review and continuity safeguards

- Editor-changing actions always require review and approval.
- Long proposals and diffs are scrollable; unrelated keystrokes do not silently reject them.
- A request-time revision check prevents a slow response from overwriting a story that changed while Codex was thinking.
- The Continuity Gate reports story impact and checks character knowledge, chronology, locations, world rules, setups/payoffs, and voice.
- A direct conflict with confirmed canon requires an additional intentional-conflict approval.

### Story Memory and edit history

- **Story Memory** is an evidence-based continuity map for characters, knowledge, timeline, plot threads, setups/payoffs, world rules, voice, and continuity risks.
- Memory is marked stale when the underlying story changes and can be refreshed or corrected by the writer.
- The latest 100 Codex-applied changes per screenplay are kept in local edit history with date, method, instruction, summary, before/after state, impact, and continuity findings.
- Earlier screenplay, logline, synopsis, treatment, character, and relationship states can be reviewed and restored where supported.

### Character safety and merge rollback

- Character removal first shows a dependency report and moves the profile to STARC’s Recycle Bin; it does not search-and-delete screenplay prose.
- A character merge requires a surviving character, a duplicate, and explicit resolutions for conflicting profile fields.
- The merge preserves the survivor’s stable ID, transfers unique photos and relationships, reassigns native character cues across supported script documents, and preserves the duplicate in Recycle Bin.
- A transaction journal snapshots every affected character and script document. Edit history can roll the complete merge back to its pre-merge state.

### Stability and navigation work

This fork also includes fixes for opening existing screenplays and navigating Treatment, Characters, Locations, Worlds, Statistics, and related module views. Character, Location, and World containers remain visible even when empty.

Some advanced module implementations are separate upstream submodules or locally restored prebuilt plugins. They are not included as binaries in this repository; see [Optional plugin modules](#optional-plugin-modules).

## How the assistant works

```text
Writer request
    |
    v
Live screenplay + linked STARC tabs + relevant local memory
    |
    v
Local `codex app-server` process + selected story skill
    |
    v
One validated STARC Action Protocol V3 response
    |
    +--> conversational answer -> chat
    |
    `--> proposed change -> review + Continuity Gate -> writer approval -> native editor
                                                        |
                                                        `--> local edit history
```

The app does not use the ChatGPT web UI. It starts the locally installed Codex CLI and therefore uses the OpenAI account authenticated in that CLI. The legacy **words available / purchase** text belongs to the original STARC AI interface and is not a reliable display of Codex quota.

## Quick start on macOS

### 1. Install and authenticate Codex

Make sure `codex` is installed and signed in, then verify:

```bash
codex --version
codex app-server --help
```

The app searches `STARC_CODEX_BIN`, `PATH`, `~/.local/bin/codex`, `/opt/homebrew/bin/codex`, and `/usr/local/bin/codex`, in that order.

### 2. Make the story skills available

The local build expects a workspace containing `.agents/skills`. Set it explicitly when the skills are not in `~/Documents/GitHub/AI-Storyboard`:

```bash
export STARC_CODEX_WORKSPACE="/absolute/path/to/AI-Storyboard"
```

That workspace currently supplies:

- `.agents/skills/edit-story/SKILL.md`
- `.agents/skills/eric-edson-story-skill/SKILL.md`
- `.agents/skills/create-storyboard/SKILL.md` for storyboard requests

### 3. Open a `.starc` project

Open the screenplay, select **AI Assistant**, choose a story method, and chat normally. Use direct verbs when you want an editor action:

```text
Generate a new scene at the end in which Mara discovers the forged passport.

Replace the selected dialogue so the detective hides her fear without stating it.

Update the logline to emphasize the cost of exposing the conspiracy.

Remove the character named OLD JOHN from the Characters tab.
```

Questions and idea requests remain conversational:

```text
What is the current logline?

Which setup in Act One still needs a payoff?

Give me three possible complications for the next scene.
```

## Saving, memory, and privacy

- Story projects are saved in user-selected `.starc` files. Auto-save is enabled by default.
- Backups are enabled by default, with seven retained in `~/Documents/starc/backups` unless changed in Settings.
- Chat, Story Memory, edit history, selected method, and merge journals are stored in local Qt settings keyed by screenplay UUID; they are not embedded in the `.starc` file.
- On macOS, Qt normally stores these settings under the Story Apps / Story Architect preferences domain in `~/Library/Preferences`.
- Moving only a `.starc` file to another computer does not move its Codex chat or edit history.
- Requests include the live story package and are sent through the authenticated local Codex CLI to OpenAI. Do not use the assistant for material you are not permitted to send to that service.
- `.starc`, Fountain, Final Draft, credentials, logs, crash reports, local assistant state, and prebuilt binaries are ignored by this repository’s `.gitignore`.

## Build from source

This is a Qt/qmake C++17 project. The current local macOS build has been exercised with Qt 6; upstream workflows also describe Qt 5 and Qt 6 builds.

```bash
git clone <your-fork-url> starc
cd starc
git submodule update --init --recursive
cd src
qmake starc.pro
make -j4
```

The app bundle is produced under `src/_build/starcapp.app`.

For a distributable macOS bundle, deployment must happen **after the final link** and must use the same Qt installation that built the app:

```bash
macdeployqt src/_build/starcapp.app -always-overwrite
codesign --force --deep --sign - src/_build/starcapp.app
```

Before launching, verify that the executable, `libcorelib`, and plugins do not still reference a different Homebrew Qt runtime:

```bash
otool -L src/_build/starcapp.app/Contents/MacOS/starcapp
otool -L src/_build/starcapp.app/Contents/Frameworks/libcorelib.dylib
```

Mixing bundled Qt frameworks with plugins linked to `/opt/homebrew` was the cause of a previous startup crash. Re-run `macdeployqt` after relinking any core library or plugin, then sign the final bundle again.

For an iterative local update that preserves compatible optional plugins already present in an installed bundle, the canonical repository includes:

```bash
scripts/install-local-codex-build-macos.sh \
  src/_build/starcapp.app \
  "/Applications/Story Architect Codex.app"
```

The script copies the rebuilt core library, core plugin, and screenplay plugin into the existing app; rewrites absolute Qt framework references to the bundled frameworks; refuses any remaining non-bundled Qt dependency; and ad-hoc signs and verifies the result. The target app must already exist and be compatible. Back it up before using this developer shortcut.

### Optional plugin modules

The upstream repository declares several feature modules as Git submodules, including character details/relations, location details/map, worlds, statistics, cards, timelines, breakdown, presentation, and mind map. A public checkout may not have access to all of them.

Locally restored prebuilt `.dylib` plugins can make those modules available, but they are machine-specific and may have separate licensing. They belong in `.local-prebuilt-plugins/` and must not be committed or redistributed without permission. A source-only public build should treat missing optional modules as unavailable rather than bundling private binaries.

## Release checklist

Before publishing a build:

1. initialize every accessible submodule;
2. build from a clean generated-file state;
3. restore only legally distributable optional plugins;
4. run `macdeployqt` after the final link;
5. verify all Mach-O dependency paths;
6. sign the completed app bundle;
7. launch, create a blank project, and reopen an existing project;
8. test Screenplay, Synopsis, Treatment, Characters, Relations, Locations, Map, Worlds, and Statistics;
9. test chat send/copy/scroll/cancel, both story methods, Writer’s Room, and Story Memory;
10. test one approved direct insertion, one rejected proposal, stale-response protection, edit restoration, and character-merge rollback; and
11. confirm no project files, credentials, logs, crash reports, local settings, or prebuilt binaries are staged.

## Troubleshooting

| Symptom | Check |
| --- | --- |
| The app does not launch | Inspect the macOS crash report and verify all Qt/plugin dependency paths with `otool -L`. Re-deploy and re-sign the final bundle. |
| “Codex CLI was not found” | Install Codex or set `STARC_CODEX_BIN` to the executable’s absolute path. |
| A story method is missing | Verify `STARC_CODEX_WORKSPACE` and the corresponding `.agents/skills/<name>/SKILL.md`. |
| Character/location sub-tabs are absent | Initialize accessible submodules or restore legally obtained compatible plugins; check that the plugin loaded. |
| Codex answers instead of editing | Use an explicit direct-edit instruction and specify the target or select text first. |
| A proposal is refused as stale | The story changed after the request began. Send the request again from the current draft. |
| Chat history is missing on another Mac | History is local Qt settings, not part of the `.starc` project package. |
| A large request takes a long time | The full story package is large. Keep the app open, watch the activity state, or cancel and request a smaller scene/section. |

## Documentation

- [Complete user and maintainer guide](docs/Story_Architect_Codex_Guide.md)
- [Printable PDF handbook](output/pdf/Story_Architect_Codex_Guide.pdf)

To regenerate the PDF after editing the Markdown guide:

```bash
python3 -m pip install reportlab
python3 docs/generate_pdf.py
```

## Current limitations

- This remains beta software; preserve independent project backups.
- A one-prompt feature screenplay is possible in principle but is not the recommended workflow. Generate and review in sequences or scenes so continuity and format remain inspectable.
- Chat, Story Memory, and edit history are local to one machine.
- Codex response time and available usage depend on the authenticated Codex account and request size.
- Some advanced STARC modules are not present in the public source checkout.
- Continuity checks are safeguards, not a substitute for the writer’s review.

## Upstream and license

Story Architect was created by Story Apps, the team behind [KIT Scenarist](https://github.com/dimkanovikov/KITScenarist). This fork retains the upstream project’s [GNU General Public License v3](LICENSE). Modified builds should be clearly identified as modified and must comply with the license and any separate terms governing optional modules or third-party dependencies.

For the official project, visit [starc.app](https://starc.app/) or the [upstream repository](https://github.com/story-apps/starc).
