#pragma once

#include "kz/kz.h"
#include "sdk/entity/ctriggermultiple.h"

/*
	TriggerFix overrides trigger events generated by Valve's code
	to fix cases where players can dodge triggers given the right conditions (edgebug, jumpbugs, perfs).

	Note before proceeding:
	1. Triggers touching events are stored in an array that is filled by vphysics every tick.
	2. When the player starts touching a trigger, StartTouch and Touch are fired.
	3. When the player ends touching a trigger, Touch and EndTouch are fired.
	4. As long as the player is touching a trigger, Touch is fired.
	Due to 2 and 4, this means the first time the player touches a trigger, Touch is fired twice.
	This is different from CS:GO, where Touch can only fire once with StartTouch and it does not fire with EndTouch.

	Modes that use this service need to implement their own trigger touching using TouchTriggersAlongPath and UpdateTriggerTouchList (see VNL).
*/

struct KZCourseDescriptor;
struct Modifier;

class KZTriggerService : public KZBaseService
{
public:
	using KZBaseService::KZBaseService;

	virtual void Reset();

	struct TriggerTouchTracker
	{
		CEntityHandle triggerHandle;
		const KzTrigger *kzTrigger = NULL;

		bool operator==(const TriggerTouchTracker &other)
		{
			return triggerHandle == other.triggerHandle;
		}

		// Trigger to player
		bool startedTouch {};
		// Player to trigger.
		bool touchedThisTick {};

		// Server time
		f32 startTouchTime {};
		bool isPossibleLegacyBhopTrigger {};

		bool CanStartTouch()
		{
			return !startedTouch;
		}

		bool CanTouch()
		{
			return !touchedThisTick || HighFrequencyTouchAllowed(*this);
		}

		bool CanEndTouch()
		{
			// The interaction hasn't even fully started yet.
			// This should never happen but we make the check anyway just to be sure.
			if (!startedTouch)
			{
				META_CONPRINTF("[KZ::Trigger] EndTouch check is called before the start touch interaction even finished!\n");
				return false;
			}
			return true;
		}
	};

private:
	// Touchlist related functions.
	CUtlVector<TriggerTouchTracker> triggerTrackers;
	Vector preTouchOrigin;
	Vector preTouchVelocity;

	// OnTriggerStartTouchPre is an exception as the touch tracker might not exist yet.
	bool OnTriggerStartTouchPre(CBaseTrigger *trigger);
	void OnTriggerStartTouchPost(CBaseTrigger *trigger, TriggerTouchTracker tracker);
	bool OnTriggerTouchPre(CBaseTrigger *trigger, TriggerTouchTracker tracker);
	void OnTriggerTouchPost(CBaseTrigger *trigger, TriggerTouchTracker tracker);
	bool OnTriggerEndTouchPre(CBaseTrigger *trigger, TriggerTouchTracker tracker);
	void OnTriggerEndTouchPost(CBaseTrigger *trigger, TriggerTouchTracker tracker);

	// Mapping API stuff.
	struct Modifiers
	{
		i32 disablePausingCount;
		i32 disableCheckpointsCount;
		i32 disableTeleportsCount;
		i32 disableJumpstatsCount;
		i32 enableSlideCount;
	};

	Modifiers modifiers {};
	Modifiers lastModifiers {};
	bool antiBhopActive;
	bool lastAntiBhopActive;

	CEntityHandle lastTouchedSingleBhop {};
	i32 bhopTouchCount {};

	class CSequentialBhopBuffer : public CFixedSizeCircularBuffer<CEntityHandle, 64>
	{
		virtual void ElementAlloc(CEntityHandle &element) {};
		virtual void ElementRelease(CEntityHandle &element) {};
	};

	CSequentialBhopBuffer lastTouchedSequentialBhops {};

	void TouchModifierTrigger(TriggerTouchTracker tracker);
	void TouchAntibhopTrigger(TriggerTouchTracker tracker);
	bool TouchTeleportTrigger(TriggerTouchTracker tracker);
	void ResetBhopState();

	void UpdateModifiersInternal();

	void ApplySlide(bool replicate = false);
	void CancelSlide(bool replicate = false);

	void ApplyAntiBhop(bool replicate = false);
	void CancelAntiBhop(bool replicate = false);

	void OnMappingApiTriggerStartTouchPost(TriggerTouchTracker tracker);
	void OnMappingApiTriggerTouchPost(TriggerTouchTracker tracker);
	void OnMappingApiTriggerEndTouchPost(TriggerTouchTracker tracker);

public:
	// Movement function hooks
	void OnPhysicsSimulate();
	void OnPhysicsSimulatePost();

	void OnProcessMovement();
	void OnProcessMovementPost();

	void OnStopTouchGround();

	// Touchlist related functions (public)

	// Hit all triggers from start to end with the specified bounds,
	// and call Touch/StartTouch on triggers that the player is touching.
	void TouchTriggersAlongPath(const Vector &start, const Vector &end, const bbox_t &bounds);

	// Update the list of triggers that the player is touching, and call StartTouch/EndTouch appropriately.
	void UpdateTriggerTouchList();

	// Call Touch() on all currently touched triggers.
	void TouchAll();
	void EndTouchAll();

	static bool IsPossibleLegacyBhopTrigger(CTriggerMultiple *trigger);
	// Return true if this interaction is managed by TriggerFix.
	static bool IsManagedByTriggerService(CBaseEntity *toucher, CBaseEntity *touched);

	static bool ShouldTouchOnStartTouch(TriggerTouchTracker tracker)
	{
		// TODO: Let mapping API decide whether it is not the case.
		return true;
	}

	static bool ShouldTouchBeforeEndTouch(TriggerTouchTracker tracker)
	{
		// TODO: Let mapping API decide whether it is not the case.
		return true;
	}

	// Whether Touch should be called on every possible change in origin.
	// Usually true for triggers that need to update the player very frequently.
	static bool HighFrequencyTouchAllowed(TriggerTouchTracker tracker);

	TriggerTouchTracker *GetTriggerTracker(CBaseTrigger *trigger);

	void StartTouch(CBaseTrigger *trigger);
	void Touch(CBaseTrigger *trigger, bool silent = false);
	void EndTouch(CBaseTrigger *trigger);

	void UpdatePreTouchData();
	void UpdatePlayerPostTouch();

	bool InAntiPauseArea();
	bool InBhopTriggers();
	bool InAntiCpArea();
	bool CanTeleportToCheckpoints();
	bool ShouldDisableJumpstats();
};

inline bool operator==(const KZTriggerService::TriggerTouchTracker &left, const KZTriggerService::TriggerTouchTracker &right)
{
	return left.triggerHandle == right.triggerHandle;
}
