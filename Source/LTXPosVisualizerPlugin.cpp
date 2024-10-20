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
    recordedPosSamps.clear();
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

std::vector<PosVisualizerPlugin::PosSample> PosVisualizerPlugin::getRecordedPosSamps() {
    const ScopedLock sl(lock);
    return recordedPosSamps;
}

void PosVisualizerPlugin::clearRecording() {
    const ScopedLock sl(lock);
    if (!isRecording) {
        recordedPosSamps.clear();
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

        int offset;
        if (isRecording) {
            offset = recordedPosSamps.size();
            recordedPosSamps.resize(offset + numSamples);
        }

        for (auto chan : stream->getContinuousChannels()) {
             int chanIndex = chan->getGlobalIndex();
             for (int i = 0; i < numSamples; i++) {
                 float v = buffer.getReadPointer(chanIndex)[i];
                 PosSample& samp = isRecording ? recordedPosSamps[offset + i] : latestPosSamp;
                 switch (chanIndex) {
                 case 0:
                     samp.timestamp = v;
                     break;
                 case 1:
                     samp.x1 = v;
                     break;
                 case 2:
                     samp.y1 = v;
                     break;
                 case 3:
                     samp.x2 = v;
                     break;
                 case 4:
                     samp.y2 = v;
                     break;
                 case 5:
                     samp.numpix1 = v;
                     break;
                 case 6:
                     samp.numpix2 = v;
                     break;
                 }
             }
        }

        if (isRecording) {
            std::memcpy(&latestPosSamp, &recordedPosSamps.back(), sizeof(PosSample));
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