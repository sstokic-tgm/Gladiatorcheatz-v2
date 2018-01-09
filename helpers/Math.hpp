#pragma once

#include "../SDK.hpp"

#define PI 3.14159265358979323846f
#define PI_F	((float)(PI)) 
#define DEG2RAD( x ) ( ( float )( x ) * ( float )( ( float )( PI ) / 180.0f ) )
#define RAD2DEG( x ) ( ( float )( x ) * ( float )( 180.0f / ( float )( PI ) ) )

namespace Math
{
    void NormalizeAngles(QAngle& angles);
	void NormalizeVector(Vector& vec);
    void ClampAngles(QAngle& angles);
    void VectorTransform(const Vector& in1, const matrix3x4_t& in2, Vector& out);
    void AngleVectors(const QAngle &angles, Vector& forward);
    void AngleVectors(const QAngle &angles, Vector& forward, Vector& right, Vector& up);
    void VectorAngles(const Vector& forward, QAngle& angles);
	Vector CrossProduct(const Vector &a, const Vector &b);
	void VectorAngles(const Vector& forward, Vector& up, QAngle& angles);
    bool WorldToScreen(const Vector& in, Vector& out);
	bool screen_transform(const Vector& in, Vector& out);
	void SinCos(float a, float* s, float*c);
	float GetFov(const QAngle &viewAngle, const QAngle &aimAngle);
	float GetDistance(Vector src, Vector dst);
	QAngle CalcAngle(Vector src, Vector dst);
	void SmoothAngle(QAngle src, QAngle &dst, float factor);

	inline float ClampYaw(float yaw)
	{
		while (yaw > 180.f)
			yaw -= 360.f;
		while (yaw < -180.f)
			yaw += 360.f;
		return yaw;
	}

	float __fastcall AngleDiff(float a1, float a2);
}