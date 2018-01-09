#pragma once

#include "../Singleton.hpp"
#include "../Structs.hpp"

class CUserCmd;
class CRecvProxyData;
class Vector;

enum AA_PITCH
{
	AA_PITCH_OFF,
	AA_PITCH_DYNAMIC,
	AA_PITCH_DOWN,
	AA_PITCH_STRAIGHT,
	AA_PITCH_UP
};

enum AA_YAW
{
	AA_YAW_OFF,
	AA_YAW_BACKWARD,
	AA_YAW_JITTER,
	AA_YAW_INVADE_JITTER,
	AA_YAW_RANDOM_JITTER,
	AA_YAW_SYNCHRONIZE,
	AA_YAW_SPIN,
	AA_YAW_FASTSPIN,
	AA_YAW_LBY_BREAKER
};

enum AA_FAKEYAW
{
	AA_FAKEYAW_OFF,
	AA_FAKEYAW_FORWARD,
	AA_FAKEYAW_JITTER,
	AA_FAKEYAW_EVADE,
	AA_FAKEYAW_SYNCHRONIZE,
	AA_FAKEYAW_SPIN,
	AA_FAKEYAW_FASTSPIN,
	AA_FAKEYAW_LBY_BREAKER
};

class AntiAim : public Singleton<AntiAim>
{

public:

	void Work(CUserCmd *usercmd);
	void UpdateLowerBodyBreaker(CUserCmd *userCMD);
	void Fakewalk(CUserCmd *userCMD);

private:

	float GetPitch();
	float GetYaw(float pos);
	float GetFakeYaw(float pos);
	void Friction(Vector &outVel);

	bool m_bBreakLowerBody = false;
	float_t m_flSpawnTime = 0.f;
	float_t m_flNextBodyUpdate = 0.f;
	CBaseHandle *m_ulEntHandle = nullptr;

	CCSGOPlayerAnimState *m_serverAnimState = nullptr;

	CUserCmd *usercmd = nullptr;
};