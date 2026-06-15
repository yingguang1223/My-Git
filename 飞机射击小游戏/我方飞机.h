#ifndef NOMINMAX
#define NOMINMAX
#endif

#pragma once

#include <windows.h>

struct Bullet
{
	float x;
	float y;
	float speedX;
	float speedY;
	float width;
	float height;
	bool active;
	bool fromPlayer;
	int spriteIndex;
};

class MyPlane
{
public:
	MyPlane();

	void Reset(float startX, float startY);
	void Move(float dx, float dy, float minX, float minY, float maxX, float maxY);
	void Tick();
	bool CanShoot() const;
	Bullet Shoot();
	void Hit();
	RECT Bounds() const;

	float X() const;
	float Y() const;
	float Width() const;
	float Height() const;
	int Lives() const;
	bool IsInvincible() const;

	void SetSize(float width, float height);

private:
	float x_;
	float y_;
	float width_;
	float height_;
	int lives_;
	int shootCooldown_;
	int invincibleTicks_;
	int frameTick_;
};

