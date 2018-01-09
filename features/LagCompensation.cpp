#include "LagCompensation.hpp"

#include "../Options.hpp"
#include "AimRage.hpp"

#include "../helpers/Utils.hpp"
#include "../helpers/Math.hpp"
#include "PredictionSystem.hpp"
#include "RebuildGameMovement.hpp"

void LagRecord::SaveRecord(C_BasePlayer *player)
{
	m_flSimulationTime = player->m_flSimulationTime();
	m_vecMins = player->GetCollideable()->OBBMins();
	m_vecMax = player->GetCollideable()->OBBMaxs();
	m_arrflPoseParameters = player->m_flPoseParameter();
	m_nFlags = player->m_fFlags();
	m_vecVelocity = player->m_vecVelocity();
	m_vecHeadSpot = player->GetBonePos(8);
	m_iTickCount = g_GlobalVars->tickcount;
	m_vecOrigin = player->m_vecOrigin();
	m_angAngles = player->m_angEyeAngles();

	int layerCount = player->GetNumAnimOverlays();
	for (int i = 0; i < layerCount; i++)
	{
		AnimationLayer *currentLayer = player->GetAnimOverlay(i);
		m_LayerRecords[i].m_nOrder = currentLayer->m_nOrder;
		m_LayerRecords[i].m_nSequence = currentLayer->m_nSequence;
		m_LayerRecords[i].m_flWeight = currentLayer->m_flWeight;
		m_LayerRecords[i].m_flCycle = currentLayer->m_flCycle;
	}
}

void CMBacktracking::CacheInfo(C_BasePlayer* player)
{
	int ent_index = player->EntIndex();
	float sim_time = player->m_flSimulationTime();
	LagRecord cur_lagrecord;
	auto& lag_records = this->m_LagRecord[ent_index];
	RemoveBadRecords(ent_index, lag_records);

	// Should probably extrapolate position aswell... If we had p choked ticks I would with 1 fcall.
	int priority_level = GetPriorityLevel(player, &cur_lagrecord);
	if (cur_lagrecord.m_flSimulationTime == sim_time && !priority_level)
	{
		return;
	}

	cur_lagrecord.m_iPriority = priority_level;
	cur_lagrecord.SaveRecord(player);

	lag_records.emplace_back(cur_lagrecord);
}

void CMBacktracking::ProcessCMD(int iTargetIdx, CUserCmd *usercmd)
{
	LagRecord recentLR = m_RestoreLagRecord[iTargetIdx].first;
	if (!IsTickValid(TIME_TO_TICKS(recentLR.m_flSimulationTime)))
	{
		usercmd->tick_count = TIME_TO_TICKS(C_BasePlayer::GetPlayerByIndex(iTargetIdx)->m_flSimulationTime() + GetLerpTime());
	}
	else
	{
		usercmd->tick_count = TIME_TO_TICKS(recentLR.m_flSimulationTime + GetLerpTime());
	}
}

void CMBacktracking::RemoveBadRecords(int Idx, std::deque<LagRecord>& records)
{
	auto& m_LagRecords = records; // Should use rbegin but can't erase
	for (auto lag_record = m_LagRecords.begin(); lag_record != m_LagRecords.end(); lag_record++)
	{
		if (!IsTickValid(TIME_TO_TICKS(lag_record->m_flSimulationTime)))
		{
			m_LagRecords.erase(lag_record);
			if (!m_LagRecords.empty())
				lag_record = m_LagRecords.begin();
			else break;
		}
	}
}

bool CMBacktracking::StartLagCompensation(C_BasePlayer *player)
{
	backtrack_records.clear();

	enum
	{
		// Only try to awall the "best" records, otherwise fail.
		TYPE_BEST_RECORDS,
		// Only try to awall the newest and the absolute best record.
		TYPE_BEST_AND_NEWEST,
		// Awall everything (fps killer)
		TYPE_ALL_RECORDS,
	};

	auto& m_LagRecords = this->m_LagRecord[player->EntIndex()];
	m_RestoreLagRecord[player->EntIndex()].second.SaveRecord(player);

	switch (g_Options.rage_lagcompensation_type)
	{
	case TYPE_BEST_RECORDS:
	{
		for (auto it : m_LagRecords)
		{
			if (it.m_iPriority >= 1)
				backtrack_records.emplace_back(it);
		}
		break;
	}
	case TYPE_BEST_AND_NEWEST:
	{
		LagRecord newest_record = LagRecord();
		for (auto it : m_LagRecords)
		{
			if (it.m_flSimulationTime > newest_record.m_flSimulationTime)
				newest_record = it;

			if (it.m_iPriority >= 1 /*&& !(it.m_nFlags & FL_ONGROUND) && it.m_vecVelocity.Length2D() > 150*/)
				backtrack_records.emplace_back(it);
		}
		backtrack_records.emplace_back(newest_record);
		break;
	}
	case TYPE_ALL_RECORDS:
		// Ouch, the fps drop will be H U G E.
		backtrack_records = m_LagRecords;
		break;
	}

	std::sort(backtrack_records.begin(), backtrack_records.end(), [](LagRecord const &a, LagRecord const &b) { return a.m_iPriority > b.m_iPriority; });
	return backtrack_records.size() > 0;
}

