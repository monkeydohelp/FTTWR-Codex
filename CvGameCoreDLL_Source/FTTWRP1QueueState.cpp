#include "CvGameCoreDLL.h"
#include "FTTWRP1QueueState.h"

#include <algorithm>
#include <stdio.h>

FTTWRQueueEntry::FTTWRQueueEntry()
	: iSerial(0), iOrderType(-1), iData1(-1), iData2(-1),
	  iOriginalQueueIndex(-1), iCurrentQueueIndex(-1), iProgress(0),
	  iProjectFamily(0), iRecruitTier(0), iHumanCapacityCost(0),
	  iRobotCapacityCost(0), iPopulationCost(0), iCapacityCostCount(0),
	  iStatus(FTTWR_QUEUE_STATUS_ACTIVE),
	  iFreezeReason(FTTWR_QUEUE_REASON_NONE),
	  bCapacityConsumed(false), bRestorePending(false)
{
	for (int i = 0; i < FTTWRP1_HUMAN_TIER_COUNT; ++i)
	{
		iHumanCapacityAllocation[i] = 0;
	}
	for (int i = 0; i < FTTWRP1_MAX_CAPACITY_COSTS; ++i)
	{
		iCapacityResource[i] = -1;
		iCapacityCost[i] = 0;
		iCapacityAllocation[i] = 0;
	}
}

FTTWRQueueNotification::FTTWRQueueNotification()
	: iSerial(0), iStatus(FTTWR_QUEUE_STATUS_ACTIVE),
	  iFreezeReason(FTTWR_QUEUE_REASON_NONE)
{
}

FTTWRP1QueueState::FTTWRP1QueueState()
{
	reset();
}

void FTTWRP1QueueState::reset()
{
	m_bRestorePriority = false;
	m_iNextSerial = 1;
	m_iHumanCapacityCommitted = 0;
	m_iRobotCapacityCommitted = 0;
	m_iPopulationCommitted = 0;
	for (int i = 0; i < FTTWRP1_HUMAN_TIER_COUNT; ++i)
		m_aiHumanCapacityCommitted[i] = 0;
	m_aEntries.clear();
	m_aNotifications.clear();
	m_aCapacityCommitted.clear();
}

void FTTWRP1QueueState::setRestorePriority(bool bEnabled)
{
	m_bRestorePriority = bEnabled;
}

bool FTTWRP1QueueState::isRestorePriority() const
{
	return m_bRestorePriority;
}

void FTTWRP1QueueState::onOrderInserted(int iQueueIndex)
{
	if (iQueueIndex < 0)
		return;

	for (std::vector<FTTWRQueueEntry>::iterator it = m_aEntries.begin();
		it != m_aEntries.end(); ++it)
	{
		if (it->iCurrentQueueIndex >= iQueueIndex)
			++it->iCurrentQueueIndex;
	}
}

void FTTWRP1QueueState::onOrderRemoved(int iQueueIndex)
{
	if (iQueueIndex < 0)
		return;

	for (std::vector<FTTWRQueueEntry>::iterator it = m_aEntries.begin();
		it != m_aEntries.end(); ++it)
	{
		if (it->iCurrentQueueIndex == iQueueIndex)
		{
			m_aEntries.erase(it);
			break;
		}
	}

	for (std::vector<FTTWRQueueEntry>::iterator it = m_aEntries.begin();
		it != m_aEntries.end(); ++it)
	{
		if (it->iCurrentQueueIndex > iQueueIndex)
			--it->iCurrentQueueIndex;
	}
}

bool FTTWRP1QueueState::isTrackedFamily(int iProjectFamily,
	int iHumanCapacityCost, int iRobotCapacityCost) const
{
	// FTTWR_PROJECT_FAMILY_ORDINARY is zero in the candidate enum.  Keep
	// unsupported mutant/review rows tracked so they fail closed visibly.
	return (iProjectFamily != 0) &&
		(iHumanCapacityCost > 0 || iRobotCapacityCost > 0 ||
			 iProjectFamily >= 3);
}

bool FTTWRP1QueueState::isCostContractValid(int iProjectFamily,
	int iRecruitTier, int iHumanCapacityCost, int iRobotCapacityCost) const
{
	if (iProjectFamily == 0)
		return (iHumanCapacityCost == 0 && iRobotCapacityCost == 0);

	if (iProjectFamily == 1) // EXCEPTIONAL
	{
		return (iRecruitTier >= FTTWRP1_HUMAN_TIER_BASIC &&
			iRecruitTier <= FTTWRP1_HUMAN_TIER_VETERAN &&
			iHumanCapacityCost > 0 &&
			iRobotCapacityCost == 0);
	}

	if (iProjectFamily == 2) // ROBOT
	{
		return (iRecruitTier == 0 && iHumanCapacityCost == 0 &&
			iRobotCapacityCost > 0);
	}

	return false;
}

int FTTWRP1QueueState::findCapacityCommitment(int iCapacityResource) const
{
	for (int i = 0; i < (int)m_aCapacityCommitted.size(); ++i)
	{
		if (m_aCapacityCommitted[i].iResource == iCapacityResource)
			return i;
	}
	return -1;
}

void FTTWRP1QueueState::changeCapacityCommitted(int iCapacityResource,
	int iChange)
{
	if (iCapacityResource < 0 || iChange == 0)
		return;

	int iIndex = findCapacityCommitment(iCapacityResource);
	if (iIndex < 0)
	{
		if (iChange <= 0)
			return;
		FTTWRCapacityCommitment kCommitment;
		kCommitment.iResource = iCapacityResource;
		kCommitment.iCommitted = iChange;
		m_aCapacityCommitted.push_back(kCommitment);
		return;
	}

	m_aCapacityCommitted[iIndex].iCommitted = std::max(0,
		m_aCapacityCommitted[iIndex].iCommitted + iChange);
	if (m_aCapacityCommitted[iIndex].iCommitted == 0)
		m_aCapacityCommitted.erase(m_aCapacityCommitted.begin() + iIndex);
}

int FTTWRP1QueueState::getCapacityCommitted(int iCapacityResource) const
{
	int iIndex = findCapacityCommitment(iCapacityResource);
	return (iIndex < 0) ? 0 : m_aCapacityCommitted[iIndex].iCommitted;
}

bool FTTWRP1QueueState::canReserveCostGeneric(int iPopulationCost,
	int iCapacityCostCount, const int* aiCapacityResource,
	const int* aiCapacityCost, const int* aiCapacityAvailable,
	int iCapacityCount, int iPopulationAvailable) const
{
	if (iPopulationCost < 0 || iPopulationCost > iPopulationAvailable ||
		iCapacityCostCount < 0 ||
		iCapacityCostCount > FTTWRP1_MAX_CAPACITY_COSTS)
		return false;
	// A population-only contract is valid and deliberately does not require a
	// resource vector.  This keeps the generic ledger useful for settlement
	// capacity without inventing a dummy BonusTypes identity.
	if (iCapacityCostCount == 0)
		return (iPopulationCost > 0);
	if (aiCapacityAvailable == NULL || iCapacityCount <= 0)
		return false;

	std::vector<int> aiRequired(iCapacityCount, 0);
	for (int i = 0; i < iCapacityCostCount; ++i)
	{
		if (aiCapacityResource == NULL || aiCapacityCost == NULL ||
			aiCapacityResource[i] < 0 ||
			aiCapacityResource[i] >= iCapacityCount || aiCapacityCost[i] <= 0)
			return false;
		aiRequired[aiCapacityResource[i]] += aiCapacityCost[i];
	}

	for (int i = 0; i < iCapacityCount; ++i)
	{
		if (aiRequired[i] > std::max(0, aiCapacityAvailable[i]))
			return false;
	}
	return true;
}

