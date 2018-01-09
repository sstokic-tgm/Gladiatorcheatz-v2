#pragma once

#include "../interfaces/IGameEventmanager.hpp"
#include "../Singleton.hpp"

#include "../math/Vector.hpp"

#include <vector>

class BulletImpactEvent : public IGameEventListener2, public Singleton<BulletImpactEvent>
{
public:

	void FireGameEvent(IGameEvent *event);
	int  GetEventDebugID(void);

	void RegisterSelf();
	void UnregisterSelf();
};