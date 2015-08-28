/*
 * SigmaServer.h
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

#ifndef SIGMASERVER_H_
#define SIGMASERVER_H_

#include "ServerPacket.h"
#include <SFML/Network.hpp>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <set>

class SigmaClient;
using namespace std;

class SigmaServer {
	public:
		typedef vector<unsigned> Board;
		typedef vector<pair<unsigned, unsigned>> PairVec;
		typedef unsigned uint;
		friend class SigmaClient;

		SigmaServer();
		virtual ~SigmaServer();

		void start();
		void stop();

	private:
		bool doLog;
		ostream dump;
		ostream& log();

		void receivePacket(ServerPacket &p);
		void sendPacket(ServerPacket &p, SigmaClient *client);
		void sendPacket(ServerPacket &p, sf::Socket *socket);
		void sendPacket(ServerPacket &p);
		void sendPacketToAll(ServerPacket &p);

		void sendPacketChangePhase(Phase phase);
		void sendPacketChangeCell(unsigned id, unsigned val);
		void sendPacketBoardState();
		void sendPacketChangeTileset(unsigned tileset);
		void sendPacketChangeInteraction(unsigned player, bool interaction);

		vector<vector<unsigned> > getSubGame(unsigned x, unsigned y);
		vector<vector<unsigned> > getSuperGame();
		void onlyPlayer(vector<vector<unsigned> >&);

		void loop();

		void gameStartPackets();
		void boardReset();

		void procClick(ServerPacket &);
		void procChooseSuper(unsigned id);
		void procChooseCell(unsigned id);
		void doVictoryCheck();
		void changePlayer();

		void boardSetAndNotify(unsigned id, unsigned val);
		void boardSetAndNotify(vector<pair<unsigned, unsigned> > vals);

		void eachAt(unsigned x, unsigned y,
				function<void(unsigned, unsigned, unsigned)>);
		void each(function<void(unsigned)>);
		void eachBig(function<void(unsigned, unsigned, unsigned)>);

		bool isInSubGame(unsigned x, unsigned y, unsigned i);

		int victoryCheck(vector<vector<unsigned> >, PairVec&);

		bool running;

		Board board;

		unsigned currentPlayer;
		Phase currentPhase;
		unsigned currentSuperPos;
		unsigned subGameX;
		unsigned subGameY;

		unsigned targetX;
		unsigned targetY;

		mutex loopMutex;
		condition_variable loopUnlocker;
		bool needsLoop;

		SigmaClient *sp;

		set<pair<uint,uint> > done;
};

#endif /* SIGMASERVER_H_ */
