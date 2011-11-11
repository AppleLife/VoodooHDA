//
//  VoodooHdaSettingsLoader.m
//  VoodooHdaSettingsLoader
//
//  Created by Ben on 10/11/11.
//  Copyright (c) 2011 VoodooHDA. All rights reserved.
//

#import "VoodooHdaSettingsLoader.h"

static io_service_t getService() {
	io_service_t service = 0;
	mach_port_t masterPort;
	io_iterator_t iter;
	kern_return_t ret;
	io_string_t path;
	
	ret = IOMasterPort(MACH_PORT_NULL, &masterPort);
	if (ret != KERN_SUCCESS) {
		NSLog( @"Can't get masterport" );		
		goto failure;
	}
	
	ret = IOServiceGetMatchingServices(masterPort, IOServiceMatching(kVoodooHDAClassName), &iter);
	if (ret != KERN_SUCCESS) {
		NSLog( @"VoodooHDA is not running" );		
		goto failure;
	}
	
	service = IOIteratorNext(iter);
	IOObjectRelease(iter);
	
	ret = IORegistryEntryGetPath(service, kIOServicePlane, path);
	if (ret != KERN_SUCCESS) {
		NSLog( @"Can't get registry-entry path");		
		goto failure;
	}
	
failure:
	return service;
}	

static bool sendAction(UInt8 ch, UInt8 dev, UInt8 val)
{ 
    //value of slider to driver
	io_service_t service = getService();
	if(!service)
		goto failure;
	kern_return_t ret;
	io_connect_t connect = 0;
	ret = IOServiceOpen(service, mach_task_self(), 0, &connect);
	if (ret != KERN_SUCCESS) {
		NSLog(@"Can't open IO Service");		
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
		NSLog( @"Can't connect to StructMethod to send commands");		
        //		goto failure;  //anyway
	}
	
failure:
    
	if(connect) {
		ret = IOServiceClose(connect);
		if (ret != KERN_SUCCESS) {
			NSLog( @"IOServiceClose failed");		
		}
	}
    
	if(service)
		IOObjectRelease(service);
        
        return (ret == KERN_SUCCESS || ret == kIOReturnSuccess)?true:false;
}

