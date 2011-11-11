//
//  AppDelegate.m
//  VoodooHdaSettingsLoader
//
//  Created by Ben on 10/11/11.
//  Copyright (c) 2011 VoodooHDA. All rights reserved.
//

#import "AppDelegate.h"

#import "VoodooHdaSettingsLoader.h"

@implementation AppDelegate

- (void)dealloc
{
    [super dealloc];
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    // Insert code here to initialize your application
    
    VoodooHdaSettingsLoader* loader = [[VoodooHdaSettingsLoader alloc]init];
    [loader load];
    exit(0);
}

@end
