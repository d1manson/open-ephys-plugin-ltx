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
        ensureParamsExist();
    }


    inline void multiply(uint32 size, float* buffer, float factor) {
        // multiplies the float buffer in place

        // this could presumably use SIMD instructions
        for (; size; size--) {
            buffer[0] *= factor;
            buffer++;
        }
    }

    void GainProcessorPlugin::process(AudioBuffer<float>& buffer)
    {
        for (auto stream : getDataStreams()) {
            const uint32 numSamples = getNumSamplesInBlock(stream->getStreamId());
            for (auto chan : stream->getContinuousChannels())
            {
                int chanIndex = chan->getGlobalIndex();
                multiply(numSamples, 
                    buffer.getWritePointer(chanIndex),
                    chan->getParameter("Gain")->getValue());
            }       
        }
    }

    std::vector<FloatParameter*> GainProcessorPlugin::GetChanParamsForStreamId(uint16 streamId)
    {
        ensureParamsExist();

        std::vector<FloatParameter*> params;
        auto stream = getDataStream(streamId);
        for (auto chan : stream->getContinuousChannels() ) {
            params.push_back((FloatParameter*)chan->getParameter("Gain"));
        }
        return params;
    }

    std::vector<String> GainProcessorPlugin::GetChanInfosForStreamId(uint16 streamId)
    {
        std::vector<String> infos;
        auto stream = getDataStream(streamId);

        for (auto chan : stream->getContinuousChannels())
        {
            infos.push_back(chan->getName());
        }
        return infos;
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

    void GainProcessorPlugin::ensureParamsExist() {
        for (auto stream : getDataStreams()) {
            for (auto chan : stream->getContinuousChannels())
            {
                if (!chan->hasParameter("Gain")) {
                    chan->addParameter(new FloatParameter(this, Parameter::CONTINUOUS_CHANNEL_SCOPE, "Gain", "Multiply the voltage by x", 1.0, -2.0, 2.0, 0.05));
                }
            }
        }
    }

    void GainProcessorPlugin::saveCustomParametersToXml(XmlElement* xml)
    {
        // seems that only GLOBAL_SCOPE parameters are auomtically saved, so we do this custom logic
        ensureParamsExist();

        for (auto stream : getDataStreams()){
            XmlElement* streamXml = xml->createNewChildElement("STREAM");
            streamXml->setAttribute("name", stream->getName());
            for (auto chan : stream->getContinuousChannels())
            {
                XmlElement* chanXml = streamXml->createNewChildElement("CHANNEL");
                chanXml->setAttribute("global_index", chan->getGlobalIndex());
                chan->getParameter("Gain")->toXml(chanXml);
            }

        }
    }


    void GainProcessorPlugin::loadCustomParametersFromXml(XmlElement* xml)
    {
        ensureParamsExist();
        for (auto stream : getDataStreams()) {
            XmlElement* streamXml = xml->getChildByAttribute("name", stream->getName());
            if (!streamXml) {
                continue;
            }
            for (auto chan : stream->getContinuousChannels()){
                XmlElement* chanXml = streamXml->getChildByAttribute("global_index", String(chan->getGlobalIndex()));
                if (!chanXml) {
                    continue;
                }

                //chan->getParameter("Gain")->setNextValue(chanXml->getDoubleAttribute("Gain", 1.0));
                chan->getParameter("Gain")->fromXml(chanXml);
            }
        }
    }

}