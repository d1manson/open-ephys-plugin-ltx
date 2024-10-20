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

namespace LTX {

PosVisualizerPlugin::PosVisualizerPlugin()
    : GenericProcessor("Pos Viewer")
{
    this->addIntParameter(Parameter::GLOBAL_SCOPE, "Width", "Window Width", 1000, 200, 5000, false);
    this->addIntParameter(Parameter::GLOBAL_SCOPE, "Height", "Window Height", 1000, 200, 5000, false);
    this->addFloatParameter(Parameter::GLOBAL_SCOPE, "PPM", "Pixels Per Meter", 400.0f, 100.0f, 1000.0f, 0.1, false);
    
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
    const ScopedLock sl(lock);
    isRecording = true;
    recordedPosPoints.clear();
}

void PosVisualizerPlugin::stopRecording() {
    const ScopedLock sl(lock);
    isRecording = false;
}

bool PosVisualizerPlugin::getIsRecording() {
    const ScopedLock sl(lock);
    return isRecording; 
};

PosVisualizerPlugin::PosSample PosVisualizerPlugin::getLatestPosSamp() {
    const ScopedLock sl(lock);
    return latestPosSamp;
}

std::vector<PosVisualizerPlugin::PosPoint> PosVisualizerPlugin::getRecordedPosPoints() {
    const ScopedLock sl(lock);
    return recordedPosPoints;
}

void PosVisualizerPlugin::clearRecording() {
    const ScopedLock sl(lock);
    if (!isRecording) {
        recordedPosPoints.clear();
    }
}

void PosVisualizerPlugin::process(AudioBuffer<float>& buffer)
{
    const ScopedLock sl(lock);
    
    for (auto stream : getDataStreams())
    {
        const uint32 numSamples = getNumSamplesInBlock(stream->getStreamId());
        if (numSamples == 0) {
            return;
        }

        if (isRecording) {
            // store x1 and y1 values into recordedPosPoints for all samples
            int originalSize = recordedPosPoints.size();
            recordedPosPoints.resize(originalSize + numSamples);
            for (auto chan : stream->getContinuousChannels()) {
                int chanIndex = chan->getGlobalIndex();
                if(chanIndex == ChannelMapping::x1) {
                    for (int i = 0; i < numSamples; i++) {
                        recordedPosPoints[originalSize + i].x = buffer.getReadPointer(chanIndex)[i];
                    }
                } else if (chanIndex == ChannelMapping::y1) {
                    for (int i = 0; i < numSamples; i++) {
                        recordedPosPoints[originalSize + i].y = buffer.getReadPointer(chanIndex)[i];
                   }
                }
                // we don't check numpix1, instead we assume x1 will be nan already if numpix1 is 0
                recordedPosPoints.erase(
                    std::remove_if(recordedPosPoints.begin() + originalSize, recordedPosPoints.end(),
                        [](const PosPoint& samp) { return std::isnan(samp.x); }),
                        recordedPosPoints.end());
            }
        }

        // get the last value into latestPosSamp
        for (auto chan : stream->getContinuousChannels()) {
             int chanIndex = chan->getGlobalIndex();
             float v = buffer.getReadPointer(chanIndex)[numSamples-1];
             switch (chanIndex) {
             case ChannelMapping::Timestamp:
                 latestPosSamp.timestamp = v;
                 break;
             case ChannelMapping::x1:
                 latestPosSamp.x1 = v;
                 break;
             case ChannelMapping::y1:
                 latestPosSamp.y1 = v;
                 break;
             case ChannelMapping::x2:
                 latestPosSamp.x2 = v;
                 break;
             case ChannelMapping::y2:
                 latestPosSamp.y2 = v;
                 break;
             case ChannelMapping::numpix1:
                 latestPosSamp.numpix1 = v;
                 break;
             case ChannelMapping::numpix2:
                 latestPosSamp.numpix2 = v;
                 break;
             }
        }

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

}