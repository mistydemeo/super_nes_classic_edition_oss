/*
SoLoud audio engine
Copyright (c) 2013-2015 Jari Komppa

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
*/

#include <limits.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "soloud.h"
#include "soloud_wav.h"
#include "soloud_file.h"
#include "stb_vorbis.h"
#if defined(__GNUC__) && defined(__arm__)
#include <arm_neon.h>
#endif

namespace SoLoud
{
	WavInstance::WavInstance(Wav *aParent)
	{
		mParent = aParent;
		mOffset = 0;
	}

	void WavInstance::getAudio(float *aBuffer, unsigned int aSamples)
	{		
		if (mParent->mData == NULL)
			return;

		// Buffer size may be bigger than samples, and samples may loop..

		unsigned int written = 0;
		unsigned int maxwrite = (aSamples > mParent->mSampleCount) ?  mParent->mSampleCount : aSamples;
		unsigned int channels = mChannels;

		while (written < aSamples)
		{
			unsigned int copysize = maxwrite;
			if (mFlags & AudioSourceInstance::LOOPING)
			{
				if (mOffset <= mParent->mLoopEndOffset && mOffset + copysize > mParent->mLoopEndOffset)
				{
					copysize = mParent->mLoopEndOffset - mOffset;
				}
			}

			if (copysize + mOffset > mParent->mSampleCount)
			{
				copysize = mParent->mSampleCount - mOffset;
			}

			if (copysize + written > aSamples)
			{
				copysize = aSamples - written;
			}

			unsigned int i;
			for (i = 0; i < channels; i++)
			{
				memcpy(aBuffer + i * aSamples + written, mParent->mData + mOffset + i * mParent->mSampleCount, sizeof(float) * copysize);
			}

			written += copysize;
			mOffset += copysize;				
		
			if (copysize != maxwrite)
			{
				if (mFlags & AudioSourceInstance::LOOPING)
				{
					if (mOffset == mParent->mLoopEndOffset)
					{
						mOffset = mParent->mLoopStartOffset;
						mLoopCount++;
					}
					else if (mOffset == mParent->mSampleCount)
					{
						mOffset = 0;
						mLoopCount++;
					}
				}
				else
				{
					for (i = 0; i < channels; i++)
					{
						memset(aBuffer + copysize + i * aSamples, 0, sizeof(float) * (aSamples - written));
					}
					mOffset += aSamples - written;
					written = aSamples;
				}
			}
		}
	}

	result WavInstance::rewind()
	{
		mOffset = 0;
		mStreamTime = 0;
		return 0;
	}

	bool WavInstance::hasEnded()
	{
		if (!(mFlags & AudioSourceInstance::LOOPING) && mOffset >= mParent->mSampleCount)
		{
			return 1;
		}
		return 0;
	}

	unsigned int WavInstance::getSamplePosition()
	{
		return mOffset;
	}

	Wav::Wav()
	{
		mData = NULL;
		mSampleCount = 0;
		mLoopStartOffset = 0;
		mLoopEndOffset = UINT_MAX;
	}
	
	Wav::~Wav()
	{
		stop();
		delete[] mData;
	}

#define MAKEDWORD(a,b,c,d) (((d) << 24) | ((c) << 16) | ((b) << 8) | (a))

	static void convertSamplesStereo16(float* pOut, const int16_t* pIn, size_t sampleCount)
	{
		const float invertedMax = 1.0f / 0x8000;
		size_t i = 0;

#if defined(__GNUC__) && defined(__arm__)
		float32x4_t k = vdupq_n_f32(invertedMax);
		size_t count4 = sampleCount & ~3;
		const int16_t* pInIt = pIn;
		float* pOut0It = pOut;
		float* pOut1It = pOut + sampleCount;

		for (; i < count4; i += 4)
		{
			int16x4x2_t inputs = vld2_s16(pInIt);
			int16x4_t channel0_16 = inputs.val[0];
			int16x4_t channel1_16 = inputs.val[1];

			int32x4_t channel0_32 = vmovl_s16(channel0_16);
			int32x4_t channel1_32 = vmovl_s16(channel1_16);

			float32x4_t channel0_f = vcvtq_f32_s32(channel0_32);
			float32x4_t channel1_f = vcvtq_f32_s32(channel1_32);

			channel0_f = vmulq_f32(channel0_f, k);
			channel1_f = vmulq_f32(channel1_f, k);

			vst1q_f32(pOut0It, channel0_f);
			vst1q_f32(pOut1It, channel1_f);
			pInIt += 8;
			pOut0It += 4;
			pOut1It += 4;
		}
#endif
		for (; i < sampleCount; i++)
		{
			pOut[i              ] = pIn[i * 2    ] * invertedMax;
			pOut[i + sampleCount] = pIn[i * 2 + 1] * invertedMax;
		}
	}

