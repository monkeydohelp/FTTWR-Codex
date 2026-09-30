// advc: New class; see CvDLLLogger.h.

#include "CvGameCoreDLL.h"
#include "CvGame.h"
#include "CvPlayer.h"
#include "CvCity.h"
#include "CvUnit.h"
#include "CvSelectionGroup.h"
#include "CvSelectionGroupAI.h"
#include "CvUnitAI.h"
#include "CvTeamAI.h"
#include "CvInfo_Build.h"
#include "LinkedListTraversal.h"
// <advc.mapstat>
#include "CvMap.h"
#include "CvArea.h"
#include "CvInfo_Terrain.h"
// </advc.mapstat>
#include "AgentIterator.h" // advc.tsl

CvDLLLogger::CvDLLLogger(bool bEnabled, bool bRandEnabled)
:	m_iWorkerOrderTraceDepth(0), m_bEnabled(bEnabled), m_bRandEnabled(bRandEnabled) {}

// Cut from CvRandom::getInt
void CvDLLLogger::logRandomNumber(const TCHAR* szMsg, unsigned short usNum,
	unsigned int uiSeed, int iData1, int iData2,
	CvString const* pszFileName) // advc.007b
{
	FAssert(isEnabledRand()); // Caller should handle this, for performance reasons.
	if (szMsg == NULL)
		return;
	int const iTurnSlice = GC.getGame().getTurnSlice();
	if (iTurnSlice <= 0)
		return;
	TCHAR szOut[1024];
	// <advc.007>
	CvString szData;
	if (iData1 > MIN_INT)
	{
		if(iData2 == MIN_INT)
			szData.Format(" (%d)", iData1);
		else szData.Format(" (%d, %d)", iData1, iData2);
	}
	bool bNetworkMP = GC.getGame().isNetworkMultiPlayer();
	/*  Don't show iTurnSlice in singleplayer b/c that makes it harder to
		compare log files. */
	int iOn = iTurnSlice;
	if (!bNetworkMP)
		iOn = GC.getGame().getGameTurn(); // (any more useful info to put here?)
	// The second and last %s are new
	std::sprintf(szOut, "Rand = %u / %hu (%s%s) on %s%d\n", uiSeed, usNum,
			szMsg, szData.c_str(), bNetworkMP ? "" : "t", iOn);
	// <advc.007b>
	if (pszFileName != NULL)
		gDLL->logMsg(pszFileName->c_str(), szOut, false, false);
	else // </advc.007b>
	if (GC.getDefineBOOL(CvGlobals::PER_PLAYER_MESSAGE_CONTROL_LOG) && bNetworkMP)
	{
		CvString logName = CvString::format("MPLog%d.log",
				(int)GC.getGame().getActivePlayer());
		gDLL->logMsg(logName.c_str(), szOut, false, false);
	}
	else // </advc.007>
		gDLL->messageControlLog(szOut);
}

// Cut from CvPlayer::setTurnActive
void CvDLLLogger::logTurnActive(PlayerTypes ePlayer)
{
	if (!isEnabled()) // || gDLL->getChtLvl() <= 0) // advc.007
		return;
	TCHAR szOut[1024];
	std::sprintf(szOut, "Player %d Turn ON\n", ePlayer);
	gDLL->messageControlLog(szOut);
}

// Cut from CvCity::init
void CvDLLLogger::logCityBuilt(CvCity const& kCity)
{
	if (!isEnabled()) //|| gDLL->getChtLvl() <= 0) // advc.007
		return;
	TCHAR szOut[1024];
	std::sprintf(szOut, "Player %d City %d built at %d:%d\n", kCity.getOwner(),
			kCity.getID(), kCity.getX(), kCity.getY());
	gDLL->messageControlLog(szOut);
}

