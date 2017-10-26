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
#include "soloud_wavstream.h"
#include "soloud_file.h"
#include "stb_vorbis.h"
#if defined(__GNUC__) && defined(__arm__)
#include <arm_neon.h>
#endif

namespace SoLoud
{
	WavStreamInstance::WavStreamInstance(WavStream *aParent)
	{
		mParent = aParent;
		mOffset = 0;
		mOgg = 0;
		mFile = 0;
		mReadBuffer = 0;
		mReadBufferSize = 0;
		if (aParent->mMemFile)
		{
			MemoryFile *mf = new MemoryFile();
			mFile = mf;
			mf->openMem(aParent->mMemFile->getMemPtr(), aParent->mMemFile->length(), false, false);
		}
		else
		if (aParent->mFilename)
		{
			DiskFile *df = new DiskFile;
			mFile = df;
			df->open(aParent->mFilename);
		}
		else
		if (aParent->mStreamFile)
		{
			mFile = aParent->mStreamFile;
			mFile->seek(0); // stb_vorbis assumes file offset to be at start of ogg
		}
		else
		{
			return;
		}
		
		if (mFile)
		{
			if (mParent->mOgg)
			{
				int e;

				mOgg = stb_vorbis_open_file((Soloud_Filehack *)mFile, 0, &e, 0);

				if (!mOgg)
				{
					if (mFile != mParent->mStreamFile)
						delete mFile;
					mFile = 0;
				}
				mOggFrameSize = 0;
				mOggFrameOffset = 0;
				mOggOutputs = 0;
			}
			else
			{		
				mFile->seek(aParent->mDataOffset);
			}
		}
	}

	WavStreamInstance::~WavStreamInstance()
	{
		if (mReadBuffer)
		{
			free(mReadBuffer);
			mReadBuffer = NULL;
			mReadBufferSize = 0;
		}
		if (mOgg)
		{
			stb_vorbis_close(mOgg);
		}
		if (mFile != mParent->mStreamFile)
		{
			delete mFile;
		}
	}

