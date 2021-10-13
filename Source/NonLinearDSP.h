#pragma once
#include "oversampling/Oversampling.h"
#include "oversampling/LatencyHandler.h"

namespace dsp {
	static constexpr float tau = 6.28318530718f;
	static constexpr float pi = tau * .5f;
	static constexpr float tau2 = tau * 2.f;

	struct Phasor
	{
		Phasor() :
			fsInv(0.f),
			phase(0.f),
			inc(0.f)
		{}
		void prepareToPlay(double sampleRate) { fsInv = static_cast<float>(2. / sampleRate); }
		void setFrequency(float f) { inc = f * fsInv; }
		float process() noexcept {
			phase += inc;
			while (phase >= 1.f)
				phase -= 2.f;
			return phase;
		}
	protected:
		float fsInv, phase, inc;
	};

	struct SineOsc {
		SineOsc() :
			phasor()
		{}
		void prepareToPlay(double sampleRate) { phasor.prepareToPlay(sampleRate); }
		void setFrequency(float f) { phasor.setFrequency(f); }
		float process() noexcept { return std::sin(phasor.process() * pi); }
	protected:
		Phasor phasor;
	};

	struct RingMod :
		public oversampling::Processor::Listener
	{
		RingMod(int numChannels) :
			lfo()
		{
			lfo.resize(numChannels);
		}
		void prepareToPlay(double sampleRate) { for (auto& l : lfo) l.prepareToPlay(sampleRate); }
		void setFrequency(float f) { for (auto& l : lfo) l.setFrequency(f); }
		void processBlock(juce::AudioBuffer<float>& buffer) noexcept {
			auto samples = buffer.getArrayOfWritePointers();
			for (auto ch = 0; ch < buffer.getNumChannels(); ++ch)
				for (auto s = 0; s < buffer.getNumSamples(); ++s)
					samples[ch][s] *= lfo[ch].process();
		}
	protected:
		std::vector<SineOsc> lfo;

		void updateOversampling(double Fs, int blockSize, int, int) override {
			prepareToPlay(Fs);
		}
	};

	struct Wavefolder {
		Wavefolder() :
			drive(1.f),
			driveHalf(.5f),
			driveInv(1.f)
		{}
		void setDrive(float d) noexcept {
			drive = d;
			driveHalf = d * .5f;
			driveInv = 1.f / drive;
		}
		void processBlock(juce::AudioBuffer<float>& buffer) {
			const auto num = buffer.getNumSamples();
			for (auto ch = 0; ch < buffer.getNumChannels(); ++ch) {
				auto samples = buffer.getWritePointer(ch);
				juce::FloatVectorOperations::multiply(samples, driveHalf, num);
				juce::FloatVectorOperations::add(samples, .5f, num);
				for (auto s = 0; s < num; ++s) {
					while (samples[s] > 1.f)
						--samples[s];
					while (samples[s] < 0.f)
						++samples[s];
				}
				juce::FloatVectorOperations::multiply(samples, 2.f, num);
				juce::FloatVectorOperations::add(samples, -1.f, num);
				juce::FloatVectorOperations::multiply(samples, driveInv, num);
			}
		}
	protected:
		float drive, driveHalf, driveInv;
	};

	struct Saturator
	{
		Saturator() :
			drive(0.f)
		{}
		void setDrive(float d) noexcept { drive = d; }
		void processBlock(juce::AudioBuffer<float>& buffer) {
			const auto num = buffer.getNumSamples();
			for (auto ch = 0; ch < buffer.getNumChannels(); ++ch) {
				auto samples = buffer.getWritePointer(ch);
				for (auto s = 0; s < num; ++s) {
					const auto p = samples[s] > 0.f ? 1.f : -1.f;
					const auto b = p * std::sqrt(p * samples[s]);
					const auto c = p * std::sqrt(p * b);
					samples[s] += drive * (c - samples[s]);
				}
			}
		}
	protected:
		float drive;
	};

	struct Vibrato :
		public oversampling::Processor::Listener,
		public latency::Inducer
	{
		Vibrato() :
			lfo(),
			ringBuffer(),
			depth(1.f),
			writeHead(0),
			size(0)
		{
		}
		void prepareToPlay(double sampleRate, int blockSize) {
			size = static_cast<int>(sampleRate * 7. / 1000.);
			lfo.prepareToPlay(sampleRate);
			lfo.setFrequency(1.f);
			ringBuffer.resize(size + 1, 0.f);
			writeHead = 0;
			latencySamples = size / 2;
			latencyUpdated = true;
		}
		void setFrequency(float f) noexcept { lfo.setFrequency(f); }
		void process(float* samples, int numSamples) {
			for (auto s = 0; s < numSamples; ++s) {
				writeHead = (writeHead + 1) % size;
				const auto lfoNormal = .9f * depth * lfo.process() * .5f + .5f;
				const auto lfoMapped = lfoNormal * static_cast<float>(size);
				auto readHead = static_cast<float>(writeHead) - lfoMapped;
				if (readHead < 0.f)
					readHead += static_cast<float>(size);

				ringBuffer[writeHead] = samples[s];
				samples[s] = lerp(readHead);
			}
		}

		SineOsc lfo;
		std::vector<float> ringBuffer;
		float depth;
		int writeHead, size;

		float lerp(float rHead) noexcept {
			const auto xFloor = int(rHead);
			const auto x = rHead - xFloor;
			const auto xCeil = (xFloor + 1) % size;
			return ringBuffer[xFloor] + x * (ringBuffer[xCeil] - ringBuffer[xFloor]);
		}

		void updateOversampling(double Fs, int blockSize, int, int) override {
			prepareToPlay(Fs, blockSize);
		}
	};
}