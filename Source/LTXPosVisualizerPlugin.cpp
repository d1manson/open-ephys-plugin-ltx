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

#include "LTXPosVisualizerPlugin.h"
#include "LTXPosVisualizerPluginEditor.h"
#include "LTXSharedState.h"
#include "util.h"

namespace LTX {

PosVisualizerPlugin::PosVisualizerPlugin() : GenericProcessor("Pos Viewer")
{
    this->addIntParameter(Parameter::GLOBAL_SCOPE, "Width", "Window width in the 'pixel' units sent by Bonsai.", 1000, 200, 5000, false);
    this->addIntParameter(Parameter::GLOBAL_SCOPE, "Height", "Window height  in the 'pixel' units sent by Bonsai.", 1000, 200, 5000, false);
    this->addFloatParameter(Parameter::GLOBAL_SCOPE, "PPM", "Pixels Per Meter", 400.0f, 100.0f, 1000.0f, 0.1, false);

    paramWidth = reinterpret_cast<IntParameter*>(getParameter("Width"));
    paramHeight = reinterpret_cast<IntParameter*>(getParameter("Height"));
}


PosVisualizerPlugin::~PosVisualizerPlugin()
{

}


AudioProcessorEditor* PosVisualizerPlugin::createEditor()
{
    editor = std::make_unique<PosVisualizerPluginEditor>(this);
    return editor.get();
}


void PosVisualizerPlugin::updateSettings()
{
}


void PosVisualizerPlugin::startRecording() {
    isRecording = true;
    recordingBuffer.clear();
}

void PosVisualizerPlugin::stopRecording() {
    isRecording = false;
}


void PosVisualizerPlugin::clearRecording() {
    recordingBuffer.clear();
}

void PosVisualizerPlugin::process(AudioBuffer<float>& buffer)
{
    const float width = static_cast<float>(paramWidth->getValue());
    const float height = static_cast<float>(paramHeight->getValue());

    for (auto stream : getDataStreams())
    {
        const uint32 numSamples = getNumSamplesInBlock(stream->getStreamId());
        if (numSamples == 0) {
            return;
        }

        if (isRecording) {
            // store x1 and y1 values into recordingBuffer, clamped to [0, width] x [0, height], ignoring NaN values
            auto x1_buffer = buffer.getReadPointer(ChannelMapping::x1);
            auto y1_buffer = buffer.getReadPointer(ChannelMapping::y1);
            
            for (int i = 0; i < numSamples; i++) {
                PosPoint point {clamp(x1_buffer[i], 0.f, width), clamp(y1_buffer[i], 0.f, height)};
                recordingBuffer.write(point, !std::isnan(x1_buffer[i]));
            }

        }

        // get the last value into latestPosSamp, clamped to [0, width] x [0, height]
        latestPosSamp.timestamp = buffer.getReadPointer(ChannelMapping::Timestamp)[numSamples-1];
        latestPosSamp.x1 = clamp(buffer.getReadPointer(ChannelMapping::x1)[numSamples-1], 0.f, width);
        latestPosSamp.y1 = clamp(buffer.getReadPointer(ChannelMapping::y1)[numSamples-1], 0.f, height);
        latestPosSamp.x2 = clamp(buffer.getReadPointer(ChannelMapping::x2)[numSamples-1], 0.f, width);
        latestPosSamp.y2 = clamp(buffer.getReadPointer(ChannelMapping::y2)[numSamples-1], 0.f, height);
        latestPosSamp.numpix1 = buffer.getReadPointer(ChannelMapping::numpix1)[numSamples-1];
        latestPosSamp.numpix2 = buffer.getReadPointer(ChannelMapping::numpix2)[numSamples-1];
        return; // should only be one data stream
    }

}


void PosVisualizerPlugin::handleTTLEvent(TTLEventPtr event)
{

}


void PosVisualizerPlugin::handleSpike(SpikePtr spike)
{

}


void PosVisualizerPlugin::handleBroadcastMessage(String message)
{

}


void PosVisualizerPlugin::saveCustomParametersToXml(XmlElement* parentElement)
{

}


void PosVisualizerPlugin::loadCustomParametersFromXml(XmlElement* parentElement)
{

}


void PosVisualizerPlugin::parameterValueChanged(Parameter* param) {
    LTX::SharedState::window_max_x = getParameter("Width")->getValue();
    LTX::SharedState::window_max_y = getParameter("Height")->getValue();
    LTX::SharedState::pixels_per_metre = getParameter("PPM")->getValue();
};

}