# Cloud connection and synchronization indicator

These QtTest cases exercise the actual icon and details popup using the built corelib.
No cloud account is required. They cover independent uploads/downloads, concurrent
edits during an upload, reconnecting without losing pending changes, late confirmations
after closing a project, repeated bundle subscriptions with silent server replies,
subscription reset on reconnect/close, server rejection and recovery, confirmation timeouts,
Material Design glyphs, keyboard access, and popup layout after a connection change.
Auxiliary bundle subscriptions may receive no reply under the server protocol; tests
cover 32 silent background subscriptions alongside a confirmed screenplay download,
and ensure a directly requested document still times out when its reply is missing.
The combined panel cases also check hover expansion and delayed collapse, stable
collaborator ordering and navigation, scrolling, small windows, color pulsation,
and one-time problem notifications.

```sh
mkdir -p work/cloud-sync-tests
cd work/cloud-sync-tests
qmake ../../tests/cloud_sync_status/cloud_sync_status.pro
make -j2
QT_QPA_PLATFORM=offscreen ./tst_cloud_sync_status
```

The timeout case takes about 31 seconds. On Linux, add the Qt and `src/_build/libs`
directories to `LD_LIBRARY_PATH` if they are not on the system library search path.
The cloud-enabled application also needs the accompanying `src/cloud` submodule
changes, which expose document-open acknowledgements and upload errors.
