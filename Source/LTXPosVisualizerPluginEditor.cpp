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
#include "util.h"

namespace LTX {
PosVisualizerPluginEditor::PosVisualizerPluginEditor(GenericProcessor* p)
    : VisualizerEditor(p, "Pos", 190)
{
    // todo: show inline latest pos
    
    addTextBoxParameterEditor("Width", 10, 25);
    addTextBoxParameterEditor("Height", 100, 25);
    addTextBoxParameterEditor("PPM", 10, 65);
    
    clearButton = std::make_unique<UtilityButton>("Clear Path", Font("Fira Code", "Regular", 10));
    clearButton->setBounds(100, 83, 80, 20);
    clearButton->addListener(this);
    clearButton->setTooltip("Can only clear display after recording has been stopped.");
    addAndMakeVisible(clearButton.get()); // makes the button a child component of the editor and makes it visible
    
    infoText = std::make_unique<Label>("info text");
    infoText->setFont(Font(12));
    infoText->setBounds(10, 108, 180, 20);
    infoText->setColour(Label::textColourId, Colours::black);
    addAndMakeVisible(infoText.get());

    startTimer(50);
}

void PosVisualizerPluginEditor::timerCallback() {
    PosVisualizerPlugin::PosSample posSamp = reinterpret_cast<PosVisualizerPlugin*>(getProcessor())->getLatestPosSamp();

    if (posSamp.timestamp == 0) {
        infoText->setText("t,x1,y1,x2,y2,numpix1,numpix2", dontSendNotification);
    } else {
        infoText->setText(
            String(formatAsMinSecs(posSamp.timestamp, 1)) + " (" + String(posSamp.x1) + ", " + String(posSamp.y1) + ")",
            dontSendNotification);
    }
}

Visualizer* PosVisualizerPluginEditor::createNewCanvas()
{
    return new PosVisualizerPluginCanvas((PosVisualizerPlugin*) getProcessor());;
}

void PosVisualizerPluginEditor::buttonClicked(Button* button) {
    reinterpret_cast<PosVisualizerPlugin*>(getProcessor())->clearRecording();
}

}