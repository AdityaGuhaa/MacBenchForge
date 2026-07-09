#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/// Launch a native macOS window with an embedded WKWebView pointing
/// at the local server.  This function takes over the main thread
/// (runs the NSApplication run-loop) and returns only after the
/// user closes the window or quits the app.
///
/// @param port  The localhost port the HTTP server is listening on.
/// @param on_close  Callback invoked when the window is closed so
///                  the caller can trigger a graceful server shutdown.
void launch_native_window(int port, void (*on_close)(void));

#ifdef __cplusplus
}
#endif