bool FTTWRP1QueueState::canReserveCost(int iProjectFamily,
	int iRecruitTier, int iHumanCapacityCost, int iRobotCapacityCost,
	int iPopulationCost, int iHumanBasicAvailable,
	int iHumanAdvancedAvailable, int iHumanVeteranAvailable,
	int iRobotCapacityAvailable, int iPopulationAvailable) const
{
	iHumanCapacityCost = std::max(0, iHumanCapacityCost);
	iRobotCapacityCost = std::max(0, iRobotCapacityCost);
	iPopulationCost = std::max(0, iPopulationCost);
	iHumanBasicAvailable = std::max(0, iHumanBasicAvailable);
	iHumanAdvancedAvailable = std::max(0, iHumanAdvancedAvailable);
	iHumanVeteranAvailable = std::max(0, iHumanVeteranAvailable);
	iRobotCapacityAvailable = std::max(0, iRobotCapacityAvailable);
	iPopulationAvailable = std::max(0, iPopulationAvailable);

	if (!isCostContractValid(iProjectFamily, iRecruitTier,
		iHumanCapacityCost, iRobotCapacityCost))
		return false;

	if (iPopulationCost > iPopulationAvailable ||
		iRobotCapacityCost > iRobotCapacityAvailable)
		return false;

	return allocateHumanCapacity(iRecruitTier, iHumanCapacityCost,
		iHumanBasicAvailable, iHumanAdvancedAvailable,
		iHumanVeteranAvailable, NULL);
}

bool FTTWRP1QueueState::allocateHumanCapacity(int iRecruitTier,
	int iHumanCapacityCost, int iHumanBasicAvailable,
	int iHumanAdvancedAvailable, int iHumanVeteranAvailable,
	int* aiHumanAllocation) const
{
	if (aiHumanAllocation != NULL)
	{
		for (int i = 0; i < FTTWRP1_HUMAN_TIER_COUNT; ++i)
			aiHumanAllocation[i] = 0;
	}

	if (iHumanCapacityCost <= 0)
		return true;
	if (iRecruitTier < FTTWRP1_HUMAN_TIER_BASIC ||
		iRecruitTier > FTTWRP1_HUMAN_TIER_VETERAN)
		return false;

	int aiAvailable[FTTWRP1_HUMAN_TIER_COUNT];
	aiAvailable[0] = std::max(0, iHumanBasicAvailable);
	aiAvailable[1] = std::max(0, iHumanAdvancedAvailable);
	aiAvailable[2] = std::max(0, iHumanVeteranAvailable);
	int iRemaining = iHumanCapacityCost;
	for (int iTier = FTTWRP1_HUMAN_TIER_BASIC;
		iTier <= FTTWRP1_HUMAN_TIER_VETERAN; ++iTier)
	{
		if (iTier < iRecruitTier)
			continue;
		int iTake = std::min(iRemaining, aiAvailable[iTier - 1]);
		if (iTake > 0)
		{
			if (aiHumanAllocation != NULL)
				aiHumanAllocation[iTier - 1] = iTake;
			iRemaining -= iTake;
		}
		if (iRemaining == 0)
			break;
	}
	return (iRemaining == 0);
}

bool FTTWRP1QueueState::canReserveCostCombined(int iProjectFamily,
	int iRecruitTier, int iHumanCapacityCost, int iRobotCapacityCost,
	int iPopulationCost, int iHumanBasicAvailable,
	int iHumanAdvancedAvailable, int iHumanVeteranAvailable,
	int iRobotCapacityAvailable, int iPopulationAvailable,
	const int* aiCapacityAvailable, int iCapacityCount,
	int iCapacityCostCount, const int* aiCapacityResource,
	const int* aiCapacityCost) const
{
	bool const bFamilyCost = (iHumanCapacityCost > 0 ||
		iRobotCapacityCost > 0);
	if (bFamilyCost && !canReserveCost(iProjectFamily, iRecruitTier,
		iHumanCapacityCost, iRobotCapacityCost, 0,
		iHumanBasicAvailable, iHumanAdvancedAvailable,
		iHumanVeteranAvailable, iRobotCapacityAvailable, 0))
		return false;
	if (iCapacityCostCount > 0 || iPopulationCost > 0)
	{
		return canReserveCostGeneric(iPopulationCost, iCapacityCostCount,
			aiCapacityResource, aiCapacityCost, aiCapacityAvailable,
			iCapacityCount, iPopulationAvailable);
	}
	return bFamilyCost;
}

int FTTWRP1QueueState::trackOrderCombined(int iOrderType, int iData1,
	int iData2, int iQueueIndex, int iProgress, int iProjectFamily,
	int iRecruitTier, int iHumanCapacityCost, int iRobotCapacityCost,
	int iPopulationCost, int iCapacityCostCount,
	const int* aiCapacityResource, const int* aiCapacityCost)
{
	bool const bFamilyCost = (iHumanCapacityCost > 0 ||
		iRobotCapacityCost > 0);
	if ((int)m_aEntries.size() >= FTTWRP1_QUEUE_MAX_TRACKED ||
		iPopulationCost < 0 || iCapacityCostCount < 0 ||
		iCapacityCostCount > FTTWRP1_MAX_CAPACITY_COSTS ||
		(iCapacityCostCount > 0 &&
			(aiCapacityResource == NULL || aiCapacityCost == NULL)) ||
		(iCapacityCostCount == 0 && iPopulationCost <= 0 && !bFamilyCost))
		return -1;
	if (bFamilyCost && !isTrackedFamily(iProjectFamily,
		iHumanCapacityCost, iRobotCapacityCost))
		return -1;
	for (int i = 0; i < iCapacityCostCount; ++i)
	{
		if (aiCapacityResource[i] < 0 || aiCapacityCost[i] <= 0)
			return -1;
	}

	FTTWRQueueEntry kEntry;
	kEntry.iSerial = m_iNextSerial++;
	if (m_iNextSerial <= 0)
		m_iNextSerial = 1;
	kEntry.iOrderType = iOrderType;
	kEntry.iData1 = iData1;
	kEntry.iData2 = iData2;
	kEntry.iOriginalQueueIndex = iQueueIndex;
	kEntry.iCurrentQueueIndex = iQueueIndex;
	kEntry.iProgress = std::max(0, iProgress);
	kEntry.iProjectFamily = iProjectFamily;
	kEntry.iRecruitTier = iRecruitTier;
	kEntry.iHumanCapacityCost = std::max(0, iHumanCapacityCost);
	kEntry.iRobotCapacityCost = std::max(0, iRobotCapacityCost);
	kEntry.iPopulationCost = std::max(0, iPopulationCost);
	kEntry.iCapacityCostCount = std::min(iCapacityCostCount,
		(int)FTTWRP1_MAX_CAPACITY_COSTS);
	for (int i = 0; i < kEntry.iCapacityCostCount; ++i)
	{
		kEntry.iCapacityResource[i] = aiCapacityResource[i];
		kEntry.iCapacityCost[i] = std::max(0, aiCapacityCost[i]);
		kEntry.iCapacityAllocation[i] = 0;
	}
	kEntry.iStatus = FTTWR_QUEUE_STATUS_ACTIVE;
	kEntry.iFreezeReason = FTTWR_QUEUE_REASON_NONE;
	kEntry.bCapacityConsumed = false;
	kEntry.bRestorePending = false;
	m_aEntries.push_back(kEntry);
	return kEntry.iSerial;
}
int FTTWRP1QueueState::trackOrder(int iOrderType, int iData1, int iData2,
	int iQueueIndex, int iProgress, int iProjectFamily,
	int iRecruitTier, int iHumanCapacityCost,
	int iRobotCapacityCost, int iPopulationCost)
{
	if (!isTrackedFamily(iProjectFamily, iHumanCapacityCost,
		iRobotCapacityCost))
	{
		return -1;
	}

	if ((int)m_aEntries.size() >= FTTWRP1_QUEUE_MAX_TRACKED)
		return -1;

	FTTWRQueueEntry kEntry;
	kEntry.iSerial = m_iNextSerial++;
	if (m_iNextSerial <= 0)
		m_iNextSerial = 1;
	kEntry.iOrderType = iOrderType;
	kEntry.iData1 = iData1;
	kEntry.iData2 = iData2;
	kEntry.iOriginalQueueIndex = iQueueIndex;
	kEntry.iCurrentQueueIndex = iQueueIndex;
	kEntry.iProgress = std::max(0, iProgress);
	kEntry.iProjectFamily = iProjectFamily;
	kEntry.iRecruitTier = iRecruitTier;
	kEntry.iHumanCapacityCost = std::max(0, iHumanCapacityCost);
	kEntry.iRobotCapacityCost = std::max(0, iRobotCapacityCost);
	kEntry.iPopulationCost = std::max(0, iPopulationCost);
	kEntry.iStatus = FTTWR_QUEUE_STATUS_ACTIVE;
	kEntry.iFreezeReason = FTTWR_QUEUE_REASON_NONE;
	kEntry.bCapacityConsumed = false;
	kEntry.bRestorePending = false;
	m_aEntries.push_back(kEntry);
	return kEntry.iSerial;
}

