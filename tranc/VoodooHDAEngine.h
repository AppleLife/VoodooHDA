#include "License.h"

#ifndef _VOODOO_HDA_ENGINE_H
#define _VOODOO_HDA_ENGINE_H

#include <IOKit/IOLib.h>

#include <IOKit/audio/IOAudioEngine.h>

#include "Private.h"

class VoodooHDADevice;

class IOAudioPort;
class IOAudioSelectorControl;
class IOAudioLevelControl;
class IOAudioToggleControl;

class VoodooHDAEngine : public IOAudioEngine
{
	OSDeclareDefaultStructors(VoodooHDAEngine)

public:
	UInt32 mVerbose;

	UInt32 mBufferSize;
	UInt32 mSampleSize;
	UInt32 mNumSampleFrames;
	UInt32 Boost;
/*	bool vectorize;
	int noiseLevel;
	bool useStereo;
	int StereoBase;*/

	Channel *mChannel;
	VoodooHDADevice *mDevice;
	IOAudioStream *mStream;
	bool emptyStream;
	float *floatMixBufOld;

	const char *mPortName;
	const char *mName;
	IOAudioPort *mPort;

	const char *mDescription;

	int mActiveOssDev;

	IOAudioSelectorControl *mSelControl;
	IOAudioLevelControl *mVolumeControl;
	IOAudioToggleControl *mMuteControl;
	
	UInt32					oldOutVolumeLeft;
	UInt32					oldOutVolumeRight;
	UInt32					oldInputGain;

	// cue8chalk: flag for volume change fix
	bool mEnableVolumeChangeFix;
    // VertexBZ: flag for mute fix
	bool mEnableMuteFix;
	
	void messageHandler(UInt32 type, const char *format, ...) __attribute__ ((format (printf, 3, 4)));

	void setPinName(/*UInt32 type, */const char* name);
//	void enumiratePinNames(void);
	const char *getPortName();
	const char *getDescription();
	void identifyPaths();
	UInt32 getNumCtls(UInt32 dev);
	UInt64 getMinMaxDb(UInt32 dev);

	bool validateOssDev(int ossDev);
	const char *getOssDevName(int ossDev);
	void setActiveOssDev(int ossDev);
	int getActiveOssDev();

	IOAudioStreamDirection getEngineDirection();
	int getEngineId();

	bool createAudioStream(IOAudioStreamDirection direction, void *sampleBuffer,
			UInt32 sampleBufferSize, IOAudioSampleRate minSampleRate, IOAudioSampleRate maxSampleRate,
			UInt32 supPcmSizeRates, UInt32 sampleFormat, UInt32 channels);
	bool createAudioStream();

	bool createAudioControls();
	
	static IOReturn volumeChangeHandler(IOService *target, IOAudioControl *volumeControl, SInt32 oldValue, SInt32 newValue);
	static IOReturn muteChangeHandler(IOService *target, IOAudioControl *muteControl, SInt32 oldValue, SInt32 newValue);
	static IOReturn gainChangeHandler(IOService *target, IOAudioControl *gainControl, SInt32 oldValue, SInt32 newValue);


	IOReturn volumeChanged(IOAudioControl *volumeControl, SInt32 oldValue, SInt32 newValue);
	IOReturn muteChanged(IOAudioControl *muteControl, SInt32 oldValue, SInt32 newValue);
    IOReturn gainChanged(IOAudioControl *gainControl, SInt32 oldValue, SInt32 newValue);

	virtual bool init(Channel *channel);
	virtual void free();
	virtual bool initHardware(IOService *provider);
	virtual void stop(IOService *provider);

	virtual IOReturn performAudioEngineStart();
	virtual IOReturn performAudioEngineStop();

	virtual UInt32 getCurrentSampleFrame();

	virtual IOReturn performFormatChange(IOAudioStream *audioStream, const IOAudioStreamFormat *newFormat,
			const IOAudioSampleRate *newSampleRate);

	virtual IOReturn clipOutputSamples(const void *mixBuf, void *sampleBuf, UInt32 firstSampleFrame,
			UInt32 numSampleFrames, const IOAudioStreamFormat *streamFormat, IOAudioStream *audioStream);
	virtual IOReturn convertInputSamples(const void *sampleBuf, void *destBuf, UInt32 firstSampleFrame,
			UInt32 numSampleFrames, const IOAudioStreamFormat *streamFormat, IOAudioStream *audioStream);
	virtual OSString *getLocalUniqueID();
};

#endif
