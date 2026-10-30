# Cloud connection and synchronization indicator

These QtTest cases exercise the actual icon and details popup using the built corelib.
No cloud account is required. They cover independent uploads/downloads, concurrent
edits during an upload, reconnecting without losing pending changes, late confirmations
after closing a project, server rejection and recovery, confirmation timeouts,
Material Design glyphs, keyboard access, and popup layout after a connection change.

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
