#include "Install.hpp"
#include "Gamehooking.hpp"
#include "helpers/Utils.hpp"
#include "features/Glow.hpp"
#include "features/HitMarker.hpp"
#include "features/BulletImpact.hpp"

IVEngineClient			*g_EngineClient = nullptr;
IBaseClientDLL			*g_CHLClient = nullptr;
IClientEntityList		*g_EntityList = nullptr;
CGlobalVarsBase			*g_GlobalVars = nullptr;
IEngineTrace			*g_EngineTrace = nullptr;
ICvar					*g_CVar = nullptr;
IPanel					*g_VGuiPanel = nullptr;
IClientMode				*g_ClientMode = nullptr;
IVDebugOverlay			*g_DebugOverlay = nullptr;
ISurface				*g_VGuiSurface = nullptr;
CInput					*g_Input = nullptr;
IVModelInfoClient		*g_MdlInfo = nullptr;
IVModelRender			*g_MdlRender = nullptr;
IVRenderView			*g_RenderView = nullptr;
IMaterialSystem			*g_MatSystem = nullptr;
IGameEventManager2		*g_GameEvents = nullptr;
IMoveHelper				*g_MoveHelper = nullptr;
IMDLCache				*g_MdlCache = nullptr;
IPrediction				*g_Prediction = nullptr;
CGameMovement			*g_GameMovement = nullptr;
IEngineSound			*g_EngineSound = nullptr;
CGlowObjectManager		*g_GlowObjManager = nullptr;
CClientState			*g_ClientState = nullptr;
IPhysicsSurfaceProps	*g_PhysSurface = nullptr;
IInputSystem			*g_InputSystem = nullptr;
DWORD					*g_InputInternal = nullptr;
IMemAlloc				*g_pMemAlloc = nullptr;
IViewRenderBeams	    *g_RenderBeams = nullptr;
ILocalize				*g_Localize = nullptr;
C_BasePlayer		    *g_LocalPlayer = nullptr;

namespace Offsets
{
	DWORD invalidateBoneCache = 0x00;
	DWORD smokeCount = 0x00;
	DWORD playerResource = 0x00;
	DWORD bOverridePostProcessingDisable = 0x00;
	DWORD getSequenceActivity = 0x00;
	DWORD lgtSmoke = 0x00;
	DWORD dwCCSPlayerRenderablevftable = 0x00;
}

std::unique_ptr<VFTableHook> g_pVguiPanelHook = nullptr;
std::unique_ptr<VFTableHook> g_pClientModeHook = nullptr;
std::unique_ptr<VFTableHook> g_pVguiSurfHook = nullptr;
std::unique_ptr<VFTableHook> g_pD3DDevHook = nullptr;
std::unique_ptr<VFTableHook> g_pClientHook = nullptr;
std::unique_ptr<VFTableHook> g_pMaterialSystemHook = nullptr;
std::unique_ptr<VFTableHook> g_pDMEHook = nullptr;
std::unique_ptr<VFTableHook> g_pInputInternalHook = nullptr;
std::unique_ptr<VFTableHook> g_pSceneEndHook = nullptr;
std::unique_ptr<VFTableHook> g_pFireBulletHook = nullptr;
std::unique_ptr<VFTableHook> g_pPredictionHook = nullptr;
std::unique_ptr<VFTableHook> g_pConvarHook = nullptr;

PaintTraverse_t o_PaintTraverse = nullptr;
CreateMove_t o_CreateMove = nullptr;
PlaySound_t o_PlaySound = nullptr;
EndScene_t o_EndScene = nullptr;
Reset_t o_Reset = nullptr;
FrameStageNotify_t o_FrameStageNotify = nullptr;
BeginFrame_t o_BeginFrame = nullptr;
OverrideView_t o_OverrideView = nullptr;
SetMouseCodeState_t o_SetMouseCodeState = nullptr;
SetKeyCodeState_t o_SetKeyCodeState = nullptr;
FireBullets_t o_FireBullets = nullptr;
InPrediction_t o_OriginalInPrediction = nullptr;
TempEntities_t o_TempEntities = nullptr;
SceneEnd_t o_SceneEnd = nullptr;
SetupBones_t o_SetupBones = nullptr;
GetBool_t o_GetBool = nullptr;
GetViewmodelFov_t o_GetViewmodelFov = nullptr;
RunCommand_t o_RunCommand = nullptr;