bool CMBacktracking::FindViableRecord(C_BasePlayer *player, LagRecord* record)
{
	auto& m_LagRecords = this->m_LagRecord[player->EntIndex()];

	// Ran out of records to check. Go back.
	if (backtrack_records.empty())
	{
		return false;
	}

	LagRecord
		recentLR = *backtrack_records.begin(),
		prevLR;

	// Should still use m_LagRecords because we're checking for LC break.
	auto iter = std::find(m_LagRecords.begin(), m_LagRecords.end(), recentLR);
	auto idx = std::distance(m_LagRecords.begin(), iter);
	if (0 != idx) prevLR = *std::prev(iter);

	// Saving first record for processcmd.
	m_RestoreLagRecord[player->EntIndex()].first = recentLR;

	if (!IsTickValid(TIME_TO_TICKS(recentLR.m_flSimulationTime)))
	{
		backtrack_records.pop_front();
		return backtrack_records.size() > 0; // RET_NO_RECORDS true false
	}

	// Remove a record...
	backtrack_records.pop_front();

	if ((0 != idx) && (recentLR.m_vecOrigin - prevLR.m_vecOrigin).LengthSqr() > 4096.f)
	{
		float simulationTimeDelta = recentLR.m_flSimulationTime - prevLR.m_flSimulationTime;

		int simulationTickDelta = clamp(TIME_TO_TICKS(simulationTimeDelta), 1, 15);

		for (; simulationTickDelta > 0; simulationTickDelta--)
			RebuildGameMovement::Get().FullWalkMove(player);

		// Bandage fix so we "restore" to the lagfixed player.
		m_RestoreLagRecord[player->EntIndex()].second.SaveRecord(player);
		*record = m_RestoreLagRecord[player->EntIndex()].second;

		// Clear so we don't try to bt shit we can't
		backtrack_records.clear();

		return true; // Return true so we still try to aimbot.
	}
	else
	{
		player->InvalidateBoneCache();

		player->GetCollideable()->OBBMins() = recentLR.m_vecMins;
		player->GetCollideable()->OBBMaxs() = recentLR.m_vecMax;
		player->m_flPoseParameter() = recentLR.m_arrflPoseParameters;

		int layerCount = player->GetNumAnimOverlays();
		for (int i = 0; i < layerCount; ++i)
		{
			AnimationLayer *currentLayer = player->GetAnimOverlay(i);
			currentLayer->m_nOrder = recentLR.m_LayerRecords[i].m_nOrder;
			currentLayer->m_nSequence = recentLR.m_LayerRecords[i].m_nSequence;
			currentLayer->m_flWeight = recentLR.m_LayerRecords[i].m_flWeight;
			currentLayer->m_flCycle = recentLR.m_LayerRecords[i].m_flCycle;
		}

		player->m_angRotation() = QAngle(0, 0, 0);
		player->m_angAbsRotation() = QAngle(0, 0, 0);
		player->SetPoseAngles(recentLR.m_angAngles.yaw, recentLR.m_angAngles.pitch);

		player->SetAbsAngles(QAngle(0, recentLR.m_angAngles.yaw, 0));
		player->SetAbsOrigin(recentLR.m_vecOrigin);

		//player->UpdateClientSideAnimation();

		*record = recentLR;
		return true;
	}
}

