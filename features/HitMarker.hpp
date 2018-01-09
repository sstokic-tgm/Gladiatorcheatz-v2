#pragma once

#include "../interfaces/IGameEventmanager.hpp"
#include "../Singleton.hpp"

#include <vector>

struct HitMarkerInfo
{
	float m_flExpTime;
	int m_iDmg;
};

class HitMarkerEvent : public IGameEventListener2, public Singleton<HitMarkerEvent>
{
public:

	void FireGameEvent(IGameEvent *event);
	int  GetEventDebugID(void);

	void RegisterSelf();
	void UnregisterSelf();

	void Paint(void);

private:

	std::vector<HitMarkerInfo> hitMarkerInfo;
};