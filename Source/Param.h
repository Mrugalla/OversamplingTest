#pragma once
#include <JuceHeader.h>

namespace param {
	enum class ID {
		Gain,
		VibratoFreq,
		VibratoDepth,
		WaveFolderDrive,
		SaturatorDrive,
		EnumSize
	};

	// PARAMETER ID STUFF
	static juce::String getName(ID i) {
		switch (i) {
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

	// NORMALISABLE RANGE STUFF
	static juce::NormalisableRange<float> getBiasedRange(float min, float max, float bias) noexcept {
		bias = 1.f - bias;
		const auto biasInv = 1.f / bias;
		const auto range = max - min;
		const auto rangeInv = 1.f / range;
		return juce::NormalisableRange<float>(min, max,
			[b = bias, bInv = biasInv, r = range](float start, float end, float normalised) {
				return start + r * std::pow(normalised, bInv);
			},
			[b = bias, rInv = rangeInv](float start, float end, float denormalized) {
				return std::pow((denormalized - start) * rInv, b);
			}
			);
	}

	// PARAMETER CREATION STUFF
	static std::unique_ptr<juce::AudioParameterBool> createPBool(ID i, bool defaultValue, std::function<juce::String(bool value, int maxLen)> func) {
		return std::make_unique<juce::AudioParameterBool>(
			getID(i), getName(i), defaultValue, getName(i), func
			);
	}
	static std::unique_ptr<juce::AudioParameterChoice> createPChoice(ID i, const juce::StringArray& choices, int defaultValue) {
		return std::make_unique<juce::AudioParameterChoice>(
			getID(i), getName(i), choices, defaultValue, getName(i)
			);
	}
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

	// TEMPOSYNC VS FREE STUFF
	struct MultiRange {
		struct Range
		{
			Range(const juce::String& rID, const juce::NormalisableRange<float>& r) :
				id(rID),
				range(r)
			{}
			bool operator==(const juce::Identifier& rID) const { return id == rID; }
			const juce::NormalisableRange<float>& operator()() const { return range; }
			const juce::Identifier& getID() const { return id; }
		protected:
			const juce::Identifier id;
			const juce::NormalisableRange<float> range;
		};
		MultiRange() :
			ranges()
		{}
		void add(const juce::String& rID, const juce::NormalisableRange<float>&& r) { ranges.push_back({ rID, r }); }
		void add(const juce::String& rID, const juce::NormalisableRange<float>& r) { ranges.push_back({ rID, r }); }
		const juce::NormalisableRange<float>& operator()(const juce::Identifier& rID) const {
			for (const auto& r : ranges)
				if (r == rID)
					return r();
			return ranges[0]();
		}
		const juce::Identifier& getID(const juce::String&& idStr) const {
			for (const auto& r : ranges)
				if (r.getID().toString() == idStr)
					return r.getID();
			return ranges[0].getID();
		}
	private:
		std::vector<Range> ranges;
	};

	static std::vector<float> getTempoSyncValues(int range) {
		/*
		* range = 6 => 2^6=64 => [1, 64] =>
		* 1, 1., 1t, 1/2, 1/2., 1/2t, 1/4, 1/4., 1/4t, ..., 1/64
		*/
		const auto numRates = range * 3 + 1;
		std::vector<float> rates;
		rates.reserve(numRates);
		for (auto i = 0; i < range; ++i) {
			const auto denominator = 1 << i;
			const auto beat = 1.f / static_cast<float>(denominator);
			const auto euclid = beat * 3.f / 4.f;
			const auto triplet = beat * 2.f / 3.f;
			rates.emplace_back(beat);
			rates.emplace_back(euclid);
			rates.emplace_back(triplet);
		}
		const auto denominator = 1 << range;
		const auto beat = 1.f / static_cast<float>(denominator);
		rates.emplace_back(beat);
		return rates;
	}
	static std::vector<juce::String> getTempoSyncStrings(int range) {
		/*
		* range = 6 => 2^6=64 => [1, 64] =>
		* 1, 1., 1t, 1/2, 1/2., 1/2t, 1/4, 1/4., 1/4t, ..., 1/64
		*/
		const auto numRates = range * 3 + 1;
		std::vector<juce::String> rates;
		rates.reserve(numRates);
		for (auto i = 0; i < range; ++i) {
			const auto denominator = 1 << i;
			juce::String beat("1/" + juce::String(denominator));
			rates.emplace_back(beat);
			rates.emplace_back(beat + ".");
			rates.emplace_back(beat + "t");
		}
		const auto denominator = 1 << range;
		juce::String beat("1/" + juce::String(denominator));
		rates.emplace_back(beat);
		return rates;
	}
	static juce::NormalisableRange<float> getTempoSyncRange(const std::vector<float>& rates) {
		return juce::NormalisableRange<float>(
			0.f, static_cast<float>(rates.size()),
			[rates](float, float end, float normalized) {
				const auto idx = static_cast<int>(normalized * end);
				if (idx >= end) return rates[idx - 1];
				return rates[idx];
			},
			[rates](float, float end, float mapped) {
				for (auto r = 0; r < end; ++r)
					if (rates[r] == mapped)
						return static_cast<float>(r) / end;
				return 0.f;
			}
			);
	}
	static std::function<juce::String(float, int)> getRateStr(
		const juce::AudioProcessorValueTreeState& apvts,
		const ID syncID,
		const juce::NormalisableRange<float>& freeRange,
		const std::vector<juce::String>& syncStrings)
	{
		return [&apvts, syncID, freeRange, syncStrings](float value, int) {
			const auto syncP = apvts.getRawParameterValue(getID(syncID));
			const auto sync = syncP->load();
			if (sync < .5f) {
				value = freeRange.convertFrom0to1(value);
				if (value < 10) {
					value = std::rint(value * 100.f) * .01f;
					const juce::String valueStr(value);
					if (valueStr.length() < 3)
						return valueStr + ".00 hz";
					else if (valueStr.length() < 4)
						return valueStr + "0 hz";
					else
						return valueStr + " hz";
				}
				else
					return static_cast<juce::String>(std::rint(value)) + " hz";
			}
			else {
				const auto idx = static_cast<int>(value * syncStrings.size());
				return value < 1.f ? syncStrings[idx] : syncStrings[idx - 1];
			}
		};
	}


	static juce::AudioProcessorValueTreeState::ParameterLayout createParameters() {
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