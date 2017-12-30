#import <Cocoa/Cocoa.h>
#import "GBJoystickListener.h"

@interface GBPreferencesWindow : NSWindow <NSTableViewDelegate, NSTableViewDataSource, GBJoystickListener>
@property IBOutlet NSTableView *controlsTableView;
@property IBOutlet NSPopUpButton *graphicsFilterPopupButton;
@property (strong) IBOutlet NSButton *aspectRatioCheckbox;
@property (strong) IBOutlet NSPopUpButton *highpassFilterPopupButton;
@property (strong) IBOutlet NSPopUpButton *colorCorrectionPopupButton;
@property (strong) IBOutlet NSButton *configureJoypadButton;
@property (strong) IBOutlet NSButton *skipButton;
@end
