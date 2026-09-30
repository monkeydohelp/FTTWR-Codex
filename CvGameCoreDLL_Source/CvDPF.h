#pragma once

#ifndef CV_DPF_H
#define CV_DPF_H

#include "CvGameCoreDLL.h"

// Dynamic Progression Framework (DPF) v1.  Definitions are deliberately
// generic and player-owned; ACBase Test supplies the first disposable fixture.
enum DPFStatus
{
	DPF_STATUS_DORMANT = 0,
	DPF_STATUS_ACTIVE,
	DPF_STATUS_BLOCKED,
	DPF_STATUS_FAILED,
	DPF_STATUS_COMPLETED,
	DPF_STATUS_WAITING_FOR_CHOICE
};

enum DPFEventType
{
	DPF_EVENT_IMPROVEMENT_COMPLETED = 1,
	DPF_EVENT_FEATURE_REMOVED,
	DPF_EVENT_TECHNOLOGY_ACQUIRED,
	DPF_EVENT_CIVIC_CHANGED,
	DPF_EVENT_TRAIT_CHANGED,
	DPF_EVENT_CITY_CREATED,
	DPF_EVENT_CITY_LOST,
	DPF_EVENT_RESOURCE_CHANGED,
	DPF_EVENT_TRANSITION_COMMITTED
};

class CvDPF
{
public:
	static void initStatics();
	static void freeStatics();
	static void resetPlayer(PlayerTypes ePlayer);

	static bool isEnabled();
	static void queueEvent(PlayerTypes eTarget, DPFEventType eType,
		int iTurn, PlayerTypes eActor, int iX, int iY, int iObject,
		int iAmount = 1);
	static void queueImprovementCompleted(PlayerTypes eActor, PlayerTypes eOwner,
		int iX, int iY, int iImprovement, int iAmount = 1);
	static void queueFeatureRemoved(PlayerTypes eActor, PlayerTypes eOwner,
		int iX, int iY, int iFeature, int iAmount = 1);
	// First-class adapters for engine action callsites. They all feed queueEvent
	// and therefore share the same authoritative sequencing and settlement path.
	static void queueTechnologyAcquired(PlayerTypes eActor, PlayerTypes eOwner,
		int iTechnology, int iAmount = 1);
	static void queueCivicChanged(PlayerTypes eActor, PlayerTypes eOwner,
		int iCivic, int iAmount = 1);
	static void queueTraitChanged(PlayerTypes eActor, PlayerTypes eOwner,
		int iTrait, int iAmount = 1);
	static void queueCityCreated(PlayerTypes eActor, PlayerTypes eOwner,
		int iX, int iY, int iCity, int iAmount = 1);
	static void queueCityLost(PlayerTypes eActor, PlayerTypes eOwner,
		int iX, int iY, int iCity, int iAmount = 1);
	static void queueResourceChanged(PlayerTypes eActor, PlayerTypes eOwner,
		int iResource, int iAmount = 1);
	static void queueTransitionCommitted(PlayerTypes eActor, PlayerTypes eOwner,
		int iTransition, int iAmount = 1);

	// Called once per player at the opening of the player's turn, after engine
	// automatic processing and before warnings/UI input.
	static void settlePlayer(PlayerTypes ePlayer, int iTurn);

	static bool hasTraitClaim(PlayerTypes ePlayer, TraitTypes eTrait);
	static CvString getStateDigest(PlayerTypes ePlayer);

	// Read-only query surface for Python/UI. These methods never initialize,
	// settle, or mutate DPF state.
	static int getInstanceCount(PlayerTypes ePlayer);
	static int getInstanceDefinitionIdAt(PlayerTypes ePlayer, int iIndex);
	static int getInstanceStatus(PlayerTypes ePlayer, int iDefinition);
	static int getInstanceStage(PlayerTypes ePlayer, int iDefinition);
	static int getInstanceVariable(PlayerTypes ePlayer, int iDefinition, int iVariable);
	static int getInstanceEventCount(PlayerTypes ePlayer, int iDefinition, int iEventType);
	// Native presentation queries. These are read-only and return deterministic
	// definition/state-derived text; Python may use them instead of raw IDs.
	static CvString getInstanceDisplayName(PlayerTypes ePlayer, int iDefinition);
	static CvString getInstanceDescription(PlayerTypes ePlayer, int iDefinition);
	static CvString getInstanceStatusName(PlayerTypes ePlayer, int iDefinition);
	static CvString getInstanceStageName(PlayerTypes ePlayer, int iDefinition);
	static CvString getInstanceNextHint(PlayerTypes ePlayer, int iDefinition);
	// Pending-choice query/command surface. Queries are read-only; submitChoice
	// is the single authoritative mutation path shared by human and AI callers.
	static bool hasPendingChoice(PlayerTypes ePlayer);
	static int getPendingChoiceDefinitionId(PlayerTypes ePlayer);
	static int getPendingChoiceRequestId(PlayerTypes ePlayer);
	static int getPendingChoiceCreationTurn(PlayerTypes ePlayer);
	static int getPendingChoiceExpiryTurn(PlayerTypes ePlayer);
	static int getPendingChoiceDefaultOption(PlayerTypes ePlayer);
	static int getPendingChoiceOptionCount(PlayerTypes ePlayer);
	static int getPendingChoiceOptionIdAt(PlayerTypes ePlayer, int iIndex);
	static CvString getPendingChoiceOptionName(PlayerTypes ePlayer, int iOptionId);
	static CvString getPendingChoiceOptionDescription(PlayerTypes ePlayer, int iOptionId);
	static bool submitChoice(PlayerTypes ePlayer, int iRequestId, int iOptionId,
		int iTurn);

	static int getPendingEventCount(PlayerTypes ePlayer);
	static int getTraceCount(PlayerTypes ePlayer);
	static CvString getTraceEntry(PlayerTypes ePlayer, int iIndex);

	// Player save extension.  The caller gates these on the player save flag;
	// the extension itself is versioned and bounded.
	static void readPlayer(PlayerTypes ePlayer, FDataStreamBase* pStream);
	static void writePlayer(PlayerTypes ePlayer, FDataStreamBase* pStream);
};

// Kept as a free function so CvPlayer.h does not need to include the DPF
// implementation header from its ABI-sensitive inline surface.
bool dpfHasTraitClaim(PlayerTypes ePlayer, TraitTypes eTrait);

#endif
