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

#include "LTXGainProcessorPluginEditor.h"
#include "LTXGainProcessorPlugin.h"

namespace LTX {

    GainPopupComponent::GainPopupComponent(std::vector<FloatParameter*> gain_params_):
        gain_params(gain_params_)
    {

        const int sliderWidth = 18;
        const int sliderHeight = 60;
        const int nCols = 8;
        const int nRows = 4;
        const int padding = 10;

        setSize((sliderWidth + padding) * nCols + padding * 2,
               (sliderHeight + padding)* nRows + padding * 2);
        sliders.clear();

        for (int i = 0; i < gain_params.size(); i++)
        {
            Slider* slider = new Slider("GAIN_SLIDER" + String(i + 1));
            slider->setSliderStyle(Slider::LinearBarVertical);
            slider->setTextBoxStyle(Slider::NoTextBox, false, sliderWidth, 10);
            slider->setColour(Slider::textBoxTextColourId, Colours::white);
            slider->setColour(Slider::backgroundColourId, Colours::darkgrey);
            slider->setColour(Slider::trackColourId, Colours::blue);
            slider->setValue(gain_params[i]->getFloatValue(), dontSendNotification);
            slider->addListener(this);
            slider->setSize(sliderWidth, sliderHeight);
            slider->setTransform(AffineTransform::translation(padding + (sliderWidth+padding)*(i % nCols), (sliderHeight+padding)*(i/nCols)));

            sliders.add(slider);
            addAndMakeVisible(slider);
        }
    }

    GainPopupComponent::~GainPopupComponent() {
    
    }

    
    void GainPopupComponent::sliderValueChanged(Slider* slider) {
        gain_params[sliders.indexOf(slider)]->setNextValue(slider->getValue());
    }




    GainProcessorPluginEditor::GainProcessorPluginEditor(GenericProcessor* parentNode)
        : GenericEditor(parentNode)
    {

        desiredWidth = 100;

        configureButton = std::make_unique<UtilityButton>("configure", titleFont);
        configureButton->addListener(this);
        configureButton->setRadius(3.0f);
        configureButton->setBounds(10, 60, 80, 30);
        addAndMakeVisible(configureButton.get());

    }


    void GainProcessorPluginEditor::buttonClicked(Button* button)
    {
        LOGC("Configure clicked!")
        if (button != configureButton.get()){
            return;
        }

        GainProcessorPlugin* processor = (GainProcessorPlugin*)getProcessor();

        std::vector<FloatParameter*> params = processor->getChanParamsForStreamId(getCurrentStream());
        
        currentPopupWindow = new GainPopupComponent(params);

        CallOutBox& myBox
            = CallOutBox::launchAsynchronously(std::unique_ptr<Component>(currentPopupWindow),
                button->getScreenBounds(),
                nullptr);

        myBox.setDismissalMouseClicksAreAlwaysConsumed(true);


    }

}