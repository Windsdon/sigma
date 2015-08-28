/*
 * SigmaClient.cpp
 *
 *  Created on: 23/08/2015
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

#include "SigmaClient.h"
#include <iostream>
#include <SFML/System/Vector2.hpp>
#include <thread>
using namespace std;

#define THIRD 0.333333333

SigmaClient::SigmaClient() :
		board(Board(90, 0)), hovers(Board(90, 0)), server(NULL) {
	running = false;
	window = NULL;
	currentPhase = Phase::WaitingConnection;
	enableInteraction = false;
	currentTileset = 1;

	double bigSize = 360.0 * 3; //1080
	for (unsigned i = 0; i < 81; i++) {
		float tx = (i % 9) * 120;
		float ty = (i / 9) * 120;
		float w = 118, h = 118;
		if ((i % 3) == 2) {
			w -= 4;
		}

		if ((i % 9) && !(i % 3)) {
			tx += 5;
			w -= 5;
		}

		if ((i / 9) && !((i / 9) % 3)) {
			ty += 5;
			h -= 5;
		}

		if ((i % 9) == 8) {
			w += 5;
		}

		if ((i / 9) == 8) {
			h += 5;
		}

		if (((i / 9) % 3) == 2) {
			h -= 4;
		}
		blocks.push_back(
				sf::FloatRect(tx / bigSize, ty / bigSize, w / bigSize,
						h / bigSize));
	}

	blocks.push_back(sf::FloatRect(0, 0, 355 / bigSize, 355 / bigSize));
	blocks.push_back(
			sf::FloatRect(365 / bigSize, 0, 350 / bigSize, 355 / bigSize));
	blocks.push_back(
			sf::FloatRect(725 / bigSize, 0, 355 / bigSize, 355 / bigSize));

	blocks.push_back(
			sf::FloatRect(0, 365 / bigSize, 355 / bigSize, 350 / bigSize));
	blocks.push_back(
			sf::FloatRect(365 / bigSize, 365 / bigSize, 350 / bigSize,
					350 / bigSize));
	blocks.push_back(
			sf::FloatRect(725 / bigSize, 365 / bigSize, 355 / bigSize,
					350 / bigSize));

	blocks.push_back(
			sf::FloatRect(0, 725 / bigSize, 355 / bigSize, 355 / bigSize));
	blocks.push_back(
			sf::FloatRect(365 / bigSize, 725 / bigSize, 350 / bigSize,
					355 / bigSize));
	blocks.push_back(
			sf::FloatRect(725 / bigSize, 725 / bigSize, 355 / bigSize,
					355 / bigSize));

	transp = 0;

	server = new SigmaServer();
	server->sp = this;

	new thread([this] {
		cout << "Server start!" << endl;
		server->start();
		cout << "Server end!" << endl;
	});

}

SigmaClient::~SigmaClient() {
}

void SigmaClient::start() {
	load();
	initWindow();

	running = true;
	while (running) {
		loop();
	}

	if (server) {
		server->stop();
	}

	window->close();
}

void SigmaClient::close() {
	running = false;
}

void SigmaClient::loop() {
	lock_guard<mutex> ml(lock);
	events();
	procMouse();
	render();
}

void SigmaClient::events() {
	sf::Event e;
	while (window->pollEvent(e)) {
		switch (e.type) {
			case sf::Event::Resized:
				windowResize();
				break;
			case sf::Event::Closed:
				close();
				break;
			case sf::Event::KeyPressed:
				switch (e.key.code) {
					case sf::Keyboard::F:
						fullscreenToggle();
						break;
					default:
						cout << "Pressed " << e.key.code << endl;
				}
				break;
			case sf::Event::MouseMoved:
				procMouse();
				break;
			case sf::Event::MouseButtonPressed:
				if (enableInteraction && e.mouseButton.button == 0) {
					procClick();
				}
				break;
			default:
				break;
		}
	}

}

void SigmaClient::procMouse() {
	if (!enableInteraction || !window) {
		return;
	}

	fill(hovers.begin(), hovers.end(), 0);

	sf::Vector2u winSize = window->getSize();
	sf::Vector2i mouse = sf::Mouse::getPosition(*window);
	sf::Vector2f mousePos(
			(mouse.x - winSize.x / 2.0 + winSize.y / 2.0) / winSize.y,
			mouse.y / ((float) winSize.y));

	for (unsigned i = 0; i < 90; i++) {
		if (blocks[i].contains(mousePos)) {
			if (board[i] & BIT_DISABLE) {
				continue;
			}
			if (i < 81 && currentPhase == Phase::ChooseCell) {
				hovers[i] = BIT_HIGHLIGHT | BIT_TRANSP | currentTileset;
			} else if (i > 80 && currentPhase == Phase::ChooseSuper) {
				hovers[i] = BIT_HIGHLIGHT;
			}
		}
	}
}

void SigmaClient::procClick() {
	for (unsigned i = 0; i < 90; i++) {
		if (hovers[i]) {
			ServerPacket packet(ServerPacket::CellClick);
			packet.click->id = i;
			sendPacket(packet);
		}
	}
}

void SigmaClient::render() {
	transp = (sin(timer.getElapsedTime().asSeconds() * 2) + 1) / 4 + 0.5;
	window->clear(sf::Color::Black);
	renderGame();
	window->display();
}

void SigmaClient::renderMenus() {
}

void SigmaClient::renderGame() {
	window->draw(boardSprite);

	sf::Transform big;
	big.scale(1 / 3.0, 1 / 3.0);

	sf::Transform small;
	small.scale(1 / 9.0, 1 / 9.0);
	for (unsigned i = 0; i < 90; i++) {
		unsigned v = board[i] ? board[i] : (enableInteraction ? hovers[i] : 0);

		if (!v) {
			continue;
		}

		sf::Transform t;
		int p = i - 81;
		if (v & BIT_HIGHLIGHT) {
			highlight(i);
		} else if (v & BIT_BLACKOUT) {
			blackout(i);
		}
		if (i > 80) {
			t = big;
			t.translate(p % 3, p / 3);
		} else {
			t = small;
			t.translate(i % 9, i / 9);
		}

		if ((v & MASK_ID) == 1) {
			if (v & BIT_TRANSP) {
				xSprite.setColor(sf::Color(255, 255, 255, 255 * transp));
				window->draw(xSprite, playAreaPosition * t);
				xSprite.setColor(sf::Color(255, 255, 255, 255));
			} else {
				window->draw(xSprite, playAreaPosition * t);
			}
		} else if ((v & MASK_ID) == 2) {
			if (v & BIT_TRANSP) {
				oSprite.setColor(sf::Color(255, 255, 255, 255 * transp));
				window->draw(oSprite, playAreaPosition * t);
				oSprite.setColor(sf::Color(255, 255, 255, 255));
			} else {
				window->draw(oSprite, playAreaPosition * t);
			}
		}
	}
}

void SigmaClient::blackout(unsigned id) {
	sf::FloatRect block = blocks[id];
	sf::RectangleShape blackRect(sf::Vector2f(block.width, block.height));
	blackRect.setPosition(block.left, block.top);
	blackRect.setFillColor(sf::Color(0, 0, 0, 127));
	window->draw(blackRect, playAreaPosition);
}

void SigmaClient::highlight(unsigned id, unsigned r, unsigned g, unsigned b) {
	sf::FloatRect block = blocks[id];
	sf::RectangleShape rect(sf::Vector2f(block.width, block.height));
	rect.setPosition(block.left, block.top);
	rect.setFillColor(sf::Color(r, g, b, 64 * transp));
	window->draw(rect, playAreaPosition);
}

void SigmaClient::load() {
	boardTexture.loadFromFile("res/bg.png");
	boardTexture.setSmooth(true);
	boardSprite.setTexture(boardTexture, true);

	xTexture.loadFromFile("res/x.png");
	xTexture.setSmooth(false);
	xSprite.setTexture(xTexture, true);
	xSprite.setScale(1.0 / 360, 1.0 / 360);
	xSprite.setOrigin(0, 0);

	oTexture.loadFromFile("res/o.png");
	oTexture.setSmooth(true);
	oSprite.setTexture(oTexture, true);
	oSprite.setScale(1.0 / 360.0, 1.0 / 360.0);
	oSprite.setOrigin(0, 0);
}

void SigmaClient::windowResize() {
	sf::Vector2u wsize = window->getSize();

	window->setView(
			sf::View(sf::Vector2f(wsize.y * 8 / 9.0, wsize.y * 0.5),
					sf::Vector2f(wsize.x, wsize.y)));
	playAreaPosition = sf::Transform();
	playAreaPosition.translate(wsize.y * 8 / 9.0 - wsize.y / 2.0, 0).scale(
			wsize.y, wsize.y);

	double spriteScale = wsize.y / ((double) boardTexture.getSize().y);
	cout << "scale: " << spriteScale << endl;
	boardSprite.setScale(spriteScale, spriteScale);
	boardSprite.setPosition(0, 0);
}

void SigmaClient::fullscreenToggle() {
	fullscreen = !fullscreen;

	initWindow();
}

void SigmaClient::createWindow(sf::VideoMode vm, int style,
		sf::ContextSettings settings) {
	if (!window) {
		window = new sf::RenderWindow(vm, "Sigma", style, settings);
	} else {
		window->create(vm, "Sigma", style, settings);
	}

	window->setFramerateLimit(60);

}

void SigmaClient::initWindow() {
	if (fullscreen) {
		createWindow(sf::VideoMode::getDesktopMode(), sf::Style::None,
				sf::ContextSettings(0, 0, 4));
	} else {
		createWindow(sf::VideoMode(1280, 720), sf::Style::Default,
				sf::ContextSettings(0, 0, 4));
	}

	windowResize();
}

void SigmaClient::receivePacket(ServerPacket& packet) {
	cout << "Got a packet: " << packet.type << endl;
	cout << "Waiting mutex" << endl;
	lock_guard<mutex> lm(lock);
	cout << "Got mutex!" << endl;
	switch (packet.type) {
		case ServerPacket::ConnectionAccept:
			break;
		case ServerPacket::BoardState:
			cout << "Setting board state" << endl;
			board = packet.board->state;
			break;
		case ServerPacket::TilesetChange:
			currentTileset = packet.tileset->id;
			break;
		case ServerPacket::CellChange:
			board[packet.change->id] = packet.change->newValue;
			break;
		case ServerPacket::PhaseChange:
			currentPhase = packet.phase->phase;
			break;
		case ServerPacket::InteractionChange:
			enableInteraction = packet.interaction->enable;
			break;
		default:
			break;
	}
}

void SigmaClient::sendPacket(ServerPacket& packet) {
	if (server) {
		cout << "Sending packet" << endl;
		ServerPacket *p = new ServerPacket(packet);
		new thread([this, p] {
			cout << "Sender thread start!" << endl;
			server->receivePacket(*p);
			delete p;
			cout << "Sender thread end!" << endl;
		});
	}
}
