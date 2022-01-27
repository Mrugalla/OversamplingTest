#pragma once
#include "juce_audio_basics/juce_audio_basics.h"
#include "ConvolutionFilter.h"
#include "IIRFilter.h"

namespace oversampling
{
	constexpr size_t MaxNumStages = 2;
	static constexpr size_t MaxOrder = 1 << MaxNumStages;

	inline juce::String getOversamplingOrderID() { return "oversamplingOrder"; }

	struct Processor
	{
		using Flag = std::atomic<bool>;
		using AudioBuffer = juce::AudioBuffer<float>;

		Processor(juce::AudioProcessor* p) :
			audioProcessor(p),
			Fs(0.),
			numChannels(p->getChannelCountOfBus(false, 0)),
			blockSize(0),

			buffer(),

			filterUp4(  numChannels, 176400.f, 22050.f, 44100.f, true), //  17 samples
			filterDown4(numChannels, 176400.f, 22050.f, 44100.f),
			filterUp2(numChannels),
			filterDown2(numChannels),

			FsUp(0.),
			blockSizeUp(0),

			enabled(true), wannaUpdate(false),
			enabledTmp(true),

			numSamples1x(0), numSamples2x(0), numSamples4x(0)
		{
		}

		Processor(Processor& p) :
			audioProcessor(p.audioProcessor),
			Fs(p.Fs),
			numChannels(p.numChannels), blockSize(p.blockSize),
			buffer(p.buffer),
			filterUp2(p.filterUp2), filterUp4(p.filterUp4),
			filterDown4(p.filterDown4), filterDown2(p.filterDown2),
			FsUp(p.FsUp), blockSizeUp(p.blockSizeUp),
			enabled(p.enabled.load()),
			wannaUpdate(p.wannaUpdate.load()),
			enabledTmp(p.enabledTmp),
			numSamples1x(0), numSamples2x(0), numSamples4x(0)
		{
		}

		// prepare & params
		void prepareToPlay(const double sampleRate, const int _blockSize)
		{
			Fs = sampleRate;
			blockSize = _blockSize;
			if (enabled.load())
			{
				FsUp = sampleRate * static_cast<double>(MaxOrder);
				blockSizeUp = blockSize * MaxOrder;
			}
			else
			{
				FsUp = sampleRate;
				blockSizeUp = blockSize;
			}
			buffer.setSize(numChannels, blockSize * MaxOrder, false, false, false);
		}
		/* processing methods */
		AudioBuffer* upsample(AudioBuffer& input, int numChannelsIn, int numChannelsOut)
		{
			if (wannaUpdate.load())
			{
				enabled.store(enabledTmp);
				audioProcessor->prepareToPlay(Fs, blockSize);
				wannaUpdate.store(false);
				return nullptr;
			}
			if (enabled.load())
			{
				numSamples1x = input.getNumSamples();
				numSamples2x = numSamples1x * 2;
				numSamples4x = numSamples1x * 4;

				buffer.setSize(numChannels, numSamples4x, true, false, true);
				auto samplesUp = buffer.getArrayOfWritePointers();
				const auto samplesIn = input.getArrayOfReadPointers();
				// zero stuffing + filter 2x
				for (auto ch = 0; ch < numChannelsIn; ++ch)
				{
					auto up = samplesUp[ch];
					const auto in = samplesIn[ch];

					for (auto s = 0; s < numSamples1x; ++s)
					{
						const auto s2 = s * 2;
						up[s2] = in[s];
						up[s2 + 1] = 0.f;
					}
				}
				filterUp2.processBlock(samplesUp, numSamples2x);
				// zero stuffing + filter 4x
				const auto maxSample2x = numSamples2x - 1;
				for (auto ch = 0; ch < numChannelsIn; ++ch)
				{
					auto up = samplesUp[ch];

					for (auto s = maxSample2x; s > -1; --s)
					{
						const auto s2 = s * 2;
						up[s2] = up[s] * 2.f;
						up[s2 + 1] = 0.f;
					}
				}
				// filter 4x
				filterUp4.processBlockUp(samplesUp, numSamples4x);
				if (numChannelsIn < numChannelsOut)
					juce::FloatVectorOperations::copy(samplesUp[1], samplesUp[0], numSamples4x);
				return &buffer;
			}
			return &input;
		}
		void downsample(AudioBuffer* outBuf, int numChannelsOut) noexcept
		{
			auto samplesUp = buffer.getArrayOfWritePointers();
			auto samplesOut = outBuf->getArrayOfWritePointers();
			filterDown4.processBlockDown(samplesUp, numSamples4x);
			for (auto ch = 0; ch < numChannels; ++ch)
				for (auto s = 0; s < numSamples2x; ++s)
					samplesUp[ch][s] = samplesUp[ch][s * 2];
			filterDown2.processBlock(samplesUp, numSamples2x);
			if (numChannelsOut == buffer.getNumChannels())
				for (auto ch = 0; ch < numChannels; ++ch)
					for (auto s = 0; s < numSamples1x; ++s)
						samplesOut[ch][s] = samplesUp[ch][s * 2];
			else
			{
				{
					auto sOut = samplesOut[0];
					const auto sUp = samplesUp[0];

					for (auto s = 0; s < numSamples1x; ++s)
						sOut[s] = sUp[s * 2];
				}
				if(numChannelsOut == 2)
				{
					auto sOut = samplesOut[1 % numChannelsOut];
					const auto sUp = samplesUp[1];

					for (auto s = 0; s < numSamples1x; ++s)
						sOut[s] = sUp[s * 2];
				}
			}
		}
		bool processBlockEmpty()
		{
			if (wannaUpdate.load())
			{
				enabled.store(enabledTmp);
				audioProcessor->prepareToPlay(Fs, blockSize);
				wannaUpdate.store(false);
				return true;
			}
			return false;
		}
		////////////////////////////////////////
		const double getSampleRateUpsampled() const noexcept { return FsUp; }
		const int getBlockSizeUp() const noexcept { return blockSizeUp; }
		/* returns true if changing the state was successful */
		void setEnabled(const bool e) noexcept
		{
			if (enabledTmp != e)
			{
				enabledTmp = e;
				wannaUpdate.store(true);
			}
		}
		bool isEnabled() const noexcept { return enabled.load(); }
		int getLatency() const noexcept
		{
			if (enabled.load())
				return filterUp2.getLatency() + filterDown2.getLatency() + filterUp4.getLatency() + filterDown4.getLatency();
			return 0;
		}
		static constexpr int getUpsamplingFactor() noexcept { return 4; }
	protected:
		juce::AudioProcessor* audioProcessor;
		double Fs;
		int numChannels, blockSize;

		AudioBuffer buffer;

		ConvolutionFilter filterUp4, filterDown4;
		LowkeyChebyshevFilter<float> filterUp2, filterDown2;

		double FsUp;
		int blockSizeUp;

		Flag enabled, wannaUpdate;
		bool enabledTmp;

		int numSamples1x, numSamples2x, numSamples4x;
	};
}

/*

try other filter types:
	polyphase IIR
	Halfband-Polyphase IIR
	butterworth low pass filter
automatic reaction to different sampleRates

optimisations:
convolution filter simd

*/