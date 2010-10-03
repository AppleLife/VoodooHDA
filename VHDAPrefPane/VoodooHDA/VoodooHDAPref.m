//
//  VoodooHDAPref.m
//  VoodooHDA
//
//  Created by fassl on 15.04.09.
//  Copyright (c) 2009 Voodoo Team. All rights reserved.
//
// Modded by Slice 2010

#import "VoodooHDAPref.h"
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/IOKitLib.h>

@implementation VoodooHDAPref

io_service_t getService() {
	io_service_t service = 0;
	mach_port_t masterPort;
	io_iterator_t iter;
	kern_return_t ret;
	io_string_t path;
	
	ret = IOMasterPort(MACH_PORT_NULL, &masterPort);
	if (ret != KERN_SUCCESS) {
		NSRunCriticalAlertPanel( 
								NSLocalizedString( @"Error", "MsgBox"), 
								NSLocalizedString( @"Can't get masterport", "MsgBoxBody" ), nil, nil, nil );		
		goto failure;
	}
	
	ret = IOServiceGetMatchingServices(masterPort, IOServiceMatching(kVoodooHDAClassName), &iter);
	if (ret != KERN_SUCCESS) {
		NSRunCriticalAlertPanel( 
								NSLocalizedString( @"Error", "MsgBox"), 
								NSLocalizedString( @"VoodooHDA is not running", "MsgBoxBody" ), nil, nil, nil );		
		goto failure;
	}
	
	service = IOIteratorNext(iter);
	IOObjectRelease(iter);
	
	ret = IORegistryEntryGetPath(service, kIOServicePlane, path);
	if (ret != KERN_SUCCESS) {
		NSRunCriticalAlertPanel( 
								NSLocalizedString( @"Error", "MsgBox"), 
								NSLocalizedString( @"Can't get registry-entry path", "MsgBoxBody" ), nil, nil, nil );		
		goto failure;
	}
	
failure:
	return service;
}	
//get channel info from driver
ChannelInfo *updateChannelInfo() {
	io_service_t service = getService();
	if(!service)
		goto failure;
	
	kern_return_t ret;
	io_connect_t connect = 0;
	ChannelInfo *info = 0;
#if __LP64__
	mach_vm_address_t address;
	mach_vm_size_t size;	
#else	
	vm_address_t address;
	vm_size_t size;
#endif	
	
	ret = IOServiceOpen(service, mach_task_self(), 0, &connect);
	if (ret != KERN_SUCCESS) {
		NSRunCriticalAlertPanel( 
								NSLocalizedString( @"Error", "MsgBox"), 
								NSLocalizedString( @"Can't open IO Service", "MsgBoxBody" ), nil, nil, nil );		
		goto failure;
	}
	
	ret = IOConnectMapMemory(connect, kVoodooHDAChannelNames, mach_task_self(), &address, &size,
							 kIOMapAnywhere | kIOMapDefaultCache);
	if (ret != kIOReturnSuccess) {
		NSRunCriticalAlertPanel( 
								NSLocalizedString( @"Error", "MsgBox"), 
								NSLocalizedString( @"Can't map Memory", "MsgBoxBody" ), nil, nil, nil );		
		goto failure;
	}
	
	info = (ChannelInfo*)address;
	
failure:
#if 0
	if(connect) {
		ret = IOServiceClose(connect);
		if (ret != KERN_SUCCESS) {
			NSRunCriticalAlertPanel( 
									NSLocalizedString( @"Error", "MsgBox"), 
									NSLocalizedString( @"IOServiceClose failed", "MsgBoxBody" ), nil, nil, nil );		
		}
	}
#endif
	if(service)
		IOObjectRelease(service);
	
	return info;
	//return (ret == KERN_SUCCESS || ret == kIOReturnSuccess)?true:false;	
}	