int FTTWRP1QueueState::trackOrderGeneric(int iOrderType, int iData1,
	int iData2, int iQueueIndex, int iProgress, int iProjectFamily,
	int iPopulationCost, int iCapacityCostCount,
	const int* aiCapacityResource, const int* aiCapacityCost)
{
	// Tracking is deliberately side-effect free.  Validate only the bounded
	// XML-declared shape here; reconcileGeneric performs the live availability
	// check once the city supplies its current resource snapshot.
	if ((int)m_aEntries.size() >= FTTWRP1_QUEUE_MAX_TRACKED ||
		iPopulationCost < 0 || iCapacityCostCount < 0 ||
		iCapacityCostCount > FTTWRP1_MAX_CAPACITY_COSTS ||
		(iCapacityCostCount > 0 &&
			(aiCapacityResource == NULL || aiCapacityCost == NULL)) ||
		(iCapacityCostCount == 0 && iPopulationCost <= 0))
		return -1;
	for (int i = 0; i < iCapacityCostCount; ++i)
	{
		if (aiCapacityResource[i] < 0 || aiCapacityCost[i] <= 0)
			return -1;
	}

	FTTWRQueueEntry kEntry;
	kEntry.iSerial = m_iNextSerial++;
	if (m_iNextSerial <= 0)
		m_iNextSerial = 1;
	kEntry.iOrderType = iOrderType;
	kEntry.iData1 = iData1;
	kEntry.iData2 = iData2;
	kEntry.iOriginalQueueIndex = iQueueIndex;
	kEntry.iCurrentQueueIndex = iQueueIndex;
	kEntry.iProgress = std::max(0, iProgress);
	kEntry.iProjectFamily = iProjectFamily;
	kEntry.iPopulationCost = std::max(0, iPopulationCost);
	kEntry.iCapacityCostCount = std::min(iCapacityCostCount,
		(int)FTTWRP1_MAX_CAPACITY_COSTS);
	for (int i = 0; i < kEntry.iCapacityCostCount; ++i)
	{
		kEntry.iCapacityResource[i] = aiCapacityResource[i];
		kEntry.iCapacityCost[i] = std::max(0, aiCapacityCost[i]);
		kEntry.iCapacityAllocation[i] = 0;
	}
	kEntry.iStatus = FTTWR_QUEUE_STATUS_ACTIVE;
	kEntry.iFreezeReason = FTTWR_QUEUE_REASON_NONE;
	kEntry.bCapacityConsumed = false;
	kEntry.bRestorePending = false;
	m_aEntries.push_back(kEntry);
	return kEntry.iSerial;
}

void FTTWRP1QueueState::emitTransition(const FTTWRQueueEntry& kEntry)
{
	if ((int)m_aNotifications.size() >= FTTWRP1_QUEUE_MAX_NOTIFICATIONS)
		m_aNotifications.erase(m_aNotifications.begin());

	FTTWRQueueNotification kNotification;
	kNotification.iSerial = kEntry.iSerial;
	kNotification.iStatus = kEntry.iStatus;
	kNotification.iFreezeReason = kEntry.iFreezeReason;
	m_aNotifications.push_back(kNotification);
}

void FTTWRP1QueueState::reconcile(int iHumanBasicAvailable,
	int iHumanAdvancedAvailable, int iHumanVeteranAvailable,
	int iRobotCapacityAvailable, int iPopulationAvailable)
{
	iHumanBasicAvailable = std::max(0, iHumanBasicAvailable);
	iHumanAdvancedAvailable = std::max(0, iHumanAdvancedAvailable);
	iHumanVeteranAvailable = std::max(0, iHumanVeteranAvailable);
	iRobotCapacityAvailable = std::max(0, iRobotCapacityAvailable);
	iPopulationAvailable = std::max(0, iPopulationAvailable);

	for (std::vector<FTTWRQueueEntry>::iterator it = m_aEntries.begin();
		it != m_aEntries.end(); ++it)
	{
		// XML-declared generic capacity entries are reconciled by the
		// resource ledger below.  Do not send them through the legacy
		// tier/family contract, which has no meaningful tier value for a
		// generic resource consumer and would freeze them as unsupported_tier.
		if (it->iCapacityCostCount > 0)
			continue;
		if (it->bCapacityConsumed)
		{
			// A completion has already reserved its ledger entry.  Keep that
			// order runnable until the native completion path removes it.
			it->iStatus = FTTWR_QUEUE_STATUS_ACTIVE;
			it->iFreezeReason = FTTWR_QUEUE_REASON_NONE;
			continue;
		}

		bool bAllowed = canReserveCost(it->iProjectFamily,
			it->iRecruitTier, it->iHumanCapacityCost,
			it->iRobotCapacityCost, it->iPopulationCost,
			iHumanBasicAvailable, iHumanAdvancedAvailable,
			iHumanVeteranAvailable, iRobotCapacityAvailable,
			iPopulationAvailable);
		int iReason = FTTWR_QUEUE_REASON_NONE;
		if (!bAllowed)
		{
			if (it->iProjectFamily >= 3)
				iReason = FTTWR_QUEUE_REASON_UNSUPPORTED_FAMILY;
			else if (!isCostContractValid(it->iProjectFamily,
				it->iRecruitTier, it->iHumanCapacityCost,
				it->iRobotCapacityCost))
			{
				if (it->iProjectFamily == 1 &&
					(it->iRecruitTier < FTTWRP1_HUMAN_TIER_BASIC ||
					 it->iRecruitTier > FTTWRP1_HUMAN_TIER_VETERAN))
					iReason = FTTWR_QUEUE_REASON_UNSUPPORTED_TIER;
				else
					iReason = FTTWR_QUEUE_REASON_INVALID_CONTRACT;
			}
			else if (it->iPopulationCost > iPopulationAvailable)
				iReason = FTTWR_QUEUE_REASON_POPULATION;
			else if (it->iRobotCapacityCost > iRobotCapacityAvailable)
				iReason = FTTWR_QUEUE_REASON_ROBOT_CAPACITY;
			else
				iReason = FTTWR_QUEUE_REASON_HUMAN_CAPACITY;
		}

		if (bAllowed)
		{
			if (it->iStatus == FTTWR_QUEUE_STATUS_FROZEN)
			{
				it->iStatus = FTTWR_QUEUE_STATUS_ACTIVE;
				it->iFreezeReason = FTTWR_QUEUE_REASON_NONE;
				it->bRestorePending = true;
				emitTransition(*it);
			}
			else
			{
				it->iStatus = FTTWR_QUEUE_STATUS_ACTIVE;
				it->iFreezeReason = FTTWR_QUEUE_REASON_NONE;
			}
		}
		else
		{
			if (it->iStatus == FTTWR_QUEUE_STATUS_ACTIVE)
			{
				it->iStatus = FTTWR_QUEUE_STATUS_FROZEN;
				it->iFreezeReason = iReason;
				emitTransition(*it);
			}
			else
			{
				it->iFreezeReason = iReason;
			}
		}
	}
}

