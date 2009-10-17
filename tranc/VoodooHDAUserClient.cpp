#include "License.h"

// This class represents the user client object for the driver, which
// will be instantiated by IOKit to represent a connection to the client
// process, in response to the client's call to IOServiceOpen().
// It will be destroyed when the connection is closed or the client 
// abnormally terminates, so it should track all the resources allocated
// to the client.

#include "VoodooHDAUserClient.h"
#include "VoodooHDADevice.h"
#include "Common.h"

#include "Shared.h"

#define super IOUserClient
OSDefineMetaClassAndStructors(VoodooHDAUserClient, IOUserClient);

#define logMsg(fmt, args...)	messageHandler(kVoodooHDAMessageTypeGeneral, fmt, ##args)
#define errorMsg(fmt, args...)	messageHandler(kVoodooHDAMessageTypeError, fmt, ##args)
#define dumpMsg(fmt, args...)	messageHandler(kVoodooHDAMessageTypeDump, fmt, ##args)

void VoodooHDAUserClient::messageHandler(UInt32 type, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	if (mDevice)
		mDevice->messageHandler(type, format, args);
	else if (mVerbose >= 1)
		vprintf(format, args);
	va_end(args);
}

bool VoodooHDAUserClient::initWithTask(task_t owningTask, void *securityID, UInt32 type,
		OSDictionary *properties)
{
//	logMsg("VoodooHDAUserClient[%p]::initWithTask(%ld)\n", this, type);

	return super::initWithTask(owningTask, securityID, type, properties);
}

bool VoodooHDAUserClient::start(IOService *provider)
{
//	logMsg("VoodooHDAUserClient[%p]::start\n", this);

	if (!super::start(provider))
		return false;

	mDevice = OSDynamicCast(VoodooHDADevice, provider);
	ASSERT(mDevice);
	mDevice->retain();

	mVerbose = mDevice->mVerbose;

	return true;
}

bool VoodooHDAUserClient::willTerminate(IOService *provider, IOOptionBits options)
{
//	logMsg("VoodooHDAUserClient[%p]::willTerminate\n", this);

	return super::willTerminate(provider, options);
}

bool VoodooHDAUserClient::didTerminate(IOService *provider, IOOptionBits options, bool *defer)
{
//	logMsg("VoodooHDAUserClient[%p]::didTerminate\n", this);

	// if defer is true, stop will not be called on the user client
	*defer = false;

	return super::didTerminate(provider, options, defer);
}

bool VoodooHDAUserClient::terminate(IOOptionBits options)
{
//	logMsg("VoodooHDAUserClient[%p]::terminate\n", this);

	return super::terminate(options);
}

// clientClose is called when the user process calls IOServiceClose
IOReturn VoodooHDAUserClient::clientClose()
{
//    logMsg("VoodooHDAUserClient[%p]::clientClose\n", this);

	if (!isInactive())
		terminate();

	return kIOReturnSuccess;
}

// clientDied is called when the user process terminates unexpectedly, the default
// implementation simply calls clientClose
IOReturn VoodooHDAUserClient::clientDied()
{
//	logMsg("VoodooHDAUserClient[%p]::clientDied\n", this);

	return super::clientDied();
}

void VoodooHDAUserClient::free(void)
{
//	logMsg("VoodooHDAUserClient[%p]::free\n", this);

	RELEASE(mDevice);

	super::free();
}

// stop will be called during the termination process, and should free all resources
// associated with this client
void VoodooHDAUserClient::stop(IOService *provider)
{
//	logMsg("VoodooHDAUserClient[%p]::stop\n", this);

	super::stop(provider);
}

// getTargetAndMethodForIndex looks up the external methods - supply a description of the parameters 
// available to be called 
IOExternalMethod *VoodooHDAUserClient::getTargetAndMethodForIndex(IOService **targetP, UInt32 index)
{
	//logMsg("VoodooHDAUserClient[%p]::getTargetAndMethodForIndex(%ld)\n", this, index);

	static const IOExternalMethod methodDescs[kVoodooHDANumMethods] = {
		{ NULL, (IOMethod) &VoodooHDAUserClient::actionMethod, kIOUCStructIStructO,
				kIOUCVariableStructureSize, kIOUCVariableStructureSize },
	};

	*targetP = this;
	if (index < kVoodooHDANumMethods)
		return (IOExternalMethod *) (methodDescs + index);
	else
		return NULL;
}

IOReturn VoodooHDAUserClient::actionMethod(UInt32 *dataIn, UInt32 *dataOut, IOByteCount inputSize,
		IOByteCount *outputSize)
{
	IOReturn result;
	UInt32 action;
	void *data;
	UInt32 dataSize, outputMax;

	//logMsg("VoodooHDAUserClient[%p]::actionMethod(%ld, %ld)\n", this, inputSize, *outputSize);

	if (inputSize != sizeof (UInt32))
		return kIOReturnBadArgument;
	action = *dataIn;

	result = mDevice->runAction(action, &dataSize, &data);

	// note: we can only transfer sizeof (io_struct_inband_t) bytes out at a time

	outputMax = *outputSize;
    *outputSize = dataSize;
	if (dataSize) {
		ASSERT(data);
	    if (outputMax < dataSize)
	        return kIOReturnNoSpace;
		bcopy(data, dataOut, dataSize);
	}

    return result;
}

