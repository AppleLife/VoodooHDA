#include "License.h"
#ifdef VOODOOCLIP
#include "VoodooHDADevice.h"
#include "VoodooHDAEngine.h"
#include "PCMBlitterLibDispatch.h"

#define errorMsg(fmt, args...)	do { if (mDevice) mDevice->errorMsg(fmt, ##args); } while (0)

IOReturn VoodooHDAEngine::clipOutputSamples(const void *mixBuf, void *sampleBuf, UInt32 firstSampleFrame,
		UInt32 numSampleFrames, const IOAudioStreamFormat *streamFormat,
		__unused IOAudioStream *audioStream)
{
	UInt8 *sourceBuf = (UInt8 *) sampleBuf;

	// figure out what sort of blit we need to do
	if ((streamFormat->fSampleFormat == kIOAudioStreamSampleFormatLinearPCM) && streamFormat->fIsMixable) {
		// it's mixable linear PCM, which means we will be calling a blitter, which works in samples
		// not frames
		Float32 *floatMixBuf = (Float32 *) mixBuf;
		UInt32 firstSample = firstSampleFrame * streamFormat->fNumChannels;
		UInt32 numSamples = numSampleFrames * streamFormat->fNumChannels;

		if (streamFormat->fNumericRepresentation == kIOAudioStreamNumericRepresentationSignedInt) {
			// it's some kind of signed integer, which we handle as some kind of even byte length
			bool nativeEndianInts;
			nativeEndianInts = (streamFormat->fByteOrder == kIOAudioStreamByteOrderLittleEndian);

			switch (streamFormat->fBitWidth) {
			case 16:
				if (nativeEndianInts)
					Float32ToNativeInt16(&floatMixBuf[firstSample], (SInt16 *) &sourceBuf[2 * firstSample],
							numSamples);
				else
					Float32ToSwapInt16(&floatMixBuf[firstSample], (SInt16 *) &sourceBuf[2 * firstSample],
							numSamples);
				break;

			case 24:
				if (nativeEndianInts)
					Float32ToNativeInt24(&floatMixBuf[firstSample], &sourceBuf[3 * firstSample], numSamples);
				else
					Float32ToSwapInt24(&floatMixBuf[firstSample], &sourceBuf[3 * firstSample], numSamples);
				break;

			case 32:
				if (nativeEndianInts)
					Float32ToNativeInt32(&floatMixBuf[firstSample], (SInt32 *) &sourceBuf[4 * firstSample],
							numSamples);
				else
					Float32ToSwapInt32(&floatMixBuf[firstSample], (SInt32 *) &sourceBuf[4 * firstSample],
							numSamples);
				break;

			default:
				errorMsg("clipOutputSamples: can't handle signed integers with a bit width of %d",
						streamFormat->fBitWidth);
				break;

			}
		} else if (streamFormat->fNumericRepresentation == kIOAudioStreamNumericRepresentationIEEE754Float) {
			// it is some kind of floating point format
			if ((streamFormat->fBitWidth == 32) && (streamFormat->fBitDepth == 32) &&
					(streamFormat->fByteOrder == kIOAudioStreamByteOrderLittleEndian)) {
				// it's Float32, so we are just going to copy the data
				memcpy(&((Float32 *) sampleBuf)[firstSample], &floatMixBuf[firstSample],
						numSamples * sizeof (Float32));
			} else
				errorMsg("clipOutputSamples: can't handle floats with a bit width of %d, bit depth of %d, "
						"and/or the given byte order", streamFormat->fBitWidth, streamFormat->fBitDepth);
		}
	} else {
		// it's not linear PCM or it's not mixable, so just copy the data into the target buffer
		UInt32 offset = firstSampleFrame * (streamFormat->fBitWidth / 8) * streamFormat->fNumChannels;
		UInt32 size = numSampleFrames * (streamFormat->fBitWidth / 8) * streamFormat->fNumChannels;
		memcpy(&((SInt8 *) sampleBuf)[offset], &((SInt8 *) mixBuf)[offset], size);
	}

	return kIOReturnSuccess;
}

