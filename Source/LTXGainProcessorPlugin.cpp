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
#include "util.h"

namespace LTX {

    GainProcessorPlugin::GainProcessorPlugin()
        : GenericProcessor("Gain")
    {

    }


    GainProcessorPlugin::~GainProcessorPlugin()
    {

    }


    void GainProcessorPlugin::registerParameters()
    {
        // parameters are tied to a stream not to the processor, so we don't register them here
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

    void GainProcessorPlugin::process(AudioBuffer<float>& buffer)
    {
        for (auto stream : getDataStreams()) {
            const uint32 numSamples = getNumSamplesInBlock(stream->getStreamId());
            const auto& gain_params_for_stream = gain_params[std::max(stream->getGlobalIndex(),0)];
            for (auto chan : stream->getContinuousChannels())
            {
                int chanIndex = chan->getGlobalIndex();
                FloatVectorOperations::multiply(buffer.getWritePointer(chanIndex),
                    gain_params_for_stream[chanIndex]->getFloatValue(), static_cast<size_t>(numSamples));
            }
        }
    }

    std::vector<FloatParameter*> GainProcessorPlugin::GetChanParamsForStreamId(uint16 streamId)
    {
        ensureParamsExist();
        auto stream = getDataStream(streamId);
        std::vector<FloatParameter*> params;

        for (auto chan : stream->getContinuousChannels())
        {
             params.push_back((FloatParameter*) stream->getParameter(makeGainParamName(chan->getGlobalIndex())));
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


    void GainProcessorPlugin::handleBroadcastMessage (const String& msg, const int64 messageTimeMilliseconds)
    {

    }


    void GainProcessorPlugin::ensureParamsExist() {
        gain_params.resize(getNumDataStreams());

        for (auto stream : getDataStreams()) {
            auto stream_ = getDataStream(stream->getStreamId()); // non-const version for use with adding parameters below
            auto& gain_params_stream = gain_params[std::max(stream->getGlobalIndex(), 0)];
            gain_params_stream.resize(stream->getChannelCount());

            for (auto chan : stream->getContinuousChannels()) {
                const int chanIdx = chan->getGlobalIndex();
                if (!stream->hasParameter(makeGainParamName(chanIdx))) {
                    auto param = new FloatParameter(
                        this,
                        Parameter::ParameterScope::STREAM_SCOPE,
                        makeGainParamName(chanIdx),
                        "Gain ch" + chan->getName(),
                        "Multiply the voltage by x", "x", 1.0f, -2.0f, 3.0f, 0.05f
                    );
                    stream_->addParameter(param);
                    gain_params_stream[chanIdx] = param; // Not sure if there are memory safety issues with storing a reference here. Might be better to store the raw floats instead and update based on changes
                }
            }
        }
    }


    void GainProcessorPlugin::saveCustomParametersToXml(XmlElement* xml)
    {
    }


    void GainProcessorPlugin::loadCustomParametersFromXml(XmlElement* xml)
    {
    }

}