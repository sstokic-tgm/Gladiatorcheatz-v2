#pragma once

#include "../Singleton.hpp"

#include "../Structs.hpp"

#include <deque>

class QAngle;
class C_BasePlayer;

struct STickRecord
{
	void SaveRecord(C_BasePlayer *player)
	{
		m_flLowerBodyYawTarget = player->m_flLowerBodyYawTarget();;
		m_angEyeAngles = player->m_angEyeAngles();
		m_flSimulationTime = player->m_flSimulationTime();
		m_flPoseParameter = player->m_flPoseParameter();
		m_flCurTime = g_GlobalVars->curtime;
		m_nFlags = player->m_fFlags();
		m_flVelocity = player->m_vecVelocity().Length2D();
		m_vecVelocity = player->m_vecVelocity();

		m_iLayerCount = player->GetNumAnimOverlays();
		for (int i = 0; i < m_iLayerCount; i++)
			animationLayer[i] = player->GetAnimOverlays()[i];
	}

	bool operator==(STickRecord &other)
	{
		return other.m_flSimulationTime == m_flSimulationTime;
	}

	float m_flVelocity = 0.f;
	Vector m_vecVelocity = Vector(0, 0, 0);
	float m_flSimulationTime = 0.f;
	float m_flLowerBodyYawTarget = 0.f;
	QAngle m_angEyeAngles = QAngle(0, 0, 0);
	std::array<float, 24> m_flPoseParameter = {};
	float m_flCurTime = 0.f;
	int m_nFlags = 0;

	int m_iLayerCount = 0;
	AnimationLayer animationLayer[15];
};

struct SResolveInfo
{
	std::deque<STickRecord> arr_tickRecords;

	STickRecord curTickRecord;
	STickRecord prevTickRecord;

	float m_flLastLbyTime = 0.f;

	QAngle m_angDirectionFirstMoving = QAngle(0, 0, 0);
};

class Resolver : public Singleton<Resolver>
{

public:

	void Resolve(C_BasePlayer *player);

	SResolveInfo arr_infos[64];

private:
	STickRecord GetLatestUpdateRecord(C_BasePlayer *player);

	void StartResolving(C_BasePlayer *player);
	float_t Bruteforce(C_BasePlayer *player);
	bool IsEntityMoving(C_BasePlayer *player);
	bool IsAdjustingBalance(C_BasePlayer *player, STickRecord &record, AnimationLayer *layer);

	const inline float_t LBYDelta(const STickRecord &v)
	{
		return v.m_angEyeAngles.yaw - v.m_flLowerBodyYawTarget;
	}
};