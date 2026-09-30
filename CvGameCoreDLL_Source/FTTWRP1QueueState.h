#pragma once

#ifndef FTTWRP1_QUEUE_STATE_H
#define FTTWRP1_QUEUE_STATE_H

#include "FDataStreamBase.h"
#include "FTTWRCapacityContract.h"

#include <string>
#include <vector>

// This state is intentionally internal to the candidate DLL.  It does not
// change the stable Civ4 OrderData ABI and does not reorder the native queue.
enum FTTWRQueueStatus
{
	FTTWR_QUEUE_STATUS_ACTIVE = 0,
	FTTWR_QUEUE_STATUS_FROZEN = 1
};

enum FTTWRQueueFreezeReason
{
	FTTWR_QUEUE_REASON_NONE = 0,
	FTTWR_QUEUE_REASON_HUMAN_CAPACITY = 1,
	FTTWR_QUEUE_REASON_ROBOT_CAPACITY = 2,
	FTTWR_QUEUE_REASON_POPULATION = 3,
	FTTWR_QUEUE_REASON_UNSUPPORTED_FAMILY = 4,
	FTTWR_QUEUE_REASON_UNSUPPORTED_TIER = 5,
	FTTWR_QUEUE_REASON_INVALID_CONTRACT = 6,
	FTTWR_QUEUE_REASON_RESOURCE_CAPACITY = 7
};

enum
{
	// AdvCiv's CvCity save header is a numeric version, not a bitfield.  Use a
	// high marker so legacy version values (including 17) never make an old
	// save reader consume this optional block by accident.
	FTTWRP1_QUEUE_SAVE_FLAG = 0x40000000,
	FTTWRP1_QUEUE_SAVE_VERSION = 5,
	FTTWRP1_HUMAN_TIER_COUNT = 3,
	FTTWRP1_HUMAN_TIER_BASIC = 1,
	FTTWRP1_HUMAN_TIER_ADVANCED = 2,
	FTTWRP1_HUMAN_TIER_VETERAN = 3,
	FTTWRP1_QUEUE_MAX_TRACKED = 64,
	FTTWRP1_QUEUE_MAX_CAPACITY_COMMITMENTS = 64,
	FTTWRP1_QUEUE_MAX_NOTIFICATIONS = 32
};

struct FTTWRQueueEntry
{
	FTTWRQueueEntry();

	int iSerial;
	int iOrderType;
	int iData1;
	int iData2;
	int iOriginalQueueIndex;
	int iCurrentQueueIndex;
	int iProgress;
	int iProjectFamily;
	int iRecruitTier;
	int iHumanCapacityCost;
	int iRobotCapacityCost;
	int iPopulationCost;
	int iCapacityCostCount;
	int iCapacityResource[FTTWRP1_MAX_CAPACITY_COSTS];
	int iCapacityCost[FTTWRP1_MAX_CAPACITY_COSTS];
	int iCapacityAllocation[FTTWRP1_MAX_CAPACITY_COSTS];
	int iHumanCapacityAllocation[FTTWRP1_HUMAN_TIER_COUNT];
	int iStatus;
	int iFreezeReason;
	bool bCapacityConsumed;
	bool bRestorePending;
};

struct FTTWRQueueNotification
{
	FTTWRQueueNotification();

	int iSerial;
	int iStatus;
	int iFreezeReason;
};

struct FTTWRCapacityCommitment
{
	FTTWRCapacityCommitment() : iResource(-1), iCommitted(0) {}
	int iResource;
	int iCommitted;
};

class FTTWRP1QueueState
{
public:
	FTTWRP1QueueState();

	void reset();

	void setRestorePriority(bool bEnabled);
	bool isRestorePriority() const;

	// Queue index bookkeeping is kept separate from native OrderData.  The
	// original index remains stable while a project is frozen; current indexes
	// shift only when an order is inserted or removed around it.
	void onOrderInserted(int iQueueIndex);
	void onOrderRemoved(int iQueueIndex);