- (bool) updateSliders {
	
	if (!(self->chInfo = updateChannelInfo()))  //from driver
		goto failure;

	ChannelInfo *info = (ChannelInfo*)self->chInfo;
	if (!info)
		goto failure;
	
	if(!(info[currentChannel].mixerValues[0].enabled))
		[sliderBass setEnabled:FALSE];
	else
		[sliderBass setEnabled:TRUE];
	[sliderBass setIntValue:info[currentChannel].mixerValues[0].value];
	
	if(!(info[currentChannel].mixerValues[1].enabled))
		[sliderTreble setEnabled:FALSE];
	else
		[sliderTreble setEnabled:TRUE];
	[sliderTreble setIntValue:info[currentChannel].mixerValues[1].value];
	
	if(!(info[currentChannel].mixerValues[2].enabled))
		[sliderSynth setEnabled:FALSE];
	else
		[sliderSynth setEnabled:TRUE];
	[sliderSynth setIntValue:info[currentChannel].mixerValues[2].value];
	
	if(!(info[currentChannel].mixerValues[3].enabled))
		[sliderPCM setEnabled:FALSE];
	else
		[sliderPCM setEnabled:TRUE];
	[sliderPCM setIntValue:info[currentChannel].mixerValues[3].value];
	
	if(!(info[currentChannel].mixerValues[4].enabled))
		[sliderSpeaker setEnabled:FALSE];
	else
		[sliderSpeaker setEnabled:TRUE];
	[sliderSpeaker setIntValue:info[currentChannel].mixerValues[4].value];
	
	if(!(info[currentChannel].mixerValues[5].enabled))
		[sliderLine setEnabled:FALSE];
	else
		[sliderLine setEnabled:TRUE];
	[sliderLine setIntValue:info[currentChannel].mixerValues[5].value];
	
	if(!(info[currentChannel].mixerValues[6].enabled))
		[sliderMic setEnabled:FALSE];
	else
		[sliderMic setEnabled:TRUE];
	[sliderMic setIntValue:info[currentChannel].mixerValues[6].value];
	
	if(!(info[currentChannel].mixerValues[7].enabled))
		[sliderCD setEnabled:FALSE];
	else
		[sliderCD setEnabled:TRUE];
	[sliderCD setIntValue:info[currentChannel].mixerValues[7].value];
	
	if(!(info[currentChannel].mixerValues[8].enabled))
		[sliderIMix setEnabled:FALSE];
	else
		[sliderIMix setEnabled:TRUE];
	[sliderIMix setIntValue:info[currentChannel].mixerValues[8].value];
	
	if(!(info[currentChannel].mixerValues[9].enabled))
		[sliderAltPCM setEnabled:FALSE];
	else
		[sliderAltPCM setEnabled:TRUE];
	[sliderAltPCM setIntValue:info[currentChannel].mixerValues[9].value];
	
	if(!(info[currentChannel].mixerValues[10].enabled))
		[sliderRecLev setEnabled:FALSE];
	else
		[sliderRecLev setEnabled:TRUE];
	[sliderRecLev setIntValue:info[currentChannel].mixerValues[10].value];
	
	if(!(info[currentChannel].mixerValues[11].enabled))
		[sliderIGain setEnabled:FALSE];
	else
		[sliderIGain setEnabled:TRUE];
	[sliderIGain setIntValue:info[currentChannel].mixerValues[11].value];
	
	if(!(info[currentChannel].mixerValues[12].enabled))
		[sliderOGain setEnabled:FALSE];
	else
		[sliderOGain setEnabled:TRUE];
	[sliderOGain setIntValue:info[currentChannel].mixerValues[12].value];
	
	if(!(info[currentChannel].mixerValues[13].enabled))
		[sliderLine1 setEnabled:FALSE];
	else
		[sliderLine1 setEnabled:TRUE];
	[sliderLine1 setIntValue:info[currentChannel].mixerValues[13].value];
	
	if(!(info[currentChannel].mixerValues[14].enabled))
		[sliderLine2 setEnabled:FALSE];
	else
		[sliderLine2 setEnabled:TRUE];
	[sliderLine2 setIntValue:info[currentChannel].mixerValues[14].value];
	
	if(!(info[currentChannel].mixerValues[15].enabled))
		[sliderLine3 setEnabled:FALSE];
	else
		[sliderLine3 setEnabled:TRUE];
	[sliderLine3 setIntValue:info[currentChannel].mixerValues[15].value];
	
	if(!(info[currentChannel].mixerValues[16].enabled))
		[sliderDigital1 setEnabled:FALSE];
	else
		[sliderDigital1 setEnabled:TRUE];
	[sliderDigital1 setIntValue:info[currentChannel].mixerValues[16].value];
	
	if(!(info[currentChannel].mixerValues[17].enabled))
		[sliderDigital2	setEnabled:FALSE];
	else
		[sliderDigital2 setEnabled:TRUE];
	[sliderDigital2 setIntValue:info[currentChannel].mixerValues[17].value];
	
	if(!(info[currentChannel].mixerValues[18].enabled))
		[sliderDigital3 setEnabled:FALSE];
	else
		[sliderDigital3 setEnabled:TRUE];
	[sliderDigital3 setIntValue:info[currentChannel].mixerValues[18].value];
	
	if(!(info[currentChannel].mixerValues[19].enabled))
		[sliderPhoneIn setEnabled:FALSE];
	else
		[sliderPhoneIn setEnabled:TRUE];
	[sliderPhoneIn setIntValue:info[currentChannel].mixerValues[19].value];
	
	if(!(info[currentChannel].mixerValues[20].enabled))
		[sliderPhoneOut setEnabled:FALSE];
	else
		[sliderPhoneOut setEnabled:TRUE];
	[sliderPhoneOut setIntValue:info[currentChannel].mixerValues[20].value];
	
	if(!(info[currentChannel].mixerValues[21].enabled))
		[sliderVideo setEnabled:FALSE];
	else
		[sliderVideo setEnabled:TRUE];
	[sliderVideo setIntValue:info[currentChannel].mixerValues[21].value];
	
	if(!(info[currentChannel].mixerValues[22].enabled))
		[sliderRadio setEnabled:FALSE];
	else
		[sliderRadio setEnabled:TRUE];
	[sliderRadio setIntValue:info[currentChannel].mixerValues[22].value];
	
	if(!(info[currentChannel].mixerValues[23].enabled))
		[sliderMonitor setEnabled:FALSE];
	else
		[sliderMonitor setEnabled:TRUE];
	[sliderMonitor setIntValue:info[currentChannel].mixerValues[23].value];
	
/*	if(!(info[currentChannel].mixerValues[24].enabled))
		[sliderVolume setEnabled:FALSE];
	else */
		[sliderVolume setEnabled:TRUE];
	[sliderVolume setIntValue:info[currentChannel].mixerValues[24].value];
    
    [sliderNoise setIntValue:info[currentChannel].noiseLevel];
	[sliderStereo setIntValue:info[currentChannel].StereoBase];
	
	[soundVector setState:info[currentChannel].vectorize?NSOnState:NSOffState];
	[stereoEnhance setState:info[currentChannel].useStereo?NSOnState:NSOffState];
	
	return true;
failure:
	return false;	
}

