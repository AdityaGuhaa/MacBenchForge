// ─── native_window.mm ────────────────────────────────────────────────────────
// Creates a native macOS NSWindow with an embedded WKWebView that displays
// the MacBenchForge dashboard.  Uses AppKit + WebKit frameworks — both ship
// with every Mac, so there are zero additional dependencies.
// ─────────────────────────────────────────────────────────────────────────────

#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>
#include "native_window.h"

// ─── Application Delegate ────────────────────────────────────────────────────
// Manages the app lifecycle and creates the main window.

@interface MBFAppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate, WKNavigationDelegate>
@property (nonatomic) int port;
@property (nonatomic) void (*onClose)(void);
@property (nonatomic, strong) NSWindow *window;
@property (nonatomic, strong) WKWebView *webView;
@end

@implementation MBFAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    // ── Window ───────────────────────────────────────────────────────────────
    NSRect frame = NSMakeRect(0, 0, 1280, 800);
    NSWindowStyleMask style = NSWindowStyleMaskTitled
                            | NSWindowStyleMaskClosable
                            | NSWindowStyleMaskMiniaturizable
                            | NSWindowStyleMaskResizable;

    self.window = [[NSWindow alloc] initWithContentRect:frame
                                              styleMask:style
                                                backing:NSBackingStoreBuffered
                                                  defer:NO];
    [self.window setTitle:@"MacBenchForge"];
    [self.window setMinSize:NSMakeSize(960, 600)];
    [self.window center];
    [self.window setDelegate:self];

    // ── WebView Configuration ────────────────────────────────────────────────
    WKWebViewConfiguration *config = [[WKWebViewConfiguration alloc] init];

    // Allow media autoplay (for any future visualizations)
    config.mediaTypesRequiringUserActionForPlayback = WKAudiovisualMediaTypeNone;

    // Enable developer tools (right-click → Inspect Element) in debug builds
#ifdef DEBUG
    [config.preferences setValue:@YES forKey:@"developerExtrasEnabled"];
#endif

    self.webView = [[WKWebView alloc] initWithFrame:self.window.contentView.bounds
                                      configuration:config];
    self.webView.navigationDelegate = self;
    self.webView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

    // Make the title bar blend with the page background for a cleaner look
    self.window.titlebarAppearsTransparent = YES;
    self.window.titleVisibility = NSWindowTitleVisible;

    [self.window.contentView addSubview:self.webView];

    // ── Load Dashboard ───────────────────────────────────────────────────────
    NSString *urlStr = [NSString stringWithFormat:@"http://127.0.0.1:%d", self.port];
    NSURL *url = [NSURL URLWithString:urlStr];
    NSURLRequest *request = [NSURLRequest requestWithURL:url];
    [self.webView loadRequest:request];

    [self.window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}

// ── Window Close → Quit App ──────────────────────────────────────────────────

- (BOOL)windowShouldClose:(NSWindow *)sender {
    if (self.onClose) {
        self.onClose();
    }
    return YES;
}

- (void)windowWillClose:(NSNotification *)notification {
    [NSApp terminate:nil];
}

// ── Handle Navigation Errors (e.g. server not ready yet) ─────────────────────

- (void)webView:(WKWebView *)webView
    didFailProvisionalNavigation:(WKNavigation *)navigation
              withError:(NSError *)error {
    // Server may not be ready yet — retry after a short delay
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.5 * NSEC_PER_SEC)),
                   dispatch_get_main_queue(), ^{
        NSString *urlStr = [NSString stringWithFormat:@"http://127.0.0.1:%d", self.port];
        NSURL *url = [NSURL URLWithString:urlStr];
        [webView loadRequest:[NSURLRequest requestWithURL:url]];
    });
}

// ── Standard Quit Behavior ───────────────────────────────────────────────────

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

@end

// ─── Menu Bar ────────────────────────────────────────────────────────────────
// Creates a minimal but functional macOS menu bar.

static void create_menu_bar(void) {
    NSMenu *mainMenu = [[NSMenu alloc] init];

    // App menu (MacBenchForge)
    NSMenuItem *appMenuItem = [[NSMenuItem alloc] init];
    NSMenu *appMenu = [[NSMenu alloc] initWithTitle:@"MacBenchForge"];
    [appMenu addItemWithTitle:@"About MacBenchForge"
                       action:@selector(orderFrontStandardAboutPanel:)
                keyEquivalent:@""];
    [appMenu addItem:[NSMenuItem separatorItem]];
    [appMenu addItemWithTitle:@"Quit MacBenchForge"
                       action:@selector(terminate:)
                keyEquivalent:@"q"];
    [appMenuItem setSubmenu:appMenu];
    [mainMenu addItem:appMenuItem];

    // Edit menu (enables Cmd+C, Cmd+V, etc. inside the WebView)
    NSMenuItem *editMenuItem = [[NSMenuItem alloc] init];
    NSMenu *editMenu = [[NSMenu alloc] initWithTitle:@"Edit"];
    [editMenu addItemWithTitle:@"Undo"   action:@selector(undo:)             keyEquivalent:@"z"];
    [editMenu addItemWithTitle:@"Redo"   action:@selector(redo:)             keyEquivalent:@"Z"];
    [editMenu addItem:[NSMenuItem separatorItem]];
    [editMenu addItemWithTitle:@"Cut"    action:@selector(cut:)              keyEquivalent:@"x"];
    [editMenu addItemWithTitle:@"Copy"   action:@selector(copy:)             keyEquivalent:@"c"];
    [editMenu addItemWithTitle:@"Paste"  action:@selector(paste:)            keyEquivalent:@"v"];
    [editMenu addItemWithTitle:@"Select All" action:@selector(selectAll:)    keyEquivalent:@"a"];
    [editMenuItem setSubmenu:editMenu];
    [mainMenu addItem:editMenuItem];

    // Window menu
    NSMenuItem *windowMenuItem = [[NSMenuItem alloc] init];
    NSMenu *windowMenu = [[NSMenu alloc] initWithTitle:@"Window"];
    [windowMenu addItemWithTitle:@"Minimize" action:@selector(performMiniaturize:) keyEquivalent:@"m"];
    [windowMenu addItemWithTitle:@"Zoom"     action:@selector(performZoom:)        keyEquivalent:@""];
    [windowMenuItem setSubmenu:windowMenu];
    [mainMenu addItem:windowMenuItem];

    [NSApp setMainMenu:mainMenu];
    [NSApp setWindowsMenu:windowMenu];
}

// ─── Public C Entry Point ────────────────────────────────────────────────────

void launch_native_window(int port, void (*on_close)(void)) {
    @autoreleasepool {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

        create_menu_bar();

        MBFAppDelegate *delegate = [[MBFAppDelegate alloc] init];
        delegate.port = port;
        delegate.onClose = on_close;
        [NSApp setDelegate:delegate];

        [NSApp run]; // Blocks until app terminates
    }
}
