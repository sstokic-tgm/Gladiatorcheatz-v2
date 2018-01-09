#include "BulletImpact.hpp"

#include "../Structs.hpp"
#include "Visuals.hpp"

#include "../Options.hpp"
#include "../helpers/Math.hpp"

void BulletImpactEvent::FireGameEvent(IGameEvent *event)
{
	if (!g_Options.visuals_others_bulletimpacts || !event)
		return;

	if (g_EngineClient->GetPlayerForUserID(event->GetInt("userid")) == g_EngineClient->GetLocalPlayer() && g_LocalPlayer && g_LocalPlayer->IsAlive())
	{
		float x = event->GetFloat("x"), y = event->GetFloat("y"), z = event->GetFloat("z");

		Vector vec_shotfrom = g_LocalPlayer->m_vecOrigin() + g_LocalPlayer->m_vecViewOffset() - Vector(0, 0, 1), vec_hitpos = Vector(x, y, z);

		g_DebugOverlay->AddLineOverlay(vec_shotfrom, vec_hitpos, 255, 255, 255, true, 2.f);

		// Ok no something is completely fucked, I can't use this or I get too many vertices bullshit (12 lines gives a bazzilion vertices or some shit, idfk)
		// possibly event delay related, idk, no1 else has this problem.
		// g_DebugOverlay->AddBoxOverlay(Vector(x, y, z), Vector(-2, -2, -2), Vector(2, 2, 2), QAngle(0, 0, 0), 255, 0, 0, 127, 2.f);
	}
}

int BulletImpactEvent::GetEventDebugID(void)
{
	return EVENT_DEBUG_ID_INIT;
}

void BulletImpactEvent::RegisterSelf()
{
	g_GameEvents->AddListener(this, "bullet_impact", false);
}

void BulletImpactEvent::UnregisterSelf()
{
	g_GameEvents->RemoveListener(this);
}