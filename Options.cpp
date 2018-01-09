#include "Options.hpp"

const char *opt_EspType[] = { "Off", "Bounding", "Corners" };
const char *opt_BoundsType[] = { "Static", "Dynamic" };
const char *opt_WeaponBoxType[] = { "Off", "Bounding", "3D" };
const char *opt_GrenadeESPType[] = { "Text", "Box + Text", "Icon", "Icon + Name hint" };
const char *opt_GlowStyles[] = { "Outline outer", "Cover", "Outline inner" };
const char *opt_Chams[] = { "Textured", "Textured XQZ", "Flat", "Flat XQZ" };
const char *opt_AimHitboxSpot[] = { "Head", "Neck", "Body", "Pelvis" };
const char *opt_AimSpot[] = { "Head", "Neck", "Body", "Pelvis" };
const char *opt_MultiHitboxes[14] = { "Head", "Pelvis", "Upper Chest", "Chest", "Neck", "Left Forearm", "Right Forearm", "Hands", "Left Thigh", "Right Thigh", "Left Calf", "Right Calf", "Left Foot", "Right Foot" };
const char *opt_AApitch[] = { "Off", "Dynamic", "Down", "Straight", "Up" };
const char *opt_AAyaw[] = { "Off", "Backwards", "Jitter", "Evade Jitter", "Random Jitter", "Synchronized", "Slowspin", "Fastspin", "LBY Breaker" };
const char *opt_AAfakeyaw[] = { "Off", "Forward", "Jitter", "Evade", "Synchronized", "Slowspin", "Fastspin", "LBY Breaker" };
const char *opt_LagCompType[] = { "Only best records", "Best and newest", "All records (fps warning)" };
int realAimSpot[] = { 8, 7, 6, 0 };
int realHitboxSpot[] = { 0, 1, 2, 3 };
bool input_shouldListen = false;
ButtonCode_t* input_receivedKeyval = nullptr;
bool pressedKey[256] = {};
bool menuOpen = true;

Options g_Options;