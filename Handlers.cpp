#include "Gamehooking.hpp"
#include "helpers/Utils.hpp"

#include "Menu.hpp"
#include "Options.hpp"

#include "helpers/Math.hpp"

#include "features/Visuals.hpp"
#include "features/Glow.hpp"
#include "features/Miscellaneous.hpp"
#include "features/PredictionSystem.hpp"
#include "features/AimRage.hpp"
#include "features/AimLegit.h"
#include "features/LagCompensation.hpp"
#include "features/Resolver.hpp"
#include "features/AntiAim.hpp"
#include "features/HitMarker.hpp"
#include "features/BulletImpact.hpp"
#include "features/GrenadePrediction.h"
#include "features/ServerSounds.hpp"
#include <intrin.h>

extern LRESULT ImGui_ImplDX9_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Global
{
	float smt = 0.f;
	QAngle visualAngles = QAngle(0.f, 0.f, 0.f);
	bool bSendPacket = false;
	bool bAimbotting = false;
	bool bVisualAimbotting = false;
	QAngle vecVisualAimbotAngs = QAngle(0.f, 0.f, 0.f);
	CUserCmd *userCMD = nullptr;
	int nChockedTicks = 0;
	int iShotsFired = 0;

	char *szLastFunction = "<No function was called>";
	HMODULE hmDll = nullptr;

	bool bShouldChoke = false;
	bool bFakewalking = false;
	Vector vecUnpredictedVel = Vector(0, 0, 0);
}

void debug_visuals()
{
	int x_offset = 0;
	if (g_Options.debug_showposes)
	{
		RECT bbox = Visuals::ESP_ctx.bbox;
		auto poses = Visuals::ESP_ctx.player->m_flPoseParameter();
		for (int i = 0; i < 24; ++i)
			Visuals::DrawString(Visuals::ui_font, bbox.right + 5, bbox.top + i * 12, Color(10 * i, 255, 255, 255), FONT_LEFT, "Pose %d %f", i, poses[i]);
		Visuals::DrawString(Visuals::ui_font, bbox.right + 5, bbox.top + - 12, Color(255, 0, 55, 255), FONT_LEFT, "LBY %f", (Visuals::ESP_ctx.player->m_flLowerBodyYawTarget() + 180.f) / 360.f);
		x_offset += 50;
	}

	if (g_Options.debug_showactivities)
	{
		float h = fabs(Visuals::ESP_ctx.feet_pos.y - Visuals::ESP_ctx.head_pos.y);
		float w = h / 2.0f;

		int offsety = 0;
		for (int i = 0; i < Visuals::ESP_ctx.player->GetNumAnimOverlays(); i++)
		{
			auto layer = Visuals::ESP_ctx.player->GetAnimOverlays()[i];
			int number = layer.m_nSequence,
				activity = Visuals::ESP_ctx.player->GetSequenceActivity(number);
			Visuals::DrawString(Visuals::ui_font, Visuals::ESP_ctx.head_pos.x + w + x_offset + 4, Visuals::ESP_ctx.head_pos.y + offsety, Color(255, 255, 0, 255), FONT_LEFT, std::to_string(number).c_str());
			Visuals::DrawString(Visuals::ui_font, Visuals::ESP_ctx.head_pos.x + w + x_offset + 40, Visuals::ESP_ctx.head_pos.y + offsety, Color(255, 255, 0, 255), FONT_LEFT, std::to_string(activity).c_str());
			Visuals::DrawString(Visuals::ui_font, Visuals::ESP_ctx.head_pos.x + w + x_offset + 60, Visuals::ESP_ctx.head_pos.y + offsety, Color(255, 255, 0, 255), FONT_LEFT, std::to_string(layer.m_flCycle).c_str());
			Visuals::DrawString(Visuals::ui_font, Visuals::ESP_ctx.head_pos.x + w + x_offset + 104, Visuals::ESP_ctx.head_pos.y + offsety, Color(255, 255, 0, 255), FONT_LEFT, std::to_string(layer.m_flWeight).c_str());

			/*if (activity == 979)
			{
				Visuals::DrawString(Visuals::ui_font, Visuals::ESP_ctx.head_pos.x + w + x_offset + 65, Visuals::ESP_ctx.head_pos.y + offsety, Color(255, 255, 0, 255), FONT_LEFT, std::to_string(layer.m_flWeight).c_str());
				Visuals::DrawString(Visuals::ui_font, Visuals::ESP_ctx.head_pos.x + w + x_offset + 65, Visuals::ESP_ctx.head_pos.y + offsety + 12, Color(255, 255, 0, 255), FONT_LEFT, std::to_string(layer.m_flCycle).c_str());
			}*/

			offsety += 12;
		}
		x_offset += 100;
	}

	if (g_Options.debug_headbox)
	{
		Vector headpos = Visuals::ESP_ctx.player->GetBonePos(8);
		Visuals::Draw3DCube(7.f, Visuals::ESP_ctx.player->m_angEyeAngles(), headpos, Color(40, 40, 40, 160));
	}
}

