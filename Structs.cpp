#include "Structs.hpp"
#include "helpers/Math.hpp"
#include "helpers/Utils.hpp"

bool C_BaseEntity::IsPlayer()
{
	//index: 152
	//ref: "effects/nightvision"
	//sig: 8B 92 ? ? ? ? FF D2 84 C0 0F 45 F7 85 F6
	return VT::vfunc<bool(__thiscall*)(C_BaseEntity*)>(this, 152)(this);
}

bool C_BaseEntity::IsWeapon()
{
	//index: 160
	//ref: "CNewParticleEffect::DrawModel"
	//sig: 8B 80 ? ? ? ? FF D0 84 C0 74 6F 8B 4D A4
	return VT::vfunc<bool(__thiscall*)(C_BaseEntity*)>(this, 160)(this);
}

bool C_BaseEntity::IsPlantedC4()
{
	return GetClientClass()->m_ClassID == ClassId_CPlantedC4;
}

bool C_BaseEntity::IsDefuseKit()
{
	return GetClientClass()->m_ClassID == ClassId_CBaseAnimating;
}

unsigned int C_BaseEntity::PhysicsSolidMaskForEntity()
{
	return VT::vfunc<unsigned int(__thiscall*)(C_BaseEntity*)>(this, 148)(this);
}

WeapInfo_t *C_BaseCombatWeapon::GetWeapInfo()
{
	if (!this || this == NULL)
		return NULL;

	typedef WeapInfo_t *(__thiscall *o_getWeapInfo)(void*);
	return VT::vfunc<o_getWeapInfo>(this, 446)(this); // 456
}

bool C_BaseCombatWeapon::HasBullets()
{
	return !IsReloading() && m_iClip1() > 0;
}

bool C_BaseCombatWeapon::CanFire()
{
	if (IsReloading() || m_iClip1() <= 0)
		return false;

	if (!g_LocalPlayer)
		return false;

	// Bad time. Should use predicted, as we don't use this anywhere but ragebot, move this to there.
	float flServerTime = g_LocalPlayer->m_nTickBase() * g_GlobalVars->interval_per_tick;

	return m_flNextPrimaryAttack() < flServerTime;
}

bool C_BaseCombatWeapon::IsReloading()
{
	static auto inReload = *(uint32_t*)(Utils::PatternScan(GetModuleHandle("client.dll"), "C6 87 ? ? ? ? ? 8B 06 8B CE FF 90") + 2);
	return *(bool*)((uintptr_t)this + inReload);
}

bool C_BaseCombatWeapon::IsRifle()
{
	switch (GetWeapInfo()->weapon_type)
	{
	case WEAPONTYPE_RIFLE:
		return true;
	case WEAPONTYPE_SUBMACHINEGUN:
		return true;
	case WEAPONTYPE_SHOTGUN:
		return true;
	case WEAPONTYPE_MACHINEGUN:
		return true;
	default:
		return false;
	}
}

bool C_BaseCombatWeapon::IsPistol()
{
	switch (GetWeapInfo()->weapon_type)
	{
	case WEAPONTYPE_PISTOL:
		return true;
	default:
		return false;
	}
}

bool C_BaseCombatWeapon::IsSniper()
{
	switch (GetWeapInfo()->weapon_type)
	{
	case WEAPONTYPE_SNIPER_RIFLE:
		return true;
	default:
		return false;
	}
}

bool C_BaseCombatWeapon::IsGrenade()
{
	switch (GetWeapInfo()->weapon_type)
	{
	case WEAPONTYPE_GRENADE:
		return true;
	default:
		return false;
	}
}

float C_BaseCombatWeapon::GetInaccuracy()
{
	return VT::vfunc<float(__thiscall*)(void*)>(this, 469)(this); // 483
}

float C_BaseCombatWeapon::GetSpread()
{
	return VT::vfunc<float(__thiscall*)(void*)>(this, 439)(this); // 484
}

void C_BaseCombatWeapon::UpdateAccuracyPenalty()
{
	VT::vfunc<void(__thiscall*)(void*)>(this, 470)(this); // 485
}

