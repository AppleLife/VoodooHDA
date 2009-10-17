#include "License.h"

#include "VoodooHDAEngine.h"
#include "VoodooHDADevice.h"
#include "Common.h"
#include "Verbs.h"
#include "OssCompat.h"
#include "Tables.h"

#include <IOKit/audio/IOAudioDefines.h>
#include <IOKit/audio/IOAudioPort.h>
#include <IOKit/audio/IOAudioSelectorControl.h>
#include <IOKit/audio/IOAudioLevelControl.h>
#include <IOKit/audio/IOAudioToggleControl.h>

#define super IOAudioEngine
OSDefineMetaClassAndStructors(VoodooHDAEngine, IOAudioEngine)

#define SAMPLE_CHANNELS		2	// forced stereo quirk is always enabled

#define SAMPLE_OFFSET		32	// note: these values definitely need to be tweaked
#define SAMPLE_LATENCY		16

extern const char *gDeviceTypes[], *gConnTypes[];

#define kVoodooHDAPortSubTypeBase		'voo\x40'
#define VOODOO_OSS_TO_SUBTYPE(type)		(kVoodooHDAPortSubTypeBase + 1 + type)
#define VOODOO_SUBTYPE_TO_OSS(type)		(type - 1 - kVoodooHDAPortSubTypeBase)

const char *gOssDeviceTypes[SOUND_MIXER_NRDEVICES] = {
	"Master", "Bass", "Treble", "Synthesizer", "PCM", "Speaker", "Line-in", "Microphone",
	"CD", "Input mix", "Alternate PCM", "Recording level", "Input gain", "Output gain",
	"Line #1", "Line #2", "Line #3", "Digital #1", "Digital #2", "Digital #3",
	"Phone input", "Phone output", "Video", "Radio", "Microphone #2"
};

/******************************************************************************************/
/******************************************************************************************/