static bool sendMath(UInt8 ch, bool vectorize, bool useStereo,UInt8 noiseLevel,UInt8 stereoBase)
{
	//if (!(self->chInfo = updateChannelInfo()))  //from driver
	//	goto failure;
    
	io_service_t service = getService();
	if(!service)
		goto failure;
	kern_return_t ret;
	io_connect_t connect = 0;
	ret = IOServiceOpen(service, mach_task_self(), 0, &connect);
	if (ret != KERN_SUCCESS) {
		NSLog(@"Can't open IO Service");		
		goto failure;
	}
    
	//to driver
	actionInfo in, out;
	
	in.value = 0;
	in.info.action = (UInt8)kVoodooHDAActionSetMath;
	in.info.channel = ch;
	in.info.device = (vectorize?1:0) | (useStereo?2:0);
	in.info.val = (noiseLevel & 0x0f) | ((stereoBase & 0x0f) << 4); 
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
		NSLog( @"Can't connect to StructMethod to send commands" );		
		goto failure;  //anyway
	}
	
    //failure:
	
	if(connect) {
		ret = IOServiceClose(connect);
		if (ret != KERN_SUCCESS) {
			NSLog( @"IOServiceClose failed" );		
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

//get channel info from driver
static ChannelInfo *getChannelInfoFromDriver() 
{
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
		NSLog( @"Can't open IO Service" );		
		goto failure;
	}
	
	ret = IOConnectMapMemory(connect, kVoodooHDAChannelNames, mach_task_self(), &address, &size,
							 kIOMapAnywhere | kIOMapDefaultCache);
	if (ret != kIOReturnSuccess) 
    {
		NSLog(  @"Can't map Memory");		
		goto failure;
	}
	
	info = (ChannelInfo*)address;
	
failure:
#if 0
	if(connect) {
		ret = IOServiceClose(connect);
		if (ret != KERN_SUCCESS) 
        {
            NSLog(  @"IOServiceClose failed" );		
		}
	}
#endif
	if(service)
		IOObjectRelease(service);
	
	return info;
	//return (ret == KERN_SUCCESS || ret == kIOReturnSuccess)?true:false;	
}	




@implementation VoodooHdaSettingsLoader



- (bool) loadSettings
{
	bool res = false;

	NSString *nPath = [[NSString stringWithString:@"~/Library/Preferences/VoodooHDA.settings"] stringByExpandingTildeInPath];
	const char *path = [nPath UTF8String];
	FILE *inputFile = fopen(path, "rb+");
	if(!inputFile) {
		NSLog(@"Couldn't load settings file (file opening problem: not here, or read protected");
		goto failure;
	}	
    
    fseek (inputFile, 0, SEEK_END);
    long size= ftell(inputFile);
    rewind(inputFile);
    
    if( ( (size-1) /sizeof(ChannelInfo))*sizeof(ChannelInfo) != (size-1) ) // Remove the \n at the end
    {
        NSLog(@"Settings file is not compatible with this version of VoodooHdaSettingsLoader (channel sizes are different");
        goto failure;
    }
    
    long channelCount = size/sizeof(ChannelInfo);
    NSLog(@"%ld channels to read.",channelCount);
    
    ChannelInfo* channels = (ChannelInfo*)malloc(channelCount*sizeof(ChannelInfo));
    
    int c,d;
    
    for(c=0;c<channelCount;c++)
    {
        fread(&channels[c],1,sizeof(ChannelInfo),inputFile);
        NSLog(@"Has read %s channel information.",channels[c].name);
    }
    
    fclose(inputFile);
  
    NSLog(@"Reading driver current info");
    ChannelInfo* driverInfo = getChannelInfoFromDriver();
    if(!driverInfo)
        goto failure;
    
    NSLog(@"Checking that driver matches settings file:");
    
    NSLog(@"Driver has %d channels.",driverInfo->numChannels);
    if(driverInfo->numChannels != channelCount)
    {
        NSLog(@"Channel count not matching.");
        goto failure;
    }
    
    NSLog(@"Checking all channels devices");
    for(c = 0;c<channelCount;c++)
    {
        if(strcmp(driverInfo[c].name,channels[c].name) !=0)
        {
            NSLog(@"Channel names do not match.");
            goto failure;
        }
        
        for(d = 0; d < SOUND_MIXER_NRDEVICES; d++)
        {
            if(strcmp(driverInfo[c].mixerValues[d].name,channels[c].mixerValues[d].name) !=0)
            {
                NSLog(@"Device names do not match.");
                goto failure;
            }
            
            if(driverInfo[c].mixerValues[d].enabled != channels[c].mixerValues[d].enabled)
            {
                NSLog(@"Device configurations do not match.");
                goto failure;
            }
        }
    }
    
    NSLog(@"Everything looks good, restoring settings...");
    
    for(c=0;c<channelCount;c++)
    {
        for(d=0;d<SOUND_MIXER_NRDEVICES;d++)
        {
            if(channels[c].mixerValues[d].enabled)
            {
                NSLog(@"Setting back %s, %s to %d", 
                      channels[c].name, 
                      channels[c].mixerValues[d].name, 
                      channels[c].mixerValues[d].value);
                
                sendAction(c,   channels[c].mixerValues[d].mixId,
                                channels[c].mixerValues[d].value);
           }
        }
        
        sendMath(c, channels[c].vectorize, 
                    channels[c].useStereo,
                    channels[c].noiseLevel, 
                    channels[c].StereoBase);
    }
              
    free(channels);
    res = true;
    
    NSLog(@"Settings restored succesfully!");
        
failure:
	
	return res;
}


- (void) load
{
    [self loadSettings];
}

@end