void CMBacktracking::FinishLagCompensation(C_BasePlayer *player)
{
	int idx = player->EntIndex();

	player->InvalidateBoneCache();

	player->GetCollideable()->OBBMins() = m_RestoreLagRecord[idx].second.m_vecMins;
	player->GetCollideable()->OBBMaxs() = m_RestoreLagRecord[idx].second.m_vecMax;
	player->m_flPoseParameter() = m_RestoreLagRecord[idx].second.m_arrflPoseParameters;

	int layerCount = player->GetNumAnimOverlays();
	for (int i = 0; i < layerCount; ++i)
	{
		AnimationLayer *currentLayer = player->GetAnimOverlay(i);
		currentLayer->m_nOrder = m_RestoreLagRecord[idx].second.m_LayerRecords[i].m_nOrder;
		currentLayer->m_nSequence = m_RestoreLagRecord[idx].second.m_LayerRecords[i].m_nSequence;
		currentLayer->m_flWeight = m_RestoreLagRecord[idx].second.m_LayerRecords[i].m_flWeight;
		currentLayer->m_flCycle = m_RestoreLagRecord[idx].second.m_LayerRecords[i].m_flCycle;
	}

	player->SetPoseAngles(m_RestoreLagRecord[idx].second.m_angAngles.yaw, m_RestoreLagRecord[idx].second.m_angAngles.pitch);

	player->SetAbsAngles(QAngle(0, m_RestoreLagRecord[idx].second.m_angAngles.yaw, 0));
	player->SetAbsOrigin(m_RestoreLagRecord[idx].second.m_vecOrigin);

	//player->UpdateClientSideAnimation();
}

int CMBacktracking::GetPriorityLevel(C_BasePlayer *player, LagRecord* lag_record)
{
	int priority = 0;

	if (lag_record->m_flPrevLowerBodyYaw != player->m_flLowerBodyYawTarget())
	{
		lag_record->m_angAngles.yaw = player->m_flLowerBodyYawTarget();
		priority = 3;
	}

	lag_record->m_flPrevLowerBodyYaw = player->m_flLowerBodyYawTarget();

	return priority;
}

void CMBacktracking::SetOverwriteTick(C_BasePlayer *player, QAngle angles, float_t correct_time, uint32_t priority)
{
	int idx = player->EntIndex();
	LagRecord overwrite_record;
	auto& m_LagRecords = this->m_LagRecord[player->EntIndex()];

	if (!IsTickValid(TIME_TO_TICKS(correct_time)))
		g_CVar->ConsoleColorPrintf(Color(255, 0, 0, 255), "Dev Error: failed to overwrite tick, delta too big. Priority: %d\n", priority);

	overwrite_record.SaveRecord(player);
	overwrite_record.m_angAngles = angles;
	overwrite_record.m_iPriority = priority;
	overwrite_record.m_flSimulationTime = correct_time;
	m_LagRecords.emplace_back(overwrite_record);
}

void CMBacktracking::RageBacktrack(C_BasePlayer* target, CUserCmd* usercmd, Vector &aim_point, bool &hitchanced)
{
	if (StartLagCompensation(target))
	{
		LagRecord cur_record;
		auto& m_LagRecords = this->m_LagRecord[target->EntIndex()];
		while (FindViableRecord(target, &cur_record))
		{
			auto iter = std::find(m_LagRecords.begin(), m_LagRecords.end(), cur_record);
			if (iter == m_LagRecords.end())
				continue;

			if (iter->m_bNoGoodSpots)
			{
				// Already awalled from same spot, don't try again like a dumbass.
				if (iter->m_vecLocalAimspot == g_LocalPlayer->GetEyePos())
					continue;
				else
					iter->m_bNoGoodSpots = false;
			}

			if (!iter->m_bMatrixBuilt)
			{
				if (!target->SetupBones(iter->matrix, 128, 256, g_EngineClient->GetLastTimeStamp()))
					continue;

				iter->m_bMatrixBuilt = true;
			}

			if (g_Options.rage_autobaim && Global::iShotsFired > g_Options.rage_baim_after_x_shots)
				aim_point = AimRage::Get().CalculateBestPoint(target, HITBOX_PELVIS, g_Options.rage_mindmg, g_Options.rage_prioritize, iter->matrix);
			else
				aim_point = AimRage::Get().CalculateBestPoint(target, realHitboxSpot[g_Options.rage_hitbox], g_Options.rage_mindmg, g_Options.rage_prioritize, iter->matrix);

			if (!aim_point.IsValid())
			{
				FinishLagCompensation(target);
				iter->m_bNoGoodSpots = true;
				iter->m_vecLocalAimspot = g_LocalPlayer->GetEyePos();
				continue;
			}

			QAngle aimAngle = Math::CalcAngle(g_LocalPlayer->GetEyePos(), aim_point) - (g_Options.rage_norecoil ? g_LocalPlayer->m_aimPunchAngle() * 2.f : QAngle(0,0,0));

			hitchanced = AimRage::Get().HitChance(aimAngle, target, g_Options.rage_hitchance_amount);
			break;
		}
		FinishLagCompensation(target);
		ProcessCMD(target->EntIndex(), usercmd);
	}
}