- (bool) updateMath{
	//if (!(self->chInfo = updateChannelInfo()))  //from driver
	//	goto failure;

	service = getService();
	if(!service)
		goto failure;
	kern_return_t ret;
	connect = 0;
	ret = IOServiceOpen(service, mach_task_self(), 0, &connect);
	if (ret != KERN_SUCCESS) {
		NSRunCriticalAlertPanel( 
								NSLocalizedString( @"Error", "MsgBox"), 
								NSLocalizedString( @"Can't open IO Service", "MsgBoxBody" ), nil, nil, nil );		
		goto failure;
	}
	//to driver
	actionInfo in, out;
	UInt8 ch = currentChannel;
	in.value = 0;
	in.info.action = (UInt8)kVoodooHDAActionSetMath;
	in.info.channel = currentChannel;
	in.info.device = (self->chInfo[ch].vectorize?1:0) | (self->chInfo[ch].useStereo?2:0);
	in.info.val = (self->chInfo[ch].noiseLevel & 0x0f) | ((self->chInfo[ch].StereoBase & 0x0f) << 4); 
//	[versionText setStringValue:[NSString stringWithFormat:@"Device=%d Val=0x%04x Volume=%d",
//								 in.info.device, in.info.val, self->chInfo[ch].mixerValues[24].value]];
	size_t outsize = sizeof(UInt32);
	//*outsize = sizeof(UInt32);
#if MAC_OS_X_VERSION_MIN_REQUIRED <= MAC_OS_X_VERSION_10_4
    ret = IOConnectMethodStructureIStructureO( connect,
											   kVoodooHDAActionMethod,
											   sizeof(in),			/* structureInputSize */
											   &outsize,    /* structureOutputSize */
											   &in,        /* inputStructure */
											   &out);       /* ouputStructure */
#else    	
											   
	ret = IOConnectCallStructMethod(connect,
									kVoodooHDAActionMethod,
									&in,
									sizeof(in),
									&out,
									&outsize
									);
#endif											   
	if (ret != KERN_SUCCESS) {
		NSRunCriticalAlertPanel( 
								NSLocalizedString( @"Error", "MsgBox"), 
								NSLocalizedString( @"Can't connect to StructMethod to send commands", "MsgBoxBody" ), nil, nil, nil );		
		goto failure;  //anyway
	}
	
//failure:
	
	if(connect) {
		ret = IOServiceClose(connect);
		if (ret != KERN_SUCCESS) {
			NSRunCriticalAlertPanel( 
									NSLocalizedString( @"Error", "MsgBox"), 
									NSLocalizedString( @"IOServiceClose failed", "MsgBoxBody" ), nil, nil, nil );		
		}
	}
	
	if(service)
		IOObjectRelease(service);
	
	return TRUE;
failure:
	if(service)
		IOObjectRelease(service);
	
	return false;	

}