	int trackOrder(int iOrderType, int iData1, int iData2,
		int iQueueIndex, int iProgress, int iProjectFamily,
		int iRecruitTier, int iHumanCapacityCost,
		int iRobotCapacityCost, int iPopulationCost);
	int trackOrderGeneric(int iOrderType, int iData1, int iData2,
		int iQueueIndex, int iProgress, int iProjectFamily,
		int iPopulationCost, int iCapacityCostCount,
		const int* aiCapacityResource, const int* aiCapacityCost);
	int trackOrderCombined(int iOrderType, int iData1, int iData2,
		int iQueueIndex, int iProgress, int iProjectFamily,
		int iRecruitTier, int iHumanCapacityCost, int iRobotCapacityCost,
		int iPopulationCost, int iCapacityCostCount,
		const int* aiCapacityResource, const int* aiCapacityCost);
	// Combined ResourceCapacity transaction used by native production.  The
	// generic resource and human/robot ledgers are checked and committed as one
	// bounded operation.
	void reconcileCombined(const int* aiCapacityAvailable, int iCapacityCount,
		int iHumanBasicAvailable, int iHumanAdvancedAvailable,
		int iHumanVeteranAvailable, int iRobotCapacityAvailable,
		int iPopulationAvailable);
	bool canReserveCompletionCombined(int iQueueIndex,
		const int* aiCapacityAvailable, int iCapacityCount,
		int iHumanBasicAvailable, int iHumanAdvancedAvailable,
		int iHumanVeteranAvailable, int iRobotCapacityAvailable,
		int iPopulationAvailable) const;
	bool reserveCompletionCombined(int iQueueIndex,
		const int* aiCapacityAvailable, int iCapacityCount,
		int iHumanBasicAvailable, int iHumanAdvancedAvailable,
		int iHumanVeteranAvailable, int iRobotCapacityAvailable,
		int iPopulationAvailable,
		int* aiCapacityAllocation = NULL,
		int* aiHumanAllocation = NULL);

	// Eligibility is deliberately side-effect free with respect to capacity:
	// no slot is consumed here.  The caller supplies current derived capacity
	// and population snapshots, and production remains the authority for any
	// future commit/consumption step.
	void reconcile(int iHumanBasicAvailable, int iHumanAdvancedAvailable,
		int iHumanVeteranAvailable, int iRobotCapacityAvailable,
		int iPopulationAvailable);
	void reconcileGeneric(const int* aiCapacityAvailable, int iCapacityCount,
		int iPopulationAvailable);

	// Production is the only authority allowed to commit capacity.  These
	// methods are side-effect free until reserve* succeeds and are deliberately
	// separate from reconcile() so queue inspection cannot consume a slot.
	bool canReserveCompletion(int iQueueIndex, int iHumanBasicAvailable,
		int iHumanAdvancedAvailable, int iHumanVeteranAvailable,
		int iRobotCapacityAvailable, int iPopulationAvailable) const;
	bool reserveCompletion(int iQueueIndex, int iHumanBasicAvailable,
		int iHumanAdvancedAvailable, int iHumanVeteranAvailable,
		int iRobotCapacityAvailable, int iPopulationAvailable,
		int* aiHumanAllocation = NULL);
	void releaseCompletion(int iQueueIndex);
	bool canReserveCompletionGeneric(int iQueueIndex,
		const int* aiCapacityAvailable, int iCapacityCount,
		int iPopulationAvailable) const;
	bool reserveCompletionGeneric(int iQueueIndex,
		const int* aiCapacityAvailable, int iCapacityCount,
		int iPopulationAvailable,
		int* aiCapacityAllocation = NULL);
	void releaseUnitLease(int iHumanBasicCapacity, int iHumanAdvancedCapacity,
		int iHumanVeteranCapacity, int iRobotCapacity);
	void releaseUnitLease(int iHumanCapacityCost, int iRobotCapacityCost);
	bool reserveCost(int iProjectFamily, int iRecruitTier,
		int iHumanCapacityCost, int iRobotCapacityCost, int iPopulationCost,
		int iHumanBasicAvailable, int iHumanAdvancedAvailable,
		int iHumanVeteranAvailable, int iRobotCapacityAvailable,
		int iPopulationAvailable, int* aiHumanAllocation = NULL);
	bool reserveCostGeneric(int iProjectFamily, int iPopulationCost,
		int iCapacityCostCount, const int* aiCapacityResource,
		const int* aiCapacityCost, const int* aiCapacityAvailable,
		int iCapacityCount, int iPopulationAvailable,
		int* aiCapacityAllocation = NULL);
	bool canReserveCostGeneric(int iPopulationCost, int iCapacityCostCount,
		const int* aiCapacityResource, const int* aiCapacityCost,
		const int* aiCapacityAvailable, int iCapacityCount,
		int iPopulationAvailable) const;
	void releaseCost(int iProjectFamily, int iHumanCapacityCost,
		int iRobotCapacityCost);
	void releaseCost(int iProjectFamily, int iHumanBasicCapacity,
		int iHumanAdvancedCapacity, int iHumanVeteranCapacity,
		int iRobotCapacity);
	void releaseCostGeneric(int iCapacityCostCount,
		const int* aiCapacityResource, const int* aiCapacityCost);
	void releasePopulationCost(int iPopulationCost);
	int getHumanCapacityCommitted() const;
	int getHumanCapacityCommitted(int iTier) const;
	int getRobotCapacityCommitted() const;
	int getPopulationCommitted() const;
	int getCapacityCommitted(int iCapacityResource) const;