bool CMBacktracking::IsTickValid(int tick)
{
	// better use polak's version than our old one, getting more accurate results
	
	INetChannelInfo *nci = g_EngineClient->GetNetChannelInfo();

	if (!nci)
		return false;

	float correct = clamp(nci->GetLatency(FLOW_OUTGOING) + GetLerpTime(), 0.f, 1.f/*sv_maxunlag*/);

	float deltaTime = correct - (g_GlobalVars->curtime - TICKS_TO_TIME(tick));

	return fabsf(deltaTime) < 0.2f;
}

void CMBacktracking::AnimationFix(ClientFrameStage_t stage)
{
	if (!g_LocalPlayer->IsAlive())
		return;

	static int userId[64];
	static AnimationLayer
		backupLayersUpdate[64][15],
		backupLayersInterp[64][15];

	for (int i = 1; i < g_EngineClient->GetMaxClients(); i++)
	{
		C_BasePlayer *player = C_BasePlayer::GetPlayerByIndex(i);

		if (!player ||
			player == g_LocalPlayer ||
			player->m_iTeamNum() == g_LocalPlayer->m_iTeamNum() ||
			!player->IsPlayer() ||
			player->IsDormant())
			continue;

		player_info_t player_info;
		g_EngineClient->GetPlayerInfo(i, &player_info);

		switch (stage)
		{
		case ClientFrameStage_t::FRAME_NET_UPDATE_START: // Copy new, server layers to use when drawing.
			userId[i] = player_info.userId;
			memcpy(&backupLayersUpdate[i], player->GetAnimOverlays(), (sizeof AnimationLayer) * player->GetNumAnimOverlays());
			break;
		case ClientFrameStage_t::FRAME_RENDER_START: // Render started, don't use inaccurately extrapolated layers but save them to not mess shit up either.
			if (userId[i] != player_info.userId) continue;
			memcpy(&backupLayersInterp[i], player->GetAnimOverlays(), (sizeof AnimationLayer) * player->GetNumAnimOverlays());
			memcpy(player->GetAnimOverlays(), &backupLayersUpdate[i], (sizeof AnimationLayer) * player->GetNumAnimOverlays());
			break;
		case ClientFrameStage_t::FRAME_RENDER_END: // Restore layers to keep being accurate when backtracking.
			if (userId[i] != player_info.userId) continue;
			memcpy(player->GetAnimOverlays(), &backupLayersInterp[i], (sizeof AnimationLayer) * player->GetNumAnimOverlays());
			break;
		default:
			return;
		}
	}
}

float CMBacktracking::GetLerpTime()
{
	int ud_rate = g_CVar->FindVar("cl_updaterate")->GetInt();
	ConVar *min_ud_rate = g_CVar->FindVar("sv_minupdaterate");
	ConVar *max_ud_rate = g_CVar->FindVar("sv_maxupdaterate");

	if (min_ud_rate && max_ud_rate)
		ud_rate = max_ud_rate->GetInt();

	float ratio = g_CVar->FindVar("cl_interp_ratio")->GetFloat();

	if (ratio == 0)
		ratio = 1.0f;

	float lerp = g_CVar->FindVar("cl_interp")->GetFloat();
	ConVar *c_min_ratio = g_CVar->FindVar("sv_client_min_interp_ratio");
	ConVar *c_max_ratio = g_CVar->FindVar("sv_client_max_interp_ratio");

	if (c_min_ratio && c_max_ratio && c_min_ratio->GetFloat() != 1)
		ratio = clamp(ratio, c_min_ratio->GetFloat(), c_max_ratio->GetFloat());

	return max(lerp, (ratio / ud_rate));
}

template<class T, class U>
T CMBacktracking::clamp(T in, U low, U high)
{
	if (in <= low)
		return low;

	if (in >= high)
		return high;

	return in;
}