void FTTWRP1QueueState::reconcileCombined(const int* aiCapacityAvailable,
	int iCapacityCount, int iHumanBasicAvailable,
	int iHumanAdvancedAvailable, int iHumanVeteranAvailable,
	int iRobotCapacityAvailable, int iPopulationAvailable)
{
	if (iCapacityCount < 0 || (iCapacityCount > 0 &&
		aiCapacityAvailable == NULL))
		return;
	for (std::vector<FTTWRQueueEntry>::iterator it = m_aEntries.begin();
		it != m_aEntries.end(); ++it)
	{
		if (it->iCapacityCostCount <= 0 &&
			it->iPopulationCost <= 0 &&
			it->iHumanCapacityCost <= 0 &&
			it->iRobotCapacityCost <= 0)
			continue;
		if (it->bCapacityConsumed)
		{
			it->iStatus = FTTWR_QUEUE_STATUS_ACTIVE;
			it->iFreezeReason = FTTWR_QUEUE_REASON_NONE;
			continue;
		}

		bool const bFamilyCost = (it->iHumanCapacityCost > 0 ||
			it->iRobotCapacityCost > 0);
		bool const bFamilyAllowed = !bFamilyCost ||
			canReserveCost(it->iProjectFamily, it->iRecruitTier,
				it->iHumanCapacityCost, it->iRobotCapacityCost, 0,
				iHumanBasicAvailable, iHumanAdvancedAvailable,
				iHumanVeteranAvailable, iRobotCapacityAvailable, 0);
		bool const bGenericAllowed =
			(it->iCapacityCostCount <= 0 && it->iPopulationCost <= 0) ||
			canReserveCostGeneric(it->iPopulationCost,
				it->iCapacityCostCount, it->iCapacityResource,
				it->iCapacityCost, aiCapacityAvailable, iCapacityCount,
				iPopulationAvailable);
		bool const bAllowed = bFamilyAllowed && bGenericAllowed;
		int iReason = FTTWR_QUEUE_REASON_NONE;
		if (!bAllowed)
		{
			if (bFamilyCost && !isCostContractValid(it->iProjectFamily,
				it->iRecruitTier, it->iHumanCapacityCost,
				it->iRobotCapacityCost))
			{
				if (it->iProjectFamily >= 3)
					iReason = FTTWR_QUEUE_REASON_UNSUPPORTED_FAMILY;
				else if (it->iProjectFamily == 1 &&
					(it->iRecruitTier < FTTWRP1_HUMAN_TIER_BASIC ||
					 it->iRecruitTier > FTTWRP1_HUMAN_TIER_VETERAN))
					iReason = FTTWR_QUEUE_REASON_UNSUPPORTED_TIER;
				else
					iReason = FTTWR_QUEUE_REASON_INVALID_CONTRACT;
			}
			else if (it->iPopulationCost > iPopulationAvailable)
				iReason = FTTWR_QUEUE_REASON_POPULATION;
			else if (bFamilyCost && !bFamilyAllowed)
			{
				if (it->iRobotCapacityCost > iRobotCapacityAvailable)
					iReason = FTTWR_QUEUE_REASON_ROBOT_CAPACITY;
				else
					iReason = FTTWR_QUEUE_REASON_HUMAN_CAPACITY;
			}
			else
				iReason = FTTWR_QUEUE_REASON_RESOURCE_CAPACITY;
		}

		if (bAllowed)
		{
			if (it->iStatus == FTTWR_QUEUE_STATUS_FROZEN)
			{
				it->iStatus = FTTWR_QUEUE_STATUS_ACTIVE;
				it->iFreezeReason = FTTWR_QUEUE_REASON_NONE;
				it->bRestorePending = true;
				emitTransition(*it);
			}
			else
			{
				it->iStatus = FTTWR_QUEUE_STATUS_ACTIVE;
				it->iFreezeReason = FTTWR_QUEUE_REASON_NONE;
			}
		}
		else
		{
			if (it->iStatus == FTTWR_QUEUE_STATUS_ACTIVE)
			{
				it->iStatus = FTTWR_QUEUE_STATUS_FROZEN;
				it->iFreezeReason = iReason;
				emitTransition(*it);
			}
			else
				it->iFreezeReason = iReason;
		}
	}
}

bool FTTWRP1QueueState::canReserveCompletionCombined(int iQueueIndex,
	const int* aiCapacityAvailable, int iCapacityCount,
	int iHumanBasicAvailable, int iHumanAdvancedAvailable,
	int iHumanVeteranAvailable, int iRobotCapacityAvailable,
	int iPopulationAvailable) const
{
	int const iIndex = findEntryByQueueIndex(iQueueIndex);
	if (iIndex < 0)
		return true;
	const FTTWRQueueEntry& kEntry = m_aEntries[iIndex];
	if (kEntry.bCapacityConsumed)
		return true;
	return canReserveCostCombined(kEntry.iProjectFamily,
		kEntry.iRecruitTier, kEntry.iHumanCapacityCost,
		kEntry.iRobotCapacityCost, kEntry.iPopulationCost,
		iHumanBasicAvailable, iHumanAdvancedAvailable,
		iHumanVeteranAvailable, iRobotCapacityAvailable,
		iPopulationAvailable, aiCapacityAvailable, iCapacityCount,
		kEntry.iCapacityCostCount, kEntry.iCapacityResource,
		kEntry.iCapacityCost);
}

bool FTTWRP1QueueState::reserveCompletionCombined(int iQueueIndex,
	const int* aiCapacityAvailable, int iCapacityCount,
	int iHumanBasicAvailable, int iHumanAdvancedAvailable,
	int iHumanVeteranAvailable, int iRobotCapacityAvailable,
	int iPopulationAvailable, int* aiCapacityAllocation,
	int* aiHumanAllocation)
{
	int const iIndex = findEntryByQueueIndex(iQueueIndex);
	if (iIndex < 0)
		return true;
	FTTWRQueueEntry& kEntry = m_aEntries[iIndex];
	if (kEntry.bCapacityConsumed)
	{
		if (aiCapacityAllocation != NULL)
			for (int i = 0; i < FTTWRP1_MAX_CAPACITY_COSTS; ++i)
				aiCapacityAllocation[i] = kEntry.iCapacityAllocation[i];
		if (aiHumanAllocation != NULL)
			for (int i = 0; i < FTTWRP1_HUMAN_TIER_COUNT; ++i)
				aiHumanAllocation[i] = kEntry.iHumanCapacityAllocation[i];
		return true;
	}

	int aiHumanTemp[FTTWRP1_HUMAN_TIER_COUNT];
	int aiCapacityTemp[FTTWRP1_MAX_CAPACITY_COSTS];
	if (!canReserveCompletionCombined(iQueueIndex, aiCapacityAvailable,
		iCapacityCount, iHumanBasicAvailable, iHumanAdvancedAvailable,
		iHumanVeteranAvailable, iRobotCapacityAvailable,
		iPopulationAvailable))
		return false;
	for (int i = 0; i < FTTWRP1_HUMAN_TIER_COUNT; ++i)
		aiHumanTemp[i] = 0;
	for (int i = 0; i < FTTWRP1_MAX_CAPACITY_COSTS; ++i)
		aiCapacityTemp[i] = 0;

	bool const bFamilyCost = (kEntry.iHumanCapacityCost > 0 ||
		kEntry.iRobotCapacityCost > 0);
	if (bFamilyCost)
	{
		allocateHumanCapacity(kEntry.iRecruitTier,
			kEntry.iHumanCapacityCost, iHumanBasicAvailable,
			iHumanAdvancedAvailable, iHumanVeteranAvailable,
			aiHumanTemp);
		for (int i = 0; i < FTTWRP1_HUMAN_TIER_COUNT; ++i)
			m_aiHumanCapacityCommitted[i] += aiHumanTemp[i];
		m_iHumanCapacityCommitted += std::max(0,
			kEntry.iHumanCapacityCost);
		m_iRobotCapacityCommitted += std::max(0,
			kEntry.iRobotCapacityCost);
	}
	for (int i = 0; i < kEntry.iCapacityCostCount; ++i)
	{
		int const iCost = std::max(0, kEntry.iCapacityCost[i]);
		changeCapacityCommitted(kEntry.iCapacityResource[i], iCost);
		aiCapacityTemp[i] = iCost;
	}
	m_iPopulationCommitted += std::max(0, kEntry.iPopulationCost);
	for (int i = 0; i < FTTWRP1_HUMAN_TIER_COUNT; ++i)
	{
		kEntry.iHumanCapacityAllocation[i] = aiHumanTemp[i];
		if (aiHumanAllocation != NULL)
			aiHumanAllocation[i] = aiHumanTemp[i];
	}
	for (int i = 0; i < FTTWRP1_MAX_CAPACITY_COSTS; ++i)
	{
		kEntry.iCapacityAllocation[i] = aiCapacityTemp[i];
		if (aiCapacityAllocation != NULL)
			aiCapacityAllocation[i] = aiCapacityTemp[i];
	}
	kEntry.bCapacityConsumed = true;
	return true;
}
bool FTTWRP1QueueState::canReserveCompletion(int iQueueIndex,
	int iHumanBasicAvailable, int iHumanAdvancedAvailable,
	int iHumanVeteranAvailable, int iRobotCapacityAvailable,
	int iPopulationAvailable) const
{
	int iIndex = findEntryByQueueIndex(iQueueIndex);
	if (iIndex < 0)
		return true;

	const FTTWRQueueEntry& kEntry = m_aEntries[iIndex];
	if (kEntry.bCapacityConsumed)
		return true;

	return canReserveCost(kEntry.iProjectFamily, kEntry.iRecruitTier,
		kEntry.iHumanCapacityCost, kEntry.iRobotCapacityCost,
		kEntry.iPopulationCost, iHumanBasicAvailable,
		iHumanAdvancedAvailable, iHumanVeteranAvailable,
		 iRobotCapacityAvailable, iPopulationAvailable);
}