bool C_BaseCombatWeapon::IsWeaponNonAim()
{
	int idx = m_iItemDefinitionIndex();

	return (idx == WEAPON_C4 || idx == WEAPON_KNIFE || idx == WEAPON_KNIFE_BAYONET || idx == WEAPON_KNIFE_BUTTERFLY || idx == WEAPON_KNIFE_FALCHION
		|| idx == WEAPON_KNIFE_FLIP || idx == WEAPON_KNIFE_GUT || idx == WEAPON_KNIFE_KARAMBIT || idx == WEAPON_KNIFE_M9_BAYONET || idx == WEAPON_KNIFE_PUSH
		|| idx == WEAPON_KNIFE_SURVIVAL_BOWIE || idx == WEAPON_KNIFE_T || idx == WEAPON_KNIFE_TACTICAL || idx == WEAPON_FLASHBANG || idx == WEAPON_HEGRENADE
		|| idx == WEAPON_SMOKEGRENADE || idx == WEAPON_MOLOTOV || idx == WEAPON_DECOY || idx == WEAPON_INCGRENADE);
}

bool C_BaseCombatWeapon::CanFirePostPone()
{
	float rdyTime = m_flPostponeFireReadyTime();

	if (rdyTime > 0 && rdyTime < g_GlobalVars->curtime)
		return true;

	return false;
}

char* C_BaseCombatWeapon::GetWeaponIcon()
{
	int id = this->m_iItemDefinitionIndex();
	switch (id)
	{
	case WEAPON_DEAGLE:
		return "F";
	case WEAPON_ELITE:
		return "S";
	case WEAPON_FIVESEVEN:
		return "U";
	case WEAPON_GLOCK:
		return "C";
	case WEAPON_AK47:
		return "B";
	case WEAPON_AUG:
		return "E";
	case WEAPON_AWP:
		return "R";
	case WEAPON_FAMAS:
		return "T";
	case WEAPON_G3SG1:
		return "I";
	case WEAPON_GALILAR:
		return "V";
	case WEAPON_M249:
		return "Z";
	case WEAPON_M4A1:
	case WEAPON_M4A1_SILENCER:
		return "W";
	case WEAPON_MAC10:
		return "L";
	case WEAPON_P90:
		return "M";
	case WEAPON_UMP45:
		return "Q";
	case WEAPON_XM1014:
		return "]";
	case WEAPON_BIZON:
		return "D";
	case WEAPON_MAG7:
		return "K";
	case WEAPON_NEGEV:
		return "Z";
	case WEAPON_SAWEDOFF:
		return "K";
	case WEAPON_TEC9:
		return "C";
	case WEAPON_MP7:
		return "X";
	case WEAPON_MP9:
		return "D";
	case WEAPON_NOVA:
		return "K";
	case WEAPON_P250:
	case WEAPON_USP_SILENCER:
	case WEAPON_TASER: //smh, add hints OR just show text, why not
	case WEAPON_HKP2000:
	case WEAPON_CZ75A:
		return "Y";
	case WEAPON_SCAR20:
		return "I";
	case WEAPON_SG553:
		return "[";
	case WEAPON_SSG08:
		return "N";
	case WEAPON_KNIFE:
	case WEAPON_KNIFE_T:
	case WEAPON_KNIFE_BAYONET:
	case WEAPON_KNIFE_BUTTERFLY:
	case WEAPON_KNIFE_FALCHION:
	case WEAPON_KNIFE_SURVIVAL_BOWIE:
	case WEAPON_KNIFE_FLIP:
	case WEAPON_KNIFE_GUT:
	case WEAPON_KNIFE_KARAMBIT:
	case WEAPON_KNIFE_M9_BAYONET:
	case WEAPON_KNIFE_PUSH:
	case WEAPON_KNIFE_TACTICAL:
		return "J";
	case WEAPON_FLASHBANG:
	case WEAPON_DECOY:
		return "G";
	case WEAPON_HEGRENADE:
	case WEAPON_MOLOTOV:
	case WEAPON_INCGRENADE:
		return "H";
	case WEAPON_SMOKEGRENADE:
		return "P";
	case WEAPON_C4:
		return "\\";
	case WEAPON_REVOLVER:
		return "F";
	default:
		return "";
	}
}

bool C_BaseCombatWeapon::IsInThrow()
{
	if (!m_bPinPulled() || (Global::userCMD->buttons & IN_ATTACK) || (Global::userCMD->buttons & IN_ATTACK2))
	{
		float throwTime = m_fThrowTime();

		if (throwTime > 0)
			return true;
	}
	return false;
}

float_t C_BasePlayer::m_flSpawnTime()
{
	return *(float_t*)((uintptr_t)this + 0xA290);
}

std::array<float, 24> &C_BasePlayer::m_flPoseParameter()
{
	static int _m_flPoseParameter = NetMngr::Get().getOffs("CBaseAnimating", "m_flPoseParameter");
	return *(std::array<float, 24>*)((uintptr_t)this + _m_flPoseParameter);
}

QAngle &C_BasePlayer::visuals_Angles()
{
	return *(QAngle*)((uintptr_t)this + NetMngr::Get().getOffs("CCSPlayer", "deadflag") + 4);
}

