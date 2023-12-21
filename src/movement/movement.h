#pragma once
#include "common.h"
#include "movement/datatypes.h"

#define DECLARE_MOVEMENT_DETOUR(name) DECLARE_DETOUR(name, movement::Detour_##name, &modules::server);
#define DECLARE_MOVEMENT_EXTERN_DETOUR(name) extern CDetour<decltype(movement::Detour_##name)> name;

// TODO: better error sound
#define MV_SND_ERROR "Buttons.snd8"
#define MV_SND_TIMER_START "Buttons.snd9"
#define MV_SND_TIMER_END "UI.DeathMatch.LevelUp"

class MovementPlayer;

namespace movement
{
	void InitDetours();

	f32 FASTCALL Detour_GetMaxSpeed(CCSPlayerPawn *);
	i32 FASTCALL Detour_ProcessUsercmds(CBasePlayerPawn *, void *, int, bool);
	void FASTCALL Detour_ProcessMovement(CCSPlayer_MovementServices *, CMoveData *);
	bool FASTCALL Detour_PlayerMoveNew(CCSPlayer_MovementServices *, CMoveData *);
	void FASTCALL Detour_CheckParameters(CCSPlayer_MovementServices *, CMoveData *);
	bool FASTCALL Detour_CanMove(CCSPlayerPawnBase *);
	void FASTCALL Detour_FullWalkMove(CCSPlayer_MovementServices *, CMoveData *, bool);
	bool FASTCALL Detour_MoveInit(CCSPlayer_MovementServices *, CMoveData *);
	bool FASTCALL Detour_CheckWater(CCSPlayer_MovementServices *, CMoveData *);
	void FASTCALL Detour_CheckVelocity(CCSPlayer_MovementServices *, CMoveData *, const char *);
	void FASTCALL Detour_Duck(CCSPlayer_MovementServices *, CMoveData *);
	bool FASTCALL Detour_LadderMove(CCSPlayer_MovementServices *, CMoveData *);
	void FASTCALL Detour_CheckJumpButton(CCSPlayer_MovementServices *, CMoveData *);
	void FASTCALL Detour_OnJump(CCSPlayer_MovementServices *, CMoveData *);
	void FASTCALL Detour_AirAccelerate(CCSPlayer_MovementServices *, CMoveData *, Vector &, f32, f32);
	void FASTCALL Detour_Friction(CCSPlayer_MovementServices *, CMoveData *);
	void FASTCALL Detour_WalkMove(CCSPlayer_MovementServices *, CMoveData *);
	void FASTCALL Detour_TryPlayerMove(CCSPlayer_MovementServices *, CMoveData *, Vector *, trace_t_s2 *);
	void FASTCALL Detour_CategorizePosition(CCSPlayer_MovementServices *, CMoveData *, bool);
	void FASTCALL Detour_FinishGravity(CCSPlayer_MovementServices *, CMoveData *);
	void FASTCALL Detour_CheckFalling(CCSPlayer_MovementServices *, CMoveData *);
	void FASTCALL Detour_PostPlayerMove(CCSPlayer_MovementServices *, CMoveData *);
	void FASTCALL Detour_PostThink(CCSPlayerPawnBase *);

}

class MovementPlayer
{
public:
	MovementPlayer(int i) : index(i) {}

	virtual CCSPlayerController *GetController();
	virtual CCSPlayerPawn *GetPawn();
	virtual CPlayerSlot GetPlayerSlot() { return index - 1; };
	virtual CCSPlayer_MovementServices *GetMoveServices();

	// TODO: this doesn't work during movement processing!
	
	virtual void Teleport(const Vector *origin, const QAngle *angles, const Vector *velocity);
	virtual void GetOrigin(Vector *origin);
	virtual void SetOrigin(const Vector &origin);
	virtual void GetVelocity(Vector *velocity);
	virtual void SetVelocity(const Vector &velocity);
	virtual void GetAngles(QAngle *angles);
	// It is not recommended use this to change the angle inside movement processing, it might not work!
	virtual void SetAngles(const QAngle &angles);
	
	virtual TurnState GetTurning();
	virtual bool IsButtonDown(InputBitMask_t button, bool onlyDown = false);
	virtual void RegisterTakeoff(bool jumped);
	virtual void RegisterLanding(const Vector &landingVelocity, bool distbugFix = true);
	virtual f32 GetGroundPosition();
	virtual void InvalidateTimer(bool playErrorSound = true);

	virtual void Reset();
	virtual f32 GetPlayerMaxSpeed();