void __fastcall Handlers::PaintTraverse_h(void *thisptr, void*, unsigned int vguiPanel, bool forceRepaint, bool allowForce)
{
	static uint32_t HudZoomPanel;
	if (!HudZoomPanel)
		if (!strcmp("HudZoom", g_VGuiPanel->GetName(vguiPanel)))
			HudZoomPanel = vguiPanel;

	if (HudZoomPanel == vguiPanel && g_Options.removals_scope && g_LocalPlayer && g_LocalPlayer->m_hActiveWeapon().Get())
	{
		if (g_LocalPlayer->m_hActiveWeapon().Get()->IsSniper() && g_LocalPlayer->m_bIsScoped())
			return;
	}

	o_PaintTraverse(thisptr, vguiPanel, forceRepaint, allowForce);

	static uint32_t FocusOverlayPanel;
	if (!FocusOverlayPanel)
	{
		const char* szName = g_VGuiPanel->GetName(vguiPanel);

		if (lstrcmpA(szName, "FocusOverlayPanel") == 0)
		{
			FocusOverlayPanel = vguiPanel;

			Visuals::InitFont();

			g_EngineClient->ExecuteClientCmd("clear");
			g_CVar->ConsoleColorPrintf(Color(0, 153, 204, 255), "Gladiatorcheatz v2 - rev.20171024\n");
			g_EngineClient->ExecuteClientCmd("version");
			g_EngineClient->ExecuteClientCmd("toggleconsole");
		}
	}

	if (FocusOverlayPanel == vguiPanel)
	{
		if (g_EngineClient->IsInGame() && g_EngineClient->IsConnected() && g_LocalPlayer)
		{
			ServerSound::Get().Start();
			for (int i = 1; i <= g_EntityList->GetHighestEntityIndex(); i++)
			{
				C_BasePlayer *entity = C_BasePlayer::GetPlayerByIndex(i);

				if (!entity)
					continue;

				if (i < 65 && Visuals::ValidPlayer(entity))
				{
					if (Visuals::Begin(entity))
					{
						Visuals::RenderFill();
						Visuals::RenderBox();

						if (g_Options.esp_player_snaplines) Visuals::RenderSnapline();
						if (g_Options.esp_player_weapons) Visuals::RenderWeapon();
						if (g_Options.esp_player_name) Visuals::RenderName();
						if (g_Options.esp_player_health) Visuals::RenderHealth();
						if (g_Options.esp_player_skelet) Visuals::RenderSkelet();
						if (g_Options.esp_backtracked_player_skelet) Visuals::RenderBacktrackedSkelet();
						if (g_Options.esp_player_anglelines) Visuals::DrawAngleLines();

						debug_visuals();						
					}
				}
				else if (g_Options.esp_dropped_weapons && entity->IsWeapon())
					Visuals::RenderWeapon((C_BaseCombatWeapon*)entity);
				else if (entity->IsPlantedC4())
					if (g_Options.esp_planted_c4)
						Visuals::RenderPlantedC4(entity);

				Visuals::RenderNadeEsp((C_BaseCombatWeapon*)entity);
			}
			ServerSound::Get().Finish();

			if (g_Options.removals_scope && (g_LocalPlayer && g_LocalPlayer->m_hActiveWeapon().Get() && g_LocalPlayer->m_hActiveWeapon().Get()->IsSniper() && g_LocalPlayer->m_bIsScoped()))
			{
				int screenX, screenY;
				g_EngineClient->GetScreenSize(screenX, screenY);
				g_VGuiSurface->DrawSetColor(Color::Black);
				g_VGuiSurface->DrawLine(screenX / 2, 0, screenX / 2, screenY);
				g_VGuiSurface->DrawLine(0, screenY / 2, screenX, screenY / 2);
			}

			if (g_Options.misc_spectatorlist)
				Visuals::RenderSpectatorList();

			if (g_Options.visuals_others_grenade_pred)
				CCSGrenadeHint::Get().Paint();

			if (g_Options.visuals_others_hitmarker)
				HitMarkerEvent::Get().Paint();
		}

		if (g_Options.visuals_others_watermark)
			Visuals::DrawWatermark();
	}
}

