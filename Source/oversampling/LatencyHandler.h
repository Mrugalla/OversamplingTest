#pragma once
#include "juce_audio_basics/juce_audio_basics.h"

namespace latency {
	struct Inducer
	{
		Inducer() :
			latencySamples(0),
			latencyAll(0),
			latencyUpdated(false)
		{}
		int latencySamples, latencyAll;
		bool latencyUpdated;
	};

	struct Processor :
		public juce::Timer
	{
		Processor(juce::AudioProcessor& p) :
			processor(p),
			inducers(),
			latencySamples(0)
		{
			startTimer(1000);
		}
		void addInducer(Inducer* i) { inducers.push_back(i); }
		void removeInducer(Inducer* inducer) {
			for(auto i = 0; i < inducers.size(); ++i)
				if (inducers[i] == inducer) {
					inducers.erase(inducers.begin() + i);
					return;
				}
		}

		juce::AudioProcessor& processor;
		std::vector<Inducer*> inducers;
		int latencySamples;
	protected:
		void update() {
			latencySamples = 0;
			for (auto inducer : inducers) {
				latencySamples += inducer->latencySamples;
				inducer->latencyUpdated = false;
			}
			for (auto inducer : inducers)
				inducer->latencyAll = latencySamples;
			processor.setLatencySamples(latencySamples);
		}
		void timerCallback() override {
			for (auto inducer : inducers)
				if (inducer->latencyUpdated)
					return update();
		}
	};
}