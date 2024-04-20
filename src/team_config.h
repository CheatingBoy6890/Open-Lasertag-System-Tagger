#include <Arduino.h>

struct team // struct to hold team information+
{
    String name;
    uint32_t color;
    int16_t score;
};

// save file with iso 8859-1 encoding to have german umlautes working
team TeamList[4] = {
    // set to exactly the number of teams or the display will look starnge
    {"Rot", 0xFF0000U, 0},  // german for red
    {"Gr�n", 0x00FF00U, 0}, // german for green requires iso 8859-1 encoding
    {"Blau", 0x0000FFU, 0}, // blue
    {"Gelb", 0x7F7F00U, 0}, // yellow
};

//Damage values for Milestag2 protocol from https://wiki.cuvoodoo.info/lib/exe/fetch.php?media=ir-cock-grenade:mt2proto.pdf
uint8_t Damage[16] = {
    1,2,4,5,7,10,15,17,20,25,30,35,40,50,75,100
};

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
uint8_t myPlayerId =0;
uint32_t myPoints = 0;

std::vector<uint32_t> players;
std::vector<uint32_t> you_killed_me;

// player PlayerList[32] = {};