bool __stdcall Handlers::CreateMove_h(float smt, CUserCmd *userCMD)
{
	if (!userCMD->command_number || !g_EngineClient->IsInGame() || !g_LocalPlayer || !g_LocalPlayer->IsAlive())
		return o_CreateMove(g_ClientMode, smt, userCMD);
	
	// Update tickbase correction.
	AimRage::Get().GetTickbase(userCMD);

	AntiAim::Get().UpdateLowerBodyBreaker(userCMD);

	QAngle org_angle = userCMD->viewangles;

	uintptr_t *framePtr;
	__asm mov framePtr, ebp;

	Global::smt = smt;
	Global::bSendPacket = true;
	Global::userCMD = userCMD;
	Global::vecUnpredictedVel = g_LocalPlayer->m_vecVelocity();

	if (g_Options.misc_bhop)
		Miscellaneous::Get().Bhop(userCMD);

	if (g_Options.misc_autostrafe)
		Miscellaneous::Get().AutoStrafe(userCMD);

	if (g_Options.misc_fakelag_value)
		Miscellaneous::Get().Fakelag();

	QAngle wish_angle = userCMD->viewangles;
	userCMD->viewangles = org_angle;

	// -----------------------------------------------
	// Do engine prediction
	PredictionSystem::Get().Start(userCMD, g_LocalPlayer);
	{
		Miscellaneous::Get().AutoPistol(userCMD);

		AimLegit::Get().Work(userCMD);

		AimRage::Get().Work(userCMD);

		Miscellaneous::Get().AntiAim(userCMD);

		Miscellaneous::Get().FixMovement(userCMD, wish_angle);
	}
	PredictionSystem::Get().End(g_LocalPlayer);

	CCSGrenadeHint::Get().Tick(userCMD->buttons);

	AntiAim::Get().Fakewalk(userCMD);

	if (g_Options.rage_enabled && Global::bAimbotting && userCMD->buttons & IN_ATTACK)
		*(bool*)(*framePtr - 0x1C) = false;

	*(bool*)(*framePtr - 0x1C) = Global::bSendPacket;

	if (g_Options.hvh_show_real_angles)
	{
		if (!Global::bSendPacket)
			Global::visualAngles = userCMD->viewangles;
	}
	else if(Global::bSendPacket)
		Global::visualAngles = userCMD->viewangles;

	if (Global::bShouldChoke)
		Global::bSendPacket = Global::bShouldChoke = false;

	if (!Global::bSendPacket)
		Global::nChockedTicks++;
	else
		Global::nChockedTicks = 0;

	userCMD->forwardmove = Miscellaneous::Get().clamp(userCMD->forwardmove, -450.f, 450.f);
	userCMD->sidemove = Miscellaneous::Get().clamp(userCMD->sidemove, -450.f, 450.f);
	userCMD->upmove = Miscellaneous::Get().clamp(userCMD->upmove, -320.f, 320.f);
	userCMD->viewangles.Clamp();

	if (!g_Options.rage_silent && Global::bVisualAimbotting)
		g_EngineClient->SetViewAngles(Global::vecVisualAimbotAngs);

	return false;
}

