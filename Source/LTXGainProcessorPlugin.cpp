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

#include "LTXGainProcessorPlugin.h"

#include "LTXGainProcessorPluginEditor.h"

namespace LTX {

    GainProcessorPlugin::GainProcessorPlugin()
        : GenericProcessor("Gain")
    {
        // TODO: switch to per-channel gain
        sourceNode->addFloatParameter(Parameter::GLOBAL_SCOPE, "Global Gain", "In development", 0.25, 20, 1, true);
    }


    GainProcessorPlugin::~GainProcessorPlugin()
    {

    }


    AudioProcessorEditor* GainProcessorPlugin::createEditor()
    {
        editor = std::make_unique<GainProcessorPluginEditor>(this);
        return editor.get();
    }


    void GainProcessorPlugin::updateSettings()
    {


    }


    void multiply(uint32 size, float* buffer, float factor) {
        // multiplies the float buffer in place

        // this could presumably use SIMD instructions
        for (; size; size--) {
            buffer[0] *= factor;
            buffer++;
        }
    }

    void GainProcessorPlugin::process(AudioBuffer<float>& buffer)
    {
        float factor = getParameter("Gain")->getValue();

        for (auto stream : getDataStreams())
        {
            const uint16 streamId = stream->getStreamId();
            const uint32 numSamples = getNumSamplesInBlock(streamId);

            for (auto chan :  stream->getContinuousChannels())
            {
                float* ptr = buffer.getWritePointer(chan->getGlobalIndex());
                multiply(numSamples, ptr, ((chan->getGlobalIndex() % 4) + 1) /* dummy vary across channels */ * factor);
            }
            
        }
    }


    void GainProcessorPlugin::handleTTLEvent(TTLEventPtr event)
    {

    }


    void GainProcessorPlugin::handleSpike(SpikePtr spike)
    {

    }


    void GainProcessorPlugin::handleBroadcastMessage(String message)
    {

    }


    void GainProcessorPlugin::saveCustomParametersToXml(XmlElement* parentElement)
    {

    }


    void GainProcessorPlugin::loadCustomParametersFromXml(XmlElement* parentElement)
    {

    }

}