bool FTTWRP1QueueState::canReserveCompletionGeneric(int iQueueIndex,
	const int* aiCapacityAvailable, int iCapacityCount,
	int iPopulationAvailable) const
{
	int iIndex = findEntryByQueueIndex(iQueueIndex);
	if (iIndex < 0)
		return true;

	const FTTWRQueueEntry& kEntry = m_aEntries[iIndex];
	if (kEntry.bCapacityConsumed)
		return true;
	if (kEntry.iCapacityCostCount <= 0 && kEntry.iPopulationCost <= 0)
		return false;

	return canReserveCostGeneric(kEntry.iPopulationCost,
		kEntry.iCapacityCostCount, kEntry.iCapacityResource,
		kEntry.iCapacityCost, aiCapacityAvailable, iCapacityCount,
		iPopulationAvailable);
}

bool FTTWRP1QueueState::reserveCost(int iProjectFamily, int iRecruitTier,
	int iHumanCapacityCost, int iRobotCapacityCost, int iPopulationCost,
	int iHumanBasicAvailable, int iHumanAdvancedAvailable,
	int iHumanVeteranAvailable, int iRobotCapacityAvailable,
	int iPopulationAvailable, int* aiHumanAllocation)
{
	if (!canReserveCost(iProjectFamily, iRecruitTier, iHumanCapacityCost,
		iRobotCapacityCost, iPopulationCost, iHumanBasicAvailable,
		iHumanAdvancedAvailable, iHumanVeteranAvailable,
		iRobotCapacityAvailable, iPopulationAvailable))
		return false;

	int aiAllocation[FTTWRP1_HUMAN_TIER_COUNT];
	allocateHumanCapacity(iRecruitTier, std::max(0, iHumanCapacityCost),
		iHumanBasicAvailable, iHumanAdvancedAvailable,
		iHumanVeteranAvailable, aiAllocation);
	for (int i = 0; i < FTTWRP1_HUMAN_TIER_COUNT; ++i)
	{
		m_aiHumanCapacityCommitted[i] += aiAllocation[i];
		if (aiHumanAllocation != NULL)
			aiHumanAllocation[i] = aiAllocation[i];
	}
	m_iHumanCapacityCommitted += std::max(0, iHumanCapacityCost);
	m_iRobotCapacityCommitted += std::max(0, iRobotCapacityCost);
	m_iPopulationCommitted += std::max(0, iPopulationCost);
	return true;
}

bool FTTWRP1QueueState::reserveCostGeneric(int iProjectFamily,
	int iPopulationCost, int iCapacityCostCount,
	const int* aiCapacityResource, const int* aiCapacityCost,
	const int* aiCapacityAvailable, int iCapacityCount,
	int iPopulationAvailable, int* aiCapacityAllocation)
{
	// The project family is retained in the API for diagnostics and future
	// policy hooks, but an explicit XML capacity list is family-agnostic.
	(void)iProjectFamily;
	if (!canReserveCostGeneric(iPopulationCost, iCapacityCostCount,
		aiCapacityResource, aiCapacityCost, aiCapacityAvailable,
		iCapacityCount, iPopulationAvailable))
		return false;

	if (aiCapacityAllocation != NULL)
	{
		for (int i = 0; i < FTTWRP1_MAX_CAPACITY_COSTS; ++i)
			aiCapacityAllocation[i] = 0;
	}

	// Aggregate duplicate resource entries through the same commitment ledger
	// used by distinct entries.  The XML list is intentionally small and
	// bounded, so no fixed resource enum or per-resource DLL branch is needed.
	for (int i = 0; i < iCapacityCostCount; ++i)
	{
		int iCost = std::max(0, aiCapacityCost[i]);
		changeCapacityCommitted(aiCapacityResource[i], iCost);
		if (aiCapacityAllocation != NULL)
			aiCapacityAllocation[i] = iCost;
	}
	m_iPopulationCommitted += std::max(0, iPopulationCost);
	return true;
}

bool FTTWRP1QueueState::reserveCompletion(int iQueueIndex,
	int iHumanBasicAvailable, int iHumanAdvancedAvailable,
	int iHumanVeteranAvailable, int iRobotCapacityAvailable,
	int iPopulationAvailable, int* aiHumanAllocation)
{
	int iIndex = findEntryByQueueIndex(iQueueIndex);
	if (iIndex < 0)
		return true;

	FTTWRQueueEntry& kEntry = m_aEntries[iIndex];
	if (kEntry.bCapacityConsumed)
	{
		if (aiHumanAllocation != NULL)
		{
			for (int i = 0; i < FTTWRP1_HUMAN_TIER_COUNT; ++i)
				aiHumanAllocation[i] = kEntry.iHumanCapacityAllocation[i];
		}
		return true;
	}

	int aiAllocation[FTTWRP1_HUMAN_TIER_COUNT];
	if (!reserveCost(kEntry.iProjectFamily, kEntry.iRecruitTier,
		kEntry.iHumanCapacityCost, kEntry.iRobotCapacityCost,
		kEntry.iPopulationCost, iHumanBasicAvailable,
		iHumanAdvancedAvailable, iHumanVeteranAvailable,
		iRobotCapacityAvailable, iPopulationAvailable, aiAllocation))
		return false;

	for (int i = 0; i < FTTWRP1_HUMAN_TIER_COUNT; ++i)
	{
		kEntry.iHumanCapacityAllocation[i] = aiAllocation[i];
		if (aiHumanAllocation != NULL)
			aiHumanAllocation[i] = aiAllocation[i];
	}
	kEntry.bCapacityConsumed = true;
	return true;
}

bool FTTWRP1QueueState::reserveCompletionGeneric(int iQueueIndex,
	const int* aiCapacityAvailable, int iCapacityCount,
	int iPopulationAvailable, int* aiCapacityAllocation)
{
	int iIndex = findEntryByQueueIndex(iQueueIndex);
	if (iIndex < 0)
		return true;

	FTTWRQueueEntry& kEntry = m_aEntries[iIndex];
	if (kEntry.bCapacityConsumed)
	{
		// A queue reservation may have been made when the order was inserted.
		// Completion transfers that exact allocation to the resulting unit.
		if (aiCapacityAllocation != NULL)
		{
			for (int i = 0; i < FTTWRP1_MAX_CAPACITY_COSTS; ++i)
				aiCapacityAllocation[i] = kEntry.iCapacityAllocation[i];
		}
		return true;
	}
	if (kEntry.iCapacityCostCount <= 0 && kEntry.iPopulationCost <= 0)
		return false;

	int aiAllocation[FTTWRP1_MAX_CAPACITY_COSTS];
	if (!reserveCostGeneric(kEntry.iProjectFamily,
		kEntry.iPopulationCost, kEntry.iCapacityCostCount,
		kEntry.iCapacityResource, kEntry.iCapacityCost,
		aiCapacityAvailable, iCapacityCount, iPopulationAvailable,
		aiAllocation))
		return false;

	for (int i = 0; i < FTTWRP1_MAX_CAPACITY_COSTS; ++i)
	{
		kEntry.iCapacityAllocation[i] = aiAllocation[i];
		if (aiCapacityAllocation != NULL)
			aiCapacityAllocation[i] = aiAllocation[i];
	}
	kEntry.bCapacityConsumed = true;
	return true;
}

