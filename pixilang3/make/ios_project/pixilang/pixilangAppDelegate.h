//
//  pixilangAppDelegate.h
//  pixilang
//
//  Created by alex on 21.03.11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>

@class MainViewController;

@interface pixilangAppDelegate : NSObject <UIApplicationDelegate> 
{
    UIWindow* window;
    MainViewController* mainViewController;
}

@property (nonatomic, retain) IBOutlet UIWindow* window;
@property (nonatomic, retain) IBOutlet MainViewController* mainViewController;

@end
