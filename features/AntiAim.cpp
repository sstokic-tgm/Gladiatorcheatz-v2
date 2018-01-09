#include "AntiAim.hpp"

#include "../Structs.hpp"
#include "../Options.hpp"

#include "AimRage.hpp"
#include "Resolver.hpp"
#include "RebuildGameMovement.hpp"

#include "../helpers/Utils.hpp"
#include "../helpers/Math.hpp"

void AntiAim::Work(CUserCmd *usercmd)
{
	this->usercmd = usercmd;

	if (usercmd->buttons & IN_USE)
		return;

	if (g_LocalPlayer->m_bGunGameImmunity() || g_LocalPlayer->m_fFlags() & FL_FROZEN)
		return;

	auto weapon = g_LocalPlayer->m_hActiveWeapon().Get();

	if (!weapon)
		return;

	if (weapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER)
	{
		if (usercmd->buttons & IN_ATTACK2)
			return;

		if (weapon->CanFirePostPone() && (usercmd->buttons & IN_ATTACK))
			return;
	}
	else if (weapon->GetWeapInfo()->weapon_type == WEAPONTYPE_GRENADE)
	{
		if (weapon->IsInThrow())
			return;
	}
	else
	{
		if (weapon->GetWeapInfo()->weapon_type == WEAPONTYPE_KNIFE && ((usercmd->buttons & IN_ATTACK) || (usercmd->buttons & IN_ATTACK2)))
			return;
		else if ((usercmd->buttons & IN_ATTACK) && (weapon->m_iItemDefinitionIndex() != WEAPON_C4 || g_Options.hvh_antiaim_x != AA_PITCH_OFF))
			return;
	}

	if (g_LocalPlayer->GetMoveType() == MOVETYPE_NOCLIP || g_LocalPlayer->GetMoveType() == MOVETYPE_LADDER)
		return;

	if (g_Options.hvh_antiaim_x == AA_PITCH_OFF && g_Options.hvh_antiaim_y_fake == AA_FAKEYAW_FORWARD)
	{
		INetChannelInfo *nci = g_EngineClient->GetNetChannelInfo();
		if (nci)
			if (nci->GetAvgLoss(FLOW_INCOMING) > 0 || nci->GetAvgLoss(FLOW_OUTGOING) > 0)
				return;
	}

	float curpos = usercmd->viewangles.yaw + 180.f;

	usercmd->viewangles = QAngle(GetPitch(), g_Options.hvh_antiaim_y_fake > AA_FAKEYAW_OFF ? (usercmd->command_number % 2 ? GetFakeYaw(curpos) : GetYaw(curpos)) : GetYaw(curpos), 0);

	if (g_Options.hvh_antiaim_y_fake > AA_FAKEYAW_OFF)
		Global::bSendPacket = usercmd->command_number % 2;
}