void FTTWRP1QueueState::releaseCost(int iProjectFamily,
	int iHumanCapacityCost, int iRobotCapacityCost)
{
	if (iProjectFamily == 1)
	{
		// Legacy callers do not carry the source-tier split.  Release from the
		// lowest tier first; all current BASIC-only saves are represented this
		// way, while tier-aware callers use the overload below.
		int iRemaining = std::max(0, iHumanCapacityCost);
		for (int i = 0; i < FTTWRP1_HUMAN_TIER_COUNT && iRemaining > 0; ++i)
		{
			int iTake = std::min(iRemaining, m_aiHumanCapacityCommitted[i]);
			m_aiHumanCapacityCommitted[i] -= iTake;
			iRemaining -= iTake;
		}
		m_iHumanCapacityCommitted = std::max(0,
			m_iHumanCapacityCommitted - std::max(0, iHumanCapacityCost));
	}
	else if (iProjectFamily == 2)
		m_iRobotCapacityCommitted = std::max(0,
			m_iRobotCapacityCommitted - std::max(0, iRobotCapacityCost));
}

void FTTWRP1QueueState::releaseCost(int iProjectFamily,
	int iHumanBasicCapacity, int iHumanAdvancedCapacity,
	int iHumanVeteranCapacity, int iRobotCapacity)
{
	if (iProjectFamily == 1)
	{
		int aiRelease[FTTWRP1_HUMAN_TIER_COUNT];
		aiRelease[0] = std::max(0, iHumanBasicCapacity);
		aiRelease[1] = std::max(0, iHumanAdvancedCapacity);
		aiRelease[2] = std::max(0, iHumanVeteranCapacity);
		int iReleased = 0;
		for (int i = 0; i < FTTWRP1_HUMAN_TIER_COUNT; ++i)
		{
			int iTake = std::min(aiRelease[i], m_aiHumanCapacityCommitted[i]);
			m_aiHumanCapacityCommitted[i] -= iTake;
			iReleased += iTake;
		}
		m_iHumanCapacityCommitted = std::max(0,
			m_iHumanCapacityCommitted - iReleased);
	}
	else if (iProjectFamily == 2)
	{
		m_iRobotCapacityCommitted = std::max(0,
			m_iRobotCapacityCommitted - std::max(0, iRobotCapacity));
	}
}

void FTTWRP1QueueState::releaseCostGeneric(int iCapacityCostCount,
	const int* aiCapacityResource, const int* aiCapacityCost)
{
	if (iCapacityCostCount <= 0 ||
		iCapacityCostCount > FTTWRP1_MAX_CAPACITY_COSTS ||
		aiCapacityResource == NULL || aiCapacityCost == NULL)
		return;

	for (int i = 0; i < iCapacityCostCount; ++i)
	{
		if (aiCapacityResource[i] >= 0 && aiCapacityCost[i] > 0)
			changeCapacityCommitted(aiCapacityResource[i],
				-std::max(0, aiCapacityCost[i]));
	}
}

void FTTWRP1QueueState::releasePopulationCost(int iPopulationCost)
{
	m_iPopulationCommitted = std::max(0, m_iPopulationCommitted -
		std::max(0, iPopulationCost));
}

void FTTWRP1QueueState::reconcileGeneric(const int* aiCapacityAvailable,
	int iCapacityCount, int iPopulationAvailable)
{
	if (iCapacityCount < 0 || (iCapacityCount > 0 &&
		aiCapacityAvailable == NULL))
		return;

	for (std::vector<FTTWRQueueEntry>::iterator it = m_aEntries.begin();
		it != m_aEntries.end(); ++it)
	{
		if ((it->iCapacityCostCount <= 0 && it->iPopulationCost <= 0) ||
			it->iHumanCapacityCost > 0 || it->iRobotCapacityCost > 0)
			continue;
		if (it->bCapacityConsumed)
		{
			it->iStatus = FTTWR_QUEUE_STATUS_ACTIVE;
			it->iFreezeReason = FTTWR_QUEUE_REASON_NONE;
			continue;
		}

		bool bAllowed = canReserveCostGeneric(it->iPopulationCost,
			it->iCapacityCostCount, it->iCapacityResource,
			it->iCapacityCost, aiCapacityAvailable, iCapacityCount,
			iPopulationAvailable);
		if (bAllowed)
		{
			if (it->iStatus == FTTWR_QUEUE_STATUS_FROZEN)
			{
				it->iStatus = FTTWR_QUEUE_STATUS_ACTIVE;
				it->iFreezeReason = FTTWR_QUEUE_REASON_NONE;
				it->bRestorePending = true;
				emitTransition(*it);
			}
			else
			{
				it->iStatus = FTTWR_QUEUE_STATUS_ACTIVE;
				it->iFreezeReason = FTTWR_QUEUE_REASON_NONE;
			}
		}
		else
		{
			it->iFreezeReason = (it->iPopulationCost > iPopulationAvailable) ?
				FTTWR_QUEUE_REASON_POPULATION :
				FTTWR_QUEUE_REASON_RESOURCE_CAPACITY;
			if (it->iStatus == FTTWR_QUEUE_STATUS_ACTIVE)
			{
				it->iStatus = FTTWR_QUEUE_STATUS_FROZEN;
				emitTransition(*it);
			}
		}
	}
}

void FTTWRP1QueueState::releaseCompletion(int iQueueIndex)
{
	int iIndex = findEntryByQueueIndex(iQueueIndex);
	if (iIndex < 0 || !m_aEntries[iIndex].bCapacityConsumed)
		return;

	FTTWRQueueEntry& kEntry = m_aEntries[iIndex];
	if (kEntry.iCapacityCostCount > 0)
	{
		releaseCostGeneric(kEntry.iCapacityCostCount,
			kEntry.iCapacityResource, kEntry.iCapacityAllocation);
	}
	if (kEntry.iHumanCapacityCost > 0 || kEntry.iRobotCapacityCost > 0)
	{
		releaseCost(kEntry.iProjectFamily,
			kEntry.iHumanCapacityAllocation[0],
			kEntry.iHumanCapacityAllocation[1],
			kEntry.iHumanCapacityAllocation[2],
			kEntry.iRobotCapacityCost);
	}
	if (kEntry.iPopulationCost > 0)
		releasePopulationCost(kEntry.iPopulationCost);
	for (int i = 0; i < FTTWRP1_HUMAN_TIER_COUNT; ++i)
		kEntry.iHumanCapacityAllocation[i] = 0;
	for (int i = 0; i < FTTWRP1_MAX_CAPACITY_COSTS; ++i)
		kEntry.iCapacityAllocation[i] = 0;
	kEntry.bCapacityConsumed = false;
}
void FTTWRP1QueueState::releaseUnitLease(int iHumanBasicCapacity,
	int iHumanAdvancedCapacity, int iHumanVeteranCapacity,
	int iRobotCapacity)
{
	releaseCost(1, iHumanBasicCapacity, iHumanAdvancedCapacity,
		iHumanVeteranCapacity, 0);
	releaseCost(2, 0, 0, 0, iRobotCapacity);
}

void FTTWRP1QueueState::releaseUnitLease(int iHumanCapacityCost, int iRobotCapacityCost)
{
	releaseCost(1, iHumanCapacityCost, 0, 0, 0);
	releaseCost(2, 0, 0, 0, iRobotCapacityCost);
}

int FTTWRP1QueueState::getHumanCapacityCommitted() const
{
	return m_iHumanCapacityCommitted;
}

