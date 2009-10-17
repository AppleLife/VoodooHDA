#include "License.h"

#ifndef _SHARED_H
#define _SHARED_H

#define kVoodooHDAClassName	"VoodooHDADevice"

enum {
    kVoodooHDAActionMethod = 0,
    kVoodooHDANumMethods
};

enum {
	kVoodooHDAActionTest = 0x1000
};

enum {
	kVoodooHDAMemoryMessageBuffer = 0x2000,
	kVoodooHDAMemoryPinDump,
	kVoodooHDAMemoryCommand = 0x3000,
	kVoodooHDAMemoryExtMessageBuffer
};

typedef struct _sliderInfo{
	unsigned char num; //Порядковый	номер регулятора
	unsigned char value; //Значение уселения от 0 до 0x64
	char sliderName[0x20]; //Название регулятора
	unsigned char enabled; //Можно ли пользователю менять значение регулятора
}sliderInfo;

#define MAX_SLIDER_TAB_NAME_LENGTH 32
typedef struct sliders{
	char tabName[MAX_SLIDER_TAB_NAME_LENGTH]; //Название PinComplex для которого менятся значения уселения
	sliderInfo info[24];
	unsigned char size; //Число структур
	unsigned char non[3];
}sliders;

enum {
	kVoodooHDAChannelNames = 0x3000
};

enum {
	kVoodooHDAActionSetMixer = 0x40
};

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

#endif
