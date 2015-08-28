/*
 * LightsController.cpp
 *
 *  Created on: 28/08/2015
 *      Author: Windsdon

 *  Copyright (C) 2015  Windsdon
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "LightsController.h"
#include <random>

using namespace std;

LightsController::LightsController(double scale) :
		scale(scale) {
	overlay.create(1920 * scale, 1080 * scale);
	overlay.clear(sf::Color::White);

	overlaySprite.setTexture(overlay.getTexture(), true);

	default_random_engine engine;
	uniform_real_distribution<double> dist(0, 180);

	leftPhase = dist(engine);
	rightPhase = dist(engine);

	fadeInDuration = 2;
	fadeOutDuration = 2;
	audioPlayDuration = 5;
	needsUpdate = false;
	fadeOutStart = -1;
	lastT = 0;
}

LightsController::~LightsController() {
}

void LightsController::load() {
	rotatingTex.loadFromFile("res/light_color.png");
	rotatingTexMatte.loadFromFile("res/light_overlay.png");
	gradientTex.loadFromFile("res/bg_overlay_lights.png");

	rotatingLeft.setTexture(rotatingTex, true);
	rotatingLeft.setScale(scale, scale);
	rotatingLeft.setOrigin(640, 640);
	rotatingLeft.setPosition(150 * scale, 540 * scale);

	rotatingRight.setTexture(rotatingTex, true);
	rotatingRight.setScale(scale, scale);
	rotatingRight.setOrigin(640, 640);
	rotatingRight.setPosition(1770 * scale, 540 * scale);

	rotatingLeftMatte.setTexture(rotatingTexMatte, true);
	rotatingLeftMatte.setScale(scale, scale);
	rotatingLeftMatte.setOrigin(640, 640);
	rotatingLeftMatte.setPosition(150 * scale, 540 * scale);

	rotatingRightMatte.setTexture(rotatingTexMatte, true);
	rotatingRightMatte.setScale(scale, scale);
	rotatingRightMatte.setOrigin(640, 640);
	rotatingRightMatte.setPosition(1770 * scale, 540 * scale);

	gradient.setTexture(gradientTex, true);
	gradient.setScale(scale, scale);

	warnBuffer.loadFromFile("res/alarm1.wav");
	warn.setBuffer(warnBuffer);
}

void LightsController::start() {
	timer.restart();
	needsUpdate = true;
	fadeOutStart = -1;
}

void LightsController::stop() {
	fadeOutStart = timer.getElapsedTime().asSeconds();
}

void LightsController::update() {
	if (!needsUpdate) {
		return;
	}


	overlay.clear(sf::Color::White);

	double t = timer.getElapsedTime().asSeconds();
	double delta = lastT - t;
	lastT = t;
	unsigned opacity = 255;
	double speed = 360; //degrees per second


	if(warn.getStatus() == sf::Sound::Stopped && t < audioPlayDuration) {
		warn.play();
	}

	if (t < fadeInDuration) {
		//draw fade in
		opacity *= (t / fadeInDuration);
		speed *= (t / fadeInDuration);
	} else if (fadeOutStart > 0) {
		if ((fadeOutStart + fadeOutDuration) < t) {
			needsUpdate = false;
			return;
		} else {
			//draw fade out
			opacity *= (1 - (t - fadeOutStart) / fadeOutDuration);
			speed *= (1 - (t - fadeOutStart) / fadeOutDuration);
		}
	}

	leftPhase += speed * delta;
	rightPhase += speed * delta;

	if (leftPhase > 360) {
		leftPhase -= 360;
	}

	if (rightPhase > 360) {
		rightPhase -= 360;
	}

	sf::Color opacityColor(255, 255, 255, opacity);

	rotatingLeft.setRotation(leftPhase);
	rotatingLeft.setColor(opacityColor);
	rotatingLeftMatte.setRotation(leftPhase);
	rotatingLeftMatte.setColor(opacityColor);
	rotatingRight.setRotation(rightPhase);
	rotatingRight.setColor(opacityColor);
	rotatingRightMatte.setRotation(rightPhase);
	rotatingRightMatte.setColor(opacityColor);

	gradient.setColor(opacityColor);

	overlay.draw(gradient);

	overlay.draw(rotatingLeftMatte);
	overlay.draw(rotatingRightMatte);

	overlay.display();
}

void LightsController::draw(sf::RenderTarget& target,
		sf::RenderStates& states) const {
	states.blendMode = sf::BlendMultiply;
	target.draw(overlaySprite, states);

	states.blendMode = sf::BlendAdd;
	target.draw(rotatingLeft, states);
	target.draw(rotatingRight, states);
}

const sf::RenderTexture& LightsController::getOverlay() const {
	return overlay;
}
