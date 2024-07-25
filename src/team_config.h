/*
Copyright (c) 2024, Silas Hille and Linus Hille
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.

*/

#include <Arduino.h>
#include <ArduinoJson.h>

struct team // struct to hold team information+
{
    String name;
    uint32_t color;
    int16_t score;
};

// save file with iso 8859-1 encoding to have german umlautes working
team TeamList[4] = {
    // set to exactly the number of teams or the display will look strange
    // If you want special non-english characters eg. 'ä', 'é' ..., use the western iso 8859-1.
    {"Red", 0xFF0000U, 0},
    {"Blue", 0x0000FFU, 0},
    {"Yellow", 0x7F7F00U, 0},
    {"Green", 0x00FF00U, 0},
};

JsonDocument pointsToSync;

// struct player
// {
//     uint32_t NodeIp;
//     uint8_t TeamId;
//     String name;

//     bool operator<(const player &plr) const
//     {
//         return (NodeIp < plr.NodeIp);
//     }

//     player(uint32_t n, uint8_t t = NULL, String nm = "") : NodeIp(n), TeamId(t), name(nm) {}
// };

uint8_t myTeamId = 0;
uint8_t myPlayerId = 0;
uint32_t myPoints = 0;

std::vector<uint32_t> players;
std::vector<uint32_t> you_killed_me;

// player PlayerList[32] = {};
