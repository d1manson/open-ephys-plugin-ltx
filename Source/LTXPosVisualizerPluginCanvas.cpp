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

#include "LTXPosVisualizerPluginCanvas.h"
#include "LTXPosVisualizerPlugin.h"
#include "util.h"
#include <chrono>
#include <atomic>

namespace LTX{

	const int margin = 80;
    const int MAX_PATH_POINTS_RENDERED = 45000;

	PosPlot::PosPlot(PosVisualizerPlugin* processor_)
		: processor(processor_) {
		paramWidth = reinterpret_cast<IntParameter*>(processor->getParameter("Width"));
		paramHeight = reinterpret_cast<IntParameter*>(processor->getParameter("Height"));
		paramPPM = reinterpret_cast<FloatParameter*>(processor->getParameter("PPM"));
	}

	PosPlot::~PosPlot(){}

	void PosPlot::paint(Graphics& g)
	{

		float W = static_cast<float>(paramWidth->getValue());
		float H = static_cast<float>(paramHeight->getValue());
		float ppm = paramPPM->getValue();

        PosVisualizerPlugin::PosSample posSamp;
        bool isRecording;
        processor->consumeRecentData(posSamp, recordedPosPoints, isRecording);

		const float pixelFactor = std::min(
			(getWidth() - margin * 2) / static_cast<float>(W),
			(getHeight() - margin * 2) / static_cast<float>(H)
		);

		auto toXPixels = [pixelFactor](float v) -> int { return margin + v * pixelFactor;};
		auto toYPixels = [pixelFactor](float v) -> int { return margin + v * pixelFactor;};

		g.setColour(Colours::white);
		g.fillRect(margin, margin, static_cast<int>(W*pixelFactor), static_cast<int>(H*pixelFactor));

		g.setFont(Font("Arial", 14, Font::FontStyleFlags::bold));

		g.drawSingleLineText("< " + formatFloat(W * 100 / ppm, 0) + "cm >", toXPixels(W / 2), toYPixels(0) - 6, Justification::centred);
		g.saveState();
		g.addTransform(AffineTransform::rotation(-MathConstants<float>::halfPi, toXPixels(0) - 6, toYPixels(H / 2)));
		g.drawSingleLineText("< " + formatFloat(H * 100 / ppm, 0) + "cm >", toXPixels(0) - 6, toYPixels(H / 2), Justification::centred);
		g.restoreState();

        if(recordedPosPoints.size() >= 2){
            // we always render the path (if there is any), just in a different shade when recording is not currently active

            // * pixelFactor + margin
            g.setColour(isRecording ? Colours::black : Colours::grey);

            // JUCE's line drawing performs very poorly (possibly this will change in v8), so if there are very large number of
            // points to be drawn, we ignore the oldest ones, and just render the most recent MAX_PATH_POINTS_RENDERED points.
            size_t startIdx = recordedPosPoints.size() > MAX_PATH_POINTS_RENDERED ? recordedPosPoints.size() - MAX_PATH_POINTS_RENDERED : 0;
            for (size_t i = startIdx; i < recordedPosPoints.size() - 1; ++i) {
                g.drawLine(
					recordedPosPoints[i].x * pixelFactor + margin,
					recordedPosPoints[i].y * pixelFactor + margin,
					recordedPosPoints[i + 1].x * pixelFactor + margin,
					recordedPosPoints[i + 1].y * pixelFactor + margin,
					1.0f
                );
            }
		}


		// render latest pos samp as two blobs

		if (posSamp.numpix1 > 0) {
			g.setColour(Colours::green);
			g.fillEllipse(toXPixels(posSamp.x1), toYPixels(posSamp.y1), std::sqrt(posSamp.numpix1)+1, std::sqrt(posSamp.numpix1)+1);
			g.drawSingleLineText("(" + formatFloat(posSamp.x1, 1) + ", " + formatFloat(posSamp.y1, 1) + ")", toXPixels(0), toYPixels(H)+16);
		}

		if (posSamp.numpix2 > 0) {
			g.setColour(Colours::red);
			g.fillEllipse(toXPixels(posSamp.x2), toYPixels(posSamp.y2), std::sqrt(posSamp.numpix2)+1, std::sqrt(posSamp.numpix2)+1);
			g.drawSingleLineText("(" + formatFloat(posSamp.x2, 1) + ", " + formatFloat(posSamp.y2, 1) + ")", toXPixels(0), toYPixels(H) +32);
		}

		// render timestamp
		if (isRecording) {
			g.setColour(Colours::red);
			g.fillEllipse(toXPixels(W) - 10, toYPixels(H) + 7, 10, 10);
			g.drawSingleLineText(formatAsMinSecs(posSamp.timestamp, 1), toXPixels(W)-16, toYPixels(H) + 16, Justification::right);
		} else {
			g.setColour(Colours::white);
			g.drawSingleLineText(formatAsMinSecs(posSamp.timestamp, 1), toXPixels(W), toYPixels(H) + 16, Justification::right);
		}





	}


	PosVisualizerPluginCanvas::PosVisualizerPluginCanvas(PosVisualizerPlugin* processor_)
		: processor(processor_), plt(processor_) {
		
		addAndMakeVisible(plt);
		plt.setBounds(0, 0, getWidth(), getHeight());

	}

	PosVisualizerPluginCanvas::~PosVisualizerPluginCanvas()
	{

	}


	void PosVisualizerPluginCanvas::resized()
	{
		plt.setBounds(0, 0, getWidth(), getHeight());
	}

	void PosVisualizerPluginCanvas::refreshState()
	{

	}


	void PosVisualizerPluginCanvas::update()
	{

	}


	void PosVisualizerPluginCanvas::refresh()
	{
	}

	void PosVisualizerPluginCanvas::paint(Graphics& g){
		g.fillAll(Colours::black);
		plt.repaint();
	}

}