void __stdcall Handlers::PlaySound_h(const char *folderIme)
{
	o_PlaySound(g_VGuiSurface, folderIme);

	if (!g_Options.misc_autoaccept) return;

	if (!strcmp(folderIme, "!UI/competitive_accept_beep.wav"))
	{
		Utils::IsReady();

		FLASHWINFO flash;
		flash.cbSize = sizeof(FLASHWINFO);
		flash.hwnd = window;
		flash.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
		flash.uCount = 0;
		flash.dwTimeout = 0;
		FlashWindowEx(&flash);
	}
}

HRESULT __stdcall Handlers::EndScene_h(IDirect3DDevice9 *pDevice)
{
	pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, 0xFFFFFFFF);

	if (!GladiatorMenu::d3dinit)
		GladiatorMenu::GUI_Init(window, pDevice);

	ImGui::GetIO().MouseDrawCursor = menuOpen;

	ImGui_ImplDX9_NewFrame();
	
	if (menuOpen)
	{
		GladiatorMenu::mainWindow();
	}

	ImGui::Render();

	if (g_Options.misc_revealAllRanks && g_InputSystem->IsButtonDown(KEY_TAB))
		Utils::RankRevealAll();

	return o_EndScene(pDevice);
}

HRESULT __stdcall Handlers::Reset_h(IDirect3DDevice9 *pDevice, D3DPRESENT_PARAMETERS *pPresentationParameters)
{
	if (!GladiatorMenu::d3dinit)
		return o_Reset(pDevice, pPresentationParameters);

	ImGui_ImplDX9_InvalidateDeviceObjects();

	auto hr = o_Reset(pDevice, pPresentationParameters);

	if (hr >= 0)
	{
		ImGui_ImplDX9_CreateDeviceObjects();
		Visuals::InitFont();
	}

	return hr;
}

LRESULT __stdcall Handlers::WndProc_h(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_LBUTTONDOWN:

		pressedKey[VK_LBUTTON] = true;
		break;

	case WM_LBUTTONUP:

		pressedKey[VK_LBUTTON] = false;
		break;

	case WM_RBUTTONDOWN:

		pressedKey[VK_RBUTTON] = true;
		break;

	case WM_RBUTTONUP:

		pressedKey[VK_RBUTTON] = false;
		break;

	case WM_KEYDOWN:

		pressedKey[wParam] = true;
		break;

	case WM_KEYUP:

		pressedKey[wParam] = false;
		break;

	default: break;
	}

	GladiatorMenu::openMenu();

	if (GladiatorMenu::d3dinit && menuOpen && ImGui_ImplDX9_WndProcHandler(hWnd, uMsg, wParam, lParam) && !input_shouldListen)
		return true;

	return CallWindowProc(oldWindowProc, hWnd, uMsg, wParam, lParam);
}