	static void convertSamplesStereo16(float* pOut, const int16_t* pIn, size_t sampleCount, size_t pitch)
	{
		const float invertedMax = 1.0f / 0x8000;
		size_t i = 0;

#if defined(__GNUC__) && defined(__arm__)
		float32x4_t k = vdupq_n_f32(invertedMax);
		size_t count4 = sampleCount & ~3;
		const int16_t* pInIt = pIn;
		float* pOut0It = pOut;
		float* pOut1It = pOut + pitch;

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
			pOut[i] = pIn[i * 2] * invertedMax;
			pOut[i + pitch] = pIn[i * 2 + 1] * invertedMax;
		}
	}

	void WavStreamInstance::getWavData(File * aFile, float * aBuffer, unsigned int aSamples, unsigned int aPitch, unsigned int /*aChannels*/, unsigned int aSrcChannels, unsigned int aBits)
	{
		const unsigned int sampleBufferSize = aSamples * aSrcChannels * (aBits / 8);

		void* sampleBuffer = aFile->map(sampleBufferSize);

		if (sampleBuffer == NULL)
		{
			if (sampleBufferSize > mReadBufferSize)
			{
				free(mReadBuffer);
				mReadBufferSize = 0;
				mReadBuffer = malloc(sampleBufferSize);
				if (mReadBuffer == NULL)
				{
					return;
				}
				mReadBufferSize = sampleBufferSize;
			}

			aFile->read((unsigned char*)mReadBuffer, (unsigned int)sampleBufferSize);
			sampleBuffer = mReadBuffer;
		}

		const unsigned int sampleCount = aSamples;

		if (aBits == 8)
		{
			const uint8_t* samples = (uint8_t*)sampleBuffer;

			if (aSrcChannels == 1)
			{
				for (unsigned int i = 0; i < aSamples; i++)
				{
					aBuffer[i] = ((signed)samples[i] - 128) / (float)0x80;
				}
			}
			else
			{
				for (unsigned int i = 0; i < aSamples; i++)
				{
					aBuffer[i] = ((signed)samples[i * aSrcChannels] - 128) / (float)0x80;
					aBuffer[i + aPitch] = ((signed)samples[i * aSrcChannels + 1] - 128) / (float)0x80;
				}
			}
		}
		else if (aBits == 16)
		{
			const int16_t* samples = (int16_t*)sampleBuffer;

			if (aSrcChannels == 2)
			{
				convertSamplesStereo16(aBuffer, samples, aSamples, aPitch);
			}
			else
			{
				const float invertedMax = 1.0f / 0x8000;

				if (aSrcChannels == 1)
				{
					for (unsigned int i = 0; i < aSamples; i++)
					{
						aBuffer[i] = samples[i] * invertedMax;
					}
				}
				else
				{
					for (unsigned int i = 0; i < aSamples; i++)
					{
						aBuffer[i] = samples[i * aSrcChannels] * invertedMax;
						aBuffer[i + aPitch] = samples[i * aSrcChannels + 1] * invertedMax;
					}
				}
			}
		}
	}

	static int getOggData(float **aOggOutputs, float *aBuffer, int aSamples, int aPitch, int aFrameSize, int aFrameOffset, int aChannels)
	{			
		if (aFrameSize <= 0)
			return 0;

		int samples = aSamples;
		if (aFrameSize - aFrameOffset < samples)
		{
			samples = aFrameSize - aFrameOffset;
		}

		if (aChannels == 1)
		{
			memcpy(aBuffer, aOggOutputs[0] + aFrameOffset, sizeof(float) * samples);
		}
		else
		{
			memcpy(aBuffer, aOggOutputs[0] + aFrameOffset, sizeof(float) * samples);
			memcpy(aBuffer + aPitch, aOggOutputs[1] + aFrameOffset, sizeof(float) * samples);
		}
		return samples;
	}

	void WavStreamInstance::getAudio(float *aBuffer, unsigned int aSamples)
	{			
		unsigned int channels = mChannels;

		if (mFile == NULL)
			return;

		if (mOgg)
		{
			unsigned int offset = 0;			
			if (mOggFrameOffset < mOggFrameSize)
			{
				int b = getOggData(mOggOutputs, aBuffer, aSamples, aSamples, mOggFrameSize, mOggFrameOffset, channels);
				mOffset += b;
				offset += b;
				mOggFrameOffset += b;
			}

			while (offset < aSamples)
			{
				mOggFrameSize = stb_vorbis_get_frame_float(mOgg, NULL, &mOggOutputs);
				mOggFrameOffset = 0;
				int b;
				b = getOggData(mOggOutputs, aBuffer + offset, aSamples - offset, aSamples, mOggFrameSize, mOggFrameOffset, channels);
				mOffset += b;
				offset += b;
				mOggFrameOffset += b;
				if (mOffset >= mParent->mSampleCount)
				{
					if (mFlags & AudioSourceInstance::LOOPING)
					{
						stb_vorbis_seek_start(mOgg);
						mOffset = aSamples - offset;
						mLoopCount++;
					}
					else
					{
						unsigned int i;
						for (i = 0; i < channels; i++)
							memset(aBuffer + offset + i * aSamples, 0, sizeof(float) * (aSamples - offset));
						mOffset += aSamples - offset;
						offset = aSamples;
					}
				}
			}
		}
		else
		{
			// Buffer size may be bigger than samples, and samples may loop..

			unsigned int written = 0;
			unsigned int maxwrite = (aSamples > mParent->mSampleCount) ? mParent->mSampleCount : aSamples;

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

				getWavData(mFile, aBuffer, copysize, aSamples, channels, mParent->mChannels, mParent->mBits);

				written += copysize;
				mOffset += copysize;

				if (copysize != maxwrite)
				{
					if (mFlags & AudioSourceInstance::LOOPING)
					{
						if (mOffset == mParent->mLoopEndOffset)
						{
							mOffset = mParent->mLoopStartOffset;
							mFile->seek(mParent->mDataOffset + mOffset * mParent->mChannels * (mParent->mBits / 8));
							mLoopCount++;
						}
						else if (mOffset == mParent->mSampleCount)
						{
							mOffset = 0;
							mFile->seek(mParent->mDataOffset);
							mLoopCount++;
						}
					}
					else
					{
						for (unsigned int i = 0; i < channels; i++)
						{
							memset(aBuffer + copysize + i * aSamples, 0, sizeof(float) * (aSamples - written));
						}
						mOffset += aSamples - written;
						written = aSamples;
					}
				}
			}
		}
	}

	result WavStreamInstance::rewind()
	{
		if (mOgg)
		{
			stb_vorbis_seek_start(mOgg);
		}
		else
		if (mFile)
		{
			mFile->seek(mParent->mDataOffset);
		}
		mOffset = 0;
		mStreamTime = 0;
		return 0;
	}

	bool WavStreamInstance::hasEnded()
	{
		if (mOffset >= mParent->mSampleCount && !(mFlags & AudioSourceInstance::LOOPING))
		{
			return 1;
		}
		return 0;
	}
	
	unsigned int WavStreamInstance::getSamplePosition()
	{
		return mOffset;
	}

	WavStream::WavStream()
	{
		mFilename = 0;
		mSampleCount = 0;
		mOgg = 0;
		mDataOffset = 0;
		mBits = 0;
		mMemFile = 0;
		mStreamFile = 0;
		mLoopStartOffset = 0;
		mLoopEndOffset = UINT_MAX;
	}
	
	WavStream::~WavStream()
	{
		stop();
		delete[] mFilename;
		delete mMemFile;
	}
	
