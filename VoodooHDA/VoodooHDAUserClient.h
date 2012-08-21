#include "License.h"

#ifndef _VOODOO_HDA_USER_CLIENT_H
#define _VOODOO_HDA_USER_CLIENT_H

#include <IOKit/IOLib.h>
#include <IOKit/IOUserClient.h>

class VoodooHDADevice;

class VoodooHDAUserClient : public IOUserClient
{
	OSDeclareDefaultStructors(VoodooHDAUserClient);

private:
	UInt32 mVerbose;
	VoodooHDADevice *mDevice;

public:
	void messageHandler(UInt32 type, const char *format, ...) __attribute__ ((format (printf, 3, 4)));

	/* IOService overrides */
	virtual void free();
	virtual bool start(IOService *provider);
	virtual void stop(IOService *provider);

	/* IOUserClient overrides */
	virtual bool initWithTask(task_t owningTask, void *securityID, UInt32 type, OSDictionary *properties);
	virtual IOReturn clientClose();
	virtual IOReturn clientDied();

	virtual bool willTerminate(IOService *provider, IOOptionBits options);
	virtual bool didTerminate(IOService *provider, IOOptionBits options, bool *defer);
	virtual bool terminate(IOOptionBits options = 0);

	virtual IOExternalMethod *getTargetAndMethodForIndex(IOService **targetP, UInt32 index);

	virtual IOReturn clientMemoryForType(UInt32 type, IOOptionBits *options, IOMemoryDescriptor **memory);

	/* External methods */
	IOReturn actionMethod(UInt32 *dataIn, UInt32 *dataOut, IOByteCount inputSize, IOByteCount *outputSize);
};

#endif