void __stdcall Handlers::FrameStageNotify_h(ClientFrameStage_t stage)
{
	int32_t idx = g_EngineClient->GetLocalPlayer();
	auto local_player = C_BasePlayer::GetPlayerByIndex(idx);

	if (!local_player || !g_EngineClient->IsInGame() || !g_EngineClient->IsConnected())
		return o_FrameStageNotify(stage);

	g_LocalPlayer = local_player;

	QAngle aim_punch_old;
	QAngle view_punch_old;

	QAngle *aim_punch = nullptr;
	QAngle *view_punch = nullptr;

	if (stage == ClientFrameStage_t::FRAME_NET_UPDATE_START)
	{
		for (int i = 1; i < g_EntityList->GetHighestEntityIndex(); i++)
		{
			C_BasePlayer *player = C_BasePlayer::GetPlayerByIndex(i);

			if (!player)
				continue;

			if (player == g_LocalPlayer)
				continue;

			if (player->m_iTeamNum() == g_LocalPlayer->m_iTeamNum())
				continue;

			VarMapping_t *map = player->VarMapping();
			if (map)
			{
				for (int i = 0; i < map->m_nInterpolatedEntries; i++)
				{
					map->m_Entries[i].m_bNeedsToInterpolate = !g_Options.rage_lagcompensation;
				}
			}
		}
	}

	CMBacktracking::Get().AnimationFix(stage);

	if (stage == ClientFrameStage_t::FRAME_NET_UPDATE_POSTDATAUPDATE_START)
	{
		Miscellaneous::Get().PunchAngleFix_FSN();

		for (int i = 1; i <= g_GlobalVars->maxClients; i++)
		{
			C_BasePlayer *player = C_BasePlayer::GetPlayerByIndex(i);

			if (!player)
				continue;

			if (player == g_LocalPlayer)
				continue;

			if (player->IsDormant())
				continue;

			if (!player->IsAlive())
				continue;

			if (player->m_iTeamNum() == g_LocalPlayer->m_iTeamNum() && g_Options.hvh_resolver) {
				player->m_angEyeAngles().yaw = player->m_flLowerBodyYawTarget();
				continue;
			}

			if (g_Options.hvh_resolver)
				Resolver::Get().Resolve(player);

			if (g_Options.rage_lagcompensation)
				CMBacktracking::Get().CacheInfo(player);

			if (g_Options.rage_fixup_entities)
			{
				player->SetAbsAngles(QAngle(0, player->m_angEyeAngles().yaw, 0));
				player->SetAbsOrigin(player->m_vecOrigin());
			}
		}
	}

	if (stage == ClientFrameStage_t::FRAME_RENDER_START)
	{
		*(bool*)Offsets::bOverridePostProcessingDisable = g_Options.removals_postprocessing;

		// PVS Fix.
		for (int i = 1; i <= g_GlobalVars->maxClients; i++)
		{
			C_BasePlayer *player = C_BasePlayer::GetPlayerByIndex(i);

			if (!player)
				continue;

			if (player == g_LocalPlayer)
				continue;

			*(int*)((uintptr_t)player + 0xA30) = g_GlobalVars->framecount;
			*(int*)((uintptr_t)player + 0xA28) = 0;
		}

		if (g_LocalPlayer->IsAlive())
		{
			if (g_Options.removals_novisualrecoil)
			{
				aim_punch = &g_LocalPlayer->m_aimPunchAngle();
				view_punch = &g_LocalPlayer->m_viewPunchAngle();

				aim_punch_old = *aim_punch;
				view_punch_old = *view_punch;

				*aim_punch = QAngle(0.f, 0.f, 0.f);
				*view_punch = QAngle(0.f, 0.f, 0.f);
			}

			if (*(bool*)((uintptr_t)g_Input + 0xA5))
				g_LocalPlayer->visuals_Angles() = Global::visualAngles;
		}

		if (g_Options.removals_flash && g_LocalPlayer)
			if (g_LocalPlayer->m_flFlashDuration() > 0.f)
				g_LocalPlayer->m_flFlashDuration() = 0.f;

		if (g_Options.removals_smoke)
			*(int*)Offsets::smokeCount = 0;
	}

	o_FrameStageNotify(stage);

	if (g_LocalPlayer->IsAlive())
	{
		if (g_Options.removals_novisualrecoil && (aim_punch && view_punch))
		{
			*aim_punch = aim_punch_old;
			*view_punch = view_punch_old;
		}
	}
}

void __fastcall Handlers::BeginFrame_h(void *thisptr, void*, float ft)
{
	Miscellaneous::Get().NameChanger();
	Miscellaneous::Get().ChatSpamer();
	Miscellaneous::Get().ClanTag();

	o_BeginFrame(thisptr, ft);
}

void __fastcall Handlers::SetKeyCodeState_h(void* thisptr, void* EDX, ButtonCode_t code, bool bDown)
{
	if (input_shouldListen && bDown)
	{
		input_shouldListen = false;
		if (input_receivedKeyval)
			*input_receivedKeyval = code;
	}

	return o_SetKeyCodeState(thisptr, code, bDown);
}

void __fastcall Handlers::SetMouseCodeState_h(void* thisptr, void* EDX, ButtonCode_t code, MouseCodeState_t state)
{
	if (input_shouldListen && state == BUTTON_PRESSED)
	{
		input_shouldListen = false;
		if (input_receivedKeyval)
			*input_receivedKeyval = code;
	}

	return o_SetMouseCodeState(thisptr, code, state);
}