// Cut from CvUnit::setCombatUnit
void CvDLLLogger::logCombat(CvUnit const& kAttacker, CvUnit const& kDefender)
{
	if (!isEnabled()) //|| gDLL->getChtLvl() <= 0) // advc.007
		return;
	char szOut[1024];
	std::sprintf( szOut, "*** KOMBAT!\n     ATTACKER: Player %d Unit %d (%S's %S), CombatStrength=%d\n"
		"     DEFENDER: Player %d Unit %d (%S's %S), CombatStrength=%d\n",
			kAttacker.getOwner(), kAttacker.getID(), GET_PLAYER(kAttacker.getOwner()).getName(), kAttacker.getName().GetCString(), kAttacker.currCombatStr(NULL, NULL),
			kDefender.getOwner(), kDefender.getID(), GET_PLAYER(kDefender.getOwner()).getName(), kDefender.getName().GetCString(), kDefender.currCombatStr(kDefender.plot(), &kAttacker));
	gDLL->messageControlLog(szOut);
}

// <fttwr.workerloopdiag>
// Scope order logging to the final worker updates immediately before the loop watchdog.
bool CvDLLLogger::beginWorkerOrderTrace(CvSelectionGroup const& kGroup,
	int iAttempt)
{
	if (m_iWorkerOrderTraceDepth >= WORKER_ORDER_TRACE_MAX_DEPTH)
		return false;
	CvUnitAI const* pWorker = kGroup.AI().AI_getHeadUnit();
	if (pWorker == NULL || pWorker->AI_getUnitAIType() != UNITAI_WORKER)
		return false;
	WorkerOrderTraceContext& kContext =
			m_aWorkerOrderTrace[m_iWorkerOrderTraceDepth++];
	kContext.iGroupID = kGroup.getID();
	kContext.iOwner = kGroup.getOwner();
	kContext.iAttempt = iAttempt;
	return true;
}

void CvDLLLogger::endWorkerOrderTrace(bool bPushed)
{
	if (bPushed && m_iWorkerOrderTraceDepth > 0)
		m_iWorkerOrderTraceDepth--;
}

void CvDLLLogger::logWorkerMissionPush(CvSelectionGroup const& kGroup,
	MissionTypes eMission, int iData1, int iData2, MovementFlags eFlags,
	bool bAppend, bool bManual, MissionAITypes eMissionAI,
	CvPlot const* pMissionAIPlot, CvUnit const* pMissionAIUnit,
	bool bModified)
{
	if (m_iWorkerOrderTraceDepth <= 0)
		return;
	WorkerOrderTraceContext const& kContext =
			m_aWorkerOrderTrace[m_iWorkerOrderTraceDepth - 1];
	if (kContext.iGroupID != kGroup.getID() ||
		kContext.iOwner != (int)kGroup.getOwner())
		return;
	CvUnitAI const* pWorker = kGroup.AI().AI_getHeadUnit();
	if (pWorker == NULL || pWorker->AI_getUnitAIType() != UNITAI_WORKER)
		return;
	CvPlot const* pPlot = pWorker->plot();
	int iBuild = -1;
	int iBuildCanBuild = -1;
	if (eMission == MISSION_BUILD)
	{
		iBuild = iData1;
		if (pPlot != NULL && iBuild >= 0 && iBuild < GC.getNumBuildInfos())
			iBuildCanBuild = pWorker->canBuild(*pPlot,
				(BuildTypes)iBuild, false, false);
	}
	CvPlayer const& kOwner = GET_PLAYER(kGroup.getOwner());
	char szOut[1024];
	std::sprintf(szOut,
			"  worker-order attempt=%d turn=%d slice=%d owner=%d group=%d unit=%d from=%d,%d moves=%d mission=%d data=%d,%d flags=%d append=%d manual=%d modified=%d missionAI=%d target=%d,%d targetUnit=%d currentBuild=%d buildId=%d buildCanBuild=%d gold=%d\n",
			kContext.iAttempt, GC.getGame().getGameTurn(),
			GC.getGame().getTurnSlice(), (int)kGroup.getOwner(),
			kGroup.getID(), pWorker->getID(), pWorker->getX(), pWorker->getY(),
			pWorker->movesLeft(), (int)eMission, iData1, iData2,
			(int)eFlags, (int)bAppend, (int)bManual, (int)bModified,
			(int)eMissionAI,
			(pMissionAIPlot != NULL ? pMissionAIPlot->getX() : -1),
			(pMissionAIPlot != NULL ? pMissionAIPlot->getY() : -1),
			(pMissionAIUnit != NULL ? pMissionAIUnit->getID() : -1),
			(int)pWorker->getBuildType(), iBuild, iBuildCanBuild,
			kOwner.getGold());
	gDLL->logMsg("UnitStuck.log", szOut, false, false);
	if (isEnabled())
		gDLL->messageControlLog(szOut);
}

