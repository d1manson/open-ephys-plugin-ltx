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

    class NarrowSlider : public Slider  {
    public:
        using Slider::Slider;
        String getTextFromValue(double value) override {
            char buffer[10];
            std::snprintf(buffer, sizeof(buffer), "%.*f", 2, value);
            return std::string(buffer);
        }
    };

    GainPopupComponent::GainPopupComponent(std::vector<FloatParameter*> gain_params_,
        std::vector<String> channel_infos):
        gain_params(gain_params_)
    {

        const int sliderWidth = 24;
        const int sliderHeight = 60;
        const int labelHeight = 20;
        const int padding = 10;
        int nCols;
        int nRows;

        if (gain_params.size() <= 8) {
            nCols = 8;
            nRows = 1;
        } else if (gain_params.size() <= 16) {
            nCols = 8;
            nRows = 2;
        } else if (gain_params.size() <= 32) {
            nCols = 16;
            nRows = 2;
        } else if (gain_params.size() <= 64) {
            nCols = 16;
            nRows = 4;
        } else {
            nCols = 8;
            nRows = 8;
        }
        
        setSize((sliderWidth + padding) * nCols + padding * 2,
               (sliderHeight + labelHeight + padding)* nRows + padding * 2);
        sliders.clear();

        for (int i = 0; i < gain_params.size(); i++)
        {
            const int x = padding + (sliderWidth + padding) * (i % nCols);
            const int y = padding + (sliderHeight + labelHeight + padding) * (i / nCols);

            // given we're tight for space we just show a number above the slider, and then
            // show the full channel name as a tooltip (configured on the slider below)
            Label* chan_label = new Label("CHAN_" + String(i+1), String(i+1));
            chan_label->setBounds(x, y, sliderWidth, labelHeight);
            chan_label->setFont(Font(10));
            chan_label->setColour(Label::textColourId, Colours::beige);
            chan_label->setJustificationType(Justification::centred);
            addAndMakeVisible(chan_label);
            chan_labels.add(chan_label);

            Slider* slider = new NarrowSlider("GAIN_SLIDER" + String(i + 1));
            slider->setTooltip(channel_infos[i]);
            slider->setSliderStyle(Slider::LinearBarVertical);
            slider->setTextBoxStyle(Slider::TextBoxAbove, false, sliderWidth, 20);
            slider->setMinValue(gain_params[i]->getMinValue());
            slider->setMaxValue(gain_params[i]->getMaxValue());
            slider->setColour(Slider::textBoxTextColourId, Colours::white);
            slider->setColour(Slider::backgroundColourId, Colours::darkgrey);
            slider->setColour(Slider::trackColourId, Colours::blue);
            slider->setValue(gain_params[i]->getFloatValue(), dontSendNotification);
            slider->addListener(this);
            slider->setSize(sliderWidth, sliderHeight);
            slider->setTransform(AffineTransform::translation(x,y+labelHeight));
            sliders.add(slider);
            addAndMakeVisible(slider);
            for (Component* c : slider->getChildren()) {
                if (Label* label = dynamic_cast<Label*>(c)) {
                    label->setFont(Font(8));
                }
            }
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
        
        currentPopupWindow = new GainPopupComponent(
            processor->GetChanParamsForStreamId(getCurrentStream()),
            processor->GetChanInfosForStreamId(getCurrentStream()));

        CallOutBox& myBox
            = CallOutBox::launchAsynchronously(std::unique_ptr<Component>(currentPopupWindow),
                button->getScreenBounds(),
                nullptr);

        myBox.setDismissalMouseClicksAreAlwaysConsumed(true);


    }

}