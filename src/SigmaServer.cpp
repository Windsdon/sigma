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
		needsLoop = false;
		loop();

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

	eachAt(x, y, [this, &v](unsigned x, unsigned y, unsigned i) {
		v[y][x] = board[i];
	});

	return v;
}

vector<vector<unsigned> > SigmaServer::getSuperGame() {
	vector<vector<unsigned> > v(3, vector<unsigned>(3, 0));
	eachBig([&] (uint x, uint y, uint i) {
		v[y][x] = board[i];
	});
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
		case Phase::CheckVictory:
			doVictoryCheck();
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

void SigmaServer::sendPacketChangeTileset(unsigned tileset) {
	ServerPacket packetChange(ServerPacket::TilesetChange);
	packetChange.tileset->id = tileset;

	sendPacketToAll(packetChange);
}

void SigmaServer::boardReset() {
	fill(board.begin(), board.end(), 0);
	done.clear();
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
		return;
	}

	board[id] |= currentPlayer;
	log() << "Set cell " << id << " to " << currentPlayer << endl;

	targetX = id % 9 - subGameX * 3;
	targetY = id / 9 - subGameY * 3;

	log() << "Target is " << targetX << ", " << targetY << endl;

	each([&](unsigned i) {
		if(i < 81) {
			if(!(board[i] & SigmaClient::MASK_ID)) {
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

	currentPhase = Phase::CheckVictory;
	sendPacketChangePhase(currentPhase);
	needsLoop = true;
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
	return (((i % 9) / 3 == x) && (i / 27 == y));
}

int SigmaServer::victoryCheck(vector<vector<unsigned> > v, PairVec& list) {
	PairVec q;
	uint p;
	for (p = 1; p <= 2; p++) {
		q.clear();
		// diagonal
		for (uint i = 0; i < 3; i++) {
			if (v[i][i] != p) {
				break;
			} else {
				q.push_back(make_pair(i, i));
			}
		}

		if (q.size() == 3) {
			break;
		}

		q.clear();
		// anti-diagonal
		for (uint i = 0; i < 3; i++) {
			if (v[i][2 - i] != p) {
				break;
			} else {
				q.push_back(make_pair(i, 2 - i));
			}
		}

		if (q.size() == 3) {
			break;
		}

		//lines
		for (uint l = 0; l < 3; l++) {
			q.clear();
			for (uint i = 0; i < 3; i++) {
				if (v[l][i] != p) {
					break;
				} else {
					q.push_back(make_pair(l, i));
				}
			}

			if (q.size() == 3) {
				break;
			}
		}

		if (q.size() == 3) {
			break;
		}

		//cols
		for (uint l = 0; l < 3; l++) {
			q.clear();
			for (uint i = 0; i < 3; i++) {
				if (v[i][l] != p) {
					break;
				} else {
					q.push_back(make_pair(i, l));
				}
			}

			if (q.size() == 3) {
				break;
			}
		}

		if (q.size() == 3) {
			break;
		}
	}

	if (q.size() == 3) {
		list = q;
		log() << "win" << endl;
		return p;
	}

	return 0;
}

void SigmaServer::doVictoryCheck() {
	for (uint x = 0; x < 3; x++) {
		for (uint y = 0; y < 3; y++) {
			pair<uint, uint> pp = make_pair(x, y);
			if (done.find(pp) != done.end()) {
				continue;
			}

			vector<vector<unsigned> > game = getSubGame(x, y);
			PairVec p;
			onlyPlayer(game);
			int victory = victoryCheck(game, p);

			if (victory < 0) {
				//draw
			} else if (victory) {
				done.insert(pp);
				uint id = 81 + y * 3 + x;
				boardSetAndNotify(id,
						board[id] | victory | SigmaClient::BIT_BLACKOUT);
			} else {

			}
		}
	}

	auto superGame = getSuperGame();
	onlyPlayer(superGame);
	PairVec pp;
	int superVictory = victoryCheck(superGame, pp);
	if (superVictory < 0) {
		currentPhase = Phase::Endgame;
		sendPacketChangePhase(currentPhase);
	} else if (superVictory) {
		for (uint i = 0; i < 3; i++) {
			log() << "winning at " << pp[i].first << ", " << pp[i].second << endl;
			uint k = 81 + pp[i].second + pp[i].first * 3;
			board[k] = (board[k] & (~SigmaClient::BIT_BLACKOUT))
					| SigmaClient::BIT_HIGHLIGHT;
		}
		sendPacketBoardState();
		currentPhase = Phase::Endgame;
		sendPacketChangePhase(currentPhase);
		return;
	}

	changePlayer();
	if (superGame[targetY][targetX] || (targetX == subGameX && targetY == subGameY)) {
		currentPhase = Phase::ChooseSuper;
		sendPacketChangePhase(currentPhase);
	} else {
		log() << "Can place there! (" << targetX << ", " << targetY << ")" << endl;
		procChooseSuper(81 + targetX + targetY * 3);
	}

}

void SigmaServer::onlyPlayer(vector<vector<unsigned> >& o) {
	for (uint i = 0; i < 3; i++) {
		for (uint j = 0; j < 3; j++) {
			o[i][j] &= SigmaClient::MASK_ID;
		}
	}
}

void SigmaServer::changePlayer() {
	sendPacketChangeInteraction(currentPlayer, false);
	if (currentPlayer == 1) {
		currentPlayer = 2;
	} else {
		currentPlayer = 1;
	}
	sendPacketChangeInteraction(currentPlayer, true);

	sendPacketChangeTileset(currentPlayer);
}

void SigmaServer::sendPacketChangeInteraction(unsigned player,
		bool interaction) {
	//TODO
	ServerPacket packet(ServerPacket::InteractionChange);
	packet.interaction->enable = interaction;
	sendPacketToAll(packet);
}