// Record the final worker AI-update steps before the loop watchdog fires.
void CvDLLLogger::logUnitStuckStep(CvSelectionGroup const& kGroup,
	int iAttempt, int iMaxAttempts, bool bAfterAIUpdate, bool bShouldAbort)
{
	CvUnitAI const* pWorker = kGroup.AI().AI_getHeadUnit();
	if (pWorker == NULL || pWorker->AI_getUnitAIType() != UNITAI_WORKER)
		return;

	CvSelectionGroupAI const& kAIGroup = kGroup.AI();
	CvPlot const* pMissionAIPlot = kAIGroup.AI_getMissionAIPlot();
	CLLNode<MissionData>* pMissionNode = kGroup.headMissionQueueNode();
	int iQueuedMissionType = (pMissionNode != NULL ?
			(int)pMissionNode->m_data.eMissionType : -1);
	int iQueuedMissionData1 = (pMissionNode != NULL ?
			pMissionNode->m_data.iData1 : -1);
	int iQueuedMissionData2 = (pMissionNode != NULL ?
			pMissionNode->m_data.iData2 : -1);
	CvPlayer const& kOwner = GET_PLAYER(kGroup.getOwner());
	char szOut[1024];
	std::sprintf(szOut,
			"  worker-loop step turn=%d slice=%d attempt=%d/%d stage=%s result=%d owner=%d group=%d unit=%d at=%d,%d moves=%d canMove=%d build=%d missionAI=%d target=%d,%d queued=%d firstMission=%d,%d,%d gold=%d\n",
			GC.getGame().getGameTurn(), GC.getGame().getTurnSlice(),
			iAttempt, iMaxAttempts, (bAfterAIUpdate ? "after" : "before"),
			(bAfterAIUpdate ? (int)bShouldAbort : -1),
			(int)kGroup.getOwner(), kGroup.getID(),
			pWorker->getID(), pWorker->getX(), pWorker->getY(),
			pWorker->movesLeft(), pWorker->canMove(),
			(int)pWorker->getBuildType(), (int)kAIGroup.AI_getMissionAIType(),
			(pMissionAIPlot != NULL ? pMissionAIPlot->getX() : -1),
			(pMissionAIPlot != NULL ? pMissionAIPlot->getY() : -1),
			kGroup.getLengthMissionQueue(), iQueuedMissionType,
			iQueuedMissionData1, iQueuedMissionData2, kOwner.getGold());
	gDLL->logMsg("UnitStuck.log", szOut, false, false);
	if (isEnabled())
		gDLL->messageControlLog(szOut);
}
// </fttwr.workerloopdiag>

