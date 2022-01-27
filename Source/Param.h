#pragma once
#include <JuceHeader.h>

namespace param
{
	enum class ID
	{
		Gain,
		VibratoFreq,
		VibratoDepth,
		WaveFolderDrive,
		SaturatorDrive,
		EnumSize
	};

	// PARAMETER ID STUFF
	static juce::String getName(ID i)
	{
		switch (i)
		{
		case ID::Gain: return "Gain";
		case ID::VibratoFreq: return "Vibrato Freq";
		case ID::VibratoDepth: return "Vibrato Depth";
		case ID::WaveFolderDrive: return "WaveFolder Drive";
		case ID::SaturatorDrive: return "Saturator Drive";
		default: return "";
		}
	}
	static juce::String getName(int i) { getName(static_cast<ID>(i)); }
	static juce::String getID(const ID i) { return getName(i).toLowerCase().removeCharacters(" "); }
	static juce::String getID(const int i) { return getName(i).toLowerCase().removeCharacters(" "); }

	// PARAMETER CREATION STUFF
	static std::unique_ptr<juce::AudioParameterFloat> createParameter(ID i, float defaultValue,
		std::function<juce::String(float value, int maxLen)> stringFromValue,
		const juce::NormalisableRange<float>& range) {
		return std::make_unique<juce::AudioParameterFloat>(
			getID(i), getName(i), range, defaultValue, getName(i), juce::AudioProcessorParameter::Category::genericParameter,
			stringFromValue
			);
	}
	static std::unique_ptr<juce::AudioParameterFloat> createParameter(ID i, float defaultValue = 1.f,
		std::function<juce::String(float value, int maxLen)> stringFromValue = nullptr,
		const float min = 0.f, const float max = 1.f, const float interval = -1.f) {
		if (interval != -1.f)
			return createParameter(i, defaultValue, stringFromValue, juce::NormalisableRange<float>(min, max, interval));
		else
			return createParameter(i, defaultValue, stringFromValue, juce::NormalisableRange<float>(min, max));
	}

	static juce::AudioProcessorValueTreeState::ParameterLayout createParameters()
	{
		std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

		// string lambdas
		// [0, 1]:
		auto percentStr = [](float value, int) {
			if (value == 1.f) return juce::String("100 %");
			value *= 100.f;
			if (value > 9.f) return static_cast<juce::String>(value).substring(0, 2) + " %";
			return static_cast<juce::String>(value).substring(0, 1) + " %";
		};
		// [-1, 1]:
		const auto percentStrM1 = [](float value, int) {
			if (value == 1.f) return juce::String("100 %");
			value = (value + 1.f) * 50.f;
			if (value > 9.f) return static_cast<juce::String>(value).substring(0, 2) + " %";
			return static_cast<juce::String>(value).substring(0, 1) + " %";
		};
		// [0, 1]:
		const auto normBiasStr = [](float value, int) {
			value = std::floor(value * 100.f);
			return juce::String(value) + " %";

		};
		const auto freqStr = [](float value, int) {
			if (value < 10.f)
				return static_cast<juce::String>(value).substring(0, 3) + " hz";
			return static_cast<juce::String>(value).substring(0, 2) + " hz";
		};
		// [-1, 1]:
		const auto mixStr = [](float value, int) {
			auto nV = static_cast<int>(std::rint((value + 1.f) * 5.f));
			switch (nV) {
			case 0: return juce::String("Dry");
			case 10: return juce::String("Wet");
			default: return static_cast<juce::String>(10 - nV) + " : " + static_cast<juce::String>(nV);
			}
		};
		// [0, 1]
		const auto modsMixStr = [](float value, int) {
			// [0,1] >> [25:75] etc
			const auto mapped = value * 100.f;
			const auto beautified = std::floor(mapped);
			return static_cast<juce::String>(100.f - beautified) + " : " + static_cast<juce::String>(beautified);
		};
		const auto lrMsStr = [](float value, int) {
			return value < .5f ? juce::String("L/R") : juce::String("M/S");
		};
		const auto dbStr = [](float value, int) {
			return static_cast<juce::String>(std::rint(value)).substring(0, 5) + " db";
		};
		const auto msStr = [](float value, int) {
			return static_cast<juce::String>(std::rint(value)).substring(0, 5) + " ms";
		};
		const auto syncStr = [](bool value, int) {
			return value ? static_cast<juce::String>("Sync") : static_cast<juce::String>("Free");
		};
		const auto wtStr = [](float value, int) {
			return value < 1 ?
				juce::String("SIN") : value < 2 ?
				juce::String("TRI") : value < 3 ?
				juce::String("SQR") :
				juce::String("SAW");
		};
		const auto octStr = [](float value, int) {
			return juce::String(static_cast<int>(value)) + " oct";
		};
		const auto voicesStr = [](float value, int) {
			return juce::String(static_cast<int>(value)) + " voices";
		};
		const auto polarityStr = [](float value, int) {
			return value < .5f ?
				juce::String("Polarity Off") :
				juce::String("Polarity On");
		};
		// [-1, 1]
		const auto phaseStr = [](float value, int) {
			return juce::String(std::floor(value * 180.f)) + " dgr";
		};

		parameters.push_back(createParameter(ID::Gain, 0.f, dbStr, -40.f, 40.f));
		parameters.push_back(createParameter(ID::VibratoFreq, 0.f, freqStr, .1f, 20.f));
		parameters.push_back(createParameter(ID::VibratoDepth, 1.f, percentStr));
		parameters.push_back(createParameter(ID::WaveFolderDrive, 0.f, dbStr, 0.f, 24.f));
		parameters.push_back(createParameter(ID::SaturatorDrive, 0.f, percentStr));
		
		return { parameters.begin(), parameters.end() };
	}
};