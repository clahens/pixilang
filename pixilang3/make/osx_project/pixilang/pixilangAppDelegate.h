#import <Cocoa/Cocoa.h>

@interface pixilangAppDelegate : NSObject <NSApplicationDelegate> 
{
    NSView* subview;
    
    IBOutlet NSWindow* window;
    IBOutlet NSView* view;
}

@end