// Cut from CvSelectionGroupAI::AI_update
void CvDLLLogger::logUnitStuck(CvSelectionGroup const& kGroup,
	int iAttempts, int iMaxAttempts)
{
	char szOut[1024];
	CvSelectionGroupAI const& kAIGroup = kGroup.AI();
	CvPlot const* pMissionAIPlot = kAIGroup.AI_getMissionAIPlot();
	CvUnitAI const* pMissionAIUnit = kAIGroup.AI_getMissionAIUnit();
	int const iMissionQueueLength = kGroup.getLengthMissionQueue();
	std::sprintf(szOut,
			"Unit stuck in loop: turn=%d slice=%d owner=%d (%S) group=%d units=%d plot=%d,%d activity=%d attempts=%d/%d\n",
			GC.getGame().getGameTurn(), GC.getGame().getTurnSlice(),
			(int)kGroup.getOwner(), GET_PLAYER(kGroup.getOwner()).getName(),
			kGroup.getID(), kGroup.getNumUnits(),
			kGroup.getX(), kGroup.getY(), (int)kGroup.getActivityType(),
			iAttempts, iMaxAttempts);
	gDLL->logMsg("UnitStuck.log", szOut, false, false);
	if (isEnabled())
		gDLL->messageControlLog(szOut);
	std::sprintf(szOut,
			"  group AI: missionAI=%d targetPlot=%d,%d targetUnit=%d owner=%d at=%d,%d queuedMissions=%d\n",
			(int)kAIGroup.AI_getMissionAIType(),
			(pMissionAIPlot != NULL ? pMissionAIPlot->getX() : -1),
			(pMissionAIPlot != NULL ? pMissionAIPlot->getY() : -1),
			(pMissionAIUnit != NULL ? pMissionAIUnit->getID() : -1),
			(pMissionAIUnit != NULL ? (int)pMissionAIUnit->getOwner() : -1),
			(pMissionAIUnit != NULL ? pMissionAIUnit->getX() : -1),
			(pMissionAIUnit != NULL ? pMissionAIUnit->getY() : -1),
			iMissionQueueLength);
	gDLL->logMsg("UnitStuck.log", szOut, false, false);
	if (isEnabled())
		gDLL->messageControlLog(szOut);

	// Keep this diagnostic bounded if a malformed queue is unexpectedly large.
	int const iMaxLoggedMissions = 12;
	int iMissionIndex = 0;
	for (CLLNode<MissionData>* pMissionNode = kGroup.headMissionQueueNode();
		pMissionNode != NULL && iMissionIndex < iMaxLoggedMissions;
		pMissionNode = kGroup.nextMissionQueueNode(pMissionNode), iMissionIndex++)
	{
		MissionData const& kMission = pMissionNode->m_data;
		int const iBuild = (kMission.eMissionType == MISSION_BUILD ?
			kMission.iData1 : -1);
		std::sprintf(szOut,
				"  queued[%d] mission=%d data=%d,%d flags=%d pushTurn=%d modified=%d build=%d\n",
			iMissionIndex, (int)kMission.eMissionType,
				kMission.iData1, kMission.iData2, (int)kMission.eFlags,
			kMission.iPushTurn, (int)kMission.bModified, iBuild);
		gDLL->logMsg("UnitStuck.log", szOut, false, false);
		if (isEnabled())
			gDLL->messageControlLog(szOut);
	}
	if (iMissionQueueLength > iMaxLoggedMissions)
	{
		std::sprintf(szOut, "  queued missions truncated: logged=%d total=%d\n",
				iMaxLoggedMissions, iMissionQueueLength);
		gDLL->logMsg("UnitStuck.log", szOut, false, false);
		if (isEnabled())
			gDLL->messageControlLog(szOut);
	}

	FOR_EACH_UNIT_IN(pUnit, kGroup)
	{
		std::sprintf(szOut,
				"  unit=%d type=%d ai=%d at=%d,%d moves=%d/%d canMove=%d damage=%d name=%S\n",
				pUnit->getID(), (int)pUnit->getUnitType(),
				(int)pUnit->AI_getUnitAIType(),
				pUnit->getX(), pUnit->getY(), pUnit->movesLeft(),
				pUnit->maxMoves(), pUnit->canMove(), pUnit->getDamage(),
				pUnit->getName().GetCString());
		gDLL->logMsg("UnitStuck.log", szOut, false, false);
		if (isEnabled())
			gDLL->messageControlLog(szOut);
	}

	// <fttwr.workerbuilddiag> Capture build gates only when a worker trips the
	// stuck-group assert; this does not influence worker decisions.
	CvUnitAI const* pWorker = kAIGroup.AI_getHeadUnit();
	if (pWorker != NULL && pWorker->AI_getUnitAIType() == UNITAI_WORKER &&
		kAIGroup.AI_getMissionAIType() == MISSIONAI_BUILD && pMissionAIPlot != NULL)
	{
		CvPlayer const& kOwner = GET_PLAYER(kGroup.getOwner());
		CvTeamAI const& kTeam = GET_TEAM(kOwner.getTeam());
		std::sprintf(szOut,
				"  worker-build target=(%d,%d) ownerGold=%d plotOwner=%d feature=%d improvement=%d route=%d\n",
				pMissionAIPlot->getX(), pMissionAIPlot->getY(), kOwner.getGold(),
				(int)pMissionAIPlot->getOwner(), (int)pMissionAIPlot->getFeatureType(),
				(int)pMissionAIPlot->getImprovementType(),
				(int)pMissionAIPlot->getRouteType());
		gDLL->logMsg("UnitStuck.log", szOut, false, false);
		if (isEnabled())
			gDLL->messageControlLog(szOut);

		int iBuildsLogged = 0;
		int const iMaxBuildsLogged = 64;
		FOR_EACH_ENUM(Build)
		{
			CvBuildInfo const& kBuild = GC.getInfo(eLoopBuild);
			if (!pWorker->getUnitInfo().getBuilds(eLoopBuild))
				continue;
			if (iBuildsLogged >= iMaxBuildsLogged)
			{
				std::sprintf(szOut,
						"  worker build candidates truncated at %d\n", iMaxBuildsLogged);
				gDLL->logMsg("UnitStuck.log", szOut, false, false);
				if (isEnabled())
					gDLL->messageControlLog(szOut);
				break;
			}
			iBuildsLogged++;

			TechTypes const eBuildTech = kBuild.getTechPrereq();
			bool const bBuildTech = eBuildTech == NO_TECH ||
				kTeam.isHasTech(eBuildTech);
			ImprovementTypes const eImprovement = kBuild.getImprovement();
			TechTypes eImprovementTech = NO_TECH;
			if (eImprovement != NO_IMPROVEMENT)
			{
				eImprovementTech = GC.getInfo(eImprovement).getPrereqTech();
			}
			bool const bImprovementTech = eImprovementTech == NO_TECH ||
				kTeam.isHasTech(eImprovementTech);
			TechTypes eFeatureTech = NO_TECH;
			bool bFeatureTech = true;
			if (pMissionAIPlot->isFeature())
			{
				eFeatureTech = kBuild.getFeatureTech(
						pMissionAIPlot->getFeatureType());
				bFeatureTech = kTeam.isHasTech(eFeatureTech);
			}

			bool const bPlotAllows = pMissionAIPlot->canBuild(eLoopBuild,
					kOwner.getID(), false, false);
			bool const bCivic = pWorker->isCivicPrereqMet();
			bool const bDomain = pWorker->isValidDomainForAction(*pMissionAIPlot);
			int const iCost = kOwner.getBuildCost(*pMissionAIPlot, eLoopBuild);
			int const iGold = std::max(0, kOwner.getGold());
			bool const bGold = iGold >= iCost;
			bool const bUnitCanBuild = pWorker->canBuild(*pMissionAIPlot,
					eLoopBuild, false, false);
			std::sprintf(szOut,
					"  worker buildId=%d supported=1 plot=%d civic=%d domain=%d buildTechId=%d buildTechOK=%d improvementId=%d improvementTechId=%d improvementTechOK=%d featureTechId=%d featureTechOK=%d gold=%d/%d affordable=%d progress=%d canBuild=%d\n",
					(int)eLoopBuild, bPlotAllows, bCivic, bDomain,
					(int)eBuildTech, bBuildTech, (int)eImprovement,
					(int)eImprovementTech, bImprovementTech,
					(int)eFeatureTech, bFeatureTech, iGold,
					iCost, bGold, pMissionAIPlot->getBuildProgress(eLoopBuild),
					bUnitCanBuild);
			gDLL->logMsg("UnitStuck.log", szOut, false, false);
			if (isEnabled())
				gDLL->messageControlLog(szOut);
		}
	}
	// </fttwr.workerbuilddiag>
}

