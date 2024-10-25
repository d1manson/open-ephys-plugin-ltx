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

//This prevents include loops. We recommend changing the macro to a name suitable for your plugin
#ifndef VISUALIZERPLUGIN_H_DEFINED
#define VISUALIZERPLUGIN_H_DEFINED

#include <ProcessorHeaders.h>

namespace LTX {
/** 
	A plugin that includes a canvas for displaying incoming data
	or an extended settings interface.
*/

class PosVisualizerPlugin : public GenericProcessor
{
public:

   /* note that the record engine only explicitly knows channel zero is the timestamp channel, for the others
    * it just converts to int16 and stores the value without worrying about the semantic meaning. */
	enum ChannelMapping {
		Timestamp = 0,
		x1 = 1,
		y1 = 2,
		x2 = 3,
		y2 = 4,
		numpix1 = 5,
		numpix2 = 6
	};

	struct PosSample {
		float timestamp;
		float x1;
		float y1;
		float x2;
		float y2;
		float numpix1;
		float numpix2;
	};

    struct PosPoint {
        float x;
        float y;
    };

	/** The class constructor, used to initialize any members.*/
	PosVisualizerPlugin();

	/** The class destructor, used to deallocate memory*/
	~PosVisualizerPlugin();

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
	void handleBroadcastMessage(String message) override;

	/** Saving custom settings to XML. This method is not needed to save the state of
		Parameter objects */
	void saveCustomParametersToXml(XmlElement* parentElement) override;

	/** Load custom settings from XML. This method is not needed to load the state of
		Parameter objects*/
	void loadCustomParametersFromXml(XmlElement* parentElement) override;

	void startRecording() override;
	void stopRecording() override;

	/* If recording is no long active it is possible to wipe the recording from the visualisation */
	void clearRecording();


    /* For use by the LTXPosVisualiserPluginCanvas to communicate data across threads. */
    void consumeRecentData(PosSample& latestPosSamp_, std::vector<PosPoint>& posPoints, bool& isRecording_);


private:

    /* The following bits are collected up on the hot-path signal processing thread and can then be communicated
     * to the GUI thread via the consumeRecentData method. The lock is used for simple thread-safety. */
	CriticalSection lock;
	PosSample latestPosSamp = {};
	std::vector<PosPoint> posPointsBuffer;
	bool isRecording = false;
	bool clearRequired = false;


	/** Generates an assertion if this class leaks */
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PosVisualizerPlugin);

};
}
#endif // VISUALIZERPLUGIN_H_DEFINED