void AntiAim::UpdateLowerBodyBreaker(CUserCmd *userCMD)
{
	bool
		allocate = (m_serverAnimState == nullptr),
		change = (!allocate) && (&g_LocalPlayer->GetRefEHandle() != m_ulEntHandle),
		reset = (!allocate && !change) && (g_LocalPlayer->m_flSpawnTime() != m_flSpawnTime);

	// player changed, free old animation state.
	if (change)
		g_pMemAlloc->Free(m_serverAnimState);

	// need to reset? (on respawn)
	if (reset) {
		// reset animation state.
		C_BasePlayer::ResetAnimationState(m_serverAnimState);

		// note new spawn time.
		m_flSpawnTime = g_LocalPlayer->m_flSpawnTime();
	}

	// need to allocate or create new due to player change.
	if (allocate || change) {

		// only works with games heap alloc.
		CCSGOPlayerAnimState *state = (CCSGOPlayerAnimState*)g_pMemAlloc->Alloc(sizeof(CCSGOPlayerAnimState));

		if (state != nullptr)
			g_LocalPlayer->CreateAnimationState(state);

		// used to detect if we need to recreate / reset.
		m_ulEntHandle = const_cast<CBaseHandle*>(&g_LocalPlayer->GetRefEHandle());
		m_flSpawnTime = g_LocalPlayer->m_flSpawnTime();

		// note anim state for future use.
		m_serverAnimState = state;
	}

	// back up some data to not mess with game.. 

	float cur_time = cur_time = TICKS_TO_TIME(AimRage::Get().GetTickbase());
	if (!g_ClientState->chokedcommands)
	{
		C_BasePlayer::UpdateAnimationState(m_serverAnimState, userCMD->viewangles);

		// calculate vars
		float delta = std::abs(Math::ClampYaw(userCMD->viewangles.yaw - g_LocalPlayer->m_flLowerBodyYawTarget()));

		// walking, delay next update by .22s.
		if (m_serverAnimState->GetVelocity() > 0.1f &&
			(g_LocalPlayer->m_fFlags() & FL_ONGROUND))
			m_flNextBodyUpdate = cur_time + 0.22f;

		// standing, update every 1.1s.
		else if (cur_time >= m_flNextBodyUpdate) {

			if (delta > 35.f)
				; //server will update lby.

			m_flNextBodyUpdate = cur_time + 1.1f;
		}
	}

	// if was jumping and then onground and bsendpacket true, we're gonna update.
	m_bBreakLowerBody = (g_LocalPlayer->m_fFlags() & FL_ONGROUND) && ((m_flNextBodyUpdate - cur_time) <= g_GlobalVars->interval_per_tick);

	// bandaid fix, sends 1 command before break to clear commands, sends another for break. IDK how 2 fix yet.
	/*if (m_bBreakLowerBody)
		Global::bSendPacket = true;*/
}

float AntiAim::GetPitch()
{
	switch (g_Options.hvh_antiaim_x)
	{
	case AA_PITCH_OFF:

		return usercmd->viewangles.pitch;
		break;

	case AA_PITCH_DYNAMIC:

		return g_LocalPlayer->m_hActiveWeapon().Get()->IsSniper() ? (g_LocalPlayer->m_hActiveWeapon().Get()->m_zoomLevel() != 0 ? 87.f : 85.f) : 88.99f;
		break;

	case AA_PITCH_DOWN:

		return 88.99f;
		break;

	case AA_PITCH_STRAIGHT:

		return 0.f;
		break;

	case AA_PITCH_UP:

		return -88.99f;
		break;
	}

	return usercmd->viewangles.pitch;
}

float AntiAim::GetYaw(float pos)
{
	static bool state = false;
	state = !state;

	switch (g_Options.hvh_antiaim_y)
	{
	case AA_YAW_BACKWARD:

		return pos;
		break;

	case AA_YAW_JITTER:

		return state ? pos + 33.3f : pos - 33.3f;
		break;

	case AA_YAW_INVADE_JITTER:
	{
		QAngle lby = QAngle(0, g_LocalPlayer->m_flLowerBodyYawTarget(), 0);
		QAngle result = state ? QAngle(0, pos + 33.3f, 0) : QAngle(0, pos - 33.3f, 0);

		return result.RealYawDifference(lby) < 27.5f ? pos : result.yaw;
		break;
	}
	case AA_YAW_RANDOM_JITTER:

		return pos + Utils::RandomFloat(-135.f, 135.f);
		break;

	case AA_YAW_SYNCHRONIZE:
	{
		QAngle lby = QAngle(0, g_LocalPlayer->m_flLowerBodyYawTarget(), 0);
		QAngle result = QAngle(0, pos - 180.f, 0);

		return (result.RealYawDifference(lby) < 75.f || result.RealYawDifference(lby) > 165.f) ? lby.yaw + 90.f : lby.yaw + 180.f;
		break;
	}
	case AA_YAW_SPIN:

		return fmod(g_GlobalVars->curtime / 1.f * 360.f, 360.f) + 180;
		break;

	case AA_YAW_FASTSPIN:

		return fmod(g_GlobalVars->curtime * 15.0f * 360.0f, 360.0f) + 180;
		break;

	case AA_YAW_LBY_BREAKER:

		float lby = 0.f;

		
		if (!Global::bSendPacket)
        {
			if (m_bBreakLowerBody)
				lby = usercmd->viewangles.yaw + 90.f;
            else
                lby = usercmd->viewangles.yaw - 90.f;
        }
        else
        {
			lby = usercmd->viewangles.yaw + 90.f;
        }

		return lby;
		break;
	}

	return 0;
}