int32_t C_BasePlayer::GetMoveType()
{
	return *(int32_t*)((uintptr_t)this + 0x258);
}

void C_BasePlayer::SetPoseAngles(float_t yaw, float_t pitch)
{
	auto &poses = m_flPoseParameter();
	poses[11] = (pitch + 90) / 180;
	poses[2] = (yaw + 180) / 360;
}

void C_BasePlayer::InvalidateBoneCache()
{
	unsigned long g_iModelBoneCounter = **(unsigned long**)(Offsets::invalidateBoneCache + 10);
	*(unsigned int*)((DWORD)this + 0x2914) = 0xFF7FFFFF; // m_flLastBoneSetupTime = -FLT_MAX;
	*(unsigned int*)((DWORD)this + 0x2680) = (g_iModelBoneCounter - 1); // m_iMostRecentModelBoneCounter = g_iModelBoneCounter - 1;
}

int C_BasePlayer::GetNumAnimOverlays()
{
	return *(int*)((DWORD)this + 0x297C);
}

AnimationLayer *C_BasePlayer::GetAnimOverlays()
{
	// to find offset: use 9/12/17 dll
	// sig: 55 8B EC 51 53 8B 5D 08 33 C0
	return *(AnimationLayer**)((DWORD)this + 10608);
}

AnimationLayer *C_BasePlayer::GetAnimOverlay(int i)
{
	if (i < 15)
		return &GetAnimOverlays()[i];
}

int C_BasePlayer::GetSequenceActivity(int sequence)
{
	auto hdr = g_MdlInfo->GetStudiomodel(this->GetModel());

	if (!hdr)
		return -1;

	// sig for stuidohdr_t version: 53 56 8B F1 8B DA 85 F6 74 55
	// sig for C_BaseAnimating version: 55 8B EC 83 7D 08 FF 56 8B F1 74 3D
	// c_csplayer vfunc 242, follow calls to find the function.

	static auto get_sequence_activity = reinterpret_cast<int(__fastcall*)(void*, studiohdr_t*, int)>(Offsets::getSequenceActivity);

	return get_sequence_activity(this, hdr, sequence);
}

CBasePlayerAnimState *C_BasePlayer::GetBasePlayerAnimState()
{
	static int basePlayerAnimStateOffset = 0x3874;
	return *(CBasePlayerAnimState**)((DWORD)this + basePlayerAnimStateOffset);
}

CCSPlayerAnimState *C_BasePlayer::GetPlayerAnimState()
{
	return *(CCSPlayerAnimState**)((DWORD)this + 0x3870);
}

void C_BasePlayer::UpdateAnimationState(CCSGOPlayerAnimState *state, QAngle angle)
{
	static auto UpdateAnimState = Utils::PatternScan(GetModuleHandle("client.dll"), "55 8B EC 83 E4 F8 83 EC 18 56 57 8B F9 F3 0F 11 54 24");
	if (!UpdateAnimState)
		return;

	__asm
	{
		mov ecx, state

		movss xmm1, dword ptr[angle + 4]
		movss xmm2, dword ptr[angle]

		call UpdateAnimState
	}
}

void C_BasePlayer::ResetAnimationState(CCSGOPlayerAnimState *state)
{
	using ResetAnimState_t = void(__thiscall*)(CCSGOPlayerAnimState*);
	static auto ResetAnimState = (ResetAnimState_t)Utils::PatternScan(GetModuleHandle("client.dll"), "56 6A 01 68 ? ? ? ? 8B F1");
	if (!ResetAnimState)
		return;

	ResetAnimState(state);
}

void C_BasePlayer::CreateAnimationState(CCSGOPlayerAnimState *state)
{
	using CreateAnimState_t = void(__thiscall*)(CCSGOPlayerAnimState*, C_BasePlayer*);
	static auto CreateAnimState = (CreateAnimState_t)Utils::PatternScan(GetModuleHandle("client.dll"), "55 8B EC 56 8B F1 B9 ? ? ? ? C7 46");
	if (!CreateAnimState)
		return;

	CreateAnimState(state, this);
}

Vector C_BasePlayer::GetEyePos()
{
	return m_vecOrigin() + m_vecViewOffset();
}

player_info_t C_BasePlayer::GetPlayerInfo()
{
	player_info_t info;
	g_EngineClient->GetPlayerInfo(EntIndex(), &info);
	return info;
}