void __stdcall Handlers::OverrideView_h(CViewSetup* pSetup)
{
	// Do no zoom aswell.
	pSetup->fov += g_Options.visuals_others_player_fov;

	o_OverrideView(pSetup);

	if (g_EngineClient->IsInGame() && g_EngineClient->IsConnected())
	{
		if (g_LocalPlayer)
		{
			CCSGrenadeHint::Get().View();

			Miscellaneous::Get().ThirdPerson();
		}
	}
}

void Proxies::didSmokeEffect(const CRecvProxyData *pData, void *pStruct, void *pOut)
{
	if (g_Options.removals_smoke)
		*(bool*)((DWORD)pOut + 0x1) = true;

	o_didSmokeEffect(pData, pStruct, pOut);
}

bool __stdcall Handlers::InPrediction_h() {
	if (g_Options.rage_fixup_entities) {
		// Breaks more than it fixes.
		//// xref : "%8.4f : %30s : %5.3f : %4.2f  +\n" https://github.com/ValveSoftware/source-sdk-2013/blob/master/mp/src/game/client/c_baseanimating.cpp#L1808
		//static DWORD inprediction_check = (DWORD)Utils::PatternScan(GetModuleHandle("client.dll"), "84 C0 74 17 8B 87");
		//if (inprediction_check == (DWORD)_ReturnAddress()) {
		//	return true; // no sequence transition / decay
		//}
	}
	return o_OriginalInPrediction(g_Prediction);
}

bool __fastcall Handlers::SetupBones_h(void* ECX, void* EDX, matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime)
{
	// Supposed to only setupbones tick by tick, instead of frame by frame.
	if (g_Options.rage_fixup_entities)
	{
		if (ECX && ((IClientRenderable*)ECX)->GetIClientUnknown())
		{
			IClientNetworkable* pNetworkable = ((IClientRenderable*)ECX)->GetIClientUnknown()->GetClientNetworkable();
			if (pNetworkable && pNetworkable->GetClientClass() && pNetworkable->GetClientClass()->m_ClassID == ClassId::ClassId_CCSPlayer)
			{
				static auto host_timescale = g_CVar->FindVar(("host_timescale"));
				auto player = (C_BasePlayer*)ECX;
				float OldCurTime = g_GlobalVars->curtime;
				float OldRealTime = g_GlobalVars->realtime;
				float OldFrameTime = g_GlobalVars->frametime;
				float OldAbsFrameTime = g_GlobalVars->absoluteframetime;
				float OldAbsFrameTimeStart = g_GlobalVars->absoluteframestarttimestddev;
				float OldInterpAmount = g_GlobalVars->interpolation_amount;
				int OldFrameCount = g_GlobalVars->framecount;
				int OldTickCount = g_GlobalVars->tickcount;

				g_GlobalVars->curtime = player->m_flSimulationTime();
				g_GlobalVars->realtime = player->m_flSimulationTime();
				g_GlobalVars->frametime = g_GlobalVars->interval_per_tick * host_timescale->GetFloat();
				g_GlobalVars->absoluteframetime = g_GlobalVars->interval_per_tick * host_timescale->GetFloat();
				g_GlobalVars->absoluteframestarttimestddev = player->m_flSimulationTime() - g_GlobalVars->interval_per_tick * host_timescale->GetFloat();
				g_GlobalVars->interpolation_amount = 0;
				g_GlobalVars->framecount = TIME_TO_TICKS(player->m_flSimulationTime()); // Wrong if backtracking.
				g_GlobalVars->tickcount = TIME_TO_TICKS(player->m_flSimulationTime());

				*(int*)((int)player + 236) |= 8; // IsNoInterpolationFrame
				bool ret_value = o_SetupBones(player, pBoneToWorldOut, nMaxBones, boneMask, g_GlobalVars->curtime);
				*(int*)((int)player + 236) &= ~8; // (1 << 3)

				g_GlobalVars->curtime = OldCurTime;
				g_GlobalVars->realtime = OldRealTime;
				g_GlobalVars->frametime = OldFrameTime;
				g_GlobalVars->absoluteframetime = OldAbsFrameTime;
				g_GlobalVars->absoluteframestarttimestddev = OldAbsFrameTimeStart;
				g_GlobalVars->interpolation_amount = OldInterpAmount;
				g_GlobalVars->framecount = OldFrameCount;
				g_GlobalVars->tickcount = OldTickCount;
				return ret_value;
			}
		}
	}
	return o_SetupBones(ECX, pBoneToWorldOut, nMaxBones, boneMask, currentTime);
}

