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

namespace LTX{

	const int margin = 80;

	PosPlot::PosPlot(PosVisualizerPlugin* processor_)
		: processor(processor_) {
		paramWidth = reinterpret_cast<IntParameter*>(processor->getParameter("Width"));
		paramHeight = reinterpret_cast<IntParameter*>(processor->getParameter("Height"));
	}

	PosPlot::~PosPlot(){}
	
	void PosPlot::paint(Graphics& g)
	{
		float W = static_cast<float>(paramWidth->getValue());
		float H = static_cast<float>(paramHeight->getValue());

        bool isRecording = processor->getIsRecording();
		std::vector<PosVisualizerPlugin::PosPoint> recordedPosPoints = processor->getRecordedPosPoints();
        PosVisualizerPlugin::PosSample posSamp = processor->getLatestPosSamp();


		const float pixelFactor = std::min(
			(getWidth() - margin * 2) / static_cast<float>(W),
			(getHeight() - margin * 2) / static_cast<float>(H)
		);

		auto toXPixels = [pixelFactor, W](float v) -> int { return margin + std::min(std::max(v, 0.0f), W) * pixelFactor;};
		auto toYPixels = [pixelFactor, H](float v) -> int { return margin + std::min(std::max(v, 0.0f), H) * pixelFactor;};

		g.setColour(Colours::white);
		g.fillRect(margin, margin, static_cast<int>(W*pixelFactor), static_cast<int>(H*pixelFactor));

		g.setFont(Font("Arial", 14, Font::FontStyleFlags::bold));
		

        if(recordedPosPoints.size()){
            // we always render the recordedPosPoints (if there are any), just in a different shade when recording is not currently active
            g.setColour(isRecording ? Colours::black : Colours::grey);
            PosVisualizerPlugin::PosPoint lastPoint = recordedPosPoints[0];
            for (PosVisualizerPlugin::PosPoint& point : recordedPosPoints) {
                g.drawLine(toXPixels(lastPoint.x), toYPixels(lastPoint.y), toXPixels(point.x), toYPixels(point.y), 1);
                lastPoint = point;
            }
		}


		// render latest pos samp as two blobs

		if (posSamp.numpix1 > 0) {
			g.setColour(Colours::green);
			g.fillEllipse(toXPixels(posSamp.x1), toYPixels(posSamp.y1), std::sqrt(posSamp.numpix1)+1, std::sqrt(posSamp.numpix1)+1);
			g.drawSingleLineText("(" + String(posSamp.x1) + ", " + String(posSamp.y1) + ")", toXPixels(0), toYPixels(H)+16);
		}

		if (posSamp.numpix2 > 0) {
			g.setColour(Colours::red);
			g.fillEllipse(toXPixels(posSamp.x2), toYPixels(posSamp.y2), std::sqrt(posSamp.numpix2)+1, std::sqrt(posSamp.numpix2)+1);
			g.drawSingleLineText("(" + String(posSamp.x2) + ", " + String(posSamp.y2) + ")", toXPixels(0), toYPixels(H) +32);
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