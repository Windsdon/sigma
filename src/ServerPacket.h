/*
 * ServerPacket.h
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

#ifndef SERVERPACKET_H_
#define SERVERPACKET_H_

#include <string>
#include <vector>

using namespace std;

enum Phase {
	WaitingConnection, ChooseSuper, ChooseCell, CheckVictory, ChangePlayer, Endgame
};

class ServerPacket {
	public:
		enum ConnectionType {
			SINGLEPLAYER, MULTIPLAYER
		};

		enum PacketType {
			Unknown, // wtf
			ConnectionStart, // data in packet.start
			ConnectionAccept, // no data
			BoardState, // data in envent.board
			CellChange, // data in event.change
			CellClick, // data in event.click
			TilesetChange, // data in event.tileset
			InteractionChange, // data in event.interaction
			PhaseChange, // data in event.phase
		};

		struct ConnectionStartPacket {
				ConnectionType type;
				string playerName;
		};

		struct BoardStatePacket {
				vector<unsigned> state;
		};

		struct CellChangePacket {
				unsigned id;
				unsigned newValue;
		};

		struct CellClickPacket {
				unsigned id;
		};

		struct TilesetChangePacket {
				unsigned id;
		};

		struct InteractionPacket {
				bool enable;
		};

		struct PhasePacket {
				Phase phase;
		};

		PacketType type;
		unsigned from;

		union {
				ConnectionStartPacket *start;
				BoardStatePacket *board;
				CellChangePacket *change;
				CellClickPacket *click;
				TilesetChangePacket *tileset;
				InteractionPacket *interaction;
				PhasePacket *phase;
		};

		ServerPacket(PacketType);
		ServerPacket(char*);
		ServerPacket(const ServerPacket&);

		virtual ~ServerPacket();
};

#endif /* SERVERPACKET_H_ */