void __fastcall Handlers::SceneEnd_h(void* thisptr, void* edx)
{
	if (!g_LocalPlayer)
		return o_SceneEnd(thisptr);

	if (g_Options.esp_player_chams)
	{
		constexpr float color_gray[4] = { 166, 167, 169, 255 };
		IMaterial *mat =
			(g_Options.esp_player_chams_type < 2 ?
				g_MatSystem->FindMaterial("chams", TEXTURE_GROUP_MODEL) :
				g_MatSystem->FindMaterial("debug/debugdrawflat", TEXTURE_GROUP_MODEL));

		if (!mat || mat->IsErrorMaterial())
			return;

		for (int i = 1; i < g_GlobalVars->maxClients; ++i) {
			auto ent = static_cast<C_BasePlayer*>(g_EntityList->GetClientEntity(i));
			if (ent && ent->IsAlive() && !ent->IsDormant()) {

				if (g_Options.esp_enemies_only && ent->m_iTeamNum() == g_LocalPlayer->m_iTeamNum())
					continue;

				if (g_Options.esp_player_chams_type == 1 || g_Options.esp_player_chams_type == 3)
				{	// XQZ Chams
					g_RenderView->SetColorModulation(ent->m_bGunGameImmunity() ? color_gray : (ent->m_iTeamNum() == 2 ? g_Options.esp_player_chams_color_t : g_Options.esp_player_chams_color_ct));

					mat->IncrementReferenceCount();
					mat->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, true);

					g_MdlRender->ForcedMaterialOverride(mat);

					ent->DrawModel(0x1, 255);
					g_MdlRender->ForcedMaterialOverride(nullptr);

					g_RenderView->SetColorModulation(ent->m_bGunGameImmunity() ? color_gray : (ent->m_iTeamNum() == 2 ? g_Options.esp_player_chams_color_t_visible : g_Options.esp_player_chams_color_ct_visible));

					mat->IncrementReferenceCount();
					mat->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, false);

					g_MdlRender->ForcedMaterialOverride(mat);

					ent->DrawModel(0x1, 255);
					g_MdlRender->ForcedMaterialOverride(nullptr);
				}
				else
				{	// Normal Chams
					g_RenderView->SetColorModulation(ent->m_iTeamNum() == 2 ? g_Options.esp_player_chams_color_t_visible : g_Options.esp_player_chams_color_ct_visible);

					g_MdlRender->ForcedMaterialOverride(mat);

					ent->DrawModel(0x1, 255);

					g_MdlRender->ForcedMaterialOverride(nullptr);
				}
			}
		}
	}

	if (g_Options.glow_enabled)
	{
		Glow::RenderGlow();
	}

	return o_SceneEnd(thisptr);
}

void __stdcall FireBullets_PostDataUpdate(C_TEFireBullets *thisptr, DataUpdateType_t updateType)
{
	if (g_Options.hvh_resolver && g_Options.rage_lagcompensation && thisptr)
	{
		int iPlayer = thisptr->m_iPlayer + 1;
		if (iPlayer < 64)
		{
			auto player = C_BasePlayer::GetPlayerByIndex(iPlayer);
			
			if (player && player != g_LocalPlayer && !player->IsDormant() && player->m_iTeamNum() != g_LocalPlayer->m_iTeamNum())
			{
				float
					event_time = g_GlobalVars->tickcount,
					player_time = player->m_flSimulationTime();

				// Extrapolate tick to hit scouters etc (TODO)
				auto lag_records = CMBacktracking::Get().m_LagRecord[iPlayer];

				float shot_time = TICKS_TO_TIME(event_time);
				for (auto& record : lag_records)
				{
					if (record.m_iTickCount <= g_GlobalVars->tickcount)
					{
						shot_time = record.m_flSimulationTime + TICKS_TO_TIME(event_time - record.m_iTickCount); // also get choked from this
#ifdef _DEBUG
						g_CVar->ConsoleColorPrintf(Color(0, 255, 0, 255), "Found <<exact>> shot time: %f, ticks choked to get here: %d\n", shot_time, event_time - record.m_iTickCount);
#endif
						break;
					}
#ifdef _DEBUG
					else
						g_CVar->ConsolePrintf("Bad curtime difference, EVENT: %f, RECORD: %f\n", event_time, record.m_iTickCount);
#endif
				}
#ifdef _DEBUG
				g_CVar->ConsolePrintf("CURTIME_TICKOUNT: %f, SIMTIME: %f, CALCED_TIME: %f\n", event_time, player_time, shot_time);
#endif
				CMBacktracking::Get().SetOverwriteTick(player, thisptr->m_vecAngles, shot_time, 1);
			}
		}
	}

	o_FireBullets(thisptr, updateType);
}