float AntiAim::GetFakeYaw(float pos)
{
	static bool state;
	state = !state;

	switch (g_Options.hvh_antiaim_y_fake)
	{
	case AA_FAKEYAW_FORWARD:

		return usercmd->viewangles.yaw;
		break;

	case AA_FAKEYAW_JITTER:

		return state ? pos + 33.3f : pos - 33.3f;
		break;

	case AA_FAKEYAW_EVADE:

		return state ? pos + 80.f : pos - 80.f;
		break;

	case AA_FAKEYAW_SYNCHRONIZE:

		return g_LocalPlayer->m_flLowerBodyYawTarget();
		break;

	case AA_FAKEYAW_SPIN:

		return fmod(g_GlobalVars->curtime / 1.f * 360.f, 360.f);
		break;

	case AA_FAKEYAW_FASTSPIN:

		return fmod(g_GlobalVars->curtime * 15.0f * 360.0f, 360.0f);
		break;

	case AA_FAKEYAW_LBY_BREAKER:

		float lby = 0.f;

		if (!Global::bSendPacket)
        {
            if (m_bBreakLowerBody)
                lby = usercmd->viewangles.yaw - 90.f;
            else
                lby = usercmd->viewangles.yaw + 90.f;
        }
        else
        {
			lby = usercmd->viewangles.yaw - 90.f;
        }

		return lby;
		break;
	}

	return 0;
}

void Accelerate(C_BasePlayer *player, Vector &wishdir, float wishspeed, float accel, Vector &outVel)
{
	// See if we are changing direction a bit
	float currentspeed = outVel.Dot(wishdir);

	// Reduce wishspeed by the amount of veer.
	float addspeed = wishspeed - currentspeed;

	// If not going to add any speed, done.
	if (addspeed <= 0)
		return;

	// Determine amount of accleration.
	float accelspeed = accel * g_GlobalVars->frametime * wishspeed * player->m_surfaceFriction();

	// Cap at addspeed
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	// Adjust velocity.
	for (int i = 0; i < 3; i++)
		outVel[i] += accelspeed * wishdir[i];
}