    result Wav::loadwav(File *aReader)
	{
		/*int wavsize =*/ aReader->read32();
		if (aReader->read32() != MAKEDWORD('W','A','V','E'))
		{
			return FILE_LOAD_FAILED;
		}
		int chunk = aReader->read32();
		if (chunk == MAKEDWORD('J', 'U', 'N', 'K'))
		{
			int size = aReader->read32();
			if (size & 1)
			{
				size += 1;
			}
			aReader->seek(aReader->pos() + size);
			chunk = aReader->read32();
		}
		if (chunk != MAKEDWORD('f', 'm', 't', ' '))
		{
			return FILE_LOAD_FAILED;
		}
		int subchunk1size = aReader->read32();
		int audioformat = aReader->read16();
		int channels = aReader->read16();
		int samplerate = aReader->read32();
		/*int byterate =*/ aReader->read32();
		/*int blockalign =*/ aReader->read16();
		int bitspersample = aReader->read16();

		if (audioformat != 1 ||
			subchunk1size != 16 ||
			(bitspersample != 8 && bitspersample != 16))
		{
			return FILE_LOAD_FAILED;
		}
		
		chunk = aReader->read32();
		
		if (chunk == MAKEDWORD('L','I','S','T'))
		{
			int size = aReader->read32();
			aReader->seek(aReader->pos() + size);
			chunk = aReader->read32();
		}
		
		if (chunk != MAKEDWORD('d','a','t','a'))
		{
			return FILE_LOAD_FAILED;
		}

		unsigned int readchannels = 1;

		if (channels > 1)
		{
			readchannels = 2;
			mChannels = 2;
		}

		const unsigned int sampleBufferSize = aReader->read32();

		void* sampleBuffer = aReader->map(sampleBufferSize);
		void* sampleBufferTmp = NULL;

		if (sampleBuffer != NULL)
		{
			aReader->seek(aReader->pos() + sampleBufferSize);
		}
		else
		{
			sampleBufferTmp = malloc(sampleBufferSize);

			if (sampleBufferTmp == NULL)
			{
				return OUT_OF_MEMORY;
			}

			aReader->read((unsigned char*)sampleBufferTmp, (unsigned int)sampleBufferSize);
			sampleBuffer = sampleBufferTmp;
		}

		const unsigned int sampleCount = sampleBufferSize / ((bitspersample / 8) * channels);
		
		mData = new float[sampleCount * readchannels];
		
		if (bitspersample == 8)
		{
			const uint8_t* samples = (uint8_t*)sampleBuffer;

			if (channels == 1)
			{
				for (unsigned int i = 0; i < sampleCount; i++)
				{
					mData[i] = ((signed)samples[i] - 128) / (float)0x80;
				}
			}
			else
			{
				for (unsigned int i = 0; i < sampleCount; i++)
				{
					mData[i              ] = ((signed)samples[i * channels    ] - 128) / (float)0x80;
					mData[i + sampleCount] = ((signed)samples[i * channels + 1] - 128) / (float)0x80;
				}
			}
		}
		else if (bitspersample == 16)
		{
			const int16_t* samples = (int16_t*)sampleBuffer;
			const float invertedMax = 1.0f / 0x8000;

			if (channels == 1)
			{
				for (unsigned int i = 0; i < sampleCount; i++)
				{
					mData[i] = samples[i] * invertedMax;
				}
			}
			else if (channels == 2)
			{
				convertSamplesStereo16(mData, samples, sampleCount);
			}
			else
			{
				for (unsigned int i = 0; i < sampleCount; i++)
				{
					mData[i              ] = samples[i * channels    ] * invertedMax;
					mData[i + sampleCount] = samples[i * channels + 1] * invertedMax;
				}
			}
		}

		free(sampleBufferTmp);
		sampleBufferTmp = NULL;

		mBaseSamplerate = (float)samplerate;
		mSampleCount = sampleCount;

		while (!aReader->eof())
		{
			chunk = aReader->read32();

			if (chunk == MAKEDWORD('s','m','p','l'))
			{
				/*int size = */aReader->read32();

				// Ignore dwManufacturer, dwProduct, dwSamplePeriod, dwMIDIUnityNote, dwMIDIPitchFraction, dwSMPTEFormat, dwSMPTEOffset
				aReader->seek(aReader->pos() + 7 * 4);

				int loopCount = aReader->read32();
				int samplerDataSize = aReader->read32();

				for (int i = 0; i < loopCount; i++)
				{
					
					/*unsigned int loopId = */aReader->read32();
					/*unsigned int loopType = */aReader->read32();
					unsigned int loopStart = aReader->read32();
					unsigned int loopEnd = aReader->read32();

					/*unsigned int loopFraction = */aReader->read32();
					/*unsigned int loopPlayCount = */aReader->read32();

					if (i == 0) // Use only the first loop record
					{
						mLoopStartOffset = loopStart;
						mLoopEndOffset   = loopEnd;
					}
				}

				// Ignore sampler data
				if (samplerDataSize & 1)
					samplerDataSize += 1;
				if (samplerDataSize > 0)
					aReader->seek(aReader->pos() + samplerDataSize);
			}
			else
			{
				// Ignore chunk
				int size = aReader->read32();
				if (size & 1)
					size += 1;
				if (size > 0)
					aReader->seek(aReader->pos() + size);
			}
		}

		return 0;
	}

