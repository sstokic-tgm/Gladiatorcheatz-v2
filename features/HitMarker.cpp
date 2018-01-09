#include "HitMarker.hpp"

#include "../Structs.hpp"
#include "Visuals.hpp"

#include "../Options.hpp"

void HitMarkerEvent::FireGameEvent(IGameEvent *event)
{
	if (!event)
		return;

	if (g_Options.visuals_others_hitmarker)
	{
		if (g_EngineClient->GetPlayerForUserID(event->GetInt("attacker")) == g_EngineClient->GetLocalPlayer() &&
			g_EngineClient->GetPlayerForUserID(event->GetInt("userid"))   != g_EngineClient->GetLocalPlayer())
		{
			hitMarkerInfo.push_back({ g_GlobalVars->curtime + 0.8f, event->GetInt("dmg_health") });
			g_EngineClient->ExecuteClientCmd("play buttons\\arena_switch_press_02.wav"); // No other fitting sound. Probs should use a resource
		}
	}
}

int HitMarkerEvent::GetEventDebugID(void)
{
	return EVENT_DEBUG_ID_INIT;
}

void HitMarkerEvent::RegisterSelf()
{
	g_GameEvents->AddListener(this, "player_hurt", false);
}

void HitMarkerEvent::UnregisterSelf()
{
	g_GameEvents->RemoveListener(this);
}

void HitMarkerEvent::Paint(void)
{
	static int width = 0, height = 0;
	if (width == 0 || height == 0)
		g_EngineClient->GetScreenSize(width, height);

	float alpha = 0.f;

	for (size_t i = 0; i < hitMarkerInfo.size(); i++)
	{
		float diff = hitMarkerInfo.at(i).m_flExpTime - g_GlobalVars->curtime;

		if (diff < 0.f)
		{
			hitMarkerInfo.erase(hitMarkerInfo.begin() + i);
			continue;
		}

		int dist = 24;

		float ratio = 1.f - (diff / 0.8f);
		alpha = 0.8f - diff / 0.8f;

		Visuals::DrawString(Visuals::ui_font, width / 2 + 6 + ratio * dist / 2, height / 2 + 6 + ratio * dist, Color(255, 255, 0, (int)(alpha * 255.f)), FONT_LEFT, std::to_string(hitMarkerInfo.at(i).m_iDmg).c_str());
	}

	if (hitMarkerInfo.size() > 0)
	{
		int lineSize = 12;
		g_VGuiSurface->DrawSetColor(Color(255, 255, 255, (int)(alpha * 255.f)));
		g_VGuiSurface->DrawLine(width / 2 - lineSize / 2, height / 2 - lineSize / 2, width / 2 + lineSize / 2, height / 2 + lineSize / 2);
		g_VGuiSurface->DrawLine(width / 2 + lineSize / 2, height / 2 - lineSize / 2, width / 2 - lineSize / 2, height / 2 + lineSize / 2);
	}
}