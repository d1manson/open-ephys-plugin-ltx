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
        : GenericProcessor("Plugin Name")
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


    }


    void GainProcessorPlugin::process(AudioBuffer<float>& buffer)
    {

        checkForEvents(true);

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