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
    : GenericProcessor("Pos Visualizer")
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


void PosVisualizerPlugin::process(AudioBuffer<float>& buffer)
{
    
    for (auto stream : getDataStreams())
    {
        const uint32 numSamples = getNumSamplesInBlock(stream->getStreamId());
        if (numSamples == 0) {
            return;
        }

        for (auto chan : stream->getContinuousChannels()) {
             int chanIndex = chan->getGlobalIndex();
             float latestVal = buffer.getReadPointer(chanIndex)[numSamples-1];
             switch (chanIndex) {
             case 0:
                 latestPosSamp.timestamp = latestVal;
                 break;
             case 1:
                 latestPosSamp.x1 = latestVal;
                 break;
             case 2:
                 latestPosSamp.y1 = latestVal;
                 break;
             case 3:
                 latestPosSamp.x2 = latestVal;
                 break;
             case 4:
                 latestPosSamp.y2 = latestVal;
                 break;
             case 5:
                 latestPosSamp.numpix1 = latestVal;
                 break;
             case 6:
                 latestPosSamp.numpix2 = latestVal;
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