__declspec (naked) void __stdcall Handlers::TEFireBulletsPostDataUpdate_h(DataUpdateType_t updateType)
{
	__asm
	{
		push[esp + 4]
		push ecx
		call FireBullets_PostDataUpdate
		retn 4
	}
}

bool __fastcall Handlers::TempEntities_h(void* ECX, void* EDX, void* msg)
{
	bool ret = o_TempEntities(ECX, msg);

	auto CL_ParseEventDelta = [](void *RawData, void *pToData, RecvTable *pRecvTable)
	{
		// "RecvTable_DecodeZeros: table '%s' missing a decoder.", look at the function that calls it.
		static uintptr_t CL_ParseEventDeltaF = (uintptr_t)Utils::PatternScan(GetModuleHandle("engine.dll"), ("55 8B EC 83 E4 F8 53 57"));
		__asm
		{
			mov     ecx, RawData
			mov     edx, pToData
			push	pRecvTable
			call    CL_ParseEventDeltaF
			add     esp, 8
		}
	};

	// Filtering events
	if (!g_LocalPlayer || !g_LocalPlayer->IsAlive() || !g_Options.rage_lagcompensation)
		return ret;

	uintptr_t events = (uintptr_t)g_ClientState + 0x4DE0;
	uintptr_t
		ei = *(uintptr_t*)(events + 0xC),
		next = NULL; // cl.events.Head()

	if (!ei)
		return ret;

	do
	{
		next = *(uintptr_t*)(ei + 0x38);
		uint16_t classID = *(uint16_t*)ei - 1;

		auto m_pCreateEventFn = *(IClientNetworkable*(**)(void))(*(uintptr_t*)(ei + 0xC) + 0x4); // ei->pClientClass->m_pCreateEventFn ptr
		if (!m_pCreateEventFn) continue;

		IClientNetworkable* pCE = m_pCreateEventFn();

		if (classID == ClassId::ClassId_CTEFireBullets)
		{
			// set fire_delay to zero to send out event so its not here later.
			*(float*)(ei + 0x2) = 0.0f;
		}
		ei = next;
	} while (next != NULL);

	return ret;
}

float __fastcall Handlers::GetViewModelFov_h(void* ECX, void* EDX)
{
	return g_Options.visuals_others_player_fov_viewmodel + o_GetViewmodelFov(ECX);
}

bool __fastcall Handlers::GetBool_SVCheats_h(PVOID pConVar, int edx)
{
	// xref : "Pitch: %6.1f   Yaw: %6.1f   Dist: %6.1f %16s"
	static DWORD CAM_THINK = (DWORD)Utils::PatternScan(GetModuleHandle("client.dll"), "85 C0 75 30 38 86");
	if (!pConVar)
		return false;

	if (g_Options.misc_thirdperson)
	{
		if ((DWORD)_ReturnAddress() == CAM_THINK)
			return true;
	}

	return o_GetBool(pConVar);
}

void __fastcall Handlers::RunCommand_h(void* ECX, void* EDX, C_BasePlayer* player, CUserCmd* cmd, IMoveHelper* helper)
{
	o_RunCommand(ECX, player, cmd, helper);

	Miscellaneous::Get().PunchAngleFix_RunCommand(player);
}