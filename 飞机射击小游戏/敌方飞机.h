#ifndef NOMINMAX
#define NOMINMAX
#endif

#pragma once

#include "我方飞机.h"

class EnemyPlane
{
public:
	EnemyPlane();

	void Reset(float startX, float startY, int kind, float speedX, float speedY, int hp);
	void Move(float dx, float dy);
	void Tick();
	bool CanShoot() const;
	Bullet Shoot();
	void Hit();
	void BeginDeath(int deathTicks);
	RECT Bounds() const;

	float X() const;
	float Y() const;
	float Width() const;
	float Height() const;
	int Kind() const;
	int Hp() const;
	int DeathTicks() const;
	bool IsDead() const;
	bool IsExpired() const;

	void SetSize(float width, float height);

private:
	float x_;
	float y_;
	float width_;
	float height_;
	float speedX_;
	float speedY_;
	int kind_;
	int hp_;
	int deathTicks_;
	int shootCooldown_;
	int frameTick_;
};