int FTTWRP1QueueState::getHumanCapacityCommitted(int iTier) const
{
	if (iTier < FTTWRP1_HUMAN_TIER_BASIC ||
		iTier > FTTWRP1_HUMAN_TIER_VETERAN)
		return 0;
	return m_aiHumanCapacityCommitted[iTier - 1];
}

int FTTWRP1QueueState::getRobotCapacityCommitted() const
{
	return m_iRobotCapacityCommitted;
}

int FTTWRP1QueueState::getPopulationCommitted() const
{
	return m_iPopulationCommitted;
}

int FTTWRP1QueueState::findEntryByQueueIndex(int iQueueIndex) const
{
	for (int i = 0; i < (int)m_aEntries.size(); ++i)
	{
		if (m_aEntries[i].iCurrentQueueIndex == iQueueIndex)
			return i;
	}
	return -1;
}

bool FTTWRP1QueueState::canAdvance(int iQueueIndex) const
{
	int iIndex = findEntryByQueueIndex(iQueueIndex);
	return (iIndex < 0 || m_aEntries[iIndex].iStatus != FTTWR_QUEUE_STATUS_FROZEN);
}

int FTTWRP1QueueState::getTrackedCount() const
{
	return (int)m_aEntries.size();
}

int FTTWRP1QueueState::getActiveCount() const
{
	int iCount = 0;
	for (std::vector<FTTWRQueueEntry>::const_iterator it = m_aEntries.begin();
		it != m_aEntries.end(); ++it)
	{
		if (it->iStatus == FTTWR_QUEUE_STATUS_ACTIVE)
			++iCount;
	}
	return iCount;
}

int FTTWRP1QueueState::getFrozenCount() const
{
	return getTrackedCount() - getActiveCount();
}

int FTTWRP1QueueState::getRestorePendingCount() const
{
	int iCount = 0;
	for (std::vector<FTTWRQueueEntry>::const_iterator it = m_aEntries.begin();
		it != m_aEntries.end(); ++it)
	{
		if (it->bRestorePending)
			++iCount;
	}
	return iCount;
}

int FTTWRP1QueueState::getNotificationCount() const
{
	return (int)m_aNotifications.size();
}

int FTTWRP1QueueState::getFirstRestoredQueueIndex() const
{
	if (!m_bRestorePriority)
		return -1;

	int iBest = -1;
	for (std::vector<FTTWRQueueEntry>::const_iterator it = m_aEntries.begin();
		it != m_aEntries.end(); ++it)
	{
		if (it->bRestorePending && it->iStatus == FTTWR_QUEUE_STATUS_ACTIVE &&
			(iBest < 0 || it->iCurrentQueueIndex < iBest))
		{
			iBest = it->iCurrentQueueIndex;
		}
	}
	return iBest;
}

const char* FTTWRP1QueueState::statusName(int iStatus) const
{
	return (iStatus == FTTWR_QUEUE_STATUS_FROZEN) ? "FROZEN" : "ACTIVE";
}

const char* FTTWRP1QueueState::reasonName(int iReason) const
{
	switch (iReason)
	{
	case FTTWR_QUEUE_REASON_HUMAN_CAPACITY: return "human_capacity";
	case FTTWR_QUEUE_REASON_ROBOT_CAPACITY: return "robot_capacity";
	case FTTWR_QUEUE_REASON_POPULATION: return "population";
	case FTTWR_QUEUE_REASON_UNSUPPORTED_FAMILY: return "unsupported_family";
	case FTTWR_QUEUE_REASON_UNSUPPORTED_TIER: return "unsupported_tier";
	case FTTWR_QUEUE_REASON_INVALID_CONTRACT: return "invalid_contract";
	case FTTWR_QUEUE_REASON_RESOURCE_CAPACITY: return "resource_capacity";
	default: return "none";
	}
}

std::string FTTWRP1QueueState::getSnapshot(int iCityId) const
{
	char szBuffer[2048];
	std::string szResult;
	sprintf(szBuffer,
		"{\"contract\":\"p1-recruit-queue-native-v1\",\"state_version\":5,\"city_id\":%d,\"restore_priority\":%s,\"human_capacity_committed\":%d,\"human_capacity_committed_by_tier\":{\"BASIC\":%d,\"ADVANCED\":%d,\"VETERAN\":%d},\"robot_capacity_committed\":%d,\"population_committed\":%d,\"capacity_commitments\":[",
		iCityId, m_bRestorePriority ? "true" : "false",
		m_iHumanCapacityCommitted, m_aiHumanCapacityCommitted[0],
		m_aiHumanCapacityCommitted[1], m_aiHumanCapacityCommitted[2],
		m_iRobotCapacityCommitted, m_iPopulationCommitted);
	szResult += szBuffer;
	for (std::vector<FTTWRCapacityCommitment>::const_iterator itCommit =
		m_aCapacityCommitted.begin(); itCommit != m_aCapacityCommitted.end();
		++itCommit)
	{
		if (itCommit != m_aCapacityCommitted.begin())
			szResult += ",";
		sprintf(szBuffer, "{\"resource\":%d,\"committed\":%d}",
			itCommit->iResource, itCommit->iCommitted);
		szResult += szBuffer;
	}
	szResult += "],\"tracked_count\":";
	sprintf(szBuffer, "%d,\"active_count\":%d,\"frozen_count\":%d,\"restored_pending_count\":%d,\"notification_count\":%d,\"entries\":[",
		getTrackedCount(), getActiveCount(), getFrozenCount(),
		getRestorePendingCount(), getNotificationCount());
	szResult += szBuffer;

	for (std::vector<FTTWRQueueEntry>::const_iterator it = m_aEntries.begin();
		it != m_aEntries.end(); ++it)
	{
		if (it != m_aEntries.begin())
			szResult += ",";
		sprintf(szBuffer,
			"{\"serial\":%d,\"order_type\":%d,\"data1\":%d,\"data2\":%d,\"queue_index\":%d,\"original_queue_index\":%d,\"progress\":%d,\"human_tier\":%d,\"population_cost\":%d,\"capacity_cost_count\":%d,\"capacity_consumed\":%s,\"status\":\"%s\",\"freeze_reason\":\"%s\"}",
			it->iSerial, it->iOrderType, it->iData1, it->iData2,
			it->iCurrentQueueIndex, it->iOriginalQueueIndex, it->iProgress,
			it->iRecruitTier, it->iPopulationCost, it->iCapacityCostCount,
			it->bCapacityConsumed ? "true" : "false",
			statusName(it->iStatus), reasonName(it->iFreezeReason));
		szResult += szBuffer;
	}
	szResult += "]}";
	return szResult;
}

std::string FTTWRP1QueueState::consumeNotifications(int iCityId)
{
	char szBuffer[256];
	std::string szResult;
	sprintf(szBuffer, "{\"contract\":\"p1-recruit-queue-native-v1\",\"city_id\":%d,\"notifications\":[", iCityId);
	szResult += szBuffer;

	for (std::vector<FTTWRQueueNotification>::const_iterator it = m_aNotifications.begin();
		it != m_aNotifications.end(); ++it)
	{
		if (it != m_aNotifications.begin())
			szResult += ",";
		sprintf(szBuffer, "{\"serial\":%d,\"status\":\"%s\",\"freeze_reason\":\"%s\"}",
			it->iSerial, statusName(it->iStatus), reasonName(it->iFreezeReason));
		szResult += szBuffer;
	}
	szResult += "]}";
	m_aNotifications.clear();
	return szResult;
}