RecvVarProxyFn o_didSmokeEffect = nullptr;

HWND window = nullptr;
WNDPROC oldWindowProc = nullptr;

void Installer::installGladiator()
{
	FUNCTION_GUARD;

	Utils::AttachConsole();

	Utils::ConsolePrint(true, "-= -= -= -= -= -= -= -= -= -= -= -= -= -= -= -= -= -= -= -= -= -= -= -= -= -= -= -= -\n");
	Utils::ConsolePrint(true, "         Initializing Gladiatorcheatz, built on %s at %s...\n", __DATE__, __TIME__);
	Utils::ConsolePrint(true, "-= -= -= -= -= -= -= -= -= -= -= -= -= -= -= -= -= -= -= -= -= -= -= -= -= -= -= -= -\n");
	Utils::ConsolePrint(true, "Initializing cheat...\n");

	g_CHLClient = Iface::IfaceMngr::getIface<IBaseClientDLL>("client.dll", "VClient0");
	g_EntityList = Iface::IfaceMngr::getIface<IClientEntityList>("client.dll", "VClientEntityList");
	g_Prediction = Iface::IfaceMngr::getIface<IPrediction>("client.dll", "VClientPrediction");
	g_GameMovement = Iface::IfaceMngr::getIface<CGameMovement>("client.dll", "GameMovement");
	g_MdlCache = Iface::IfaceMngr::getIface<IMDLCache>("datacache.dll", "MDLCache");
	g_EngineClient = Iface::IfaceMngr::getIface<IVEngineClient>("engine.dll", "VEngineClient");
	g_MdlInfo = Iface::IfaceMngr::getIface<IVModelInfoClient>("engine.dll", "VModelInfoClient");
	g_MdlRender = Iface::IfaceMngr::getIface<IVModelRender>("engine.dll", "VEngineModel");
	g_RenderView = Iface::IfaceMngr::getIface<IVRenderView>("engine.dll", "VEngineRenderView");
	g_EngineTrace = Iface::IfaceMngr::getIface<IEngineTrace>("engine.dll", "EngineTraceClient");
	g_DebugOverlay = Iface::IfaceMngr::getIface<IVDebugOverlay>("engine.dll", "VDebugOverlay");
	g_GameEvents = Iface::IfaceMngr::getIface<IGameEventManager2>("engine.dll", "GAMEEVENTSMANAGER002", true);
	g_EngineSound = Iface::IfaceMngr::getIface<IEngineSound>("engine.dll", "IEngineSoundClient");
	g_MatSystem = Iface::IfaceMngr::getIface<IMaterialSystem>("materialsystem.dll", "VMaterialSystem");
	g_CVar = Iface::IfaceMngr::getIface<ICvar>("vstdlib.dll", "VEngineCvar");
	g_VGuiPanel = Iface::IfaceMngr::getIface<IPanel>("vgui2.dll", "VGUI_Panel");
	g_VGuiSurface = Iface::IfaceMngr::getIface<ISurface>("vguimatsurface.dll", "VGUI_Surface");
	g_PhysSurface = Iface::IfaceMngr::getIface<IPhysicsSurfaceProps>("vphysics.dll", "VPhysicsSurfaceProps");
	g_InputSystem = Iface::IfaceMngr::getIface<IInputSystem>("inputsystem.dll", "InputSystemVersion");
	g_InputInternal = Iface::IfaceMngr::getIface<DWORD>("vgui2.dll", "VGUI_InputInternal");
	g_Localize = Iface::IfaceMngr::getIface<ILocalize>("localize.dll", "Localize_");

	g_GlobalVars = **(CGlobalVarsBase***)((*(DWORD**)(g_CHLClient))[0] + 0x1B);
	g_Input = *(CInput**)((*(DWORD**)g_CHLClient)[15] + 0x1);
	g_ClientMode = **(IClientMode***)((*(DWORD**)g_CHLClient)[10] + 0x5);
	g_pMemAlloc = *(IMemAlloc**)(GetProcAddress(GetModuleHandle("tier0.dll"), "g_pMemAlloc"));

	auto client = GetModuleHandle("client.dll");
	auto engine = GetModuleHandle("engine.dll");
	auto dx9api = GetModuleHandle("shaderapidx9.dll");

	//g_LocalPlayer = *(C_LocalPlayer*)(Utils::PatternScan(client, "8B 0D ? ? ? ? 83 FF FF 74 07") + 2); // lets not use this mothafucka
	g_ClientState = **(CClientState***)(Utils::PatternScan(engine, "A1 ? ? ? ? 8B 80 ? ? ? ? C3") + 1);
	g_GlowObjManager = *(CGlowObjectManager**)(Utils::PatternScan(client, "0F 11 05 ? ? ? ? 83 C8 01") + 3);
	g_MoveHelper = **(IMoveHelper***)(Utils::PatternScan(client, "8B 0D ? ? ? ? 8B 45 ? 51 8B D4 89 02 8B 01") + 2);
	g_RenderBeams = *(IViewRenderBeams**)(Utils::PatternScan(client, "B9 ? ? ? ? A1 ? ? ? ? FF 10 A1 ? ? ? ? B9") + 1);

	auto D3DDevice9 = **(IDirect3DDevice9***)(Utils::PatternScan(dx9api, "A1 ? ? ? ? 50 8B 08 FF 51 0C") + 1);
	auto dwFireBullets = *(DWORD**)(Utils::PatternScan(client, "55 8B EC 51 53 56 8B F1 BB ? ? ? ? B8") + 0x131);

	Offsets::lgtSmoke = (DWORD)Utils::PatternScan(client, "55 8B EC 83 EC 08 8B 15 ? ? ? ? 0F 57 C0");
	Offsets::getSequenceActivity = (DWORD)Utils::PatternScan(client, "55 8B EC 83 7D 08 FF 56 8B F1 74 3D");
	Offsets::smokeCount = *(DWORD*)(Utils::PatternScan(client, "55 8B EC 83 EC 08 8B 15 ? ? ? ? 0F 57 C0") + 0x8);
	Offsets::invalidateBoneCache = (DWORD)Utils::PatternScan(client, "80 3D ? ? ? ? ? 74 16 A1 ? ? ? ? 48 C7 81");
	Offsets::playerResource = *(DWORD*)(Utils::PatternScan(client, "8B 3D ? ? ? ? 85 FF 0F 84 ? ? ? ? 81 C7") + 2);
	Offsets::bOverridePostProcessingDisable = *(DWORD*)(Utils::PatternScan(client, "80 3D ? ? ? ? ? 53 56 57 0F 85") + 2);
	Offsets::dwCCSPlayerRenderablevftable = *(DWORD*)(Utils::PatternScan(client, "55 8B EC 83 E4 F8 83 EC 18 56 57 8B F9 89 7C 24 0C") + 0x4E);

	NetMngr::Get().init();

	g_pDMEHook = std::make_unique<VFTableHook>((PPDWORD)g_MdlRender, true);
	g_pD3DDevHook = std::make_unique<VFTableHook>((PPDWORD)D3DDevice9, true);
	g_pClientHook = std::make_unique<VFTableHook>((PPDWORD)g_CHLClient, true);
	g_pSceneEndHook = std::make_unique<VFTableHook>((PPDWORD)g_RenderView, true);
	g_pVguiPanelHook = std::make_unique<VFTableHook>((PPDWORD)g_VGuiPanel, true);
	g_pVguiSurfHook = std::make_unique<VFTableHook>((PPDWORD)g_VGuiSurface, true);
	g_pClientModeHook = std::make_unique<VFTableHook>((PPDWORD)g_ClientMode, true);
	g_pPredictionHook = std::make_unique<VFTableHook>((PPDWORD)g_Prediction, true);
	g_pFireBulletHook = std::make_unique<VFTableHook>((PPDWORD)dwFireBullets, true);
	g_pMaterialSystemHook = std::make_unique<VFTableHook>((PPDWORD)g_MatSystem, true);
	g_pInputInternalHook = std::make_unique<VFTableHook>((PPDWORD)g_InputInternal, true);
	g_pConvarHook = std::make_unique<VFTableHook>((PPDWORD)g_CVar->FindVar("sv_cheats"), true, true);

	window = FindWindow("Valve001", NULL);
	oldWindowProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)Handlers::WndProc_h);

	o_SetMouseCodeState = g_pInputInternalHook->Hook(92, (SetMouseCodeState_t)Handlers::SetMouseCodeState_h);
	o_FireBullets = g_pFireBulletHook->Hook(7, (FireBullets_t)Handlers::TEFireBulletsPostDataUpdate_h);
	o_SetKeyCodeState = g_pInputInternalHook->Hook(91, (SetKeyCodeState_t)Handlers::SetKeyCodeState_h);
	o_GetViewmodelFov = g_pClientModeHook->Hook(35, (GetViewmodelFov_t)Handlers::GetViewModelFov_h);
	o_FrameStageNotify = g_pClientHook->Hook(36, (FrameStageNotify_t)Handlers::FrameStageNotify_h);
	o_OriginalInPrediction = g_pPredictionHook->Hook(14, (InPrediction_t)Handlers::InPrediction_h);
	o_PaintTraverse = g_pVguiPanelHook->Hook(41, (PaintTraverse_t)Handlers::PaintTraverse_h);
	o_OverrideView = g_pClientModeHook->Hook(18, (OverrideView_t)Handlers::OverrideView_h);
	o_BeginFrame = g_pMaterialSystemHook->Hook(42, (BeginFrame_t)Handlers::BeginFrame_h);
	o_RunCommand = g_pPredictionHook->Hook(19, (RunCommand_t)Handlers::RunCommand_h);
	o_CreateMove = g_pClientModeHook->Hook(24, (CreateMove_t)Handlers::CreateMove_h);
	o_GetBool = g_pConvarHook->Hook(13, (GetBool_t)Handlers::GetBool_SVCheats_h);
	o_PlaySound = g_pVguiSurfHook->Hook(82, (PlaySound_t)Handlers::PlaySound_h);
	o_SceneEnd = g_pSceneEndHook->Hook(9, (SceneEnd_t)Handlers::SceneEnd_h);
	o_EndScene = g_pD3DDevHook->Hook(42, (EndScene_t)Handlers::EndScene_h);
	o_Reset = g_pD3DDevHook->Hook(16, (Reset_t)Handlers::Reset_h);

	 o_TempEntities = VFTableHook::HookManual<TempEntities_t>(*(uintptr_t**)((int)g_ClientState + 0x8), 36, (TempEntities_t)Handlers::TempEntities_h);
	 o_SetupBones = VFTableHook::HookManual<SetupBones_t>((uintptr_t*)Offsets::dwCCSPlayerRenderablevftable, 13, (SetupBones_t)Handlers::SetupBones_h);

	NetMngr::Get().hookProp("CSmokeGrenadeProjectile", "m_bDidSmokeEffect", Proxies::didSmokeEffect, o_didSmokeEffect);
	HitMarkerEvent::Get().RegisterSelf();
	BulletImpactEvent::Get().RegisterSelf();

	Utils::ConsolePrint(true, "Gladiatorcheatz was successfully initialized!\n");
}

void Installer::uninstallGladiator()
{
	Glow::ClearGlow();
	HitMarkerEvent::Get().UnregisterSelf();
	BulletImpactEvent::Get().UnregisterSelf();

	SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)oldWindowProc);

	g_pVguiPanelHook->RestoreTable();
	g_pClientModeHook->RestoreTable();
	g_pVguiSurfHook->RestoreTable();
	g_pD3DDevHook->RestoreTable();
	g_pClientHook->RestoreTable();
	g_pMaterialSystemHook->RestoreTable();
	g_pInputInternalHook->RestoreTable();
	g_pFireBulletHook->RestoreTable();
	g_pPredictionHook->RestoreTable();
	g_pSceneEndHook->RestoreTable();
	g_pConvarHook->RestoreTable();

	VFTableHook::HookManual<TempEntities_t>(*(uintptr_t**)((int)g_ClientState + 0x8), 36, o_TempEntities);
	VFTableHook::HookManual<SetupBones_t>((uintptr_t*)Offsets::dwCCSPlayerRenderablevftable, 13, o_SetupBones);

	Utils::DetachConsole();
}