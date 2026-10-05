# Third-party runtime notices

This application dynamically links Qt 6.11.2 Core, Gui, Widgets, Network and Sql,
including the Windows platform/style, QSQLITE, network-information and Schannel plugins
actually listed in the deployment manifest. Qt Concurrent is a build dependency; ship
it only if the actual runtime inventory requires its DLL. Qt is copyright The Qt Company Ltd. and
other contributors. The installed open-source distribution identifies LGPLv3 as an
available basis for these modules. The artifact includes the original Qt LGPLv3/GPLv3
license text in `licenses/Qt-LGPL-GPL.txt`.

Recipients may replace these shared Qt libraries with compatible modified versions,
and may reverse engineer this application for debugging those modifications. No
installer, signature lock, DRM or technical restriction prevents DLL replacement.
Keep the x86_64 MinGW ABI, module dependencies and plugin directory structure compatible.
`qt.conf` resolves plugins relative to this folder. Test replacements in a separate copy.
These notices impose no restriction on rights granted by the third-party licenses.

The complete Qt Base 6.11.2 corresponding source archive, including bundled third-party
sources, licenses and build system, is supplied in `licenses/qtbase-everywhere-src-6.11.2.tar.xz`.
It is obtained from the [official Qt archive](https://download.qt.io/archive/qt/6.11/6.11.2/submodules/)
and checked against its official SHA-256 sidecar. The redistribution audit records
the exact archive and installed binary provenance. This project does not modify Qt.
The installed Qt compiler/configuration metadata and matching source build documentation
are indexed by that audit. For rebuilding Qt see the archive's README and CMake configuration
files and the [Qt build documentation](https://doc.qt.io/qt-6/build-sources.html).

Original installed module attribution pages are copied without editing to
`licenses/qt-attributions/`. They cover embedded third-party code such as PCRE2,
zlib, Unicode data, double-conversion, HarfBuzz, FreeType, libpng/libjpeg, libpsl,
public-suffix data and SQLite. Platform-specific optional entries in those module
pages do not imply that all optional features are shipped. The actual component
inventory and embedded license expressions are in the redistribution audit.

MinGW GCC 13.1.0 runtime libraries (`libgcc_s_seh-1.dll`, `libstdc++-6.dll`) use
GPLv3 with the GCC Runtime Library Exception. Original texts are in
`licenses/GCC-GPLv3.txt` and `licenses/GCC-Runtime-Exception.txt`.
The winpthreads and MinGW runtime notices are included separately. Their source
is the installed MinGW-w64 toolchain; exact binary hashes are in the manifest.
Schannel and the Windows system DLLs are supplied by Windows, not redistributed here.
OpenSSL and Graphviz are not required in this Widgets runtime package.

`DeploymentProbe.exe` is a separate verification utility, not a product workflow.
It uses the same deployed Qt runtime. Delivery status and audit results belong to
generated evidence; this document does not assert that a particular package passed.