	// Movement hooks
	virtual void OnProcessUsercmds(void *, int) {};
	virtual void OnProcessUsercmdsPost(void *, int) {};
	virtual void OnProcessMovement();
	virtual void OnProcessMovementPost();
	virtual void OnPlayerMove() {};
	virtual void OnPlayerMovePost() {};
	virtual void OnCheckParameters() {};
	virtual void OnCheckParametersPost() {};
	virtual void OnCanMove() {};
	virtual void OnCanMovePost() {};
	virtual void OnFullWalkMove(bool &) {};
	virtual void OnFullWalkMovePost(bool) {};
	virtual void OnMoveInit() {};
	virtual void OnMoveInitPost() {};
	virtual void OnCheckWater() {};
	virtual void OnCheckWaterPost() {};
	virtual void OnCheckVelocity(const char *) {};
	virtual void OnCheckVelocityPost(const char *) {};
	virtual void OnDuck() {};
	virtual void OnDuckPost() {};
	virtual void OnCanUnduck() {};
	virtual void OnCanUnduckPost() {};
	virtual void OnLadderMove() {};
	virtual void OnLadderMovePost() {};
	virtual void OnCheckJumpButton() {};
	virtual void OnCheckJumpButtonPost() {};
	virtual void OnJump() {};
	virtual void OnJumpPost() {};
	virtual void OnAirAccelerate(Vector &wishdir, f32 &wishspeed, f32 &accel) {};
	virtual void OnAirAcceleratePost(Vector wishdir, f32 wishspeed, f32 accel) {};
	virtual void OnFriction() {};
	virtual void OnFrictionPost() {};
	virtual void OnWalkMove() {};
	virtual void OnWalkMovePost() {};
	virtual void OnTryPlayerMove(Vector *, trace_t_s2 *) {};
	virtual void OnTryPlayerMovePost(Vector *, trace_t_s2 *) {};
	virtual void OnCategorizePosition(bool) {};
	virtual void OnCategorizePositionPost(bool) {};
	virtual void OnFinishGravity() {};
	virtual void OnFinishGravityPost() {};
	virtual void OnCheckFalling() {};
	virtual void OnCheckFallingPost() {};
	virtual void OnPostPlayerMove() {};
	virtual void OnPostPlayerMovePost() {};
	virtual void OnPostThink();
	virtual void OnPostThinkPost() {};

	// Movement events
	virtual void OnStartTouchGround() {};
	virtual void OnStopTouchGround() {};
	virtual void OnChangeMoveType(MoveType_t oldMoveType) {};

	virtual void OnTeleport(const Vector *origin, const QAngle *angles, const Vector *velocity) {};
	
	virtual void StartZoneStartTouch();
	virtual void StartZoneEndTouch();
	virtual void EndZoneStartTouch();
	
	void PlayErrorSound();

public:
	// General
	const i32 index;

	bool processingMovement;
	CMoveData *currentMoveData{};
	CMoveData moveDataPre;
	CMoveData moveDataPost;

	f32 lastProcessedCurtime{};
	u64 lastProcessedTickcount{};

	QAngle oldAngles;

	bool processingDuck{};
	bool duckBugged{};
	bool walkMoved{};
	bool oldWalkMoved{};
	bool hitPerf{};
	bool jumped{};
	bool takeoffFromLadder{};
	Vector lastValidLadderOrigin;
	bool timerIsRunning{};

	Vector takeoffOrigin;
	Vector takeoffVelocity;
	f32 takeoffTime{};
	Vector takeoffGroundOrigin;
	
	Vector landingOrigin;
	Vector landingVelocity;
	f32 landingTime{};
	Vector landingOriginActual;
	f32 landingTimeActual{};
	
	i32 tickCount;
	i32 timerStartTick;
};

class CMovementPlayerManager
{
public:
	CMovementPlayerManager()
	{
		for (int i = 0; i < MAXPLAYERS + 1; i++)
		{
			players[i] = new MovementPlayer(i);
		}
	}
	~CMovementPlayerManager()
	{
		for (int i = 0; i < MAXPLAYERS + 1; i++)
		{
			delete players[i];
		}
	}
public:
	MovementPlayer *ToPlayer(CCSPlayer_MovementServices *ms);
	MovementPlayer *ToPlayer(CBasePlayerController *controller);
	MovementPlayer *ToPlayer(CBasePlayerPawn *pawn);
	MovementPlayer *ToPlayer(CPlayerSlot slot);
	MovementPlayer *ToPlayer(CEntityIndex entIndex);
	MovementPlayer *ToPlayer(CPlayerUserId userID);
public:
	MovementPlayer *players[MAXPLAYERS + 1];
};

extern CMovementPlayerManager *g_pPlayerManager;