std::string C_BasePlayer::GetName(bool console_safe)
{
	// Cleans player's name so we don't get new line memes. Use this everywhere you get the players name.
	// Also, if you're going to use the console for its command and use the players name, set console_safe.
	player_info_t pinfo = this->GetPlayerInfo();

	char* pl_name = pinfo.szName;
	char buf[128];
	int c = 0;

	for (int i = 0; pl_name[i]; ++i)
	{
		if (c >= sizeof(buf) - 1)
			break;

		switch (pl_name[i])
		{
		case '"': if (console_safe) break;
		case '\\':
		case ';': if (console_safe) break;
		case '\n':
			break;
		default:
			buf[c++] = pl_name[i];
		}
	}

	buf[c] = '\0';
	return std::string(buf);
}

bool C_BasePlayer::IsAlive()
{
	return m_lifeState() == LIFE_ALIVE;
}

bool C_BasePlayer::HasC4()
{
	static auto fnHasC4
		= reinterpret_cast<bool(__thiscall*)(void*)>(
			Utils::PatternScan(GetModuleHandle("client.dll"), "56 8B F1 85 F6 74 31")
			);

	return fnHasC4(this);
}

Vector C_BasePlayer::GetBonePos(int bone)
{
	matrix3x4_t boneMatrix[MAXSTUDIOBONES];

	if (SetupBones(boneMatrix, MAXSTUDIOBONES, BONE_USED_BY_HITBOX, g_EngineClient->GetLastTimeStamp())) {
		return Vector(boneMatrix[bone][0][3], boneMatrix[bone][1][3], boneMatrix[bone][2][3]);
	}
	return Vector(0, 0, 0);
}

int C_BasePlayer::GetBoneByName(const char *boneName)
{
	studiohdr_t *studioHdr = g_MdlInfo->GetStudiomodel(this->GetModel());
	if (!studioHdr)
		return -1;

	matrix3x4_t boneToWorldOut[MAXSTUDIOBONES];
	if (!this->SetupBones(boneToWorldOut, MAXSTUDIOBONES, BONE_USED_BY_HITBOX, 0))
		return -1;

	for (int i = 0; i < studioHdr->numbones; i++)
	{
		mstudiobone_t *studioKost = studioHdr->pBone(i);

		if (!studioKost)
			continue;

		if (studioKost->pszName() && strcmp(studioKost->pszName(), boneName) == 0)
			return i;
	}

	return -1;
}

bool C_BasePlayer::IsLocalInTarget(C_BasePlayer *player)
{
	if (!player)
		return false;

	Vector src, rem, forward;
	trace_t tr;
	Ray_t ray;
	CTraceFilter filter;
	filter.pSkip = player;

	QAngle viewAngles = m_angEyeAngles();
	Math::AngleVectors(viewAngles, forward);
	forward *= 8142.f;

	src = GetEyePos();
	rem = src + forward;

	ray.Init(src, rem);
	g_EngineTrace->TraceRay(ray, MASK_SHOT | CONTENTS_GRATE, &filter, &tr);

	if (tr.hit_entity == g_LocalPlayer)
		return true;
	return false;
}

VarMapping_t *C_BasePlayer::VarMapping()
{
	return reinterpret_cast<VarMapping_t*>((DWORD)this + 0x24);
}

void C_BasePlayer::SetAbsOrigin(const Vector &origin)
{
	using SetAbsOriginFn = void(__thiscall*)(void*, const Vector &origin);
	static SetAbsOriginFn SetAbsOrigin = (SetAbsOriginFn)Utils::PatternScan(GetModuleHandle("client.dll"), "55 8B EC 83 E4 F8 51 53 56 57 8B F1 E8");
	SetAbsOrigin(this, origin);
}

void C_BasePlayer::SetAbsAngles(const QAngle &angles)
{
	using SetAbsAnglesFn = void(__thiscall*)(void*, const QAngle &angles);
	static SetAbsAnglesFn SetAbsAngles = (SetAbsAnglesFn)Utils::PatternScan(GetModuleHandle("client.dll"), "55 8B EC 83 E4 F8 83 EC 64 53 56 57 8B F1 E8");
	SetAbsAngles(this, angles);
}

void C_BasePlayer::UpdateClientSideAnimation()
{
	typedef void(__thiscall *o_updateClientSideAnimation)(void*);
	VT::vfunc<o_updateClientSideAnimation>(this, 218)(this);
}

int C_BasePlayer::GetPing()
{
	// Afaik won't work since original playerresource sig included the 2 vtables so should be entindex * 4 + 8 if that's the case.
	// if ever used and gives wrong results that might be it :)
	return *(int*)((DWORD)Offsets::playerResource + NetMngr::Get().getOffs("CCSPlayerResource", "m_iPing") + (int)this->EntIndex() * 4);
}