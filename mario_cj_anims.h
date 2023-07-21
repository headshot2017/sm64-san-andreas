#ifndef MARIO_CJ_ANIMS_H_INCLUDED
#define MARIO_CJ_ANIMS_H_INCLUDED

/*
    handles and translates CJ's TASK_SIMPLE_NAMED_ANIM to play a specific Mario animation
*/

#include <string>
#include <unordered_map>

#include "CPlayerPed.h"
#include "CTaskSimpleRunNamedAnim.h"

bool animTaskExists(CPlayerPed* ped);

void resetLastAnim();
void runAnimKey(const std::string& name, const int& marioId);
void runAnimKey(CTaskSimpleRunNamedAnim* task, const int& marioId);

#endif // MARIO_CJ_ANIMS_H_INCLUDED