	bool canAdvance(int iQueueIndex) const;
	int getTrackedCount() const;
	int getActiveCount() const;
	int getFrozenCount() const;
	int getRestorePendingCount() const;
	int getNotificationCount() const;
	int getFirstRestoredQueueIndex() const;

	std::string getSnapshot(int iCityId) const;
	std::string consumeNotifications(int iCityId);

	void write(FDataStreamBase* pStream) const;
	void read(FDataStreamBase* pStream);

private:
	bool isTrackedFamily(int iProjectFamily, int iHumanCapacityCost,
		int iRobotCapacityCost) const;
	bool isCostContractValid(int iProjectFamily, int iRecruitTier,
		int iHumanCapacityCost, int iRobotCapacityCost) const;
	bool canReserveCost(int iProjectFamily, int iRecruitTier,
		int iHumanCapacityCost, int iRobotCapacityCost, int iPopulationCost,
		int iHumanBasicAvailable, int iHumanAdvancedAvailable,
		int iHumanVeteranAvailable, int iRobotCapacityAvailable,
		int iPopulationAvailable) const;
	bool canReserveCostCombined(int iProjectFamily, int iRecruitTier,
		int iHumanCapacityCost, int iRobotCapacityCost,
		int iPopulationCost, int iHumanBasicAvailable,
		int iHumanAdvancedAvailable, int iHumanVeteranAvailable,
		int iRobotCapacityAvailable, int iPopulationAvailable,
		const int* aiCapacityAvailable, int iCapacityCount,
		int iCapacityCostCount, const int* aiCapacityResource,
		const int* aiCapacityCost) const;
	bool allocateHumanCapacity(int iRecruitTier, int iHumanCapacityCost,
		int iHumanBasicAvailable, int iHumanAdvancedAvailable,
		int iHumanVeteranAvailable, int* aiHumanAllocation) const;
	int findEntryByQueueIndex(int iQueueIndex) const;
	int findCapacityCommitment(int iCapacityResource) const;
	void changeCapacityCommitted(int iCapacityResource, int iChange);
	void emitTransition(const FTTWRQueueEntry& kEntry);
	const char* statusName(int iStatus) const;
	const char* reasonName(int iReason) const;

	bool m_bRestorePriority;
	int m_iNextSerial;
	int m_iHumanCapacityCommitted;
	int m_aiHumanCapacityCommitted[FTTWRP1_HUMAN_TIER_COUNT];
	int m_iRobotCapacityCommitted;
	int m_iPopulationCommitted;
	std::vector<FTTWRQueueEntry> m_aEntries;
	std::vector<FTTWRQueueNotification> m_aNotifications;
	std::vector<FTTWRCapacityCommitment> m_aCapacityCommitted;
};

#endif
