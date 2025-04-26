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

		bool isRecording = processor->isRecording.load();

        if(processor->recordingBuffer.start_read() >= 2){
            // we always render the path (if there is any), just in a different shade when recording is not currently active

            // * pixelFactor + margin
            g.setColour(isRecording ? Colours::black : Colours::grey);

			PosPoint posSampA;
			PosPoint posSampB;
			processor->recordingBuffer.read(posSampA);
			while (processor->recordingBuffer.read(posSampB)) {
                g.drawLine(
					posSampA.x * pixelFactor + margin,
					posSampA.y * pixelFactor + margin,
					posSampB.x * pixelFactor + margin,
					posSampB.y * pixelFactor + margin,
					1.0f
                );
				posSampA = posSampB;
            }
		}

		// render latest pos samp as two blobs
		// note that these are individually atomic, so it's possible to see a parital update..but that's not that a big deal, hopefully.
		float timestamp = processor->latestPosSamp.timestamp.load();
		float x1 = processor->latestPosSamp.x1.load();
		float y1 = processor->latestPosSamp.y1.load();
		float x2 = processor->latestPosSamp.x2.load();
		float y2 = processor->latestPosSamp.y2.load();
		float numpix1 = processor->latestPosSamp.numpix1.load();
		float numpix2 = processor->latestPosSamp.numpix2.load();
		
		if (numpix1 > 0) {
			g.setColour(Colours::green);
			g.fillEllipse(toXPixels(x1), toYPixels(y1), std::sqrt(numpix1)+1, std::sqrt(numpix1)+1);
			g.drawSingleLineText("(" + formatFloat(x1, 1) + ", " + formatFloat(y1, 1) + ")", toXPixels(0), toYPixels(H)+16);
		}

		if (numpix2 > 0) {
			g.setColour(Colours::red);
			g.fillEllipse(toXPixels(x2), toYPixels(y2), std::sqrt(numpix2)+1, std::sqrt(numpix2)+1);
			g.drawSingleLineText("(" + formatFloat(x2, 1) + ", " + formatFloat(y2, 1) + ")", toXPixels(0), toYPixels(H) +32);
		}

		// render timestamp
		if (isRecording) {
			g.setColour(Colours::red);
			g.fillEllipse(toXPixels(W) - 10, toYPixels(H) + 7, 10, 10);
			g.drawSingleLineText(formatAsMinSecs(timestamp, 1), toXPixels(W)-16, toYPixels(H) + 16, Justification::right);
		} else {
			g.setColour(Colours::white);
			g.drawSingleLineText(formatAsMinSecs(timestamp, 1), toXPixels(W), toYPixels(H) + 16, Justification::right);
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