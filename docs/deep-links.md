# Deep links

Public links use HTTPS so they also work when Story Architect is not installed:

```text
https://starc.app/open/
https://starc.app/open/?teamId=12
https://starc.app/open/?projectId=34
https://starc.app/open/?teamId=12&projectId=34&documentGuid=7a46aa60-9fc3-4d43-a883-b3131f25a4d3
```

The landing page translates the parameters to `starc://open?...`. The desktop app
accepts both URL forms. IDs are positive 32-bit integers; document GUIDs use the
canonical hyphenated form without braces. Unknown/duplicate parameters, fragments,
credentials, ports and malformed identifiers are rejected. A document requires a
project ID. A team is optional for personal or shared projects; if specified with a
project, the project must belong to that team. No parameters opens the projects list.

## Application behavior

- CLI launches on Windows/Linux and `QFileOpenEvent` URLs on macOS use one parser.
- URL launches are handed to the running instance through a user-only local socket.
  Ordinary file launches remain independent. Early macOS events wait for startup.
- A link received before sign-in is retained until login completes. A newer link
  replaces a pending one; logging out cancels it.
- Navigation refreshes teams/projects from the server, checks membership and project
  ownership by team, then uses the existing save-before-switch flow.
- Team-only links expand/select the team in the projects list.
- Document links wait for the cloud structure response, then resolve the GUID using
  the navigator's permission filter (including drafts and read-only access).
- Unavailable resources show an access notification. Connection timeouts show a
  different message. Community builds without the cloud module explain that cloud
  links require the cloud-enabled build. Links never enter the local file importer.

## Deployment (required for public links)

Deploy **both files in `web/open/`** to `https://starc.app/open/`. Ensure `/open`
redirects to `/open/` **without dropping the query string**. Serve `open.js` with a
JavaScript content type. This repository does not contain the site's deployment
configuration; adding these files to the desktop repository does not publish them.
The fallback is exactly `https://starc.app/downloads/`.

The page attempts to open the native scheme and redirects to downloads after 2.5 s
if it stays active. Leaving the page or opening the browser/OS confirmation prompt
cancels the timer. Explicit Open and Download links remain available. Browsers do
not expose a reliable installed-application check: automatic launches may require
a user gesture, and canceling an OS prompt may leave the landing page visible.
Do not advertise guaranteed silent detection or deferred navigation after installation.

## Platform registration

- **Windows:** all three NSIS installers register `starc` with a quoted executable
  and URL. Uninstall removes the registration only if it still belongs to that
  installation. Portable archives do not run an installer; use the installed build
  or register the same `starc` URL protocol manually for the portable executable.
- **macOS:** the app bundle declares `CFBundleURLTypes` for `starc`. Install the app
  bundle normally so Launch Services discovers it.
- **Linux:** the desktop entry uses `%u` and declares `x-scheme-handler/starc`.
  AppImages register their persistent `APPIMAGE` path in the user's applications
  directory on first launch and use `xdg-mime` to associate the scheme. Launch the
  AppImage once after moving it. For distro packages, install the desktop entry and
  refresh the desktop database through the package's normal installation process.

Publish an updated desktop build as well as the landing page. Existing releases
without this handler cannot open the new protocol.

## Automated verification

With Qt 5 or Qt 6 and a C++17 compiler:

```sh
mkdir -p work/deep-link-tests
cd work/deep-link-tests
qmake ../../tests/deep_links/deep_links.pro
make -j4
QT_QPA_PLATFORM=offscreen ./tst_deep_links
cd ../..
node --test tests/deep_links/web.test.cjs
```

The Qt suite covers URL validation, parsed values, cold/warm macOS URL events,
file URL preservation, and actual cross-process delivery. Its IPC test needs
permission to create/connect local sockets; a sandbox that blocks sockets cannot
run that case. It uses isolated app data and does not contact a running user app.
The Node suite covers parameter forwarding, download fallback, focus/visibility
transitions, retry and invalid URLs.

Before release, test the packaged builds on Windows, macOS and Linux with the
published landing page: installed/uninstalled, cold/warm launch, browser prompt
accept/cancel, logged in/out, personal/team projects, allowed/read-only/denied
and removed documents, drafts, unsaved-project save/cancel, rapid successive links,
offline/reconnect and logout. Live cloud navigation requires the private `src/cloud`
submodule and an authorized test account. `DocumentInfo` currently has no project
ID; document delivery uses the cloud module's existing current-project routing.