IOReturn VoodooHDAEngine::convertInputSamples(const void *sampleBuf, void *destBuf,
		UInt32 firstSampleFrame, UInt32 numSampleFrames, const IOAudioStreamFormat *streamFormat,
		__unused IOAudioStream *audioStream)
{
	UInt8 *sourceBuf = (UInt8 *) sampleBuf;

	// figure out what sort of blit we need to do
	if ((streamFormat->fSampleFormat == kIOAudioStreamSampleFormatLinearPCM) && streamFormat->fIsMixable) {
		// it's linear PCM, which means the target is Float32 and we will be calling a blitter, which
		// works in samples not frames
		Float32 *floatDestBuf = (Float32 *) destBuf;
		UInt32 firstSample = firstSampleFrame * streamFormat->fNumChannels;
		UInt32 numSamples = numSampleFrames * streamFormat->fNumChannels;

		if (streamFormat->fNumericRepresentation == kIOAudioStreamNumericRepresentationSignedInt) {
			// it's some kind of signed integer, which we handle as some kind of even byte length
			bool nativeEndianInts;
			nativeEndianInts = (streamFormat->fByteOrder == kIOAudioStreamByteOrderLittleEndian);

			switch (streamFormat->fBitWidth) {
			case 16:
				if (nativeEndianInts)
					NativeInt16ToFloat32((SInt16 *) &sourceBuf[2 * firstSample], floatDestBuf, numSamples);
				else
					SwapInt16ToFloat32((SInt16 *) &sourceBuf[2 * firstSample], floatDestBuf, numSamples);
				break;

			case 24:
				if (nativeEndianInts)
					NativeInt24ToFloat32(&sourceBuf[3 * firstSample], floatDestBuf, numSamples);
				else
					SwapInt24ToFloat32(&sourceBuf[3 * firstSample], floatDestBuf, numSamples);
				break;

			case 32:
				if (nativeEndianInts) 
					NativeInt32ToFloat32((SInt32 *) &sourceBuf[4 * firstSample], floatDestBuf, numSamples);
				else
					SwapInt32ToFloat32((SInt32 *) &sourceBuf[4 * firstSample], floatDestBuf, numSamples);
				break;

			default:
				errorMsg("convertInputSamples: can't handle signed integers with a bit width of %d",
						streamFormat->fBitWidth);
				break;

			}
			
			//Меняю местами значения для левого и правого канала
			if(mDevice && mDevice->mSwitchCh && (streamFormat->fNumChannels > 1)) {
				UInt32 i;
				Float32 tempSamples;
				
				for(i = 0; i < numSamples; i+= streamFormat->fNumChannels) {
					tempSamples = floatDestBuf[i];
					floatDestBuf[i] = floatDestBuf[i+1];
					floatDestBuf[i+1] = tempSamples;
				}
			}
			
		} else if (streamFormat->fNumericRepresentation == kIOAudioStreamNumericRepresentationIEEE754Float) {
			// it is some kind of floating point format
			if ((streamFormat->fBitWidth == 32) && (streamFormat->fBitDepth == 32) &&
					(streamFormat->fByteOrder == kIOAudioStreamByteOrderLittleEndian)) {
				// it's Float32, so we are just going to copy the data
				memcpy(floatDestBuf, &((Float32 *) sampleBuf)[firstSample], numSamples * sizeof (Float32));
			} else
				errorMsg("convertInputSamples: can't handle floats with a bit width of %d, bit depth of %d, "
						"and/or the given byte order", streamFormat->fBitWidth, streamFormat->fBitDepth);
		}
	} else {
		// it's not linear PCM or it's not mixable, so just copy the data into the target buffer
		UInt32 offset = firstSampleFrame * (streamFormat->fBitWidth / 8) * streamFormat->fNumChannels;
		UInt32 size = numSampleFrames * (streamFormat->fBitWidth / 8) * streamFormat->fNumChannels;
		memcpy(destBuf, &sourceBuf[offset], size);
	}

	return kIOReturnSuccess;
}
#endif
