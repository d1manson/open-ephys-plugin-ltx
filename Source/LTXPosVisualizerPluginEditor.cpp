/*
------------------------------------------------------------------

This file is part of a plugin for the Open Ephys GUI
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


#include "LTXPosVisualizerPluginEditor.h"
#include "LTXPosVisualizerPluginCanvas.h"
#include "LTXPosVisualizerPlugin.h"

namespace LTX {
PosVisualizerPluginEditor::PosVisualizerPluginEditor(GenericProcessor* p)
    : VisualizerEditor(p, "Pos", 200)
{
    // todo: ideally state the expected format visually
    // and then provide W and H config and pixels per meter
     
    //addSelectedChannelsParameterEditor("Channels", 20, 105);

    clearButton = std::make_unique<UtilityButton>("Clear Path", Font("Fira Code", "Regular", 10));
    clearButton->setBounds(15, 95, 100, 20);
    clearButton->addListener(this);
    clearButton->setTooltip("Can only clear display after recording has been stopped.");
    addAndMakeVisible(clearButton.get()); // makes the button a child component of the editor and makes it visible


}

Visualizer* PosVisualizerPluginEditor::createNewCanvas()
{
    return new PosVisualizerPluginCanvas((PosVisualizerPlugin*) getProcessor());;
}

void PosVisualizerPluginEditor::buttonClicked(Button* button) {
    reinterpret_cast<PosVisualizerPlugin*>(getProcessor())->clearRecording();
}

}