#define MAKEDWORD(a,b,c,d) (((d) << 24) | ((c) << 16) | ((b) << 8) | (a))

	result WavStream::loadwav(File * fp)
	{
		fp->seek(4);
		int wavsize = fp->read32();
		if (fp->read32() != MAKEDWORD('W', 'A', 'V', 'E'))
		{
			return FILE_LOAD_FAILED;
		}
		int chunk = fp->read32();
		if (chunk == MAKEDWORD('J', 'U', 'N', 'K'))
		{
			int size = fp->read32();
			if (size & 1)
			{
				size += 1;
			}
			fp->seek(fp->pos() + size);
			chunk = fp->read32();
		}
		if (chunk != MAKEDWORD('f', 'm', 't', ' '))
		{
			return FILE_LOAD_FAILED;
		}
		int subchunk1size = fp->read32();
		int audioformat = fp->read16();
		int channels = fp->read16();
		int samplerate = fp->read32();
		int byterate = fp->read32();
		int blockalign = fp->read16();
		int bitspersample = fp->read16();

		if (audioformat != 1 ||
			subchunk1size != 16 ||
			(bitspersample != 8 && bitspersample != 16))
		{
			return FILE_LOAD_FAILED;
		}
		
		chunk = fp->read32();
		
		if (chunk == MAKEDWORD('L','I','S','T'))
		{
			int size = fp->read32();
			fp->seek(fp->pos() + size);
			chunk = fp->read32();
		}
		
		if (chunk != MAKEDWORD('d','a','t','a'))
		{
			return FILE_LOAD_FAILED;
		}

		mChannels = channels;

		if (channels > 1)
		{
			mChannels = 2;
		}

		int subchunk2size = fp->read32();
		
		int samples = (subchunk2size / (bitspersample / 8)) / channels;
		
		mDataOffset = fp->pos();
		mBits = bitspersample;
		mBaseSamplerate = (float)samplerate;
		mSampleCount = samples;
		mOgg = 0;

		fp->seek(fp->pos() + subchunk2size);

		while (!fp->eof())
		{
			chunk = fp->read32();

			if (chunk == MAKEDWORD('s', 'm', 'p', 'l'))
			{
				/*int size = */fp->read32();

				// Ignore dwManufacturer, dwProduct, dwSamplePeriod, dwMIDIUnityNote, dwMIDIPitchFraction, dwSMPTEFormat, dwSMPTEOffset
				fp->seek(fp->pos() + 7 * 4);

				int loopCount = fp->read32();
				int samplerDataSize = fp->read32();

				for (int i = 0; i < loopCount; i++)
				{
					/*unsigned int loopId = */fp->read32();
					/*unsigned int loopType = */fp->read32();
					unsigned int loopStart = fp->read32();
					unsigned int loopEnd = fp->read32();

					/*unsigned int loopFraction = */fp->read32();
					/*unsigned int loopPlayCount = */fp->read32();

					if (i == 0) // Use only the first loop record
					{
						mLoopStartOffset = loopStart;
						mLoopEndOffset = loopEnd;
					}
				}

				// Ignore sampler data
				if (samplerDataSize & 1)
					samplerDataSize += 1;
				if (samplerDataSize > 0)
					fp->seek(fp->pos() + samplerDataSize);
			}
			else
			{
				// Ignore chunk
				int size = fp->read32();
				if (size & 1)
					size += 1;
				if (size > 0)
					fp->seek(fp->pos() + size);
			}
		}

		fp->seek(mDataOffset);

		return 0;
	}

	result WavStream::loadogg(File * fp)
	{
		fp->seek(0);
		int e;
		stb_vorbis *v;
		v = stb_vorbis_open_file((Soloud_Filehack *)fp, 0, &e, 0);
		if (v == NULL)
			return FILE_LOAD_FAILED;
		stb_vorbis_info info = stb_vorbis_get_info(v);
		mChannels = 1;
		if (info.channels > 1)
		{
			mChannels = 2;
		}
		mBaseSamplerate = (float)info.sample_rate;
		int samples = stb_vorbis_stream_length_in_samples(v);
		stb_vorbis_close(v);
		mOgg = 1;

		mSampleCount = samples;

		return 0;
	}

	result WavStream::load(const char *aFilename)
	{
		delete[] mFilename;
		delete mMemFile;
		mMemFile = 0;
		mFilename = 0;
		mSampleCount = 0;
		DiskFile fp;
		int res = fp.open(aFilename);
		if (res != SO_NO_ERROR)
			return res;
		
		int len = (int)strlen(aFilename);
		mFilename = new char[len+1];		
		memcpy(mFilename, aFilename, len);
		mFilename[len] = 0;
		
		res = parse(&fp);

		if (res != SO_NO_ERROR)
		{
			delete[] mFilename;
			mFilename = 0;
			return res;
		}

		return 0;
	}

	result WavStream::loadMem(unsigned char *aData, unsigned int aDataLen, bool aCopy, bool aTakeOwnership)
	{
		delete[] mFilename;
		delete mMemFile;
		mStreamFile = 0;
		mMemFile = 0;
		mFilename = 0;
		mSampleCount = 0;

		if (aData == NULL || aDataLen == 0)
			return INVALID_PARAMETER;

		MemoryFile *mf = new MemoryFile();
		int res = mf->openMem(aData, aDataLen, aCopy, aTakeOwnership);
		if (res != SO_NO_ERROR)
		{
			delete mf;
			return res;
		}

		res = parse(mf);

		if (res != SO_NO_ERROR)
		{
			delete mf;
			return res;
		}

		mMemFile = mf;

		return 0;
	}

	result WavStream::loadToMem(const char *aFilename)
	{
		DiskFile df;
		int res = df.open(aFilename);
		if (res == SO_NO_ERROR)
		{
			res = loadFileToMem(&df);
		}
		return res;
	}

	result WavStream::loadFile(File *aFile)
	{
		delete[] mFilename;
		delete mMemFile;
		mStreamFile = 0;
		mMemFile = 0;
		mFilename = 0;
		mSampleCount = 0;

		int res = parse(aFile);

		if (res != SO_NO_ERROR)
		{
			return res;
		}

		mStreamFile = aFile;

		return 0;
	}

	result WavStream::loadFileToMem(File *aFile)
	{
		delete[] mFilename;
		delete mMemFile;
		mStreamFile = 0;
		mMemFile = 0;
		mFilename = 0;
		mSampleCount = 0;

		MemoryFile *mf = new MemoryFile();
		int res = mf->openFileToMem(aFile);
		if (res != SO_NO_ERROR)
		{
			delete mf;
			return res;
		}

		res = parse(mf);

		if (res != SO_NO_ERROR)
		{
			delete mf;
			return res;
		}

		mMemFile = mf;

		return res;
	}


	result WavStream::parse(File *aFile)
	{
		int tag = aFile->read32();
		int res = SO_NO_ERROR;
		if (tag == MAKEDWORD('O', 'g', 'g', 'S'))
		{
			res = loadogg(aFile);
		}
		else
		if (tag == MAKEDWORD('R', 'I', 'F', 'F'))
		{
			res = loadwav(aFile);
		}
		else
		{
			res = FILE_LOAD_FAILED;
		}
		return res;
	}

	AudioSourceInstance *WavStream::createInstance()
	{
		return new WavStreamInstance(this);
	}

	double WavStream::getLength()
	{
		if (mBaseSamplerate == 0)
			return 0;
		return mSampleCount / mBaseSamplerate;
	}

};
