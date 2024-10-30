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

#ifndef VISUALIZERCANVAS_H_INCLUDED
#define VISUALIZERCANVAS_H_INCLUDED

#include <VisualizerWindowHeaders.h>


namespace LTX{
class PosVisualizerPlugin;
struct PosPoint;

/**
* 
	The Visualiser behaves unusually in terms of repainting, so need an inner component
	and then we can call repaint() on this from within the Visualiser's paint() method.
	Otherwise we could have just had one class here.
*/


class PosPlot : public Component {
public:
	/** Constructor */
	PosPlot(PosVisualizerPlugin* processor);

	/** Destructor */
	~PosPlot();

	void paint(Graphics& g) override;

private:

	/** Pointer to the processor class */
	PosVisualizerPlugin* processor;

    Path path;
	IntParameter* paramWidth;
	IntParameter* paramHeight;
	FloatParameter* paramPPM;

	/** Generates an assertion if this class leaks */
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PosPlot);

};


class PosVisualizerPluginCanvas : public Visualizer
{
public:

	/** Constructor */
	PosVisualizerPluginCanvas(PosVisualizerPlugin* processor);

	/** Destructor */
	~PosVisualizerPluginCanvas();

	/** Updates boundaries of sub-components whenever the canvas size changes */
	void resized() override;

	/** Called when the visualizer's tab becomes visible again */
	void refreshState() override;

	/** Updates settings */
	void update() override;

	/** Called instead of "repaint()" to avoid re-painting sub-components*/
	void refresh() override;

	/** Draws the canvas background */
	void paint(Graphics& g) override;


private:

	/** Pointer to the processor class */
	PosVisualizerPlugin* processor;

	/** Class for plotting data */
	PosPlot plt;



	/** Generates an assertion if this class leaks */
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PosVisualizerPluginCanvas);
};

}
#endif // SPECTRUMCANVAS_H_INCLUDED
