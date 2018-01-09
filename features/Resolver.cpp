#include "Resolver.hpp"

#include "../Options.hpp"
#include "../helpers/Math.hpp"

#include "LagCompensation.hpp"

void Resolver::Resolve(C_BasePlayer *player)
{
	int idx = player->EntIndex();
	arr_infos[idx].curTickRecord.SaveRecord(player);
	//if (arr_infos[idx].curTickRecord.m_flSimulationTime != player->m_flSimulationTime()) { // we gonna don't have lby records in these cases.. needs to be rewritten
	if (arr_infos[idx].arr_tickRecords.size() > 16)
		arr_infos[idx].arr_tickRecords.pop_back();
	arr_infos[idx].arr_tickRecords.emplace_back(arr_infos[idx].curTickRecord);
	//}

	StartResolving(player);

	arr_infos[idx].prevTickRecord = arr_infos[idx].curTickRecord;
}

void Resolver::StartResolving(C_BasePlayer *player)
{
	int idx = player->EntIndex();
	float new_yaw = player->m_flLowerBodyYawTarget();

	AnimationLayer curBalanceLayer, prevBalanceLayer;

	SResolveInfo &player_recs = arr_infos[idx];

	if (player_recs.prevTickRecord.m_flVelocity == 0.0f && player_recs.curTickRecord.m_flVelocity != 0.0f)
		Math::VectorAngles(player_recs.curTickRecord.m_vecVelocity, player_recs.m_angDirectionFirstMoving);

	if (g_Options.debug_fliponkey)
	{
		new_yaw = player->m_flLowerBodyYawTarget();
		if (g_InputSystem->IsButtonDown(g_Options.debug_flipkey))
			new_yaw += 180.f;
		new_yaw = Math::ClampYaw(new_yaw);
		player->m_angEyeAngles().yaw = new_yaw;
		player->SetPoseAngles(new_yaw, player->m_angEyeAngles().pitch);
		player->SetAbsAngles(QAngle(0, new_yaw, 0));
		player->m_angRotation() = QAngle(0, 0, 0);
		player->m_angAbsRotation() = QAngle(0, 0, 0);
		return;
	}

	if (IsEntityMoving(player))
	{
		if ((player->m_vecVelocity().Length2D() > 6.0f && player->m_vecVelocity().Length2D() < 45.f) && !(player->m_fFlags() & FL_DUCKING)) // should be fakewalking; some guys fakewalk with speed of 75 => needs a more proper check
			new_yaw = player_recs.m_angDirectionFirstMoving.yaw + 180.f; // should be enough for now x..x, gonna do it proper later
		else
			new_yaw = player->m_flLowerBodyYawTarget();
	}
	else if (IsAdjustingBalance(player, player_recs.curTickRecord, &curBalanceLayer))
	{
		// should be fakeheading / failed lby break
		if (fabsf(LBYDelta(player_recs.curTickRecord)) > 35.f)
			new_yaw = player->m_angEyeAngles().yaw - player->m_flLowerBodyYawTarget();

		if (IsAdjustingBalance(player, player_recs.prevTickRecord, &prevBalanceLayer))
		{
			if ((prevBalanceLayer.m_flCycle != curBalanceLayer.m_flCycle) || curBalanceLayer.m_flWeight == 1.f)
			{
				float
					flAnimTime = curBalanceLayer.m_flCycle,
					flSimTime = player->m_flSimulationTime();

				if (flAnimTime < 0.01f && prevBalanceLayer.m_flCycle > 0.01f && g_Options.rage_lagcompensation && CMBacktracking::Get().IsTickValid(TIME_TO_TICKS(flSimTime - flAnimTime)))
				{
					CMBacktracking::Get().SetOverwriteTick(player, QAngle(player->m_angEyeAngles().pitch, player->m_flLowerBodyYawTarget(), 0), (flSimTime - flAnimTime), 2);
				}

				new_yaw = player->m_flLowerBodyYawTarget() - 180.f;
			}
			else if (curBalanceLayer.m_flWeight == 0.f && (prevBalanceLayer.m_flCycle > 0.92f && curBalanceLayer.m_flCycle > 0.92f)) // breaking lby with delta < 120/supressing
			{
				// Will imrpove it later, so we don't bruteforce a whole bunch of AA's.
				//
				// TODO:
				// 
				//	-) Even though we know that he breaks LBY here with a delta of under 120 / supressing the act, bruteforcing won't do much in
				//		a few(most) cases, because we won't cover a wide spectrum of AA's. (breaking lby but stays static,
				//		or lby breaking and jitterish behaviour, ..)
				//	-) To lower the spectrum, we have to detect a few lby breaker patterns like: linear fake angles, average lby delta, 
				//		fake spins, inverse etc..
				new_yaw = player->m_angEyeAngles().yaw + Bruteforce(player);
			}
		}
		else
			new_yaw = player->m_flLowerBodyYawTarget();
	}
	else
	{
		// If act 979 is not triggered or delayed atleast, we will have the act 980.
		// While inside of act 980, ppl can break lby, execute AA's etc. ==>
		// TODO is same as above, only if we don't find a breaking lby pattern / can't predict, we gonna bruteforce.
		// Will improve it later.
		new_yaw = player->m_angEyeAngles().yaw + Bruteforce(player);
	}

	new_yaw = Math::ClampYaw(new_yaw);
	player->m_angEyeAngles().yaw = new_yaw;
	player->SetPoseAngles(new_yaw, player->m_angEyeAngles().pitch);
	player->SetAbsAngles(QAngle(0, new_yaw, 0));
	player->m_angRotation() = QAngle(0, 0, 0);
	player->m_angAbsRotation() = QAngle(0, 0, 0);
}

STickRecord Resolver::GetLatestUpdateRecord(C_BasePlayer *player) {
	if (arr_infos[player->EntIndex()].arr_tickRecords.size())
		return *arr_infos[player->EntIndex()].arr_tickRecords.begin();
	return STickRecord();
}

float_t Resolver::Bruteforce(C_BasePlayer *player)
{
	auto getdst = [](int a, int b)
	{
		int x = abs(b - a) % 360;
		return x > 180 ? 360 - x : x;
	};

	auto record = arr_infos[player->EntIndex()];

	int avg = 1;
	int count = 1;

	float prevlby = 0.f;

	for (auto &r : record.arr_tickRecords)
	{
		if (r.m_flLowerBodyYawTarget == prevlby)
			continue;

		prevlby = r.m_flLowerBodyYawTarget;

		avg += getdst(static_cast<int>(r.m_angEyeAngles.yaw), static_cast<int>(r.m_flLowerBodyYawTarget));

		count++;
	}

	avg /= count;

	float add = 0;

	if (avg > 145)
		add = 180;
	else if (avg > 110)
		add = 135;
	else if (avg > 75)
		add = 90;
	else if (avg > 40)
		add = 45;

	return (Global::userCMD->command_number % 2) ? add : -add;
}

bool Resolver::IsEntityMoving(C_BasePlayer *player)
{
	return (player->m_vecVelocity().Length2D() > 0.1f && player->m_fFlags() & FL_ONGROUND);
} 

bool Resolver::IsAdjustingBalance(C_BasePlayer *player, STickRecord &record, AnimationLayer *layer)
{
	for (int i = 0; i < record.m_iLayerCount; i++)
	{
		const int activity = player->GetSequenceActivity(record.animationLayer[i].m_nSequence);
		if (activity == 979)
		{
			*layer = record.animationLayer[i];
			return true;
		}
	}
	return false;
}