void FTTWRP1QueueState::write(FDataStreamBase* pStream) const
{
	int iVersion = FTTWRP1_QUEUE_SAVE_VERSION;
	pStream->Write(iVersion);
	pStream->Write(m_bRestorePriority);
	pStream->Write(m_iNextSerial);
	pStream->Write(m_iHumanCapacityCommitted);
	pStream->Write(m_iRobotCapacityCommitted);
	pStream->Write(m_iPopulationCommitted);
	for (int i = 0; i < FTTWRP1_HUMAN_TIER_COUNT; ++i)
		pStream->Write(m_aiHumanCapacityCommitted[i]);

	int iCommitmentCount = (int)m_aCapacityCommitted.size();
	if (iCommitmentCount > FTTWRP1_QUEUE_MAX_CAPACITY_COMMITMENTS)
		iCommitmentCount = FTTWRP1_QUEUE_MAX_CAPACITY_COMMITMENTS;
	pStream->Write(iCommitmentCount);
	for (int i = 0; i < iCommitmentCount; ++i)
	{
		pStream->Write(m_aCapacityCommitted[i].iResource);
		pStream->Write(m_aCapacityCommitted[i].iCommitted);
	}

	int iCount = (int)m_aEntries.size();
	if (iCount > FTTWRP1_QUEUE_MAX_TRACKED)
		iCount = FTTWRP1_QUEUE_MAX_TRACKED;
	pStream->Write(iCount);
	for (int i = 0; i < iCount; ++i)
	{
		const FTTWRQueueEntry& kEntry = m_aEntries[i];
		pStream->Write(kEntry.iSerial);
		pStream->Write(kEntry.iOrderType);
		pStream->Write(kEntry.iData1);
		pStream->Write(kEntry.iData2);
		pStream->Write(kEntry.iOriginalQueueIndex);
		pStream->Write(kEntry.iCurrentQueueIndex);
		pStream->Write(kEntry.iProgress);
		pStream->Write(kEntry.iProjectFamily);
		pStream->Write(kEntry.iRecruitTier);
		pStream->Write(kEntry.iHumanCapacityCost);
		pStream->Write(kEntry.iRobotCapacityCost);
		pStream->Write(kEntry.iPopulationCost);
		pStream->Write(kEntry.iCapacityCostCount);
		for (int j = 0; j < FTTWRP1_MAX_CAPACITY_COSTS; ++j)
		{
			pStream->Write(kEntry.iCapacityResource[j]);
			pStream->Write(kEntry.iCapacityCost[j]);
			pStream->Write(kEntry.iCapacityAllocation[j]);
		}
		for (int j = 0; j < FTTWRP1_HUMAN_TIER_COUNT; ++j)
			pStream->Write(kEntry.iHumanCapacityAllocation[j]);
		pStream->Write(kEntry.iStatus);
		pStream->Write(kEntry.iFreezeReason);
		pStream->Write(kEntry.bCapacityConsumed);
		// Restore pending is derived from the post-load reconciliation and is
		// intentionally not persisted as a stale notification.
	}
}

void FTTWRP1QueueState::read(FDataStreamBase* pStream)
{
	reset();

	int iVersion = 0;
	pStream->Read(&iVersion);
	if (iVersion <= 0)
		return;

	pStream->Read(&m_bRestorePriority);
	pStream->Read(&m_iNextSerial);
	if (m_iNextSerial <= 0)
		m_iNextSerial = 1;
	if (iVersion >= 2)
	{
		pStream->Read(&m_iHumanCapacityCommitted);
		pStream->Read(&m_iRobotCapacityCommitted);
		if (iVersion >= 5)
			pStream->Read(&m_iPopulationCommitted);
		m_iHumanCapacityCommitted = std::max(0, m_iHumanCapacityCommitted);
		m_iRobotCapacityCommitted = std::max(0, m_iRobotCapacityCommitted);
		m_iPopulationCommitted = std::max(0, m_iPopulationCommitted);
		if (iVersion >= 3)
		{
			for (int i = 0; i < FTTWRP1_HUMAN_TIER_COUNT; ++i)
			{
				pStream->Read(&m_aiHumanCapacityCommitted[i]);
				m_aiHumanCapacityCommitted[i] = std::max(0,
					m_aiHumanCapacityCommitted[i]);
			}
			m_iHumanCapacityCommitted = 0;
			for (int i = 0; i < FTTWRP1_HUMAN_TIER_COUNT; ++i)
				m_iHumanCapacityCommitted += m_aiHumanCapacityCommitted[i];
		}
		else
		{
			m_aiHumanCapacityCommitted[0] = m_iHumanCapacityCommitted;
		}
	}
	if (iVersion >= 4)
	{
		int iRawCommitmentCount = 0;
		pStream->Read(&iRawCommitmentCount);
		if (iRawCommitmentCount < 0)
			iRawCommitmentCount = 0;
		for (int i = 0; i < iRawCommitmentCount; ++i)
		{
			FTTWRCapacityCommitment kCommitment;
			pStream->Read(&kCommitment.iResource);
			pStream->Read(&kCommitment.iCommitted);
			if (i < FTTWRP1_QUEUE_MAX_CAPACITY_COMMITMENTS &&
				kCommitment.iResource >= 0 && kCommitment.iCommitted > 0)
			{
				m_aCapacityCommitted.push_back(kCommitment);
			}
		}
	}

	int iRawCount = 0;
	pStream->Read(&iRawCount);
	if (iRawCount < 0)
		iRawCount = 0;

	// Valid writers never exceed MAX_TRACKED.  Read and discard any excess
	// records so the following game data remains aligned on a malformed save.
	for (int i = 0; i < iRawCount; ++i)
	{
		FTTWRQueueEntry kEntry;
		pStream->Read(&kEntry.iSerial);
		pStream->Read(&kEntry.iOrderType);
		pStream->Read(&kEntry.iData1);
		pStream->Read(&kEntry.iData2);
		pStream->Read(&kEntry.iOriginalQueueIndex);
		pStream->Read(&kEntry.iCurrentQueueIndex);
		pStream->Read(&kEntry.iProgress);
		pStream->Read(&kEntry.iProjectFamily);
		pStream->Read(&kEntry.iRecruitTier);
		pStream->Read(&kEntry.iHumanCapacityCost);
		pStream->Read(&kEntry.iRobotCapacityCost);
		pStream->Read(&kEntry.iPopulationCost);
		if (iVersion >= 4)
		{
			pStream->Read(&kEntry.iCapacityCostCount);
			if (kEntry.iCapacityCostCount < 0 ||
				kEntry.iCapacityCostCount > FTTWRP1_MAX_CAPACITY_COSTS)
				kEntry.iCapacityCostCount = 0;
			for (int j = 0; j < FTTWRP1_MAX_CAPACITY_COSTS; ++j)
			{
				pStream->Read(&kEntry.iCapacityResource[j]);
				pStream->Read(&kEntry.iCapacityCost[j]);
				pStream->Read(&kEntry.iCapacityAllocation[j]);
				if (kEntry.iCapacityResource[j] < 0 ||
					kEntry.iCapacityCost[j] < 0 ||
					kEntry.iCapacityAllocation[j] < 0)
				{
					kEntry.iCapacityResource[j] = -1;
					kEntry.iCapacityCost[j] = 0;
					kEntry.iCapacityAllocation[j] = 0;
				}
			}
		}
		for (int j = 0; j < FTTWRP1_HUMAN_TIER_COUNT; ++j)
			kEntry.iHumanCapacityAllocation[j] = 0;
		if (iVersion >= 3)
		{
			for (int j = 0; j < FTTWRP1_HUMAN_TIER_COUNT; ++j)
			{
				pStream->Read(&kEntry.iHumanCapacityAllocation[j]);
				kEntry.iHumanCapacityAllocation[j] = std::max(0,
					kEntry.iHumanCapacityAllocation[j]);
			}
		}
		pStream->Read(&kEntry.iStatus);
		pStream->Read(&kEntry.iFreezeReason);
		pStream->Read(&kEntry.bCapacityConsumed);
		if (iVersion < 3 && kEntry.bCapacityConsumed &&
			kEntry.iHumanCapacityCost > 0)
		{
			// Version 2 had no source-tier split and only admitted BASIC.
			kEntry.iHumanCapacityAllocation[0] =
				std::max(0, kEntry.iHumanCapacityCost);
		}
		kEntry.bRestorePending = false;

		if (i < FTTWRP1_QUEUE_MAX_TRACKED &&
			kEntry.iCurrentQueueIndex >= 0 &&
			kEntry.iOriginalQueueIndex >= 0 &&
			(kEntry.iStatus == FTTWR_QUEUE_STATUS_ACTIVE ||
			 kEntry.iStatus == FTTWR_QUEUE_STATUS_FROZEN))
		{
			m_aEntries.push_back(kEntry);
		}
	}
}
