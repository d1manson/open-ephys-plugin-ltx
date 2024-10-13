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

#ifndef PROCESSORPLUGINEDITOR_H_DEFINED
#define PROCESSORPLUGINEDITOR_H_DEFINED

#include <EditorHeaders.h>

namespace LTX {

	class GainPopupComponent :
		public Component,
		public Slider::Listener
	{
		
    public:

        GainPopupComponent(std::vector<FloatParameter*> gain_params, std::vector<String> channel_infos);
        ~GainPopupComponent();

        void sliderValueChanged(Slider* slider);


    private:
        OwnedArray<Slider> sliders;
		OwnedArray<Label> chan_labels;
		std::vector<FloatParameter*> gain_params;
	};



	class GainProcessorPluginEditor : 
		public GenericEditor,
		public Button::Listener
	{
	public:
		GainProcessorPluginEditor(GenericProcessor* parentNode);

		~GainProcessorPluginEditor() { };

		void buttonClicked(Button*) override;
	private:
		std::unique_ptr<UtilityButton> configureButton;

		GainPopupComponent* currentPopupWindow;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GainProcessorPluginEditor);
	};
}

#endif // PROCESSORPLUGINEDITOR_H_DEFINED