IOReturn VoodooHDAUserClient::clientMemoryForType(UInt32 type, IOOptionBits *options,
		IOMemoryDescriptor **memory)
{
	IOReturn result;
	IOBufferMemoryDescriptor *memDesc;
	char *msgBuffer;
	ChannelInfo *channelInfoBuffer;
	UInt32		channelInfoBufferSize = 0;


//	logMsg("VoodooHDAUserClient[%p]::clientMemoryForType(0x%lx)\n", this, type);

	// note: IOConnectUnmapMemory should not be used with this user client

	*options = 0;
	*memory = NULL;

	switch (type) {
	case kVoodooHDAMemoryMessageBuffer:
		mDevice->lockMsgBuffer();
		if (!mDevice->mMsgBufferSize) {
			errorMsg("error: message buffer size is zero\n");
			mDevice->unlockMsgBuffer();
			result = kIOReturnUnsupported;
			break;
		}
		memDesc = IOBufferMemoryDescriptor::withOptions(kIOMemoryKernelUserShared,
				mDevice->mMsgBufferSize);
		if (!memDesc) {
			errorMsg("error: couldn't allocate buffer memory descriptor (size: %ld)\n",
					mDevice->mMsgBufferSize);
			mDevice->unlockMsgBuffer();
			result = kIOReturnVMError;
			break;
		}
		msgBuffer = (char *) memDesc->getBytesNoCopy();
		bcopy(mDevice->mMsgBuffer, msgBuffer, mDevice->mMsgBufferSize);
		mDevice->unlockMsgBuffer();
		*options |= kIOMapReadOnly;
		*memory = memDesc; // automatically released after memory is mapped into task
		result = kIOReturnSuccess;
		break;
			//Разделяемая память для PrefPanel
	case kVoodooHDAMemoryCommand:
		mDevice->lockPrefPanelMemoryBuf();
		if (!mDevice->mPrefPanelMemoryBuf) {
			errorMsg("error: message buffer size is zero\n");
			mDevice->unlockPrefPanelMemoryBuf();
			result = kIOReturnUnsupported;
			break;
		}
		memDesc = IOBufferMemoryDescriptor::withOptions(kIOMemoryKernelUserShared,
														mDevice->mPrefPanelMemoryBufSize);
		if (!memDesc) {
			errorMsg("error: couldn't allocate buffer memory descriptor (size: %ld)\n",
					 mDevice->mPrefPanelMemoryBufSize);
			mDevice->unlockPrefPanelMemoryBuf();
			result = kIOReturnVMError;
			break;
		}
		msgBuffer = (char *) memDesc->getBytesNoCopy();
		bcopy(mDevice->mPrefPanelMemoryBuf, msgBuffer, mDevice->mPrefPanelMemoryBufSize);
		mDevice->unlockPrefPanelMemoryBuf();
		//*options |= kIOMapReadOnly;
		*memory = memDesc; // automatically released after memory is mapped into task
		result = kIOReturnSuccess;
		break;
			//Разделяемая память для буфера с текущеми настройками усиления
	case kVoodooHDAMemoryExtMessageBuffer:
			channelInfoBuffer = mDevice->getChannelInfo();
			if (!channelInfoBuffer)
				return kIOReturnError;
			
			channelInfoBufferSize = sizeof(*channelInfoBuffer) * channelInfoBuffer->numChannels;
			//IOLog("infoBufferSize %ld\n", channelInfoBufferSize);
			if (!channelInfoBufferSize)
				return kIOReturnError;

	
		mDevice->lockExtMsgBuffer();
		if (!mDevice->mExtMsgBufferSize) {
			errorMsg("error: ext message buffer size is zero\n");
			mDevice->unlockExtMsgBuffer();
			result = kIOReturnUnsupported;
			break;
		}
		memDesc = IOBufferMemoryDescriptor::withOptions(kIOMemoryKernelUserShared,
														mDevice->mExtMsgBufferSize);
		if (!memDesc) {
			errorMsg("error: couldn't allocate buffer memory descriptor (size: %ld)\n",
					 mDevice->mExtMsgBufferSize);
			mDevice->unlockExtMsgBuffer();
			result = kIOReturnVMError;
			break;
		}
		msgBuffer = (char *) memDesc->getBytesNoCopy();
		bcopy(mDevice->mExtMsgBuffer, msgBuffer, mDevice->mExtMsgBufferSize);
		mDevice->unlockExtMsgBuffer();
		*options |= kIOMapReadOnly;
		*memory = memDesc; // automatically released after memory is mapped into task
		result = kIOReturnSuccess;
		break;
	default:
		result = kIOReturnBadArgument;
		break;
	}

	return result;
}
