/*
 * SigmaServer.cpp
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

#include "SigmaServer.h"
#include "SigmaClient.h"
#include <iostream>

using namespace std;

SigmaServer::SigmaServer() {
	currentSuperPos = 0;
	currentPlayer = 0;
	currentPhase = Phase::WaitingConnection;
	needsLoop = true;
	running = false;
	sp = NULL;
	board = Board(90, 0);

	subGameX = 0;
	subGameY = 0;
}

SigmaServer::~SigmaServer() {
}

void SigmaServer::start() {
	running = true;
	while (running) {
		cout << "Server loop" << endl;
		loop();

		needsLoop = false;
		unique_lock<mutex> lock(loopMutex);
		loopUnlocker.wait(lock, [this] {
			return needsLoop;
		});
	}
}

void SigmaServer::stop() {
	running = false;
	needsLoop = true;
	loopUnlocker.notify_all();
}

ostream& SigmaServer::log() {
	return cout << "[SERVER] ";
}

void SigmaServer::receivePacket(ServerPacket& p) {
	log() << "Received packet: " << p.type << endl;

	if (sp) {
		p.from = currentPlayer;
	} else if (p.from != currentPlayer) {
		log() << "Ignored" << endl;
		return;
	}

	switch (p.type) {
		case ServerPacket::ConnectionStart:
			//TODO
			break;
		case ServerPacket::CellClick:
			procClick(p);
			break;
		default:
			break;
	}

	//needsLoop = true;
	if (needsLoop) {
		loopUnlocker.notify_all();
	}
}

void SigmaServer::sendPacket(ServerPacket& p, SigmaClient* client) {
	log() << "Sending packet" << endl;
	client->receivePacket(p);
}

void SigmaServer::sendPacket(ServerPacket& p, sf::Socket* socket) {
}

void SigmaServer::sendPacket(ServerPacket& p) {
	if (sp) {
		sendPacket(p, sp);
	}
}

void SigmaServer::sendPacketToAll(ServerPacket& p) {
	if (sp) {
		sendPacket(p, sp);
	}
}

vector<vector<unsigned> > SigmaServer::getSubGame(unsigned x, unsigned y) {
	vector<vector<unsigned> > v(3, vector<unsigned>(3, 0));

	for (unsigned i = 0; i < 81; i++) {
		if (i / 3 != x || i / 27 != y) {
			continue;
		}

		v[i / 9 - y * 3][i % 9 - x * 3] = board[i];
	}

	return v;
}

vector<vector<unsigned> > SigmaServer::getSuperGame() {
	vector<vector<unsigned> > v(3, vector<unsigned>(3, 0));

	return v;
}

void SigmaServer::loop() {
	switch (currentPhase) {
		case Phase::WaitingConnection:
			if (sp) {
				currentPlayer = 1;
				currentPhase = Phase::ChooseSuper;
				gameStartPackets();
				boardReset();
			}
			break;
		case Phase::ChooseSuper:
			break;
		default:
			break;
	}
}

void SigmaServer::gameStartPackets() {
	if (sp) {
		ServerPacket packetChange(ServerPacket::PhaseChange);
		packetChange.phase->phase = Phase::ChooseSuper;

		sendPacket(packetChange);

		ServerPacket packetEnable(ServerPacket::InteractionChange);
		packetEnable.interaction->enable = true;

		sendPacket(packetEnable);
	}
}

void SigmaServer::sendPacketChangePhase(Phase phase) {
	ServerPacket packetChange(ServerPacket::PhaseChange);
	packetChange.phase->phase = phase;

	sendPacket(packetChange);
}

void SigmaServer::sendPacketChangeCell(unsigned id, unsigned val) {
	ServerPacket packetChange(ServerPacket::CellChange);
	packetChange.change->id = id;
	packetChange.change->newValue = val;

	sendPacketToAll(packetChange);
}

void SigmaServer::sendPacketBoardState() {
	ServerPacket packetChange(ServerPacket::BoardState);
	packetChange.board->state = board;

	sendPacketToAll(packetChange);
}

void SigmaServer::boardReset() {
	fill(board.begin(), board.end(), 0);
}

void SigmaServer::procClick(ServerPacket &p) {
	switch (currentPhase) {
		case Phase::ChooseSuper:
			procChooseSuper(p.click->id);
			break;
		case Phase::ChooseCell:
			procChooseCell(p.click->id);
			break;
		default:
			break;
	}
}

void SigmaServer::boardSetAndNotify(unsigned id, unsigned val) {
	board[id] = val;
	sendPacketChangeCell(id, val);
}

void SigmaServer::boardSetAndNotify(vector<pair<unsigned, unsigned> > vals) {
	for (auto it = vals.begin(); it != vals.end(); ++it) {
		board[it->first] = it->second;
	}

	sendPacketBoardState();
}

void SigmaServer::procChooseSuper(unsigned id) {
	if (board[id] & SigmaClient::MASK_ID) {
		return;
	}

	log() << "Clicked on " << id << endl;

	subGameX = (id - 81) % 3;
	subGameY = (id - 81) / 3;

	log() << "SubGame: " << subGameX << ", " << subGameY << endl;

	for (unsigned x = 0; x < 3; x++) {
		for (unsigned y = 0; y < 3; y++) {
			eachAt(x, y, [&](unsigned, unsigned, unsigned i) {
				if(x == subGameX && y == subGameY) {
					if(!(board[i]&SigmaClient::MASK_ID)) {
						board[i] &= ~SigmaClient::BIT_DISABLE;
					}
				} else {
					board[i] |= SigmaClient::BIT_DISABLE;
				}
			});
		}
	}

	eachBig(
			[&](unsigned x, unsigned y, unsigned i) {
				if(subGameX == x && subGameY == y) {
					board[i] |= SigmaClient::BIT_HIGHLIGHT;
					log() << "Enable " << i << endl;
				} else {
					board[i] = (board[i] & (~SigmaClient::BIT_HIGHLIGHT)) | SigmaClient::BIT_BLACKOUT;
					log() << "Disable " << i << endl;
				}
			});

	sendPacketBoardState();

	currentPhase = Phase::ChooseCell;
	sendPacketChangePhase(currentPhase);
}

void SigmaServer::procChooseCell(unsigned id) {
	if ((board[id] & SigmaClient::MASK_ID)
			|| !isInSubGame(subGameX, subGameY, id)) {
		log() << "Ignored cell" << endl;
		if(!isInSubGame(subGameX, subGameY, id)) {
			log() << "Not in subgame" << endl;
		}
		return;
	}

	board[id] |= currentPlayer;
	log() << "Set cell " << id << " to " << currentPlayer << endl;

	each([&](unsigned i) {
		if(i < 81) {
			if(!(board[i] && SigmaClient::MASK_ID)) {
				board[i] &= ~SigmaClient::BIT_DISABLE;
			} else {
				board[i] |= SigmaClient::BIT_DISABLE;
			}
		} else {
			if(!(board[i] & SigmaClient::MASK_ID)) {
				board[i] &= ~SigmaClient::BIT_BLACKOUT;
			}

			board[i] &= ~SigmaClient::BIT_HIGHLIGHT;

		}
	});

	sendPacketBoardState();

	currentPhase = Phase::ChooseSuper;
	sendPacketChangePhase(currentPhase);
	//needsLoop = true;
}

void SigmaServer::eachAt(unsigned x, unsigned y,
		function<void(unsigned, unsigned, unsigned)> function) {
	for (unsigned xs = 0; xs < 3; xs++) {
		for (unsigned ys = 0; ys < 3; ys++) {
			unsigned i = y * 27 + 9 * ys + x * 3 + xs;
			function(xs, ys, i);
		}
	}
}

void SigmaServer::each(function<void(unsigned)> function) {
	for (unsigned i = 0; i < 90; i++) {
		function(i);
	}
}

void SigmaServer::eachBig(
		function<void(unsigned, unsigned, unsigned)> function) {
	for (unsigned xs = 0; xs < 3; xs++) {
		for (unsigned ys = 0; ys < 3; ys++) {
			unsigned i = 81 + xs + 3 * ys;
			function(xs, ys, i);
		}
	}

}

bool SigmaServer::isInSubGame(unsigned x, unsigned y, unsigned i) {
	log() << i << " in " << x << ", " << y << "? " << ((i%9) / 3 == x) << ", " << (i / 27 == y) << endl;
	return (((i%9) / 3 == x) && (i / 27 == y));
}
