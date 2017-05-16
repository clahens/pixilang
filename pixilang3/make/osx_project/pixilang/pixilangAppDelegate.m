#import "pixilangAppDelegate.h"

#include "various/osx/sundog_bridge.h"

@implementation pixilangAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
    subview = [ [ [ MyView alloc ] initWithFrame: [ view frame ] ] autorelease ];
    [ subview setAutoresizingMask: 1 | 2 | 4 | 8 | 16 | 32 ];
    [ view setAutoresizesSubviews: YES ];
    [ view addSubview: subview ];
    
    sundog_init( subview );
}

- (void)applicationWillTerminate:(NSNotification *)notification 
{
    sundog_deinit();
}

- (void)dealloc 
{
    [subview release];
    [super dealloc];
}

@end