void WalkMove(C_BasePlayer *player, Vector &outVel)
{
	Vector forward, right, up, wishvel, wishdir, dest;
	float_t fmove, smove, wishspeed;

	Math::AngleVectors(player->m_angEyeAngles(), forward, right, up);  // Determine movement angles
	// Copy movement amounts
	g_MoveHelper->SetHost(player);
	fmove = g_MoveHelper->m_flForwardMove;
	smove = g_MoveHelper->m_flSideMove;
	g_MoveHelper->SetHost(nullptr);

	if (forward[2] != 0)
	{
		forward[2] = 0;
		Math::NormalizeVector(forward);
	}

	if (right[2] != 0)
	{
		right[2] = 0;
		Math::NormalizeVector(right);
	}

	for (int i = 0; i < 2; i++)	// Determine x and y parts of velocity
		wishvel[i] = forward[i] * fmove + right[i] * smove;

	wishvel[2] = 0;	// Zero out z part of velocity

	wishdir = wishvel; // Determine maginitude of speed of move
	wishspeed = wishdir.Normalize();

	// Clamp to server defined max speed
	g_MoveHelper->SetHost(player);
	if ((wishspeed != 0.0f) && (wishspeed > g_MoveHelper->m_flMaxSpeed))
	{
		VectorMultiply(wishvel, player->m_flMaxspeed() / wishspeed, wishvel);
		wishspeed = player->m_flMaxspeed();
	}
	g_MoveHelper->SetHost(nullptr);
	// Set pmove velocity
	outVel[2] = 0;
	Accelerate(player, wishdir, wishspeed, g_CVar->FindVar("sv_accelerate")->GetFloat(), outVel);
	outVel[2] = 0;

	// Add in any base velocity to the current velocity.
	VectorAdd(outVel, player->m_vecBaseVelocity(), outVel);

	float spd = outVel.Length();

	if (spd < 1.0f)
	{
		outVel.Init();
		// Now pull the base velocity back out. Base velocity is set if you are on a moving object, like a conveyor (or maybe another monster?)
		VectorSubtract(outVel, player->m_vecBaseVelocity(), outVel);
		return;
	}

	g_MoveHelper->SetHost(player);
	g_MoveHelper->m_outWishVel += wishdir * wishspeed;
	g_MoveHelper->SetHost(nullptr);

	// Don't walk up stairs if not on ground.
	if (!(player->m_fFlags() & FL_ONGROUND))
	{
		// Now pull the base velocity back out. Base velocity is set if you are on a moving object, like a conveyor (or maybe another monster?)
		VectorSubtract(outVel, player->m_vecBaseVelocity(), outVel);
		return;
	}

	// Now pull the base velocity back out. Base velocity is set if you are on a moving object, like a conveyor (or maybe another monster?)
	VectorSubtract(outVel, player->m_vecBaseVelocity(), outVel);
}

void AntiAim::Fakewalk(CUserCmd *userCMD)
{
	Global::bFakewalking = false;

	if (!g_Options.misc_fakewalk || !g_InputSystem->IsButtonDown(g_Options.misc_fakewalk_bind))
		return;

	Vector velocity = Global::vecUnpredictedVel;

	int Iterations = 0;
	for (; Iterations < 15; ++Iterations) {
		if (velocity.Length() < 0.1)
		{
			g_CVar->ConsolePrintf("Ticks till stop %d\n", Iterations);
			break;
		}

		Friction(velocity);
		WalkMove(g_LocalPlayer, velocity);

		/*if(Iterations == 0)
			Utils::ConsolePrint(false, "========= anim %f\n", velocity.Length2D());*/
	}

	int choked_ticks = Global::nChockedTicks;

	if (Iterations > 7 - choked_ticks || !choked_ticks)
	{
		float_t speed = velocity.Length();

		QAngle direction;
		Math::VectorAngles(velocity, direction);

		direction.yaw = userCMD->viewangles.yaw - direction.yaw;

		Vector forward;
		Math::AngleVectors(direction, forward);
		Vector negated_direction = forward * -speed;

		userCMD->forwardmove = negated_direction.x;
		userCMD->sidemove = negated_direction.y;
	}

	if (Global::nChockedTicks < 7)
		Global::bShouldChoke = true;

	Global::bFakewalking = true;
}

void AntiAim::Friction(Vector &outVel)
{
	float speed, newspeed, control;
	float friction;
	float drop;

	speed = outVel.Length();

	if (speed <= 0.1f)
		return;

	drop = 0;

	// apply ground friction
	if (g_LocalPlayer->m_fFlags() & FL_ONGROUND)
	{
		friction = g_CVar->FindVar("sv_friction")->GetFloat() * g_LocalPlayer->m_surfaceFriction();

		// Bleed off some speed, but if we have less than the bleed
		// threshold, bleed the threshold amount.
		control = (speed < g_CVar->FindVar("sv_stopspeed")->GetFloat()) ? g_CVar->FindVar("sv_stopspeed")->GetFloat() : speed;

		// Add the amount to the drop amount.
		drop += control * friction * g_GlobalVars->frametime;
	}

	newspeed = speed - drop;
	if (newspeed < 0)
		newspeed = 0;

	if (newspeed != speed)
	{
		// Determine proportion of old speed we are using.
		newspeed /= speed;
		// Adjust velocity according to proportion.
		VectorMultiply(outVel, newspeed, outVel);
	}
}