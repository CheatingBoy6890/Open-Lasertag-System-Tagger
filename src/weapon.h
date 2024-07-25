/*
Copyright (c) 2024, Silas Hille
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.

*/

#define ENABLE_WEAPONS
#include <Arduino.h>
#include <TaskScheduler.h>
#ifdef ENABLE_WEAPONS
#include "icons.h"
#else
#include <U8g2lib.h>
U8G2_SH1106_128X64_NONAME_F_HW_I2C Display(U8G2_R1);
#endif

// Damage values for Milestag2 protocol from https://wiki.cuvoodoo.info/lib/exe/fetch.php?media=ir-cock-grenade:mt2proto.pdf
uint8_t Damage[16] = {
    1, 2, 4, 5, 7, 10, 15, 17, 20, 25, 30, 35, 40, 50, 75, 100};

class weapon
{
private:
    // pointer to the array holding the image
    uint8_t *icon;

    uint8_t icon_width;
    uint8_t icon_height;

    // the number of iterrations the shootTask should do 1 for semi-auto, some number for burst, infinte for full-auto
    int32_t shot_iterrations;

    // Time between the Task can be started again should be 0 for full-auto and the wanted time for burst and semi-auto
    uint32_t time_between_shooting;

    // Time it takes before the weapon shoots if you some kind of charging weapon
    uint32_t time_before_shoot;
    // The time between automatic shots, important for full and semi auto
    uint32_t shoot_interval;

    // The maximum number of bullets that "fit" in a magazine
    uint16_t max_bullets;
    // How long you have to hold before the reloading starts.
    uint32_t time_before_reload;
    // should the whole magazine be reloaded at once
    bool reload_full_magazine;
    // How long it takes for each bullet to reload. Ignored if reload_full_magazine is true.
    uint32_t time_per_reload;
    // The dammage per bullet
    uint8_t dammage;

public:
    weapon(uint8_t *ico, uint8_t w, uint8_t h, int32_t iterations, uint32_t between, uint32_t t_before_shot, uint32_t interval, uint8_t dmg, uint16_t bullets, uint32_t t_before_reload, bool all_at_once, uint32_t t_per_bullet)
    {
        icon = ico;
        icon_width = w;
        icon_height = h;
        shot_iterrations = iterations;
        time_between_shooting = between;
        time_before_shoot = t_before_shot;
        shoot_interval = interval;
        max_bullets = bullets;
        dammage = dmg;
        time_before_reload = t_before_reload;
        reload_full_magazine = all_at_once;
        time_per_reload = t_per_bullet;
    }

    void createShootTask(Task &shootTask)
    {
        shootTask.setInterval(shoot_interval);
        shootTask.setIterations(shot_iterrations);
    }

    uint8_t *getIcon()
    {
        return icon;
    }

    bool getReloadAtOnce()
    {
        return reload_full_magazine;
    }

    uint16_t getMaxBullets()
    {
        return max_bullets;
    }

    uint8_t getIconWidth()
    {
        return icon_width;
    }

    uint8_t getIconHeight()
    {
        return icon_height;
    }

    uint32_t getTimeBeforeShot()
    {
        return time_before_shoot;
    }

    void drawIcon(uint8_t x, uint8_t y)
    {
        Display.drawXBMP(x, y, icon_width, icon_height, icon);
    }

    uint32_t getReloadInterval()
    {
        return time_per_reload;
    }

    uint8_t getDammage()
    {
        return dammage;
    }

    uint32_t getTimeBeforeReload()
    {
        return time_before_reload;
    }

    uint32_t getTimebetwennshots()
    {
        return time_between_shooting;
    }
    uint32_t getIterations()
    {
        return shot_iterrations;
    }
};

// static const weapon pistol(pistol_bits, pistol_width, pistol_height, 1, 400, 0, 0, 9, 12, 1500, false, 600);
// static const weapon maschinegun(maschinegun_bits, maschinegun_width, maschinegun_height, -1, 0, 0, 10, 7, 48, 5000, false, 300);
// static const weapon burst_pistol(pistol_burst_bits, pistol_burst_width, pistol_burst_height, 3, 400, 0, 100, 8, 27, 2000, false, 450);
#ifdef ENABLE_WEAPONS
static weapon weapons[]{
    {pistol_bits, pistol_width, pistol_height, 1, 400, 0, 0, 9, 12, 1500, false, 600},                    // pistol
    {maschinegun_bits, maschinegun_width, maschinegun_height, -1, 0, 0, 10, 7, 48, 5000, false, 300},     // maschinegun
    {pistol_burst_bits, pistol_burst_width, pistol_burst_height, 3, 400, 0, 100, 8, 27, 2000, false, 450} // burst_pistol
};

#else
static weapon weapons[]{
    {nullptr, 0, 0, 1, 400, 0, 0, 9, 12, 1500, false, 600},
    {nullptr, 0, 0, 1, 400, 0, 0, 9, 12, 1500, false, 600}};
#endif
