//
//  VoodooHDAPref.h
//  VoodooHDA
//
//  Created by fassl on 15.04.09.
//  Copyright (c) 2009 __MyCompanyName__. All rights reserved.
//


#import <PreferencePanes/PreferencePanes.h>

#include <IOKit/IOKitLib.h>
#define kVoodooHDAClassName	"VoodooHDADevice"

typedef union {
	struct {
		UInt8 action;
		UInt8 channel;
		UInt8 device;
		UInt8 val;
	} info;
	UInt32 value;
} actionInfo;

typedef struct _mixerDeviceInfo {
	UInt8 mixId;
	UInt8 value;
	char name[32];
	bool enabled;
} mixerDeviceInfo;

typedef struct _ChannelInfo {
	char name[32];
	mixerDeviceInfo mixerValues[24];
	int numChannels;
} ChannelInfo;

enum {
    kVoodooHDAActionMethod = 0,
    kVoodooHDANumMethods
};

enum {
	kVoodooHDAChannelNames = 0x3000
};

enum {
	kVoodooHDAActionSetMixer = 0x40
};

@interface VoodooHDAPref : NSPreferencePane 
{
    IBOutlet NSTextField *versionText;
    IBOutlet NSSlider *sliderAltPCM;
    IBOutlet NSSlider *sliderBass;
    IBOutlet NSSlider *sliderCD;
    IBOutlet NSSlider *sliderDigital1;
    IBOutlet NSSlider *sliderDigital2;
    IBOutlet NSSlider *sliderDigital3;
    IBOutlet NSSlider *sliderIGain;
    IBOutlet NSSlider *sliderIMix;
    IBOutlet NSSlider *sliderLine;
    IBOutlet NSSlider *sliderLine1;
    IBOutlet NSSlider *sliderLine2;
    IBOutlet NSSlider *sliderLine3;
    IBOutlet NSSlider *sliderMic;
    IBOutlet NSSlider *sliderMonitor;
    IBOutlet NSSlider *sliderOGain;
    IBOutlet NSSlider *sliderPCM;
    IBOutlet NSSlider *sliderPhoneIn;
    IBOutlet NSSlider *sliderPhoneOut;
    IBOutlet NSSlider *sliderRadio;
    IBOutlet NSSlider *sliderRecLev;
    IBOutlet NSSlider *sliderSpeaker;
    IBOutlet NSSlider *sliderSynth;
    IBOutlet NSSlider *sliderTreble;
    IBOutlet NSSlider *sliderVideo;
    IBOutlet NSPopUpButton *selector;
	
	UInt8 currentChannel;
	ChannelInfo *chInfo;
	io_service_t service;
	io_connect_t connect;

}
//- (bool) updateChannelInfo;
- (bool) updateSliders;
- (void) awakeFromNib;
- (void) didUnselect;
- (bool) saveSettings;
- (IBAction)sliderMoved:(NSSlider *)sender;
- (IBAction)selectorChanged:(NSPopUpButton *)sender;
- (IBAction)enableAllSLiders:(NSButton *)sender;

- (void) changeVersionText;

@end