// <advc.mapstat>
// Don't really want to include sstream in the header. Hence free functions.
namespace
{
void appendPercentage(std::ostringstream& os, char const* szLabel, int iAbsolute, int iTotal)
{
	FAssert(iAbsolute <= iTotal);
	if (iAbsolute <= 0)
		return;
	os.precision(1);
	os << szLabel << ": " << std::fixed << (100 * iAbsolute / (float)iTotal) <<
			"%% (" << iAbsolute << ")\n";
}

void appendPercentage(std::ostringstream& os, wchar const* szLabel, int iAbsolute, int iTotal)
{
	CvWString szWide(szLabel);
	CvString szNarrow(szWide);
	appendPercentage(os, szNarrow.c_str(), iAbsolute, iTotal);
}
}

void CvDLLLogger::logMapStats(bool bAfterNormalization)
{
	if (!isEnabled() || !GC.getDefineBOOL("LOG_MAP_STATS"))
		return;
	std::ostringstream out;
	out << "\nMap stats";
	if (bAfterNormalization)
		out << " after normalization";
	out << ":\n\n";
	/*  As for map settings - will have to go to the Victory screen
		and maybe take a screenshot. */
	std::vector<int> plotTypeCounts;
	plotTypeCounts.resize(NUM_PLOT_TYPES);
	std::vector<int> landTerrainCounts;
	landTerrainCounts.resize(GC.getNumTerrainInfos());
	std::vector<int> waterTerrainCounts;
	waterTerrainCounts.resize(GC.getNumTerrainInfos());
	std::vector<int> landFeatureCounts;
	landFeatureCounts.resize(GC.getNumFeatureInfos());
	std::vector<int> waterFeatureCounts;
	waterFeatureCounts.resize(GC.getNumFeatureInfos());
	std::vector<int> landResourceCounts;
	landResourceCounts.resize(GC.getNumBonusInfos());
	std::vector<int> waterResourceCounts;
	waterResourceCounts.resize(GC.getNumBonusInfos());
	int iRiverPlots = 0;
	int iResourceTotal = 0;
	CvMap const& kMap = GC.getMap();
	for (int i = 0; i < kMap.numPlots(); i++)
	{
		CvPlot const& kPlot = kMap.getPlotByIndex(i);
		plotTypeCounts[kPlot.getPlotType()]++;
		BonusTypes eBonus = kPlot.getBonusType();
		if (eBonus != NO_BONUS)
			iResourceTotal++;
		if (kPlot.isWater())
		{
			waterTerrainCounts[kPlot.getTerrainType()]++;
			waterFeatureCounts[kPlot.getFeatureType()]++;
			if (eBonus != NO_BONUS)
				waterResourceCounts[eBonus]++;
		}
		else
		{
			landTerrainCounts[kPlot.getTerrainType()]++;
			landFeatureCounts[kPlot.getFeatureType()]++;
			if (eBonus != NO_BONUS)
				landResourceCounts[eBonus]++;
			if (kPlot.isRiver())
				iRiverPlots++;
		}
	}
	int iTotal = kMap.numPlots();
	out << "Total tile count: " << iTotal << " (" << kMap.getGridWidth()
			<< "x" << kMap.getGridHeight() << ")\n";
	int iWater = plotTypeCounts[PLOT_OCEAN];
	int iLand = iTotal - iWater;
	FAssert(iLand > 0);
	appendPercentage(out, "Land", iLand, iTotal);
	float fResourcesPerPlayer = iResourceTotal / (float)GC.getGame().getCivPlayersEverAlive();
	out.precision(2);
	out << "Resource total: " << iResourceTotal << " (" << std::fixed << fResourcesPerPlayer <<
			" per player)\n";
	for (int iPass = 0; iPass < 2; iPass++)
	{
		bool const bWater = (iPass == 1);
		out << (bWater ? "Water" : "Land") << " breakdown:\n";
		if (!bWater)
		{
			appendPercentage(out, "Hills", plotTypeCounts[PLOT_HILLS], iLand);
			appendPercentage(out, "Peak", plotTypeCounts[PLOT_PEAK], iLand);
		}
		int iTiles = (bWater ? iWater : iLand);
		std::vector<int>& terrainCounts = (bWater ? waterTerrainCounts : landTerrainCounts);
		std::vector<int>& featureCounts = (bWater ? waterFeatureCounts : landFeatureCounts);
		std::vector<int>& resourceCounts = (bWater ? waterResourceCounts : landResourceCounts);
		for (size_t i = 0; i < terrainCounts.size(); i++)
		{
			CvTerrainInfo const& kTerrain = GC.getInfo((TerrainTypes)i);
			appendPercentage(out, kTerrain.getDescription(), terrainCounts[i], iTiles);
		}
		for (size_t i = 0; i < featureCounts.size(); i++)
		{
			CvFeatureInfo const& kFeature = GC.getInfo((FeatureTypes)i);
			appendPercentage(out, kFeature.getDescription(), featureCounts[i], iTiles);
		}
		int iResources = 0;
		for (size_t i = 0; i < resourceCounts.size(); i++)
			iResources += resourceCounts[i];
		out << "Resources: " << iResources << "\n";
		for (size_t i = 0; i < resourceCounts.size(); i++)
		{
			if (resourceCounts[i] <= 0)
				continue;
			CvBonusInfo const& kBonus = GC.getInfo((BonusTypes)i);
			CvWString szWide(kBonus.getDescription());
			CvString szNarrow(szWide);
			out << szNarrow.c_str() << ": " << resourceCounts[i] << "\n";
		}
	}
	appendPercentage(out, "River plots", iRiverPlots, iLand);
	std::vector<int> majorLandmassSizes;
	int iIslands = 0;
	int iLargeIslands = 0;
	int iContinents = 0;
	FOR_EACH_AREA(pArea)
	{
		if (pArea->isWater())
			continue;
		int iSize = pArea->getNumTiles();
		if (iSize >= NUM_CITY_PLOTS)
		{
			majorLandmassSizes.push_back(iSize);
			if (iSize >= 3 * NUM_CITY_PLOTS)
				iContinents++;
			else iLargeIslands++;
		}
		else iIslands++;
	}
	out << "Continents: " << iContinents << ", large islands: " << iLargeIslands <<
			", islands: " << iIslands << "\n";
	std::sort(majorLandmassSizes.rbegin(), majorLandmassSizes.rend());
	if (!majorLandmassSizes.empty())
		out << "Major landmass sizes:\n";
	for (size_t i = 0; i < majorLandmassSizes.size(); i++)
	{
		out << majorLandmassSizes[i];
		if (i + 1 < majorLandmassSizes.size())
			out << ", ";
	}
	out << std::endl;
	gDLL->messageControlLog(const_cast<char*>(out.str().c_str()));
} // </advc.mapstat>

// advc.tsl:
void CvDLLLogger::logCivLeaders()
{
	if (!isEnabled())
		return;
	CvWString szCivNames;
	bool bFirst = true;
	for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it)
	{
		setListHelp(szCivNames, L"\nCiv leaders on map: ",
				GC.getInfo(it->getLeaderType()).getDescription(),
				L", ", bFirst);
	}
	szCivNames.append(NEWLINE);
	gDLL->messageControlLog(const_cast<char*>(CvString(szCivNames).c_str()));
}
