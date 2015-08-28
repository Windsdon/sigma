/*
 * SigmaClient.h
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

#ifndef SIGMACLIENT_H_
#define SIGMACLIENT_H_

#include <vector>
#include <SFML/Graphics.hpp>
#include "SigmaServer.h"
#include <mutex>
#include "LightsController.h"

using namespace std;

class SigmaClient {
	public:
		friend class SigmaServer;
		typedef vector<unsigned> Board;

		static const unsigned MASK_ID = 0x03;
		static const unsigned BIT_TRANSP = 0x04;
		static const unsigned BIT_HIGHLIGHT = 0x08;
		static const unsigned BIT_BLACKOUT = 0x10;
		static const unsigned BIT_DISABLE = 0x20;

		SigmaClient();
		virtual ~SigmaClient();

		void start();
		void close();

	private:
		ostream dump;
		bool doLog;
		ostream& log();
		void load();

		void loop();

		void events();
		void procMouse();
		void procClick();

		void render();

		void renderMenus();
		void renderGame();

		void blackout(unsigned id);
		void highlight(unsigned id, unsigned r = 255, unsigned g = 255,
				unsigned b = 0);

		void windowResize();
		void fullscreenToggle();
		void createWindow(sf::VideoMode vm, int style,
				sf::ContextSettings settings);
		void initWindow();

		void receivePacket(ServerPacket &packet);
		void sendPacket(ServerPacket &packet);

		bool running;
		sf::Clock timer;
		float transp;
		mutex lock;

		enum GameMode {
			MENU,
			GAME
		};
		GameMode mode;

		vector<sf::FloatRect> blocks;

		Board board;
		Board hovers;

		bool enableInteraction = true;

		sf::RenderWindow *window;
		bool fullscreen = false;

		sf::Transform playAreaPosition;

		sf::Texture boardTexture;
		sf::Sprite boardSprite;

		sf::Texture bgTexture;
		sf::Sprite bgSprite;

		sf::Texture xTexture;
		sf::Sprite xSprite;

		sf::Texture oTexture;
		sf::Sprite oSprite;

		SigmaServer *server;

		Phase currentPhase;
		unsigned currentTileset;

		LightsController lights;
		double scale;
};

#endif /* SIGMACLIENT_H_ */
