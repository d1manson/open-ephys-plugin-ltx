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

PosVisualizerPluginCanvas::PosVisualizerPluginCanvas(PosVisualizerPlugin* processor_)
	: processor(processor_)
{
}


PosVisualizerPluginCanvas::~PosVisualizerPluginCanvas()
{

}


void PosVisualizerPluginCanvas::resized()
{

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


void PosVisualizerPluginCanvas::paint(Graphics& g)
{
	constexpr int paramW = 1000; // TODO: make this a parameter
	constexpr int paramH = 1000; // TODO: make this a parameter

	auto posSamp = reinterpret_cast<PosVisualizerPlugin*>(processor)->getLatestPosSamp();

	const float pixelFactor = std::min(
		(getWidth() - margin * 2) / static_cast<float>(paramW),
		(getHeight() - margin * 2) / static_cast<float>(paramH)
	);

	auto toPixels = [pixelFactor](float v) -> int {
		return margin + v * pixelFactor;
	};

	g.fillAll(Colours::black);

	g.setColour(Colours::white);
	g.fillRect(margin, margin, static_cast<int>(paramW*pixelFactor), static_cast<int>(paramH*pixelFactor));

	g.setFont(Font("Arial", 14, Font::FontStyleFlags::bold));

	if (posSamp->numpix1 > 0) {
		g.setColour(Colours::green);
		g.fillEllipse(toPixels(posSamp->x1), toPixels(posSamp->y1), posSamp->numpix1, posSamp->numpix1);
		g.drawSingleLineText("(" + String(posSamp->x1) + ", " + String(posSamp->y1) + ")", toPixels(0), toPixels(paramH)+16);
	}


	if (posSamp->numpix2 > 0) {
		g.setColour(Colours::red);
		g.fillEllipse(toPixels(posSamp->x2), toPixels(posSamp->y2), posSamp->numpix2, posSamp->numpix2);
		g.drawSingleLineText("(" + String(posSamp->x2) + ", " + String(posSamp->y2) + ")", toPixels(0), toPixels(paramH) +32);

	}

	g.setColour(Colours::white);
	g.drawSingleLineText(String(formatFloat(posSamp->timestamp, 2)) + "s", toPixels(paramW), toPixels(paramH) + 16, Justification::right);

}
}