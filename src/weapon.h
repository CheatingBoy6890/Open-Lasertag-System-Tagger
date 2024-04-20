#include <Arduino.h>
#include "icons.h"




class weapon
{
private:
    unsigned char* icon;

    int32_t shot_iterrations;
    uint32_t time_between_shooting;

    uint32_t time_before_shoot;
    uint32_t shoot_interval;

    uint16_t max_bullets;

public:
    weapon(unsigned char *ico, int32_t iterations, uint32_t between,uint32_t t_before_shot, uint32_t interval, uint16_t bullets){
        icon = ico;
        shot_iterrations = iterations;
        time_between_shooting = between;
        time_before_shoot = t_before_shot;
        shoot_interval = interval;
        max_bullets = bullets;
    }

    //The number of times the shoot task should run when the button is pressed.
    int32_t get_iterrations(){
        return shot_iterrations;
    }
};