	result Wav::loadogg(stb_vorbis *aVorbis)
	{
        stb_vorbis_info info = stb_vorbis_get_info(aVorbis);
		mBaseSamplerate = (float)info.sample_rate;
        int samples = stb_vorbis_stream_length_in_samples(aVorbis);

		int readchannels = 1;
		if (info.channels > 1)
		{
			readchannels = 2;
			mChannels = 2;
		}
		mData = new float[samples * readchannels];
		mSampleCount = samples;
		samples = 0;
		while(1)
		{
			float **outputs;
            int n = stb_vorbis_get_frame_float(aVorbis, NULL, &outputs);
			if (n == 0)
            {
				break;
            }
			if (readchannels == 1)
			{
				memcpy(mData + samples, outputs[0],sizeof(float) * n);
			}
			else
			{
				memcpy(mData + samples, outputs[0],sizeof(float) * n);
				memcpy(mData + samples + mSampleCount, outputs[1],sizeof(float) * n);
			}
			samples += n;
		}
        stb_vorbis_close(aVorbis);

		return 0;
	}

    result Wav::testAndLoadFile(File *aReader)
    {
		delete[] mData;
		mData = 0;
		mSampleCount = 0;
        int tag = aReader->read32();
		if (tag == MAKEDWORD('O','g','g','S')) 
        {
		 	aReader->seek(0);
			int e = 0;
			stb_vorbis *v = 0;
			v = stb_vorbis_open_file((Soloud_Filehack*)aReader, 0, &e, 0);

			if (0 != v)
            {
				return loadogg(v);
            }
			return FILE_LOAD_FAILED;
		} 
        else if (tag == MAKEDWORD('R','I','F','F')) 
        {
			return loadwav(aReader);
		}
		return FILE_LOAD_FAILED;
    }

	result Wav::load(const char *aFilename)
	{
		DiskFile dr;
		int res = dr.open(aFilename);
		if (res != SO_NO_ERROR)
        {
			return res;
        }
		return testAndLoadFile(&dr);
	}

	result Wav::loadMem(unsigned char *aMem, unsigned int aLength, bool aCopy, bool aTakeOwnership)
	{
		if (aMem == NULL || aLength == 0)
			return INVALID_PARAMETER;

		MemoryFile dr;
        dr.openMem(aMem, aLength, aCopy, aTakeOwnership);
		return testAndLoadFile(&dr);
	}

	result Wav::loadFile(File *aFile)
	{
		return testAndLoadFile(aFile);
	}

	AudioSourceInstance *Wav::createInstance()
	{
		return new WavInstance(this);
	}

	double Wav::getLength()
	{
		if (mBaseSamplerate == 0)
			return 0;
		return mSampleCount / mBaseSamplerate;
	}
};
