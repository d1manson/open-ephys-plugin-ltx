/*
------------------------------------------------------------------

This file is part of the Open Ephys GUI
Copyright (C) 2022 Open Ephys

------------------------------------------------------------------

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef PROCESSORPLUGIN_H_DEFINED
#define PROCESSORPLUGIN_H_DEFINED

#include <ProcessorHeaders.h>


namespace LTX {


	class GainProcessorPlugin : public GenericProcessor
	{
	public:
		/** The class constructor, used to initialize any members. */
		GainProcessorPlugin();

		/** The class destructor, used to deallocate memory */
		~GainProcessorPlugin();

		/** All plugin parameter objects must be created inside this method */
		void registerParameters() override;

		/** If the processor has a custom editor, this method must be defined to instantiate it. */
		AudioProcessorEditor* createEditor() override;

		/** Called every time the settings of an upstream plugin are changed.
			Allows the processor to handle variations in the channel configuration or any other parameter
			passed through signal chain. The processor can use this function to modify channel objects that
			will be passed to downstream plugins. */
		void updateSettings() override;

		/** Defines the functionality of the processor.
			The process method is called every time a new data buffer is available.
			Visualizer plugins typically use this method to send data to the canvas for display purposes */
		void process(AudioBuffer<float>& buffer) override;

		std::vector<FloatParameter*> GetChanParamsForStreamId(uint16 streamId);
		std::vector<String> GetChanInfosForStreamId(uint16 streamId);

		/** Handles events received by the processor
			Called automatically for each received event whenever checkForEvents() is called from
			the plugin's process() method */
		void handleTTLEvent(TTLEventPtr event) override;

		/** Handles spikes received by the processor
			Called automatically for each received spike whenever checkForEvents(true) is called from
			the plugin's process() method */
		void handleSpike(SpikePtr spike) override;

		/** Handles broadcast messages sent during acquisition
			Called automatically whenever a broadcast message is sent through the signal chain */
		void handleBroadcastMessage (const String& message, const int64 messageTimeMilliseconds) override;

		/** Saving custom settings to XML. This method is not needed to save the state of
			Parameter objects */
		void saveCustomParametersToXml(XmlElement* parentElement) override;

		/** Load custom settings from XML. This method is not needed to load the state of
			Parameter objects*/
		void loadCustomParametersFromXml(XmlElement* parentElement) override;

	private:
		void ensureParamsExist();

		// maps std::max(stream.globalIdx(), 0) => chan.globalIdx() => FloatParameter
		// this is a cache to avoid using getParameter on every single channel within the hot process() function
		// No real effort is made to ensure memory safety here, but the process loop does defer to the stream to get the available list of channels,
		// which should have associated params pre-cached here, if everything works as expected.
		std::vector<std::vector<FloatParameter*>> gain_params;

		// parameters are owned by the steam rather than the channels (it doesn't seem there is very goood support for params on continuous channels currently)
		String makeGainParamName(int chanIdx) { return "gain_" + String(chanIdx); }
	};

}

#endif