- (void) awakeFromNib
{
	if(![self updateSliders])
		goto failure;

	ChannelInfo *info = (ChannelInfo*)self->chInfo;
	if (!info)
		goto failure;
	
	[versionText setStringValue:@"Loaded"];
	
	
	[selector removeAllItems];
	
	int i=0;
	int N = info[0].numChannels;
	if (N<=0 || N>24) {
		N=3;
		NSRunCriticalAlertPanel( 
									NSLocalizedString( @"Error", "MsgBox"), 
									NSLocalizedString( @"Wrong Channels Number 0..24", "MsgBoxBody" ), nil, nil, nil );		
		
	}
	
	for (; i < N; i++){
	//	if (sizeof(self->chInfo[i].name)) {
			[selector addItemWithTitle:[NSString stringWithFormat:@"%d: %s", i+1, info[i].name]];
	//	} else { 
	//		[selector addItemWithTitle:[NSString stringWithFormat:@"%d: Empty", i+1]];
	//	}

		
	}
	currentChannel = 0;
	
	return;
    	
failure:
	[versionText setStringValue:@"ERROR"];	
}

bool sendAction(UInt8 ch, UInt8 dev, UInt8 val) {  //value of slider to driver
	io_service_t service = getService();
	if(!service)
		goto failure;
	kern_return_t ret;
	io_connect_t connect = 0;
	ret = IOServiceOpen(service, mach_task_self(), 0, &connect);
	if (ret != KERN_SUCCESS) {
		NSRunCriticalAlertPanel( 
								NSLocalizedString( @"Error", "MsgBox"), 
								NSLocalizedString( @"Can't open IO Service", "MsgBoxBody" ), nil, nil, nil );		
		goto failure;
	}
	
	actionInfo in, out;
	in.value = 0;
	in.info.action = (UInt8)kVoodooHDAActionSetMixer;
	in.info.channel = ch;
	in.info.device = dev;
	in.info.val = val;
	
	size_t outsize = sizeof(UInt32);
	//*outsize = sizeof(UInt32);
#if MAC_OS_X_VERSION_MIN_REQUIRED <= MAC_OS_X_VERSION_10_4
    ret = IOConnectMethodStructureIStructureO( connect, kVoodooHDAActionMethod,
											  sizeof(in),			/* structureInputSize */
											  &outsize,    /* structureOutputSize */
											  &in,        /* inputStructure */
											  &out);       /* ouputStructure */
#else    	

	ret = IOConnectCallStructMethod(connect,
									kVoodooHDAActionMethod,
									&in,
									sizeof(in),
									&out,
									&outsize
									);
#endif									
	if (ret != KERN_SUCCESS) {
		NSRunCriticalAlertPanel( 
								NSLocalizedString( @"Error", "MsgBox"), 
								NSLocalizedString( @"Can't connect to StructMethod to send commands", "MsgBoxBody" ), nil, nil, nil );		
//		goto failure;  //anyway
	}
	
failure:

	if(connect) {
		ret = IOServiceClose(connect);
		if (ret != KERN_SUCCESS) {
			NSRunCriticalAlertPanel( 
									NSLocalizedString( @"Error", "MsgBox"), 
									NSLocalizedString( @"IOServiceClose failed", "MsgBoxBody" ), nil, nil, nil );		
		}
	}

	if(service)
		IOObjectRelease(service);
	
	return (ret == KERN_SUCCESS || ret == kIOReturnSuccess)?true:false;
}