#define logMsg(fmt, args...)	if(mVerbose>0)\
		messageHandler(kVoodooHDAMessageTypeGeneral, fmt, ##args)
#define errorMsg(fmt, args...)	messageHandler(kVoodooHDAMessageTypeError, fmt, ##args)
#define dumpMsg(fmt, args...)	messageHandler(kVoodooHDAMessageTypeDump, fmt, ##args)

void VoodooHDAEngine::messageHandler(UInt32 type, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	if (mDevice)
		mDevice->messageHandler(type, format, args);
	else if (mVerbose >= 1)
		vprintf(format, args);
	va_end(args);
}

/******************************************************************************************/
/******************************************************************************************/

bool VoodooHDAEngine::init(Channel *channel)
{
	bool result = false;

//	logMsg("VoodooHDAEngine[%p]::init\n", this);

	if (!channel || !super::init(NULL))
		goto done;

	mChannel = channel;
	mActiveOssDev = -1;

	result = true;
done:
	return result;
}

void VoodooHDAEngine::stop(IOService *provider)
{
//	logMsg("VoodooHDAEngine[%p]::stop\n", this);

	super::stop(provider);
}

void VoodooHDAEngine::free()
{
//	logMsg("VoodooHDAEngine[%p]::free\n", this);

	RELEASE(mStream);

	RELEASE(mSelControl);
	RELEASE(mVolumeControl);
	RELEASE(mMuteControl);

	RELEASE(mPort);

	RELEASE(mDevice);

	super::free();
}

const char *VoodooHDAEngine::getPortName()
{
	UInt32 numDacs;
	nid_t dacNid, outputNid;
	Widget *widget;
	UInt32 config;
//	const char *devType, *connType;
	//char buf[64]; // = "Unknown";
	AudioAssoc *assoc;

	if (mPortName)
		return mPortName;

	mDevice->lock(__FUNCTION__);

	for (numDacs = 0; (numDacs < 16) && (mChannel->io[numDacs] != -1); numDacs++){
		//Slice - to trace
		if (mVerbose > 2) {
			logMsg(" io[%d] in assoc %d = %d\n", (int)numDacs, (int)mChannel->assocNum, (int)mChannel->io[numDacs]);
		}
	}
	if (numDacs != 1){
		if (numDacs > 1) {
			mPortName = "Complex output";
		}
		goto done;
	}
		
	
	dacNid = mChannel->io[0];

	assoc = &mChannel->funcGroup->audio.assocs[mChannel->assocNum];
	outputNid = -1;
	for (int n = 0; (n < 16) && assoc->dacs[n]; n++)
		if (assoc->dacs[n] == dacNid)
			outputNid = assoc->pins[n];
	if (outputNid == -1)
		goto done;

	widget = mDevice->widgetGet(mChannel->funcGroup, outputNid);
	if (!widget)
		goto done;

	config = widget->pin.config;
/*	devType = gDeviceTypes[HDA_CONFIG_DEFAULTCONF_DEVICE(config)];
	connType = gConnTypes[HDA_CONFIG_DEFAULTCONF_CONNECTIVITY(config)];
	logMsg("dacNid = %d, outputNid = %d, devType = %s, connType = %s name=%s\n", dacNid, outputNid,
			devType, connType, widget->name);
*/	
	//Slice - advanced PinName

	mPortName = &widget->name[4]; 
done:
	mDevice->unlock(__FUNCTION__);

	if (!mPortName)
		mPortName = "Not connected";
	
	return mPortName;
}

const char *VoodooHDAEngine::getDescription()
{
	static char buffer[32];
	if (!mDescription) {
		PcmDevice *pcmDevice = mChannel->pcmDevice;
		snprintf(buffer, sizeof (buffer), "%s PCM #%d", (pcmDevice->digital ? "Digital" : "Analog"),
				pcmDevice->index);
		mDescription = buffer;
	}
	return mDescription;
}

void VoodooHDAEngine::identifyPaths()
{
	IOAudioStreamDirection direction = getEngineDirection();
	FunctionGroup *funcGroup = mChannel->funcGroup;

	for (int i = funcGroup->startNode; i < funcGroup->endNode; i++) {
		Widget *widget;
		UInt32 config;
		const char *devType, *connType;

		widget = mDevice->widgetGet(funcGroup, i);
		if (!widget || widget->enable == 0)
			continue;
		if (((direction == kIOAudioStreamDirectionOutput) &&
				(widget->type != HDA_PARAM_AUDIO_WIDGET_CAP_TYPE_PIN_COMPLEX)) ||
				((direction == kIOAudioStreamDirectionInput) &&
				(widget->type != HDA_PARAM_AUDIO_WIDGET_CAP_TYPE_AUDIO_INPUT)))
			continue;
		if (widget->bindAssoc != mChannel->assocNum)
			continue;
		config = widget->pin.config;
		devType = gDeviceTypes[HDA_CONFIG_DEFAULTCONF_DEVICE(config)];
		connType = gConnTypes[HDA_CONFIG_DEFAULTCONF_CONNECTIVITY(config)];
//		logMsg("[nid %d] devType = %s, connType = %s\n", i, devType, connType);
	}
}

UInt32 VoodooHDAEngine::getNumCtls(UInt32 dev)
{
	UInt32 numCtls = 0;
	AudioControl *control;

	for (int i = 0; (control = mDevice->audioCtlEach(mChannel->funcGroup, &i)); ) {
		if ((control->enable == 0) || !(control->ossmask & (1 << dev)))
			continue;
		if (!((control->widget->bindAssoc == mChannel->assocNum) || (control->widget->bindAssoc == -2)))
			continue;
		numCtls++;
	}

	return numCtls;
}

UInt64 VoodooHDAEngine::getMinMaxDb(UInt32 mask)
{
	AudioControl *control;
	IOFixed minDb, maxDb;

	minDb = ~0L;
	maxDb = ~0L;

	// xxx: we currently use the values from the first found control (ie. amplifier settings)

	for (int i = 0; (control = mDevice->audioCtlEach(mChannel->funcGroup, &i)); ) {
		if ((control->enable == 0) || !(control->ossmask & mask)) //(1 << dev)))
			continue;
		if (!((control->widget->bindAssoc == mChannel->assocNum) || (control->widget->bindAssoc == -2)))
			continue;
		if (control->step <= 0)
			continue;
		minDb = ((0 - control->offset) * (control->size + 1) / 4) << 16;
		maxDb = ((control->step - control->offset) * (control->size + 1) / 4) << 16;
		break;
	}

	return ((UInt64) minDb << 32) | maxDb;
}

bool VoodooHDAEngine::validateOssDev(int ossDev)
{
	return ((ossDev >= 0) && (ossDev < SOUND_MIXER_NRDEVICES));
}

const char *VoodooHDAEngine::getOssDevName(int ossDev)
{
	if (validateOssDev(ossDev))
		return gOssDeviceTypes[ossDev];
	else
		return "invalid";
}

void VoodooHDAEngine::setActiveOssDev(int ossDev)
{
//	logMsg("setting active OSS device: %d (%s)\n", ossDev, getOssDevName(ossDev));
	ASSERT(validateOssDev(ossDev));
	mActiveOssDev = ossDev;
}

int VoodooHDAEngine::getActiveOssDev()
{
	int ossDev = mActiveOssDev;
//	logMsg("active OSS device: %d (%s)\n", ossDev, getOssDevName(ossDev));
	ASSERT(validateOssDev(ossDev));
	return ossDev;
}

bool VoodooHDAEngine::initHardware(IOService *provider)
{
	bool result = false;
	const char *description;
	const char *portName;
	UInt32 portType;
	UInt32 subType;
	IOReturn ret;

//	logMsg("VoodooHDAEngine[%p]::initHardware\n", this);

	if (!super::initHardware(provider)) {
		errorMsg("error: IOAudioEngine::initHardware failed\n");
		goto done;
	}

	mDevice = OSDynamicCast(VoodooHDADevice, provider);
	ASSERT(mDevice);
	mDevice->retain();

	mVerbose = mDevice->mVerbose;

	identifyPaths();
	getPortName();

	description = getDescription();
	ASSERT(description);
	
	//logMsg("setDesc portName = %s\n", mPortName);
	setDescription(mPortName);

	// xxx: there must be some way to get port name to appear in the "type" column on the sound
	// preference pane - this used to be the "port" column but apparently this is no longer the
	// case in recent releases of os x

	portName = mPortName;
	ASSERT(portName);

	if (getEngineDirection() == kIOAudioStreamDirectionOutput) {
		portType = kIOAudioPortTypeOutput;
		// TODO: subType
		subType = kIOAudioOutputPortSubTypeInternalSpeaker;
	}
	else {
		portType = kIOAudioPortTypeInput;
		// TODO: subType
		subType =  kIOAudioInputPortSubTypeInternalMicrophone;
	}
	
	//logMsg("setDesc portType = %4c subType = %4c\n", portType, subType);
	
	mPort = IOAudioPort::withAttributes(portType, portName, subType);
	if (!mPort) {
		errorMsg("error: IOAudioPort::withAttributes failed\n");
		goto done;
	}
	ret = mDevice->attachAudioPort(mPort, this, NULL);
	if (ret != kIOReturnSuccess) {
		errorMsg("error: attachAudioPort failed\n");
		goto done;
	}

	setSampleOffset(SAMPLE_OFFSET);
	setSampleLatency(SAMPLE_LATENCY);

	if (!createAudioStream()) {
		errorMsg("error: createAudioStream failed\n");
		goto done;
	}

	if (!createAudioControls()) {
		errorMsg("error: createAudioControls failed\n");
		goto done;
	}

	result = true;
done:
	if (!result)
		stop(provider);

	return result;
}

bool VoodooHDAEngine::createAudioStream()
{
	bool result = false;
	IOAudioStreamDirection direction;
	IOAudioSampleRate minSampleRate, maxSampleRate;
	UInt8 *sampleBuffer;

	ASSERT(!mStream);

//	logMsg("VoodooHDAEngine[%p]::createAudioStream\n", this);

//	logMsg("recDevMask: 0x%lx, devMask: 0x%lx\n", mChannel->pcmDevice->recDevMask,
//			mChannel->pcmDevice->devMask);

	direction = getEngineDirection();

//	logMsg("formats: ");
	for (UInt32 n = 0; (n < 8) && mChannel->formats[n]; n++)
//		logMsg("0x%lx ", mChannel->formats[n]);
//	logMsg("\n");

	if (!HDA_PARAM_SUPP_STREAM_FORMATS_PCM(mChannel->supStreamFormats)) {
		errorMsg("error: channel doesn't support pcm stream format\n");
		goto done;
	}

//	logMsg("sample rates: ");
	for (UInt32 n = 0; (n < 16) && mChannel->pcmRates[n]; n++)
//		logMsg("%ld ", mChannel->pcmRates[n]);
//	logMsg("(min: %ld, max: %ld)\n", mChannel->caps.minSpeed, mChannel->caps.maxSpeed);

	ASSERT(mChannel->caps.minSpeed);
	ASSERT(mChannel->caps.maxSpeed);
	ASSERT(mChannel->caps.minSpeed <= mChannel->caps.maxSpeed);

	minSampleRate.whole = mChannel->caps.minSpeed;
	minSampleRate.fraction = 0;
	maxSampleRate.whole = mChannel->caps.maxSpeed;
	maxSampleRate.fraction = 0;

	sampleBuffer = (UInt8 *) mChannel->buffer->virtAddr;
	mBufferSize = HDA_BUFSZ_MAX; // hardcoded in pcmAttach()
	if (!createAudioStream(direction, sampleBuffer, mBufferSize, minSampleRate, maxSampleRate,
			mChannel->supPcmSizeRates, kIOAudioStreamSampleFormatLinearPCM)) {
		errorMsg("error: createAudioStream failed\n");
		goto done;
	}
#if 0
	if (HDA_PARAM_SUPP_STREAM_FORMATS_AC3(mChannel->supStreamFormats)) {
//		logMsg("adding AC3 audio stream\n");
		if (!createAudioStream(direction, sampleBuffer, mBufferSize, minSampleRate, maxSampleRate,
							   mChannel->supPcmSizeRates, kIOAudioStreamSampleFormatAC3)) {
			errorMsg("error: createAudioStream failed\n");
		}
	}
#endif
	result = true;
done:
	return result;
}

bool VoodooHDAEngine::createAudioStream(IOAudioStreamDirection direction, void *sampleBuffer,
		UInt32 sampleBufferSize, IOAudioSampleRate minSampleRate, IOAudioSampleRate maxSampleRate,
		UInt32 supPcmSizeRates, UInt32 sampleFormat)
{
	bool result = false;
	const char *description;

	IOAudioStreamFormat format = {
		2,												// number of channels
		sampleFormat, //kIOAudioStreamSampleFormatLinearPCM,			// sample format
		kIOAudioStreamNumericRepresentationSignedInt,	// numeric format
		0,												// bit depth (to be filled in)
		0,												// bit width (to be filled in)
		kIOAudioStreamAlignmentLowByte,					// low byte aligned
		kIOAudioStreamByteOrderLittleEndian,			// little endian
		true,											// format is mixable
		0												// driver-defined tag
	};

	ASSERT(!mStream);

//	logMsg("VoodooHDAEngine[%p]::createAudioStream(%d, %p, %ld)\n", this, direction, sampleBuffer,
//			sampleBufferSize);

	if (direction == kIOAudioStreamDirectionOutput) {
		if (sampleFormat == kIOAudioStreamSampleFormatAC3)
			description = "AC3 Output stream";
		else
			description = "Output stream";
	}
	else if (direction == kIOAudioStreamDirectionInput) {
		if (sampleFormat == kIOAudioStreamSampleFormatAC3)
			description = "AC3 Input stream";
		else
			description = "Input stream";
	}
	else
		BUG("unknown direction");

	mStream = new IOAudioStream;
	if (!mStream->initWithAudioEngine(this, direction, 1, description)) {
		errorMsg("error: IOAudioStream::initWithAudioEngine failed\n");
		goto done;
	}

	mStream->setSampleBuffer(sampleBuffer, sampleBufferSize); // also creates mix buffer

//	logMsg("supported bit depths:");
	if (HDA_PARAM_SUPP_PCM_SIZE_RATE_16BIT(supPcmSizeRates)) {
		format.fBitDepth = 16;
		format.fBitWidth = 16;
		mStream->addAvailableFormat(&format, &minSampleRate, &maxSampleRate);
//		logMsg(" 16");
	}
	if (HDA_PARAM_SUPP_PCM_SIZE_RATE_24BIT(supPcmSizeRates)) {
		format.fBitDepth = 24;
		format.fBitWidth = 32;
		mStream->addAvailableFormat(&format, &minSampleRate, &maxSampleRate);
//		logMsg(" 24");
	}
	if (HDA_PARAM_SUPP_PCM_SIZE_RATE_32BIT(supPcmSizeRates)) {
		format.fBitDepth = 32;
		format.fBitWidth = 32;
		mStream->addAvailableFormat(&format, &minSampleRate, &maxSampleRate);
//		logMsg(" 32");
	}
	if (!format.fBitDepth || !format.fBitWidth) {
//		logMsg(" (none)\n");
		errorMsg("error: couldn't find supported bit depth (16, 24, or 32-bit)\n");
		goto done;
	} else
//		logMsg("\n");

	mStream->setFormat(&format); // set widest format as default
	setSampleRate(&maxSampleRate);
	performFormatChange(mStream, &format, &maxSampleRate);

	addAudioStream(mStream);

	result = true;
done:
	RELEASE(mStream);

	return result;
}

IOAudioStreamDirection VoodooHDAEngine::getEngineDirection()
{
	IOAudioStreamDirection direction;

	if (mChannel->direction == PCMDIR_PLAY) {
		ASSERT(mChannel->pcmDevice->playChanId >= 0);
		direction = kIOAudioStreamDirectionOutput;
	} else if (mChannel->direction == PCMDIR_REC) {
		ASSERT(mChannel->pcmDevice->recChanId >= 0);
		direction = kIOAudioStreamDirectionInput;
	} else
		BUG("invalid direction");

	if (mStream)
		ASSERT(mStream->getDirection() == direction);

	return direction;
}

int VoodooHDAEngine::getEngineId()
{
	if (getEngineDirection() == kIOAudioStreamDirectionOutput)
		return mChannel->pcmDevice->playChanId;
	else
		return mChannel->pcmDevice->recChanId;
}

IOReturn VoodooHDAEngine::performAudioEngineStart()
{
//	logMsg("VoodooHDAEngine[%p]::performAudioEngineStart\n", this);

//	logMsg("calling channelStart() for channel %d\n", getEngineId());
	mDevice->channelStart(mChannel);

	return kIOReturnSuccess;
}

IOReturn VoodooHDAEngine::performAudioEngineStop()
{
//	logMsg("VoodooHDAEngine[%p]::performAudioEngineStop\n", this);

//	logMsg("calling channelStop() for channel %d\n", getEngineId());
	mDevice->channelStop(mChannel);

	return kIOReturnSuccess;
}
	
UInt32 VoodooHDAEngine::getCurrentSampleFrame()
{
	return (mDevice->channelGetPosition(mChannel) / mSampleSize);
}

// pauseAudioEngine, beginConfigurationChange, completeConfigurationChange, resumeAudioEngine

IOReturn VoodooHDAEngine::performFormatChange(IOAudioStream *audioStream, const IOAudioStreamFormat *newFormat,
		const IOAudioSampleRate *newSampleRate)
{
	IOReturn result = kIOReturnError;
	int setResult;
	bool wasRunning = (getState() == kIOAudioEngineRunning);

	// ASSERT(audioStream == mStream);

//	logMsg("VoodooHDAEngine[%p]::peformFormatChange(%p, %p, %p)\n", this, audioStream, newFormat,
//			newSampleRate);

	if (!newFormat && !newSampleRate) {
		errorMsg("warning: performFormatChange(%p) called with no effect\n", audioStream);
		return kIOReturnSuccess;
	}

	if (wasRunning)
		stopAudioEngine();

	if (newFormat) {
		UInt32 ossFormat = AFMT_STEREO;

		ASSERT(newFormat->fNumChannels == 2);
		//ASSERT(newFormat->fSampleFormat == kIOAudioStreamSampleFormatLinearPCM);
		ASSERT(newFormat->fNumericRepresentation == kIOAudioStreamNumericRepresentationSignedInt);
		ASSERT(newFormat->fAlignment == kIOAudioStreamAlignmentLowByte);
		ASSERT(newFormat->fByteOrder == kIOAudioStreamByteOrderLittleEndian);
		ASSERT(newFormat->fIsMixable);

		switch (newFormat->fBitDepth) {
		case 16:
			ASSERT(newFormat->fBitWidth == 16);
			ossFormat |= AFMT_S16_LE;
			break;
		case 24: // xxx: make clear distinction between 24/32-bit
		case 32:
			ASSERT(newFormat->fBitWidth == 32);
			ossFormat |= AFMT_S32_LE;
			break;
		default:
			BUG("unsupported bit depth");
			goto done;
		}

		setResult = mDevice->channelSetFormat(mChannel, ossFormat);
	//	logMsg("channelSetFormat(0x%08lx) for channel %d returned %d\n", ossFormat, getEngineId(),
	//			setResult);
		if (setResult != 0) {
			errorMsg("error: couldn't set format 0x%lx (%d-bit depth)\n", (long unsigned int)ossFormat, newFormat->fBitDepth);
			goto done;
		}

		ASSERT(mBufferSize);
		mSampleSize = (SAMPLE_CHANNELS * (newFormat->fBitWidth / 8));
		mNumSampleFrames = mBufferSize / mSampleSize;
		setNumSampleFramesPerBuffer(mNumSampleFrames);

//		logMsg("buffer size: %ld, channels: %d, bit depth: %d, # samp. frames: %ld\n", mBufferSize,
//				SAMPLE_CHANNELS, newFormat->fBitDepth, mNumSampleFrames);
	}

	if (newSampleRate) {
		setResult = mDevice->channelSetSpeed(mChannel, newSampleRate->whole);
//		logMsg("channelSetSpeed(%ld) for channel %d returned %d\n", newSampleRate->whole, getEngineId(),
//				setResult);
		if ((UInt32) setResult != newSampleRate->whole) {
			errorMsg("error: couldn't set sample rate %ld\n", (long int)newSampleRate->whole);
			goto done;
		}
	}

	if (wasRunning)
		startAudioEngine();

	result = kIOReturnSuccess;
done:
	return result;
}

bool VoodooHDAEngine::createAudioControls()
{
	bool			result = false;
	IOAudioControl	*control = new IOAudioControl;
	IOAudioStreamDirection direction;
	UInt32			usage;
	UInt64			minMaxDb;
	IOFixed			minDb,
					maxDb;
	int				initOssDev, initOssMask;
	
	if (control == NULL)
		return false;
	
	if (mChannel->funcGroup->audio.assocs[mChannel->assocNum].digital) {   //digital has no control
		return true;
	}
	direction = getEngineDirection();
	if (direction == kIOAudioStreamDirectionOutput) {
		usage = kIOAudioControlUsageOutput;
		initOssDev = SOUND_MIXER_VOLUME;
		initOssMask = SOUND_MASK_VOLUME;
	}	
	else if (direction == kIOAudioStreamDirectionInput) {
		usage = kIOAudioControlUsageInput;
		initOssDev = SOUND_MIXER_MIC;
		initOssMask = SOUND_MASK_MIC | SOUND_MASK_MONITOR;
	}
	else {
		errorMsg("uknown direction\n");
		goto Done;
	}

	minMaxDb = getMinMaxDb(initOssMask);
	minDb = (IOFixed) (minMaxDb >> 32);
	maxDb = (IOFixed) (minMaxDb & ~0UL);
//	logMsg("minDb: %d (%08lx), maxDb: %d (%08lx)\n", (SInt16) (minDb >> 16), minDb,
//		   (SInt16) (maxDb >> 16), maxDb);
	if ((minDb == ~0L) || (maxDb == ~0L)) {
		errorMsg("warning: found invalid min/max dB (using default -22.5 -> 0.0dB range)\n"); //-22.5 -> 0.0
		minDb = (-22 << 16) + (65536 / 2);
		maxDb = 0 << 16;
	}
	
	/* Create Volume controls */
	/* Left channel */
	control = IOAudioLevelControl::createVolumeControl(gMixerDefaults[initOssDev],
													   0,	
													   100,	
													   minDb,
													   maxDb,
													   kIOAudioControlChannelIDDefaultLeft,
													   kIOAudioControlChannelNameLeft,
													   0,
													   usage);
    if (!control) {
		errorMsg("error: createVolumeControl failed\n");
        goto Done;
    }
    
    control->setValueChangeHandler((IOAudioControl::IntValueChangeHandler)volumeChangeHandler, this);
    this->addDefaultAudioControl(control);
    control->release();
    
	/* Right channel */
	control = IOAudioLevelControl::createVolumeControl(gMixerDefaults[initOssDev],
													   0,	
													   100,	
													   minDb,
													   maxDb,
													   kIOAudioControlChannelIDDefaultRight,
													   kIOAudioControlChannelNameRight,
													   0,
													   usage);
    if (!control) {
		errorMsg("error: createVolumeControl failed\n");
        goto Done;
    }
    
    control->setValueChangeHandler((IOAudioControl::IntValueChangeHandler)volumeChangeHandler, this);
    this->addDefaultAudioControl(control);
    control->release();
    
	// Create mute control
    control = IOAudioToggleControl::createMuteControl(false,	// initial state - unmuted
													  kIOAudioControlChannelIDAll,	// Affects all channels
													  kIOAudioControlChannelNameAll,
													  0,
													  usage);
    if (!control) {
		errorMsg("error: createMuteControl failed\n");
        goto Done;
    }
	
    control->setValueChangeHandler((IOAudioControl::IntValueChangeHandler)muteChangeHandler, this);
    this->addDefaultAudioControl(control);
    control->release();

    // Create a left & right input gain control with an int range from 0 to 65535
    // and a db range from 0 to 22.5
    control = IOAudioLevelControl::createVolumeControl(65535,	// Initial value
													   0,		// min value
													   65535,	// max value
													   0,		// min 0.0 in IOFixed
													   (22 << 16) + (32768),	// 22.5 in IOFixed (16.16)
													   kIOAudioControlChannelIDDefaultLeft,
													   kIOAudioControlChannelNameLeft,
													   0,		// control ID - driver-defined
													   kIOAudioControlUsageInput);
    if (!control) {
        goto Done;
    }
    
    control->setValueChangeHandler((IOAudioControl::IntValueChangeHandler)gainChangeHandler, this);
    this->addDefaultAudioControl(control);
    control->release();
    
    control = IOAudioLevelControl::createVolumeControl(65535,	// Initial value
													   0,		// min value
													   65535,	// max value
													   0,		// min 0.0 in IOFixed
													   (22 << 16) + (32768),	// max 22.5 in IOFixed (16.16)
													   kIOAudioControlChannelIDDefaultRight,	// Affects right channel
													   kIOAudioControlChannelNameRight,
													   0,		// control ID - driver-defined
													   kIOAudioControlUsageInput);
    if (!control) {
        goto Done;
    }
	
    control->setValueChangeHandler((IOAudioControl::IntValueChangeHandler)gainChangeHandler, this);
    this->addDefaultAudioControl(control);
    control->release();
	
    
	if(usage == kIOAudioControlUsageOutput) {
		mSelControl = IOAudioSelectorControl::createOutputSelector(0, kIOAudioControlChannelIDAll);
	}else{
		mSelControl = IOAudioSelectorControl::createInputSelector(0, kIOAudioControlChannelIDAll, kIOAudioControlChannelNameAll, 6);
	}
	if(mSelControl != 0) {
		this->addDefaultAudioControl(mSelControl);
		enumiratePinNames();
	}
	
	
	result = true;

Done:
	return result;
}

void VoodooHDAEngine::enumiratePinNames(void)
{
	if(mSelControl == 0) 
		return;
	
	
}

void VoodooHDAEngine::setPinName(UInt32 type, const char* name)
{
	if(mSelControl == 0) 
		return;
	
	mSelControl->addAvailableSelection('test', "test");
	OSNumber* nom = OSNumber::withNumber('test', 32);
	mSelControl->hardwareValueChanged(nom);
	setDescription(name);
	mSelControl->flushValue();
	mSelControl->removeAvailableSelection('test');
	setDescription(name);
}

IOReturn VoodooHDAEngine::volumeChangeHandler(IOService *target, IOAudioControl *volumeControl, SInt32 oldValue, SInt32 newValue)
{
    IOReturn result = kIOReturnBadArgument;
    VoodooHDAEngine *audioEngine;
    
    audioEngine = (VoodooHDAEngine *)target;
    if (audioEngine) {
        result = audioEngine->volumeChanged(volumeControl, oldValue, newValue);
    }
    
    return result;
}

IOReturn VoodooHDAEngine::volumeChanged(IOAudioControl *volumeControl, SInt32 oldValue, SInt32 newValue)
{
	if(mVerbose >2)
		errorMsg("VoodooHDAEngine[%p]::volumeChanged(%p, %ld, %ld)\n", this, volumeControl, (long int)oldValue, (long int)newValue);
    
    if (volumeControl) {

		int ossDev = ( getEngineDirection() == kIOAudioStreamDirectionOutput) ? SOUND_MIXER_VOLUME:
																				SOUND_MIXER_MIC;
		
		PcmDevice *pcmDevice = mChannel->pcmDevice;
		
		switch (ossDev) {
			case SOUND_MIXER_VOLUME:
				/* Left channel */
				if(volumeControl->getChannelID() == 1) {
					oldOutVolumeLeft = newValue;
					mDevice->audioCtlOssMixerSet(pcmDevice, ossDev, newValue, pcmDevice->right[0]);
				}
				/* Right channel */
				else if(volumeControl->getChannelID() == 2) {
					oldOutVolumeRight = newValue;
					mDevice->audioCtlOssMixerSet(pcmDevice, ossDev, pcmDevice->left[0], newValue);
				}
				
				break;
			case SOUND_MIXER_MIC:
				oldInputGain = newValue;
				mDevice->audioCtlOssMixerSet(pcmDevice, ossDev, newValue, newValue);
				break;
			default:
				break;
		}
	}
	
    return kIOReturnSuccess;
}

IOReturn VoodooHDAEngine::muteChangeHandler(IOService *target, IOAudioControl *muteControl, SInt32 oldValue, SInt32 newValue)
{
    IOReturn result = kIOReturnBadArgument;
    VoodooHDAEngine *audioEngine;
    
    audioEngine = (VoodooHDAEngine *)target;
    if (audioEngine) {
        result = audioEngine->muteChanged(muteControl, oldValue, newValue);
    }
    
    return result;
}

IOReturn VoodooHDAEngine::muteChanged(IOAudioControl *muteControl, SInt32 oldValue, SInt32 newValue)
{
	if(mVerbose >2)
		errorMsg("VoodooHDAEngine[%p]::outputMuteChanged(%p, %ld, %ld)\n", this, muteControl, (long int)oldValue, (long int)newValue);
    
	int ossDev = ( getEngineDirection() == kIOAudioStreamDirectionOutput) ? SOUND_MIXER_VOLUME:
																			SOUND_MIXER_MIC;
	
	PcmDevice *pcmDevice = mChannel->pcmDevice;
	
	if (newValue) {
		mDevice->audioCtlOssMixerSet(pcmDevice, ossDev, 0, 0);
	} else {
		mDevice->audioCtlOssMixerSet(pcmDevice, ossDev,
									   (ossDev == SOUND_MIXER_VOLUME) ? oldOutVolumeLeft : oldInputGain,
									   (ossDev == SOUND_MIXER_VOLUME) ? oldOutVolumeRight: oldInputGain);
	}
    
    return kIOReturnSuccess;
}

IOReturn VoodooHDAEngine::gainChangeHandler(IOService *target, IOAudioControl *gainControl, SInt32 oldValue, SInt32 newValue)
{
    IOReturn result = kIOReturnBadArgument;
    VoodooHDAEngine *audioDevice;
    
    audioDevice = (VoodooHDAEngine *)target;
    if (audioDevice) {
        result = audioDevice->gainChanged(gainControl, oldValue, newValue);
    }
    
    return result;
}

IOReturn VoodooHDAEngine::gainChanged(IOAudioControl *gainControl, SInt32 oldValue, SInt32 newValue)
{
    IOLog("VoodooHDAEngine[%p]::gainChanged(%p, %ld, %ld)\n", this, gainControl, (long int)oldValue, (long int)newValue);
    
    if (gainControl) {
       IOLog("\t-> Channel %ld\n", (long int)gainControl->getChannelID());
    }
    
    // Add hardware gain change code here 
	
    return kIOReturnSuccess;
}
