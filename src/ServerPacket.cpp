/*
 * ServerPacket.cpp
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

#include "ServerPacket.h"

ServerPacket::ServerPacket(PacketType type) :
		type(type), from(0), start(0) {
	switch (type) {
		case PacketType::ConnectionStart:
			start = new ConnectionStartPacket();
			break;
		case PacketType::ConnectionAccept:
			break;
		case PacketType::BoardState:
			board = new BoardStatePacket();
			break;
		case PacketType::CellChange:
			change = new CellChangePacket();
			break;
		case PacketType::CellClick:
			click = new CellClickPacket();
			break;
		case PacketType::TilesetChange:
			tileset = new TilesetChangePacket();
			break;
		case PacketType::InteractionChange:
			interaction = new InteractionPacket();
			break;
		case PacketType::PhaseChange:
			phase = new PhasePacket();
			break;
		case PacketType::Unknown:
			break;
	}
}

ServerPacket::ServerPacket(char* data) {
	type = PacketType::Unknown;
	from = 0;
	start = 0;
	//TODO
}

ServerPacket::ServerPacket(const ServerPacket& p) :
		ServerPacket(p.type) {
	switch (type) {
		case PacketType::ConnectionStart:
			*start = *(p.start);
			break;
		case PacketType::ConnectionAccept:
			break;
		case PacketType::BoardState:
			*board = *(p.board);
			break;
		case PacketType::CellChange:
			*change = *(p.change);
			break;
		case PacketType::CellClick:
			*click = *(p.click);
			break;
		case PacketType::TilesetChange:
			*tileset = *(p.tileset);
			break;
		case PacketType::InteractionChange:
			*interaction = *(p.interaction);
			break;
		case PacketType::PhaseChange:
			*phase = *(p.phase);
			break;
		case PacketType::Unknown:
			break;
	}
}

ServerPacket::~ServerPacket() {
	switch (type) {
		case PacketType::ConnectionStart:
			delete start;
			break;
		case PacketType::ConnectionAccept:
			break;
		case PacketType::BoardState:
			delete board;
			break;
		case PacketType::CellChange:
			delete change;
			break;
		case PacketType::CellClick:
			delete click;
			break;
		case PacketType::TilesetChange:
			delete tileset;
			break;
		case PacketType::InteractionChange:
			delete interaction;
			break;
		case PacketType::PhaseChange:
			delete phase;
			break;
		case PacketType::Unknown:
			break;
	}
}