- (IBAction)sliderMoved:(NSSlider *)sender {
	UInt8 device = 0;
	if (sender == sliderNoise){
		self->chInfo[currentChannel].noiseLevel = [sender intValue];
		[self updateMath];
	//	sendAction(currentChannel, 0, [sender intValue]);
		return;
	}
	if (sender == sliderStereo) {
		self->chInfo[currentChannel].StereoBase = [sender intValue] + 7;
		[self updateMath];
		//	sendAction(currentChannel, 0, [sender intValue]);
		return;		
	}
	
	if(sender == sliderBass)			device=1;
	else if(sender == sliderTreble)		device=2;
	else if(sender == sliderSynth)		device=3;
	else if(sender == sliderPCM)		device=4;
	else if(sender == sliderSpeaker)	device=5;
	else if(sender == sliderLine)		device=6;
	else if(sender == sliderMic)		device=7;
	else if(sender == sliderCD)			device=8;
	else if(sender == sliderIMix)		device=9;
	else if(sender == sliderAltPCM)		device=10;
	else if(sender == sliderRecLev)		device=11;
	else if(sender == sliderIGain)		device=12;
	else if(sender == sliderOGain)		device=13;
	else if(sender == sliderLine1)		device=14;
	else if(sender == sliderLine2)		device=15;
	else if(sender == sliderLine3)		device=16;
	else if(sender == sliderDigital1)	device=17;
	else if(sender == sliderDigital2)	device=18;
	else if(sender == sliderDigital3)	device=19;
	else if(sender == sliderPhoneIn)	device=20;
	else if(sender == sliderPhoneOut)	device=21;
	else if(sender == sliderVideo)		device=22;
	else if(sender == sliderRadio)		device=23;
	else if(sender == sliderMonitor)	device=24;
	else if(sender == sliderVolume)		device=0;
	
	sendAction(currentChannel, device, [sender intValue]);
}

- (IBAction)selectorChanged:(NSPopUpButton *)sender {
	self->currentChannel = (int)[sender indexOfItem:[sender selectedItem]];
	[self updateSliders];
}

- (void) didUnselect
{
	
	[self saveSettings];
}

- (bool) saveSettings
{
	bool res = false;
	FILE *outputFile;
	NSString *nPath = [[NSString stringWithString:@"~/Library/Preferences/VoodooHDA.settings"] stringByExpandingTildeInPath];
	const char *path = [nPath UTF8String];
	outputFile = fopen(path, "wb+");
	if(!outputFile) {
		NSRunCriticalAlertPanel( 
								NSLocalizedString( @"Couldn't save settings file", "MsgBox"), 
								NSLocalizedString( @"Error opening file", "MsgBoxBody" ), nil, nil, nil );		
		goto failure;
	}	
	if (!(self->chInfo = updateChannelInfo()))
		goto failure;
	
	int i=0;
	for(;i<chInfo[0].numChannels;i++)
		fwrite(&chInfo[i], sizeof(chInfo[0]), 1, outputFile);
	fprintf(outputFile, "\n");
	fclose(outputFile);
	
	res = true;
failure:
	
	return res;
}
//Just a sample
/*- (void) changeVersionText
{
	[versionText setStringValue:@"Bla"];
}
*/
/*
- (IBAction)enableAllSLiders:(NSButton *)sender {
	[sliderBass setEnabled:TRUE];
	[sliderTreble setEnabled:TRUE];
	[sliderSynth setEnabled:TRUE];
	[sliderPCM setEnabled:TRUE];
	[sliderSpeaker setEnabled:TRUE];
	[sliderLine setEnabled:TRUE];
	[sliderMic setEnabled:TRUE];
	[sliderCD setEnabled:TRUE];
	[sliderIMix setEnabled:TRUE];
	[sliderAltPCM setEnabled:TRUE];
	[sliderRecLev setEnabled:TRUE];
	[sliderIGain setEnabled:TRUE];
	[sliderOGain setEnabled:TRUE];
	[sliderLine1 setEnabled:TRUE];
	[sliderLine2 setEnabled:TRUE];
	[sliderLine3 setEnabled:TRUE];
	[sliderDigital1 setEnabled:TRUE];
	[sliderDigital2 setEnabled:TRUE];
	[sliderDigital3 setEnabled:TRUE];
	[sliderPhoneIn setEnabled:TRUE];
	[sliderPhoneOut setEnabled:TRUE];
	[sliderVideo setEnabled:TRUE];
	[sliderRadio setEnabled:TRUE];
	[sliderMonitor setEnabled:TRUE];	
	
//	[sliderVolume setEnabled:TRUE];
//	[sliderNoise setEnabled:TRUE];
	
}
*/
- (IBAction)useStereoEnhance:(NSButton *)sender{
	bool useStereo;
	useStereo = ([stereoEnhance state]==NSOnState);
	self->chInfo[currentChannel].useStereo = useStereo;
	[self updateMath];
}

- (IBAction)SSEChanged:(NSButton *)sender{
	bool vector;
	vector = ([soundVector state]==NSOnState);
	self->chInfo[currentChannel].vectorize = vector;	
	[self updateMath];		
}

@end
