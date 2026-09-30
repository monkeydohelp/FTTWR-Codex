// advc.003x: Cut from CvInfos.cpp

#include "CvGameCoreDLL.h"
#include "CvInfo_Building.h"
#include "CvXMLLoadUtility.h"

namespace
{
	// ResourceCapacity output tokens are deliberately explicit rather than
	// resolved through the global info-type map: YieldTypes and CommerceTypes
	// are enum domains, not CvInfo classes, and the map is allowed to contain
	// unrelated IDs.  The aliases keep the XML friendly to both Civ4 naming
	// (COMMERCE_RESEARCH) and the common player-facing name (COMMERCE_SCIENCE).
	int parseResourceCapacityOutputType(CvString const& szToken)
	{
		if (szToken.CompareNoCase("UNIT_EXPERIENCE") == 0 ||
			szToken.CompareNoCase("UNIT_XP") == 0)
			return RESOURCE_CAPACITY_OUTPUT_UNIT_EXPERIENCE;
		if (szToken.CompareNoCase("HEALTH") == 0)
			return RESOURCE_CAPACITY_OUTPUT_HEALTH;
		if (szToken.CompareNoCase("HAPPINESS") == 0 ||
			szToken.CompareNoCase("HAPPY") == 0)
			return RESOURCE_CAPACITY_OUTPUT_HAPPINESS;

		if (szToken.CompareNoCase("YIELD_FOOD") == 0)
			return RESOURCE_CAPACITY_OUTPUT_YIELD_BASE + YIELD_FOOD;
		if (szToken.CompareNoCase("YIELD_PRODUCTION") == 0)
			return RESOURCE_CAPACITY_OUTPUT_YIELD_BASE + YIELD_PRODUCTION;
		if (szToken.CompareNoCase("YIELD_COMMERCE") == 0)
			return RESOURCE_CAPACITY_OUTPUT_YIELD_BASE + YIELD_COMMERCE;

		if (szToken.CompareNoCase("COMMERCE_GOLD") == 0)
			return RESOURCE_CAPACITY_OUTPUT_COMMERCE_BASE + COMMERCE_GOLD;
		if (szToken.CompareNoCase("COMMERCE_RESEARCH") == 0 ||
			szToken.CompareNoCase("COMMERCE_SCIENCE") == 0)
			return RESOURCE_CAPACITY_OUTPUT_COMMERCE_BASE + COMMERCE_RESEARCH;
		if (szToken.CompareNoCase("COMMERCE_CULTURE") == 0)
			return RESOURCE_CAPACITY_OUTPUT_COMMERCE_BASE + COMMERCE_CULTURE;
		if (szToken.CompareNoCase("COMMERCE_ESPIONAGE") == 0)
			return RESOURCE_CAPACITY_OUTPUT_COMMERCE_BASE + COMMERCE_ESPIONAGE;

		return RESOURCE_CAPACITY_OUTPUT_NONE;
	}

	int parseResourceCapacityOutputScope(CvString const& szToken)
	{
		if (szToken.CompareNoCase("VICINITY") == 0 ||
			szToken.CompareNoCase("NEARBY") == 0)
			return RESOURCE_CAPACITY_OUTPUT_SCOPE_VICINITY;
		return RESOURCE_CAPACITY_OUTPUT_SCOPE_CITY;
	}

	// Legacy bonus yield/commerce XML is a two-dimensional map: BonusTypes
	// outer key, then YieldTypes/CommerceTypes inner key.  Keep the temporary
	// compatibility maps in AdvCiv's encoded-map form so iteration and cache
	// migration use the same type as the native BonusYieldModifier maps.
	typedef Enum2IntEncMap<NonDefaultEnumMap<BonusTypes,
		YieldChangeMap::enc_t>, YieldChangeMap> BonusYieldChangeMap;
	typedef Enum2IntEncMap<NonDefaultEnumMap<BonusTypes,
		CommerceChangeMap::enc_t>, CommerceChangeMap> BonusCommerceChangeMap;
}

CvBuildingInfo::CvBuildingInfo() :
m_eBuildingClassType(NO_BUILDINGCLASS),
m_eVictoryPrereq(NO_VICTORY),
m_eFreeStartEra(NO_ERA),
m_eMaxStartEra(NO_ERA),
m_eObsoleteTech(NO_TECH),
m_ePrereqAndTech(NO_TECH),
m_eNoBonus(NO_BONUS),
m_ePowerBonus(NO_BONUS),
m_eFreeBonus(NO_BONUS),
m_iNumFreeBonuses(0),
m_eFreeBuildingClass(NO_BUILDINGCLASS),
m_eFreePromotion(NO_PROMOTION),
m_eCivicOption(NO_CIVICOPTION),
m_iAIWeight(0),
m_iProductionCost(0),
m_iHurryCostModifier(0),
m_iHurryAngerModifier(0),
m_iAdvancedStartCost(0),
m_iAdvancedStartCostIncrease(0),
m_iWorkableRadius(0),
m_iCultureFlipResistance(0),
m_iMinAreaSize(0),
m_iNumCitiesPrereq(0),
m_iNumTeamsPrereq(0),
m_iUnitLevelPrereq(0),
m_iMinLatitude(0),
m_iMaxLatitude(90),
m_iGreatPeopleRateModifier(0),
m_iGreatGeneralRateModifier(0),
m_iDomesticGreatGeneralRateModifier(0),
m_iGlobalGreatPeopleRateModifier(0),
m_iAnarchyModifier(0),
m_iGoldenAgeModifier(0),
m_iGlobalHurryModifier(0),
m_iFreeExperience(0),
m_iGlobalFreeExperience(0),
m_iFoodKept(0),
m_iAirlift(0),
m_iAirModifier(0),
m_iAirUnitCapacity(0),
m_iResourceCapacityOutputCount(0),
m_iNukeModifier(0),
m_iNukeExplosionRand(0),
m_iFreeSpecialist(0),
m_iAreaFreeSpecialist(0),
m_iGlobalFreeSpecialist(0),
m_iHappiness(0),
m_iAreaHappiness(0),
m_iGlobalHappiness(0),
m_iStateReligionHappiness(0),
m_iWorkerSpeedModifier(0),
m_iMilitaryProductionModifier(0),
m_iSpaceProductionModifier(0),
m_iGlobalSpaceProductionModifier(0),
m_iTradeRoutes(0),
m_iCoastalTradeRoutes(0),
m_iAreaTradeRoutes(0), // advc.310
m_iTradeRouteModifier(0),
m_iForeignTradeRouteModifier(0),
m_iAssetValue(0),
m_iPowerValue(0),
m_eSpecialBuildingType(NO_SPECIALBUILDING),
m_eAdvisorType(NO_ADVISOR),
m_eHolyCity(NO_RELIGION),
m_eReligionType(NO_RELIGION),
m_eStateReligion(NO_RELIGION),
m_ePrereqReligion(NO_RELIGION),
m_ePrereqCorporation(NO_CORPORATION),
m_eFoundsCorporation(NO_CORPORATION),
m_eGlobalReligionCommerce(/* advc (was 0): */ NO_RELIGION),
m_eGlobalCorporationCommerce(/* advc (was 0): */ NO_CORPORATION),
m_ePrereqAndBonus(NO_BONUS),
m_eGreatPeopleUnitClass(NO_UNITCLASS),
m_iGreatPeopleRateChange(0),
m_iConquestProbability(0),
m_iMaintenanceModifier(0),
m_iWarWearinessModifier(0),
m_iGlobalWarWearinessModifier(0),
m_iEnemyWarWearinessModifier(0),
m_iHealRateChange(0),
m_iHealth(0),
m_iAreaHealth(0),
m_iGlobalHealth(0),
m_iGlobalPopulationChange(0),
m_iFreeTechs(0),
m_iDefenseModifier(0),
m_iBombardDefenseModifier(0),
m_iAllCityDefenseModifier(0),
m_iEspionageDefenseModifier(0),
m_eMissionType(NO_MISSION),
m_eVoteSourceType(NO_VOTESOURCE),
m_fVisibilityPriority(0.0f),
m_bTeamShare(false),
m_bWater(false),
m_bRiver(false),
m_bPower(false),
m_bDirtyPower(false),
m_bAreaCleanPower(false),
m_bAreaBorderObstacle(false),
m_bConditional(false), // advc.310
m_bForceTeamVoteEligible(false),
m_bCapital(false),
m_bGovernmentCenter(false),
m_bGoldenAge(false),
m_bMapCentering(false),
m_bNoUnhappiness(false),
//m_bNoUnhealthyPopulation(false),
m_iUnhealthyPopulationModifier(0), // K-Mod
m_bBuildingOnlyHealthy(false),
m_bNeverCapture(false),
m_bNukeImmune(false),
m_bPrereqReligion(false),
m_bCenterInCity(false),
m_bStateReligion(false),
m_bAllowsNukes(false),
m_bSlaveMarket(false),
m_bPrereqVault(false),
m_bPrereqNoVault(false),
m_bFoundReligion(false),
m_bPrereqPower(false),
m_bPrereqNotVassal(false),
m_iPrereqPopulation(0),
m_ePrereqCultureLevel(NO_CULTURELEVEL),
m_ePrereqVicinityBonus(NO_BONUS),
m_piPrereqOrVicinityBonuses(NULL),
m_pbPrereqVicinityImprovement(NULL),
m_pbPrereqVicinityFeature(NULL),
m_pbPrereqOrBuildingClass(NULL),
m_pbPrereqNotBuildingClass(NULL)
{
	for (int i = 0; i < FTTWRP1_MAX_BONUS_OUTPUTS; ++i)
	{
		m_aiResourceCapacityOutputResource[i] = NO_BONUS;
		m_aiResourceCapacityOutputType[i] = RESOURCE_CAPACITY_OUTPUT_NONE;
		m_aiResourceCapacityOutputAmount[i] = 0;
		m_aiResourceCapacityOutputScope[i] = RESOURCE_CAPACITY_OUTPUT_SCOPE_CITY;
	}
}

CvBuildingInfo::~CvBuildingInfo()
{
	SAFE_DELETE_ARRAY(m_piPrereqOrVicinityBonuses);
	SAFE_DELETE_ARRAY(m_pbPrereqVicinityImprovement);
	SAFE_DELETE_ARRAY(m_pbPrereqVicinityFeature);
	SAFE_DELETE_ARRAY(m_pbPrereqOrBuildingClass);
	SAFE_DELETE_ARRAY(m_pbPrereqNotBuildingClass);
}

void CvBuildingInfo::addResourceCapacityOutput(int iResource, int iOutputType,
	int iAmount, int iScope)
{
	if (iResource < 0 || iOutputType == RESOURCE_CAPACITY_OUTPUT_NONE ||
		iAmount == 0 || m_iResourceCapacityOutputCount >= FTTWRP1_MAX_BONUS_OUTPUTS)
		return;
	m_aiResourceCapacityOutputResource[m_iResourceCapacityOutputCount] = iResource;
	m_aiResourceCapacityOutputType[m_iResourceCapacityOutputCount] = iOutputType;
	m_aiResourceCapacityOutputAmount[m_iResourceCapacityOutputCount] = iAmount;
	m_aiResourceCapacityOutputScope[m_iResourceCapacityOutputCount] =
		(iScope == RESOURCE_CAPACITY_OUTPUT_SCOPE_VICINITY) ?
		RESOURCE_CAPACITY_OUTPUT_SCOPE_VICINITY :
		RESOURCE_CAPACITY_OUTPUT_SCOPE_CITY;
	++m_iResourceCapacityOutputCount;
}

void CvBuildingInfo::addResourceCapacityCost(int iResource, int iCost)
{
	if (iResource < 0 || iCost <= 0)
		return;
	for (int i = 0; i < m_iResourceCapacityCostCount; ++i)
	{
		if (m_aiResourceCapacityCostResource[i] == iResource)
		{
			m_aiResourceCapacityCost[i] += iCost;
			return;
		}
	}
	if (m_iResourceCapacityCostCount >= FTTWRP1_MAX_CAPACITY_COSTS)
		return;
	int const iIndex = m_iResourceCapacityCostCount++;
	m_aiResourceCapacityCostResource[iIndex] = iResource;
	m_aiResourceCapacityCost[iIndex] = iCost;
}

// advc.003w:
bool CvBuildingInfo::isTechRequired(TechTypes eTech) const
{
	if (getPrereqAndTech() == eTech)
		return true;
	for (int i = 0; i < getNumPrereqAndTechs(); i++)
	{
		if (getPrereqAndTechs(i) == eTech)
			return true;
	}
	SpecialBuildingTypes eSpecial = getSpecialBuildingType();
	if (eSpecial != NO_SPECIALBUILDING && GC.getInfo(eSpecial).getTechPrereq() == eTech)
		return true;
	return false;
}

int CvBuildingInfo::getDomesticGreatGeneralRateModifier() const
{	// <advc.310>
	if(!m_bEnabledDomesticGreatGeneralRateModifier && m_bConditional)
		return 0; // </advc.310>
	return m_iDomesticGreatGeneralRateModifier;
}
// advc.310:
int CvBuildingInfo::getAreaTradeRoutes() const
{
	if(!m_bEnabledAreaTradeRoutes && m_bConditional)
		return 0;
	return m_iAreaTradeRoutes;
}

void CvBuildingInfo::setMissionType(MissionTypes eNewType)
{
	m_eMissionType = eNewType;
}

float CvBuildingInfo::getVisibilityPriority() const
{
	return m_fVisibilityPriority;
}

bool CvBuildingInfo::isAreaBorderObstacle() const
{	// <advc.310>
	if(!m_bEnabledAreaBorderObstacle && m_bConditional)
		return false; // </advc.310>
	return m_bAreaBorderObstacle;
}

const TCHAR* CvBuildingInfo::getConstructSound() const
{
	return m_szConstructSound;
}

const TCHAR* CvBuildingInfo::getArtDefineTag() const
{
	return m_szArtDefineTag;
}

const TCHAR* CvBuildingInfo::getMovieDefineTag() const
{
	return m_szMovieDefineTag;
}

// <advc.003t> Calls from Python aren't going to respect the bounds
int CvBuildingInfo::py_getPrereqAndTechs(int i) const
{
	if (i < 0 || i >= getNumPrereqAndTechs())
		return NO_TECH;
	return m_aePrereqAndTechs[i];
}

int CvBuildingInfo::py_getPrereqOrBonuses(int i) const
{
	if (i < 0 || i >= getNumPrereqOrBonuses())
		return NO_BONUS;
	return m_aePrereqOrBonuses[i];
} // </advc.003t>

const TCHAR* CvBuildingInfo::getButton() const
{
	CvArtInfoBuilding const* pBuildingArtInfo;
	pBuildingArtInfo = getArtInfo();
	if (pBuildingArtInfo == NULL)
		return NULL;
	return pBuildingArtInfo->getButton();
}

const CvArtInfoBuilding* CvBuildingInfo::getArtInfo() const
{
	return ARTFILEMGR.getBuildingArtInfo(getArtDefineTag());
}

const CvArtInfoMovie* CvBuildingInfo::getMovieInfo() const
{
	TCHAR const* pcTag = getMovieDefineTag();
	if (pcTag != NULL && _tcscmp(pcTag, "NONE") != 0)
		return ARTFILEMGR.getMovieArtInfo(pcTag);
	return NULL;
}

const TCHAR* CvBuildingInfo::getMovie() const
{
	CvArtInfoMovie const* pArt;
	pArt = getMovieInfo();
	if (pArt == NULL)
		return NULL;
	return pArt->getPath();
}
// advc.008e:
bool CvBuildingInfo::nameNeedsArticle() const
{
	if(!isWorldWonder())
		return false; // Should only be called for wonders really
	CvWString szKey = getTextKeyWide();
	CvWString szText = gDLL->getText(szKey + L"_NA");
	/*  If an _NA key exists, then gDLL will return a dot. If it doesn't, then
		an article should be used. */
	return (szText.compare(L".") != 0);
}
#if ENABLE_XML_FILE_CACHE
void CvBuildingInfo::read(FDataStreamBase* stream)
{
	base_t::read(stream);
	uint uiFlag=0;
	stream->Read(&uiFlag);
	BonusYieldChangeMap kLegacyBonusYieldChanges;
	BonusCommerceChangeMap kLegacyBonusCommerceChanges;
	BonusYieldChangeMap kLegacyVicinityBonusYieldChanges;
	BonusCommerceChangeMap kLegacyVicinityBonusCommerceChanges;

	stream->Read((int*)&m_eBuildingClassType);
	stream->Read((int*)&m_eVictoryPrereq);
	stream->Read((int*)&m_eFreeStartEra);
	stream->Read((int*)&m_eMaxStartEra);
	stream->Read((int*)&m_eObsoleteTech);
	stream->Read((int*)&m_ePrereqAndTech);
	stream->Read((int*)&m_eNoBonus);
	stream->Read((int*)&m_ePowerBonus);
	stream->Read((int*)&m_eFreeBonus);
	stream->Read((int*)&m_iNumFreeBonuses);
	stream->Read((int*)&m_eFreeBuildingClass);
	stream->Read((int*)&m_eFreePromotion);
	stream->Read((int*)&m_eCivicOption);
	stream->Read(&m_iAIWeight);
	stream->Read(&m_iProductionCost);
	stream->Read(&m_iHurryCostModifier);
	stream->Read(&m_iHurryAngerModifier);
	stream->Read(&m_iAdvancedStartCost);
	stream->Read(&m_iAdvancedStartCostIncrease);
	stream->Read(&m_iMinAreaSize);
	stream->Read(&m_iNumCitiesPrereq);
	stream->Read(&m_iNumTeamsPrereq);
	stream->Read(&m_iUnitLevelPrereq);
	stream->Read(&m_iMinLatitude);
	stream->Read(&m_iMaxLatitude);
	stream->Read(&m_iGreatPeopleRateModifier);
	stream->Read(&m_iGreatGeneralRateModifier);
	stream->Read(&m_iDomesticGreatGeneralRateModifier);
	stream->Read(&m_iGlobalGreatPeopleRateModifier);
	stream->Read(&m_iAnarchyModifier);
	stream->Read(&m_iGoldenAgeModifier);
	stream->Read(&m_iGlobalHurryModifier);
	stream->Read(&m_iFreeExperience);
	stream->Read(&m_iGlobalFreeExperience);
	stream->Read(&m_iFoodKept);
	stream->Read(&m_iAirlift);
	stream->Read(&m_iAirModifier);
	stream->Read(&m_iAirUnitCapacity);
	if ((uiFlag & FTTWR_BUILDING_INFO_SAVE_FLAG) != 0)
	{
		stream->Read(&m_iResourceCapacityOutputCount);
		m_iResourceCapacityOutputCount = std::max(0, std::min(
			m_iResourceCapacityOutputCount,
			(int)FTTWRP1_MAX_BONUS_OUTPUTS));
		int const iStoredOutputCapacity =
			((uiFlag & RESOURCE_CAPACITY_OUTPUT_SIZE_SAVE_FLAG) != 0) ?
			FTTWRP1_MAX_BONUS_OUTPUTS : FTTWRP1_LEGACY_MAX_BONUS_OUTPUTS;
		for (int i = 0; i < iStoredOutputCapacity; ++i)
		{
			stream->Read(&m_aiResourceCapacityOutputResource[i]);
			stream->Read(&m_aiResourceCapacityOutputType[i]);
			stream->Read(&m_aiResourceCapacityOutputAmount[i]);
			m_aiResourceCapacityOutputScope[i] =
				RESOURCE_CAPACITY_OUTPUT_SCOPE_CITY;
			if ((uiFlag & RESOURCE_CAPACITY_OUTPUT_SCOPE_SAVE_FLAG) != 0)
				stream->Read(&m_aiResourceCapacityOutputScope[i]);
		}
		for (int i = iStoredOutputCapacity;
			i < FTTWRP1_MAX_BONUS_OUTPUTS; ++i)
		{
			m_aiResourceCapacityOutputResource[i] = NO_BONUS;
			m_aiResourceCapacityOutputType[i] = RESOURCE_CAPACITY_OUTPUT_NONE;
			m_aiResourceCapacityOutputAmount[i] = 0;
			m_aiResourceCapacityOutputScope[i] =
				RESOURCE_CAPACITY_OUTPUT_SCOPE_CITY;
		}
	}
	if ((uiFlag & FTTWR_BUILDING_INFO_COMPATIBILITY_FLAG) != 0 &&
		(uiFlag & RESOURCE_CAPACITY_OUTPUT_SCOPE_SAVE_FLAG) == 0)
	{
		// Older candidate caches stored four compatibility maps.  Read them only
		// for migration, then fold them into the single scoped output array below.
		kLegacyBonusYieldChanges.read(stream);
		kLegacyBonusCommerceChanges.read(stream);
		kLegacyVicinityBonusYieldChanges.read(stream);
		kLegacyVicinityBonusCommerceChanges.read(stream);
		FOR_EACH_NON_DEFAULT_PAIR(kLegacyBonusYieldChanges, Bonus, YieldChangeMap)
		{
			FOR_EACH_NON_DEFAULT_PAIR(perBonusVal.second, Yield, int)
				addResourceCapacityOutput(perBonusVal.first,
					RESOURCE_CAPACITY_OUTPUT_YIELD_BASE + perYieldVal.first,
					perYieldVal.second, RESOURCE_CAPACITY_OUTPUT_SCOPE_CITY);
		}
		FOR_EACH_NON_DEFAULT_PAIR(kLegacyBonusCommerceChanges, Bonus, CommerceChangeMap)
		{
			FOR_EACH_NON_DEFAULT_PAIR(perBonusVal.second, Commerce, int)
				addResourceCapacityOutput(perBonusVal.first,
					RESOURCE_CAPACITY_OUTPUT_COMMERCE_BASE + perCommerceVal.first,
					perCommerceVal.second, RESOURCE_CAPACITY_OUTPUT_SCOPE_CITY);
		}
		FOR_EACH_NON_DEFAULT_PAIR(kLegacyVicinityBonusYieldChanges, Bonus, YieldChangeMap)
		{
			FOR_EACH_NON_DEFAULT_PAIR(perBonusVal.second, Yield, int)
				addResourceCapacityOutput(perBonusVal.first,
					RESOURCE_CAPACITY_OUTPUT_YIELD_BASE + perYieldVal.first,
					perYieldVal.second, RESOURCE_CAPACITY_OUTPUT_SCOPE_VICINITY);
		}
		FOR_EACH_NON_DEFAULT_PAIR(kLegacyVicinityBonusCommerceChanges, Bonus, CommerceChangeMap)
		{
			FOR_EACH_NON_DEFAULT_PAIR(perBonusVal.second, Commerce, int)
				addResourceCapacityOutput(perBonusVal.first,
					RESOURCE_CAPACITY_OUTPUT_COMMERCE_BASE + perCommerceVal.first,
					perCommerceVal.second, RESOURCE_CAPACITY_OUTPUT_SCOPE_VICINITY);
		}
	}
	if ((uiFlag & BUILDING_RESOURCE_CAPACITY_COST_INFO_SAVE_FLAG) != 0)
	{
		stream->Read(&m_iResourceCapacityCostCount);
		m_iResourceCapacityCostCount = std::max(0, std::min(
			m_iResourceCapacityCostCount, (int)FTTWRP1_MAX_CAPACITY_COSTS));
		for (int i = 0; i < FTTWRP1_MAX_CAPACITY_COSTS; ++i)
		{
			stream->Read(&m_aiResourceCapacityCostResource[i]);
			stream->Read(&m_aiResourceCapacityCost[i]);
		}
		stream->Read(&m_iResourceCapacityPopulationCost);
	}
	else
	{
		m_iResourceCapacityCostCount = 0;
		m_iResourceCapacityPopulationCost = 0;
		for (int i = 0; i < FTTWRP1_MAX_CAPACITY_COSTS; ++i)
		{
			m_aiResourceCapacityCostResource[i] = NO_BONUS;
			m_aiResourceCapacityCost[i] = 0;
		}
	}
	stream->Read(&m_iNukeModifier);
	stream->Read(&m_iNukeExplosionRand);
	stream->Read(&m_iFreeSpecialist);
	stream->Read(&m_iAreaFreeSpecialist);
	stream->Read(&m_iGlobalFreeSpecialist);
	stream->Read(&m_iHappiness);
	stream->Read(&m_iAreaHappiness);
	stream->Read(&m_iGlobalHappiness);
	stream->Read(&m_iStateReligionHappiness);
	stream->Read(&m_iWorkerSpeedModifier);
	stream->Read(&m_iMilitaryProductionModifier);
	stream->Read(&m_iSpaceProductionModifier);
	stream->Read(&m_iGlobalSpaceProductionModifier);
	stream->Read(&m_iTradeRoutes);
	stream->Read(&m_iCoastalTradeRoutes);
	stream->Read(&m_iAreaTradeRoutes); // advc.310
	stream->Read(&m_iTradeRouteModifier);
	stream->Read(&m_iForeignTradeRouteModifier);
	stream->Read(&m_iAssetValue);
	stream->Read(&m_iPowerValue);
	stream->Read((int*)&m_eSpecialBuildingType);
	stream->Read((int*)&m_eAdvisorType);
	stream->Read((int*)&m_eHolyCity);
	stream->Read((int*)&m_eReligionType);
	stream->Read((int*)&m_eStateReligion);
	stream->Read((int*)&m_ePrereqReligion);
	stream->Read((int*)&m_ePrereqCorporation);
	stream->Read((int*)&m_eFoundsCorporation);
	stream->Read((int*)&m_eGlobalReligionCommerce);
	stream->Read((int*)&m_eGlobalCorporationCommerce);
	stream->Read((int*)&m_ePrereqAndBonus);
	stream->Read((int*)&m_eGreatPeopleUnitClass);
	stream->Read(&m_iGreatPeopleRateChange);
	stream->Read(&m_iConquestProbability);
	stream->Read(&m_iMaintenanceModifier);
	stream->Read(&m_iWarWearinessModifier);
	stream->Read(&m_iGlobalWarWearinessModifier);
	stream->Read(&m_iEnemyWarWearinessModifier);
	stream->Read(&m_iHealRateChange);
	stream->Read(&m_iHealth);
	stream->Read(&m_iAreaHealth);
	stream->Read(&m_iGlobalHealth);
	stream->Read(&m_iGlobalPopulationChange);
	stream->Read(&m_iFreeTechs);
	stream->Read(&m_iDefenseModifier);
	stream->Read(&m_iBombardDefenseModifier);
	stream->Read(&m_iAllCityDefenseModifier);
	stream->Read(&m_iEspionageDefenseModifier);
	stream->Read((int*)&m_eMissionType);
	stream->Read((int*)&m_eVoteSourceType);
	stream->Read(&m_fVisibilityPriority);
	stream->Read(&m_bTeamShare);
	stream->Read(&m_bWater);
	stream->Read(&m_bRiver);
	stream->Read(&m_bPower);
	stream->Read(&m_bDirtyPower);
	stream->Read(&m_bAreaCleanPower);
	stream->Read(&m_bAreaBorderObstacle);
	stream->Read(&m_bConditional); // advc.310
	stream->Read(&m_bForceTeamVoteEligible);
	stream->Read(&m_bCapital);
	stream->Read(&m_bGovernmentCenter);
	stream->Read(&m_bGoldenAge);
	stream->Read(&m_bMapCentering);
	stream->Read(&m_bNoUnhappiness);
	//stream->Read(&m_bNoUnhealthyPopulation);
	stream->Read(&m_iUnhealthyPopulationModifier); // K-Mod
	stream->Read(&m_bBuildingOnlyHealthy);
	stream->Read(&m_bNeverCapture);
	stream->Read(&m_bNukeImmune);
	stream->Read(&m_bPrereqReligion);
	stream->Read(&m_bCenterInCity);
	stream->Read(&m_bStateReligion);
	stream->Read(&m_bAllowsNukes);
	stream->Read(&m_bSlaveMarket);
	if ((uiFlag & VAULT_PREREQ_INFO_SAVE_FLAG) != 0)
	{
		stream->Read(&m_bPrereqVault);
		stream->Read(&m_bPrereqNoVault);
	}
	if ((uiFlag & FOUND_RELIGION_INFO_SAVE_FLAG) != 0)
		stream->Read(&m_bFoundReligion);
	stream->ReadString(m_szConstructSound);
	stream->ReadString(m_szArtDefineTag);
	stream->ReadString(m_szMovieDefineTag);
	// <advc.003t>
	int iPrereqAndTechs;
	stream->Read(&iPrereqAndTechs);
	if (iPrereqAndTechs > 0)
	{
		m_aePrereqAndTechs.resize(iPrereqAndTechs);
		stream->Read(iPrereqAndTechs, (int*)&m_aePrereqAndTechs[0]);
	}
	int iPrereqOrBonuses;
	stream->Read(&iPrereqOrBonuses);
	if (iPrereqOrBonuses > 0)
	{
		m_aePrereqOrBonuses.resize(iPrereqOrBonuses);
		stream->Read(iPrereqOrBonuses, (int*)&m_aePrereqOrBonuses[0]);
	}
	if ((uiFlag & CIVIC_PREREQ_INFO_SAVE_FLAG) != 0)
	{
		int iPrereqAndCivics;
		stream->Read(&iPrereqAndCivics);
		if (iPrereqAndCivics > 0)
		{
			m_aePrereqAndCivics.resize(iPrereqAndCivics);
			stream->Read(iPrereqAndCivics, (int*)&m_aePrereqAndCivics[0]);
		}
		int iPrereqOrCivics;
		stream->Read(&iPrereqOrCivics);
		if (iPrereqOrCivics > 0)
		{
			m_aePrereqOrCivics.resize(iPrereqOrCivics);
			stream->Read(iPrereqOrCivics, (int*)&m_aePrereqOrCivics[0]);
		}
	}
ProductionTraits().read(stream);
	HappinessTraits().read(stream);
	SeaPlotYieldChange().read(stream);
	RiverPlotYieldChange().read(stream);
	GlobalSeaPlotYieldChange().read(stream);
	YieldChange().read(stream);
	YieldModifier().read(stream);
	PowerYieldModifier().read(stream);
	AreaYieldModifier().read(stream);
	GlobalYieldModifier().read(stream);
	CommerceChange().read(stream);
	ObsoleteSafeCommerceChange().read(stream);
	CommerceChangeDoubleTime().read(stream);
	CommerceModifier().read(stream);
	GlobalCommerceModifier().read(stream);
	SpecialistExtraCommerce().read(stream);
	StateReligionCommerce().read(stream);
	CommerceHappiness().read(stream);
	ReligionChange().read(stream);
	SpecialistCount().read(stream);
	FreeSpecialistCount().read(stream);
	BonusHealthChanges().read(stream);
	BonusHappinessChanges().read(stream);
	BonusProductionModifier().read(stream);
	UnitCombatFreeExperience().read(stream);
	DomainFreeExperience().read(stream);
	DomainProductionModifier().read(stream);
	BuildingHappinessChanges().read(stream);
	PrereqNumOfBuildingClass().read(stream);
	FlavorValue().read(stream);
	ImprovementFreeSpecialist().read(stream);
	CommerceFlexible().read(stream);
	CommerceChangeOriginalOwner().read(stream);
	BuildingClassNeededInCity().read(stream);
	SpecialistYieldChange().read(stream);
	SpecialistYieldModifier().read(stream);
	SpecialistCommerceChange().read(stream);
	SpecialistCommerceModifier().read(stream);
	BonusYieldModifier().read(stream);
	if ((uiFlag & WORKABLE_RADIUS_INFO_SAVE_FLAG) != 0)
		stream->Read(&m_iWorkableRadius);
	if ((uiFlag & CULTURE_FLIP_RESISTANCE_INFO_SAVE_FLAG) != 0)
		stream->Read(&m_iCultureFlipResistance);
	// </advc.003t>
}

void CvBuildingInfo::write(FDataStreamBase* stream)
{
	base_t::write(stream);
	uint uiFlag = FTTWR_BUILDING_INFO_SAVE_FLAG |
		RESOURCE_CAPACITY_OUTPUT_SCOPE_SAVE_FLAG |
		RESOURCE_CAPACITY_OUTPUT_SIZE_SAVE_FLAG |
		BUILDING_RESOURCE_CAPACITY_COST_INFO_SAVE_FLAG |
	CIVIC_PREREQ_INFO_SAVE_FLAG | VAULT_PREREQ_INFO_SAVE_FLAG |
		FOUND_RELIGION_INFO_SAVE_FLAG | WORKABLE_RADIUS_INFO_SAVE_FLAG |
		CULTURE_FLIP_RESISTANCE_INFO_SAVE_FLAG;
	stream->Write(uiFlag);

	stream->Write(m_eBuildingClassType);
	stream->Write(m_eVictoryPrereq);
	stream->Write(m_eFreeStartEra);
	stream->Write(m_eMaxStartEra);
	stream->Write(m_eObsoleteTech);
	stream->Write(m_ePrereqAndTech);
	stream->Write(m_eNoBonus);
	stream->Write(m_ePowerBonus);
	stream->Write(m_eFreeBonus);
	stream->Write(m_iNumFreeBonuses);
	stream->Write(m_eFreeBuildingClass);
	stream->Write(m_eFreePromotion);
	stream->Write(m_eCivicOption);
	stream->Write(m_iAIWeight);
	stream->Write(m_iProductionCost);
	stream->Write(m_iHurryCostModifier);
	stream->Write(m_iHurryAngerModifier);
	stream->Write(m_iAdvancedStartCost);
	stream->Write(m_iAdvancedStartCostIncrease);
	stream->Write(m_iMinAreaSize);
	stream->Write(m_iNumCitiesPrereq);
	stream->Write(m_iNumTeamsPrereq);
	stream->Write(m_iUnitLevelPrereq);
	stream->Write(m_iMinLatitude);
	stream->Write(m_iMaxLatitude);
	stream->Write(m_iGreatPeopleRateModifier);
	stream->Write(m_iGreatGeneralRateModifier);
	stream->Write(m_iDomesticGreatGeneralRateModifier);
	stream->Write(m_iGlobalGreatPeopleRateModifier);
	stream->Write(m_iAnarchyModifier);
	stream->Write(m_iGoldenAgeModifier);
	stream->Write(m_iGlobalHurryModifier);
	stream->Write(m_iFreeExperience);
	stream->Write(m_iGlobalFreeExperience);
	stream->Write(m_iFoodKept);
	stream->Write(m_iAirlift);
	stream->Write(m_iAirModifier);
	stream->Write(m_iAirUnitCapacity);
	stream->Write(m_iResourceCapacityOutputCount);
	for (int i = 0; i < FTTWRP1_MAX_BONUS_OUTPUTS; ++i)
	{
		stream->Write(m_aiResourceCapacityOutputResource[i]);
		stream->Write(m_aiResourceCapacityOutputType[i]);
		stream->Write(m_aiResourceCapacityOutputAmount[i]);
		stream->Write(m_aiResourceCapacityOutputScope[i]);
	}
	stream->Write(m_iResourceCapacityCostCount);
	for (int i = 0; i < FTTWRP1_MAX_CAPACITY_COSTS; ++i)
	{
		stream->Write(m_aiResourceCapacityCostResource[i]);
		stream->Write(m_aiResourceCapacityCost[i]);
	}
	stream->Write(m_iResourceCapacityPopulationCost);
	stream->Write(m_iNukeModifier);
	stream->Write(m_iNukeExplosionRand);
	stream->Write(m_iFreeSpecialist);
	stream->Write(m_iAreaFreeSpecialist);
	stream->Write(m_iGlobalFreeSpecialist);
	stream->Write(m_iHappiness);
	stream->Write(m_iAreaHappiness);
	stream->Write(m_iGlobalHappiness);
	stream->Write(m_iStateReligionHappiness);
	stream->Write(m_iWorkerSpeedModifier);
	stream->Write(m_iMilitaryProductionModifier);
	stream->Write(m_iSpaceProductionModifier);
	stream->Write(m_iGlobalSpaceProductionModifier);
	stream->Write(m_iTradeRoutes);
	stream->Write(m_iCoastalTradeRoutes);
	stream->Write(m_iAreaTradeRoutes); // advc.310
	stream->Write(m_iTradeRouteModifier);
	stream->Write(m_iForeignTradeRouteModifier);
	stream->Write(m_iAssetValue);
	stream->Write(m_iPowerValue);
	stream->Write(m_eSpecialBuildingType);
	stream->Write(m_eAdvisorType);
	stream->Write(m_eHolyCity);
	stream->Write(m_eReligionType);
	stream->Write(m_eStateReligion);
	stream->Write(m_ePrereqReligion);
	stream->Write(m_ePrereqCorporation);
	stream->Write(m_eFoundsCorporation);
	stream->Write(m_eGlobalReligionCommerce);
	stream->Write(m_eGlobalCorporationCommerce);
	stream->Write(m_ePrereqAndBonus);
	stream->Write(m_eGreatPeopleUnitClass);
	stream->Write(m_iGreatPeopleRateChange);
	stream->Write(m_iConquestProbability);
	stream->Write(m_iMaintenanceModifier);
	stream->Write(m_iWarWearinessModifier);
	stream->Write(m_iGlobalWarWearinessModifier);
	stream->Write(m_iEnemyWarWearinessModifier);
	stream->Write(m_iHealRateChange);
	stream->Write(m_iHealth);
	stream->Write(m_iAreaHealth);
	stream->Write(m_iGlobalHealth);
	stream->Write(m_iGlobalPopulationChange);
	stream->Write(m_iFreeTechs);
	stream->Write(m_iDefenseModifier);
	stream->Write(m_iBombardDefenseModifier);
	stream->Write(m_iAllCityDefenseModifier);
	stream->Write(m_iEspionageDefenseModifier);
	stream->Write(m_eMissionType);
	stream->Write(m_eVoteSourceType);

	stream->Write(m_fVisibilityPriority);

	stream->Write(m_bTeamShare);
	stream->Write(m_bWater);
	stream->Write(m_bRiver);
	stream->Write(m_bPower);
	stream->Write(m_bDirtyPower);
	stream->Write(m_bAreaCleanPower);
	stream->Write(m_bAreaBorderObstacle);
	stream->Write(m_bConditional); // advc.310
	stream->Write(m_bForceTeamVoteEligible);
	stream->Write(m_bCapital);
	stream->Write(m_bGovernmentCenter);
	stream->Write(m_bGoldenAge);
	stream->Write(m_bMapCentering);
	stream->Write(m_bNoUnhappiness);
	//stream->Write(m_bNoUnhealthyPopulation);
	stream->Write(m_iUnhealthyPopulationModifier); // K-Mod
	stream->Write(m_bBuildingOnlyHealthy);
	stream->Write(m_bNeverCapture);
	stream->Write(m_bNukeImmune);
	stream->Write(m_bPrereqReligion);
	stream->Write(m_bCenterInCity);
	stream->Write(m_bStateReligion);
	stream->Write(m_bAllowsNukes);
	stream->Write(m_bSlaveMarket);
	stream->Write(m_bPrereqVault);
	stream->Write(m_bPrereqNoVault);
	stream->Write(m_bFoundReligion);
	stream->WriteString(m_szConstructSound);
	stream->WriteString(m_szArtDefineTag);
	stream->WriteString(m_szMovieDefineTag);
	// <advc.003t>
	{
		int iPrereqAndTechs = getNumPrereqAndTechs();
		stream->Write(iPrereqAndTechs);
		if (iPrereqAndTechs > 0)
			stream->Write(iPrereqAndTechs, (int*)&m_aePrereqAndTechs[0]);
	}
	{
		int iPrereqOrBonuses = getNumPrereqOrBonuses();
		stream->Write(iPrereqOrBonuses);
		if (iPrereqOrBonuses > 0)
			stream->Write(iPrereqOrBonuses, (int*)&m_aePrereqOrBonuses[0]);
	}
	{
		int iPrereqAndCivics = getNumPrereqAndCivics();
		stream->Write(iPrereqAndCivics);
		if (iPrereqAndCivics > 0)
			stream->Write(iPrereqAndCivics, (int*)&m_aePrereqAndCivics[0]);
	}
	{
		int iPrereqOrCivics = getNumPrereqOrCivics();
		stream->Write(iPrereqOrCivics);
		if (iPrereqOrCivics > 0)
			stream->Write(iPrereqOrCivics, (int*)&m_aePrereqOrCivics[0]);
	}
ProductionTraits().write(stream);
	HappinessTraits().write(stream);
	SeaPlotYieldChange().write(stream);
	RiverPlotYieldChange().write(stream);
	GlobalSeaPlotYieldChange().write(stream);
	YieldChange().write(stream);
	YieldModifier().write(stream);
	PowerYieldModifier().write(stream);
	AreaYieldModifier().write(stream);
	GlobalYieldModifier().write(stream);
	CommerceChange().write(stream);
	ObsoleteSafeCommerceChange().write(stream);
	CommerceChangeDoubleTime().write(stream);
	CommerceModifier().write(stream);
	GlobalCommerceModifier().write(stream);
	SpecialistExtraCommerce().write(stream);
	StateReligionCommerce().write(stream);
	CommerceHappiness().write(stream);
	ReligionChange().write(stream);
	SpecialistCount().write(stream);
	FreeSpecialistCount().write(stream);
	BonusHealthChanges().write(stream);
	BonusHappinessChanges().write(stream);
	BonusProductionModifier().write(stream);
	UnitCombatFreeExperience().write(stream);
	DomainFreeExperience().write(stream);
	DomainProductionModifier().write(stream);
	BuildingHappinessChanges().write(stream);
	PrereqNumOfBuildingClass().write(stream);
	FlavorValue().write(stream);
	ImprovementFreeSpecialist().write(stream);
	CommerceFlexible().write(stream);
	CommerceChangeOriginalOwner().write(stream);
	BuildingClassNeededInCity().write(stream);
	SpecialistYieldChange().write(stream);
	SpecialistYieldModifier().write(stream);
	SpecialistCommerceChange().write(stream);
	SpecialistCommerceModifier().write(stream);
	BonusYieldModifier().write(stream);
	stream->Write(m_iWorkableRadius);
	stream->Write(m_iCultureFlipResistance);
	// </advc.003t>
}
#endif

bool CvBuildingInfo::read(CvXMLLoadUtility* pXML)
{
	// Legacy FTTW/K-Mod containers are parsed into temporary maps and folded
	// into ResourceCapacityOutputs before this info record is published.  The
	// class therefore has one runtime output representation, regardless of XML
	// spelling or source mod.
	BonusYieldChangeMap kLegacyBonusYieldChanges;
	BonusCommerceChangeMap kLegacyBonusCommerceChanges;
	BonusYieldChangeMap kLegacyVicinityBonusYieldChanges;
	BonusCommerceChangeMap kLegacyVicinityBonusCommerceChanges;
	if (!base_t::read(pXML))
		return false;

	pXML->SetInfoIDFromChildXmlVal(m_eBuildingClassType, "BuildingClass");
	pXML->SetInfoIDFromChildXmlVal(m_eSpecialBuildingType, "SpecialBuildingType");
	pXML->SetInfoIDFromChildXmlVal(m_eAdvisorType, "Advisor");
	pXML->GetChildXmlValByName(m_szArtDefineTag, "ArtDefineTag");
	pXML->GetChildXmlValByName(m_szMovieDefineTag, "MovieDefineTag");
	pXML->SetInfoIDFromChildXmlVal(m_eHolyCity, "HolyCity");
	pXML->SetInfoIDFromChildXmlVal(m_eReligionType, "ReligionType");
	pXML->SetInfoIDFromChildXmlVal(m_eStateReligion, "StateReligion");
	pXML->SetInfoIDFromChildXmlVal(m_ePrereqReligion, "PrereqReligion");
	pXML->SetInfoIDFromChildXmlVal(m_ePrereqCorporation, "PrereqCorporation");
	pXML->SetInfoIDFromChildXmlVal(m_eFoundsCorporation, "FoundsCorporation");
	pXML->SetInfoIDFromChildXmlVal(m_eGlobalReligionCommerce, "GlobalReligionCommerce");
	pXML->SetInfoIDFromChildXmlVal(m_eGlobalCorporationCommerce, "GlobalCorporationCommerce");
	pXML->SetInfoIDFromChildXmlVal(m_eVictoryPrereq, "VictoryPrereq");
	pXML->SetInfoIDFromChildXmlVal(m_eFreeStartEra, "FreeStartEra");
	pXML->SetInfoIDFromChildXmlVal(m_eMaxStartEra, "MaxStartEra");
	pXML->SetInfoIDFromChildXmlVal(m_eObsoleteTech, "ObsoleteTech");
	pXML->SetInfoIDFromChildXmlVal(m_ePrereqAndTech, "PrereqTech");

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(), "TechTypes"))
	{
		if (pXML->SkipToNextVal())
		{
			int const iNumSibs = gDLL->getXMLIFace()->GetNumChildren(pXML->GetXML());
			if (iNumSibs > 0)
			{
				CvString szTextVal;
				if (pXML->GetChildXmlVal(szTextVal))
				{	// advc.003t: The DLL can handle any number, but Python maybe not.
					FAssert(iNumSibs <= GC.getDefineINT(CvGlobals::NUM_BUILDING_AND_TECH_PREREQS));
					for (int j = 0; j < iNumSibs; j++)
					{	// <advc.003t>
						TechTypes eTech = (TechTypes)pXML->FindInInfoClass(szTextVal);
						if (eTech != NO_TECH)
							m_aePrereqAndTechs.push_back(eTech); // </advc.003t>
						if (!pXML->GetNextXmlVal(szTextVal))
							break;
					}
					gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
				}
			}
		}
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}

	pXML->SetInfoIDFromChildXmlVal(m_ePrereqAndBonus, "Bonus");

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(), "PrereqBonuses"))
	{
		if (pXML->SkipToNextVal())
		{
			int const iNumChildren = gDLL->getXMLIFace()->GetNumChildren(pXML->GetXML());
			if (iNumChildren > 0)
			{
				CvString szTextVal;
				if (pXML->GetChildXmlVal(szTextVal))
				{
					FAssert(iNumChildren <= GC.getDefineINT(CvGlobals::NUM_BUILDING_PREREQ_OR_BONUSES));
					for (int j = 0; j < iNumChildren; j++)
					{	// <advc.003t>
						BonusTypes eBonus = (BonusTypes)pXML->FindInInfoClass(szTextVal);
						if (eBonus != NO_BONUS)
							m_aePrereqOrBonuses.push_back(eBonus); // </advc.003t>
						if (!pXML->GetNextXmlVal(szTextVal))
							break;
					}
					gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
				}
			}
		}
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}


	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(), "PrereqAndCivics"))
	{
		if (pXML->SkipToNextVal())
		{
			int const iNumChildren = gDLL->getXMLIFace()->GetNumChildren(pXML->GetXML());
			if (iNumChildren > 0 && gDLL->getXMLIFace()->SetToChild(pXML->GetXML()))
			{
				for (int j = 0; j < iNumChildren; ++j)
				{
					CvString szTextVal;
					if (pXML->SkipToNextVal())
					{
						gDLL->getXMLIFace()->GetLastNodeValue(pXML->GetXML(), szTextVal);
						CivicTypes eCivic = (CivicTypes)pXML->FindInInfoClass(szTextVal);
						if (eCivic != NO_CIVIC)
							m_aePrereqAndCivics.push_back(eCivic);
					}
					if (!gDLL->getXMLIFace()->NextSibling(pXML->GetXML()))
						break;
				}
				gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
			}
		}
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(), "PrereqOrCivics"))
	{
		if (pXML->SkipToNextVal())
		{
			int const iNumChildren = gDLL->getXMLIFace()->GetNumChildren(pXML->GetXML());
			if (iNumChildren > 0 && gDLL->getXMLIFace()->SetToChild(pXML->GetXML()))
			{
				for (int j = 0; j < iNumChildren; ++j)
				{
					CvString szTextVal;
					if (pXML->SkipToNextVal())
					{
						gDLL->getXMLIFace()->GetLastNodeValue(pXML->GetXML(), szTextVal);
						CivicTypes eCivic = (CivicTypes)pXML->FindInInfoClass(szTextVal);
						if (eCivic != NO_CIVIC)
							m_aePrereqOrCivics.push_back(eCivic);
					}
					if (!gDLL->getXMLIFace()->NextSibling(pXML->GetXML()))
						break;
				}
				gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
			}
		}
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}

	// Fallout-compatible local-city prerequisite lists.  The building schema
	// stores these as direct text children rather than the normal key/value
	// arrays used by many BtS info records, so read them explicitly.
	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),
		"PrereqVicinityImprovements"))
	{
		if (pXML->SkipToNextVal())
		{
			int const iNumChildren = gDLL->getXMLIFace()->GetNumChildren(pXML->GetXML());
			if (iNumChildren > 0 && gDLL->getXMLIFace()->SetToChild(pXML->GetXML()))
			{
				m_pbPrereqVicinityImprovement = new bool[GC.getNumImprovementInfos()];
				memset(m_pbPrereqVicinityImprovement, 0,
					GC.getNumImprovementInfos() * sizeof(bool));
				for (int i = 0; i < iNumChildren; ++i)
				{
					CvString szTextVal;
					if (pXML->SkipToNextVal())
					{
						gDLL->getXMLIFace()->GetLastNodeValue(pXML->GetXML(), szTextVal);
						int const iImprovement = pXML->FindInInfoClass(szTextVal);
						if (iImprovement >= 0 && iImprovement < GC.getNumImprovementInfos())
							m_pbPrereqVicinityImprovement[iImprovement] = true;
					}
					if (!gDLL->getXMLIFace()->NextSibling(pXML->GetXML()))
						break;
				}
				gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
			}
		}
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),
		"PrereqVicinityFeatures"))
	{
		if (pXML->SkipToNextVal())
		{
			int const iNumChildren = gDLL->getXMLIFace()->GetNumChildren(pXML->GetXML());
			if (iNumChildren > 0 && gDLL->getXMLIFace()->SetToChild(pXML->GetXML()))
			{
				m_pbPrereqVicinityFeature = new bool[GC.getNumFeatureInfos()];
				memset(m_pbPrereqVicinityFeature, 0,
					GC.getNumFeatureInfos() * sizeof(bool));
				for (int i = 0; i < iNumChildren; ++i)
				{
					CvString szTextVal;
					if (pXML->SkipToNextVal())
					{
						gDLL->getXMLIFace()->GetLastNodeValue(pXML->GetXML(), szTextVal);
						int const iFeature = pXML->FindInInfoClass(szTextVal);
						if (iFeature >= 0 && iFeature < GC.getNumFeatureInfos())
							m_pbPrereqVicinityFeature[iFeature] = true;
					}
					if (!gDLL->getXMLIFace()->NextSibling(pXML->GetXML()))
						break;
				}
				gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
			}
		}
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}

	pXML->SetInfoIDFromChildXmlVal(m_ePrereqVicinityBonus, "PrereqVicinityBonus");
	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),
		"PrereqVicinityBonuses"))
	{
		if (pXML->SkipToNextVal())
		{
			int const iNumChildren = gDLL->getXMLIFace()->GetNumChildren(pXML->GetXML());
			if (iNumChildren > 0 && gDLL->getXMLIFace()->SetToChild(pXML->GetXML()))
			{
				int const iMax = GC.getDefineINT(CvGlobals::NUM_BUILDING_PREREQ_OR_BONUSES);
				m_piPrereqOrVicinityBonuses = new int[iMax];
				for (int i = 0; i < iMax; ++i)
					m_piPrereqOrVicinityBonuses[i] = NO_BONUS;
				for (int i = 0; i < iNumChildren && i < iMax; ++i)
				{
					CvString szTextVal;
					if (pXML->SkipToNextVal())
					{
						gDLL->getXMLIFace()->GetLastNodeValue(pXML->GetXML(), szTextVal);
						int const iBonus = pXML->FindInInfoClass(szTextVal);
						if (iBonus >= 0)
							m_piPrereqOrVicinityBonuses[i] = iBonus;
					}
					if (!gDLL->getXMLIFace()->NextSibling(pXML->GetXML()))
						break;
				}
				gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
			}
		}
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),
		"PrereqOrBuildingClasses"))
	{
		if (pXML->SkipToNextVal())
		{
			int const iNumChildren = gDLL->getXMLIFace()->GetNumChildren(pXML->GetXML());
			if (iNumChildren > 0 && gDLL->getXMLIFace()->SetToChild(pXML->GetXML()))
			{
				m_pbPrereqOrBuildingClass = new bool[GC.getNumBuildingClassInfos()];
				memset(m_pbPrereqOrBuildingClass, 0,
					GC.getNumBuildingClassInfos() * sizeof(bool));
				for (int i = 0; i < iNumChildren; ++i)
				{
					CvString szTextVal;
					if (pXML->SkipToNextVal())
					{
						gDLL->getXMLIFace()->GetLastNodeValue(pXML->GetXML(), szTextVal);
						int const iClass = pXML->FindInInfoClass(szTextVal);
						if (iClass >= 0 && iClass < GC.getNumBuildingClassInfos())
							m_pbPrereqOrBuildingClass[iClass] = true;
					}
					if (!gDLL->getXMLIFace()->NextSibling(pXML->GetXML()))
						break;
				}
				gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
			}
		}
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),
		"PrereqNotBuildingClasses"))
	{
		if (pXML->SkipToNextVal())
		{
			int const iNumChildren = gDLL->getXMLIFace()->GetNumChildren(pXML->GetXML());
			if (iNumChildren > 0 && gDLL->getXMLIFace()->SetToChild(pXML->GetXML()))
			{
				m_pbPrereqNotBuildingClass = new bool[GC.getNumBuildingClassInfos()];
				memset(m_pbPrereqNotBuildingClass, 0,
					GC.getNumBuildingClassInfos() * sizeof(bool));
				for (int i = 0; i < iNumChildren; ++i)
				{
					CvString szTextVal;
					if (pXML->SkipToNextVal())
					{
						gDLL->getXMLIFace()->GetLastNodeValue(pXML->GetXML(), szTextVal);
						int const iClass = pXML->FindInInfoClass(szTextVal);
						if (iClass >= 0 && iClass < GC.getNumBuildingClassInfos())
							m_pbPrereqNotBuildingClass[iClass] = true;
					}
					if (!gDLL->getXMLIFace()->NextSibling(pXML->GetXML()))
						break;
				}
				gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
			}
		}
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}

	pXML->GetChildXmlValByName(&m_iPrereqPopulation, "iPrereqPopulation", 0);
	pXML->GetChildXmlValByName(&m_bPrereqPower, "bPrereqPower", false);
	pXML->GetChildXmlValByName(&m_bPrereqNotVassal, "bPrereqNotVassal", false);
	{
		CvString szCulture;
		if (pXML->GetChildXmlValByName(szCulture, "PrereqCultureLevel", ""))
		{
			int const iCulture = pXML->FindInInfoClass(szCulture);
			if (iCulture >= 0)
				m_ePrereqCultureLevel = (CultureLevelTypes)iCulture;
		}
	}
	pXML->SetVariableListTagPair(ProductionTraits(), "ProductionTraits");
	pXML->SetVariableListTagPair(HappinessTraits(), "HappinessTraits");

	pXML->SetInfoIDFromChildXmlVal(m_eNoBonus, "NoBonus");
	pXML->SetInfoIDFromChildXmlVal(m_ePowerBonus, "PowerBonus");
	pXML->SetInfoIDFromChildXmlVal(m_eFreeBonus, "FreeBonus");

	pXML->GetChildXmlValByName(&m_iNumFreeBonuses, "iNumFreeBonuses");

	pXML->SetInfoIDFromChildXmlVal(m_eFreeBuildingClass, "FreeBuilding");
	pXML->SetInfoIDFromChildXmlVal(m_eFreePromotion, "FreePromotion");
	pXML->SetInfoIDFromChildXmlVal(m_eCivicOption, "CivicOption");
	pXML->SetInfoIDFromChildXmlVal(m_eGreatPeopleUnitClass, "GreatPeopleUnitClass");
	pXML->SetInfoIDFromChildXmlVal(m_eVoteSourceType, "DiploVoteType");

	pXML->GetChildXmlValByName(&m_iGreatPeopleRateChange, "iGreatPeopleRateChange");
	pXML->GetChildXmlValByName(&m_bTeamShare, "bTeamShare");
	pXML->GetChildXmlValByName(&m_bWater, "bWater");
	pXML->GetChildXmlValByName(&m_bRiver, "bRiver");
	pXML->GetChildXmlValByName(&m_bPower, "bPower");
	pXML->GetChildXmlValByName(&m_bDirtyPower, "bDirtyPower");
	pXML->GetChildXmlValByName(&m_bAreaCleanPower, "bAreaCleanPower");
	pXML->GetChildXmlValByName(&m_bAreaBorderObstacle, "bBorderObstacle");
	pXML->GetChildXmlValByName(&m_bConditional, "bConditional", false); // advc.310
	pXML->GetChildXmlValByName(&m_bForceTeamVoteEligible, "bForceTeamVoteEligible");
	pXML->GetChildXmlValByName(&m_bCapital, "bCapital");
	pXML->GetChildXmlValByName(&m_bGovernmentCenter, "bGovernmentCenter");
	pXML->GetChildXmlValByName(&m_bGoldenAge, "bGoldenAge");
	pXML->GetChildXmlValByName(&m_bAllowsNukes, "bAllowsNukes");
	pXML->GetChildXmlValByName(&m_bMapCentering, "bMapCentering");
	pXML->GetChildXmlValByName(&m_bNoUnhappiness, "bNoUnhappiness");
	//pXML->GetChildXmlValByName(&m_bNoUnhealthyPopulation, "bNoUnhealthyPopulation");
	pXML->GetChildXmlValByName(&m_iUnhealthyPopulationModifier, "iUnhealthyPopulationModifier"); // K-Mod
	pXML->GetChildXmlValByName(&m_bBuildingOnlyHealthy, "bBuildingOnlyHealthy");
	pXML->GetChildXmlValByName(&m_bNeverCapture, "bNeverCapture");
	pXML->GetChildXmlValByName(&m_bNukeImmune, "bNukeImmune");
	pXML->GetChildXmlValByName(&m_bPrereqReligion, "bPrereqReligion");
	pXML->GetChildXmlValByName(&m_bCenterInCity, "bCenterInCity");
	pXML->GetChildXmlValByName(&m_bStateReligion, "bStateReligion");
	pXML->GetChildXmlValByName(&m_bSlaveMarket, "bSlaveMarket", false);
	pXML->GetChildXmlValByName(&m_bPrereqVault, "bPrereqVault", false);
	pXML->GetChildXmlValByName(&m_bPrereqNoVault, "bPrereqNoVault", false);
	pXML->GetChildXmlValByName(&m_bFoundReligion, "bFoundReligion", false);
	pXML->GetChildXmlValByName(&m_iAIWeight, "iAIWeight");
	pXML->GetChildXmlValByName(&m_iProductionCost, "iCost");
	pXML->GetChildXmlValByName(&m_iHurryCostModifier, "iHurryCostModifier");
	pXML->GetChildXmlValByName(&m_iHurryAngerModifier, "iHurryAngerModifier");
	pXML->GetChildXmlValByName(&m_iAdvancedStartCost, "iAdvancedStartCost");
	pXML->GetChildXmlValByName(&m_iAdvancedStartCostIncrease, "iAdvancedStartCostIncrease");
	pXML->GetChildXmlValByName(&m_iWorkableRadius, "iWorkableRadius", 0);
	pXML->GetChildXmlValByName(&m_iMinAreaSize, "iMinAreaSize");
	pXML->GetChildXmlValByName(&m_iConquestProbability, "iConquestProb");
	pXML->GetChildXmlValByName(&m_iNumCitiesPrereq, "iCitiesPrereq");
	pXML->GetChildXmlValByName(&m_iNumTeamsPrereq, "iTeamsPrereq");
	pXML->GetChildXmlValByName(&m_iUnitLevelPrereq, "iLevelPrereq");
	pXML->GetChildXmlValByName(&m_iMinLatitude, "iMinLatitude");
	pXML->GetChildXmlValByName(&m_iMaxLatitude, "iMaxLatitude", 90);
	pXML->GetChildXmlValByName(&m_iGreatPeopleRateModifier, "iGreatPeopleRateModifier");
	pXML->GetChildXmlValByName(&m_iGreatGeneralRateModifier, "iGreatGeneralRateModifier");
	pXML->GetChildXmlValByName(&m_iDomesticGreatGeneralRateModifier, "iDomesticGreatGeneralRateModifier");
	pXML->GetChildXmlValByName(&m_iGlobalGreatPeopleRateModifier, "iGlobalGreatPeopleRateModifier");
	pXML->GetChildXmlValByName(&m_iAnarchyModifier, "iAnarchyModifier");
	pXML->GetChildXmlValByName(&m_iGoldenAgeModifier, "iGoldenAgeModifier");
	pXML->GetChildXmlValByName(&m_iGlobalHurryModifier, "iGlobalHurryModifier");
	pXML->GetChildXmlValByName(&m_iFreeExperience, "iExperience");
	pXML->GetChildXmlValByName(&m_iGlobalFreeExperience, "iGlobalExperience");
	pXML->GetChildXmlValByName(&m_iFoodKept, "iFoodKept");
	pXML->GetChildXmlValByName(&m_iAirlift, "iAirlift");
	pXML->GetChildXmlValByName(&m_iAirModifier, "iAirModifier");
	pXML->GetChildXmlValByName(&m_iAirUnitCapacity, "iAirUnitCapacity");
	// RESOURCE-CAPACITY-XML-BUILDING-01: keep the parser beside the optional
	// schema field (after iAirUnitCapacity) so the input contract stays readable.
	m_iResourceCapacityOutputCount = 0;
	for (int i = 0; i < FTTWRP1_MAX_BONUS_OUTPUTS; ++i)
	{
		m_aiResourceCapacityOutputResource[i] = NO_BONUS;
		m_aiResourceCapacityOutputType[i] = RESOURCE_CAPACITY_OUTPUT_NONE;
		m_aiResourceCapacityOutputAmount[i] = 0;
		m_aiResourceCapacityOutputScope[i] = RESOURCE_CAPACITY_OUTPUT_SCOPE_CITY;
	}
	{
		CvString szOutputs = "";
		// Resource Capacity is an optional extension.  Keep legacy AdvCiv/BtS
		// building records valid when they do not declare any outputs.
		if (pXML->GetChildXmlValByName(szOutputs, "ResourceCapacityOutputs", "") &&
			!szOutputs.IsEmpty())
		{
			std::vector<CvString> aOutputTokens;
			szOutputs.getTokens(";,", aOutputTokens);
			for (std::vector<CvString>::const_iterator it = aOutputTokens.begin();
				it != aOutputTokens.end() &&
				m_iResourceCapacityOutputCount < FTTWRP1_MAX_BONUS_OUTPUTS; ++it)
			{
				std::vector<CvString> aFields;
				it->getTokens(":=", aFields);
				if (aFields.size() < 3)
					continue;
				int iResource = pXML->FindInInfoClass(aFields[0]);
				int iOutputType = parseResourceCapacityOutputType(aFields[1]);
				int iAmount = atoi(aFields[2].c_str());
				if (iResource < 0 || iOutputType == FTTWR_BONUS_OUTPUT_NONE ||
					iAmount == 0)
					continue;
				int iScope = (aFields.size() >= 4) ?
					parseResourceCapacityOutputScope(aFields[3]) :
					RESOURCE_CAPACITY_OUTPUT_SCOPE_CITY;
				addResourceCapacityOutput(iResource, iOutputType, iAmount, iScope);
			}
		}
		// FTTW/K-Mod's original compact field is still accepted as an optional
		// input alias.  Append it after the canonical field so a migrated record
		// may contain both forms without silently dropping either behavior.
		szOutputs = "";
		if (pXML->GetChildXmlValByName(szOutputs, "FTTWRBonusOutputs", "") &&
			!szOutputs.IsEmpty())
		{
			std::vector<CvString> aOutputTokens;
			szOutputs.getTokens(";,", aOutputTokens);
			for (std::vector<CvString>::const_iterator it = aOutputTokens.begin();
				it != aOutputTokens.end() &&
				m_iResourceCapacityOutputCount < FTTWRP1_MAX_BONUS_OUTPUTS; ++it)
			{
				std::vector<CvString> aFields;
				it->getTokens(":=", aFields);
				if (aFields.size() < 3)
					continue;
				int iResource = pXML->FindInInfoClass(aFields[0]);
				int iOutputType = parseResourceCapacityOutputType(aFields[1]);
				int iAmount = atoi(aFields[2].c_str());
				if (iResource < 0 || iOutputType == FTTWR_BONUS_OUTPUT_NONE ||
					iAmount == 0)
					continue;
				addResourceCapacityOutput(iResource, iOutputType, iAmount,
					RESOURCE_CAPACITY_OUTPUT_SCOPE_CITY);
			}
		}
	}
	// Resource and population costs are optional and remain neutral for legacy
	// building records that omit the new fields.
	m_iResourceCapacityCostCount = 0;
	m_iResourceCapacityPopulationCost = 0;
	for (int i = 0; i < FTTWRP1_MAX_CAPACITY_COSTS; ++i)
	{
		m_aiResourceCapacityCostResource[i] = NO_BONUS;
		m_aiResourceCapacityCost[i] = 0;
	}
	pXML->GetChildXmlValByName(&m_iResourceCapacityPopulationCost,
		"PopulationCapacityCost", 0);
	m_iResourceCapacityPopulationCost = std::max(0,
		m_iResourceCapacityPopulationCost);
	{
		CvString szCosts = "";
		if (pXML->GetChildXmlValByName(szCosts, "ResourceCapacityCosts", "") &&
			!szCosts.IsEmpty())
		{
			std::vector<CvString> aCostTokens;
			szCosts.getTokens(";,", aCostTokens);
			for (std::vector<CvString>::const_iterator it = aCostTokens.begin();
				it != aCostTokens.end() &&
				m_iResourceCapacityCostCount < FTTWRP1_MAX_CAPACITY_COSTS; ++it)
			{
				std::vector<CvString> aFields;
				it->getTokens(":=", aFields);
				if (aFields.size() < 2)
					continue;
				int const iResource = pXML->FindInInfoClass(aFields[0]);
				int const iCost = atoi(aFields[1].c_str());
				if (iResource >= 0 && iCost > 0)
					addResourceCapacityCost(iResource, iCost);
			}
		}
	}
	pXML->GetChildXmlValByName(&m_iNukeModifier, "iNukeModifier");
	pXML->GetChildXmlValByName(&m_iNukeExplosionRand, "iNukeExplosionRand");
	pXML->GetChildXmlValByName(&m_iFreeSpecialist, "iFreeSpecialist");
	pXML->GetChildXmlValByName(&m_iAreaFreeSpecialist, "iAreaFreeSpecialist");
	pXML->GetChildXmlValByName(&m_iGlobalFreeSpecialist, "iGlobalFreeSpecialist");
	pXML->GetChildXmlValByName(&m_iMaintenanceModifier, "iMaintenanceModifier");
	pXML->GetChildXmlValByName(&m_iWarWearinessModifier, "iWarWearinessModifier");
	pXML->GetChildXmlValByName(&m_iGlobalWarWearinessModifier, "iGlobalWarWearinessModifier");
	pXML->GetChildXmlValByName(&m_iEnemyWarWearinessModifier, "iEnemyWarWearinessModifier");
	pXML->GetChildXmlValByName(&m_iHealRateChange, "iHealRateChange");
	pXML->GetChildXmlValByName(&m_iHealth, "iHealth");
	pXML->GetChildXmlValByName(&m_iAreaHealth, "iAreaHealth");
	pXML->GetChildXmlValByName(&m_iGlobalHealth, "iGlobalHealth");
	pXML->GetChildXmlValByName(&m_iHappiness, "iHappiness");
	pXML->GetChildXmlValByName(&m_iAreaHappiness, "iAreaHappiness");
	pXML->GetChildXmlValByName(&m_iGlobalHappiness, "iGlobalHappiness");
	pXML->GetChildXmlValByName(&m_iStateReligionHappiness, "iStateReligionHappiness");
	pXML->GetChildXmlValByName(&m_iWorkerSpeedModifier, "iWorkerSpeedModifier");
	pXML->GetChildXmlValByName(&m_iMilitaryProductionModifier, "iMilitaryProductionModifier");
	pXML->GetChildXmlValByName(&m_iSpaceProductionModifier, "iSpaceProductionModifier");
	pXML->GetChildXmlValByName(&m_iGlobalSpaceProductionModifier, "iGlobalSpaceProductionModifier");
	pXML->GetChildXmlValByName(&m_iTradeRoutes, "iTradeRoutes");
	pXML->GetChildXmlValByName(&m_iCoastalTradeRoutes, "iCoastalTradeRoutes");
	pXML->GetChildXmlValByName(&m_iAreaTradeRoutes, "iAreaTradeRoutes"); // advc.310
	pXML->GetChildXmlValByName(&m_iTradeRouteModifier, "iTradeRouteModifier");
	pXML->GetChildXmlValByName(&m_iForeignTradeRouteModifier, "iForeignTradeRouteModifier");
	pXML->GetChildXmlValByName(&m_iGlobalPopulationChange, "iGlobalPopulationChange");
	pXML->GetChildXmlValByName(&m_iFreeTechs, "iFreeTechs");
	pXML->GetChildXmlValByName(&m_iDefenseModifier, "iDefense");
	pXML->GetChildXmlValByName(&m_iBombardDefenseModifier, "iBombardDefense");
	pXML->GetChildXmlValByName(&m_iAllCityDefenseModifier, "iAllCityDefense");
	pXML->GetChildXmlValByName(&m_iEspionageDefenseModifier, "iEspionageDefense");
	pXML->GetChildXmlValByName(&m_iCultureFlipResistance, "iCultureFlipResistance", 0);
	pXML->GetChildXmlValByName(&m_iAssetValue, "iAsset");
	pXML->GetChildXmlValByName(&m_iPowerValue, "iPower");
	pXML->GetChildXmlValByName(&m_fVisibilityPriority, "fVisibilityPriority");

	pXML->SetYieldList(SeaPlotYieldChange(), "SeaPlotYieldChanges");
	pXML->SetYieldList(RiverPlotYieldChange(), "RiverPlotYieldChanges");
	pXML->SetYieldList(GlobalSeaPlotYieldChange(), "GlobalSeaPlotYieldChanges");
	pXML->SetYieldList(YieldChange(), "YieldChanges");
	pXML->SetYieldList(YieldModifier(), "YieldModifiers");
	pXML->SetYieldList(PowerYieldModifier(), "PowerYieldModifiers");
	pXML->SetYieldList(AreaYieldModifier(), "AreaYieldModifiers");
	pXML->SetYieldList(GlobalYieldModifier(), "GlobalYieldModifiers");

	pXML->SetCommerceList(CommerceChange(), "CommerceChanges");
	pXML->SetCommerceList(ObsoleteSafeCommerceChange(), "ObsoleteSafeCommerceChanges");
	pXML->SetCommerceList(CommerceChangeDoubleTime(), "CommerceChangeDoubleTimes");
	pXML->SetCommerceList(CommerceModifier(), "CommerceModifiers");
	pXML->SetCommerceList(GlobalCommerceModifier(), "GlobalCommerceModifiers");
	pXML->SetCommerceList(SpecialistExtraCommerce(), "SpecialistExtraCommerces");
	pXML->SetCommerceList(StateReligionCommerce(), "StateReligionCommerces");
	pXML->SetCommerceList(CommerceHappiness(), "CommerceHappinesses");

	pXML->SetVariableListTagPair(ReligionChange(), "ReligionChanges");
	pXML->SetVariableListTagPair(SpecialistCount(), "SpecialistCounts");
	pXML->SetVariableListTagPair(FreeSpecialistCount(), "FreeSpecialistCounts");

	pXML->SetVariableListPerCommerce(CommerceFlexible(), "CommerceFlexibles");
	pXML->SetVariableListPerCommerce(CommerceChangeOriginalOwner(), "CommerceChangeOriginalOwners");

	pXML->GetChildXmlValByName(m_szConstructSound, "ConstructSound");

	pXML->SetVariableListTagPair(BonusHealthChanges(), "BonusHealthChanges");
	pXML->SetVariableListTagPair(BonusHappinessChanges(), "BonusHappinessChanges");
	pXML->SetVariableListTagPair(BonusProductionModifier(), "BonusProductionModifiers");
	pXML->SetVariableListTagPair(UnitCombatFreeExperience(), "UnitCombatFreeExperiences");
	pXML->SetVariableListTagPair(DomainFreeExperience(), "DomainFreeExperiences");
	pXML->SetVariableListTagPair(DomainProductionModifier(), "DomainProductionModifiers");

	pXML->SetVariableListTagPair(PrereqNumOfBuildingClass(), "PrereqBuildingClasses");
	pXML->SetVariableListTagPair(BuildingClassNeededInCity(), "BuildingClassNeededs");

	pXML->SetVariableListTagRate(this->SpecialistYieldChange(),
			"SpecialistYieldChange", "SpecialistType", "YieldChanges");
	pXML->SetVariableListTagRate(this->SpecialistYieldModifier(),
			"SpecialistYieldModifier", "SpecialistType", "YieldModifiers");
	pXML->SetVariableListTagRate(this->SpecialistCommerceChange(),
			"SpecialistCommerceChange", "SpecialistType", "CommerceChanges");
	pXML->SetVariableListTagRate(this->SpecialistCommerceModifier(),
			"SpecialistCommerceModifier", "SpecialistType", "CommerceModifiers");
	pXML->SetVariableListTagRate(BonusYieldModifier(),
			"BonusYieldModifier", "BonusType", "YieldModifiers");
	// Legacy FTTW/K-Mod aliases.  They are optional input spellings only;
	// temporary maps are folded into the one ResourceCapacity output array below.
	pXML->SetVariableListTagRate(kLegacyBonusCommerceChanges,
			"BonusCommerceChange", "BonusType", "CommerceChanges");
	pXML->SetVariableListTagRate(kLegacyBonusYieldChanges,
			"BonusYieldChange", "BonusType", "YieldChanges");
	// The legacy vicinity containers use Bonus*Change children rather than a
	// distinct child tag.  Temporarily enter each optional container so the
	// existing AdvCiv iterator parses the same nested shape without a second
	// map implementation.
	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),
		"VicinityBonusCommerceChanges"))
	{
		pXML->SetVariableListTagRate(kLegacyVicinityBonusCommerceChanges,
			"BonusCommerceChange", "BonusType", "CommerceChanges");
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),
		"VicinityBonusYieldChanges"))
	{
		pXML->SetVariableListTagRate(kLegacyVicinityBonusYieldChanges,
			"BonusYieldChange", "BonusType", "YieldChanges");
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	FOR_EACH_NON_DEFAULT_PAIR(kLegacyBonusYieldChanges, Bonus, YieldChangeMap)
	{
		FOR_EACH_NON_DEFAULT_PAIR(perBonusVal.second, Yield, int)
			addResourceCapacityOutput(perBonusVal.first,
				RESOURCE_CAPACITY_OUTPUT_YIELD_BASE + perYieldVal.first,
				perYieldVal.second, RESOURCE_CAPACITY_OUTPUT_SCOPE_CITY);
	}
	FOR_EACH_NON_DEFAULT_PAIR(kLegacyBonusCommerceChanges, Bonus, CommerceChangeMap)
	{
		FOR_EACH_NON_DEFAULT_PAIR(perBonusVal.second, Commerce, int)
			addResourceCapacityOutput(perBonusVal.first,
				RESOURCE_CAPACITY_OUTPUT_COMMERCE_BASE + perCommerceVal.first,
				perCommerceVal.second, RESOURCE_CAPACITY_OUTPUT_SCOPE_CITY);
	}
	FOR_EACH_NON_DEFAULT_PAIR(kLegacyVicinityBonusYieldChanges, Bonus, YieldChangeMap)
	{
		FOR_EACH_NON_DEFAULT_PAIR(perBonusVal.second, Yield, int)
			addResourceCapacityOutput(perBonusVal.first,
				RESOURCE_CAPACITY_OUTPUT_YIELD_BASE + perYieldVal.first,
				perYieldVal.second, RESOURCE_CAPACITY_OUTPUT_SCOPE_VICINITY);
	}
	FOR_EACH_NON_DEFAULT_PAIR(kLegacyVicinityBonusCommerceChanges, Bonus, CommerceChangeMap)
	{
		FOR_EACH_NON_DEFAULT_PAIR(perBonusVal.second, Commerce, int)
			addResourceCapacityOutput(perBonusVal.first,
				RESOURCE_CAPACITY_OUTPUT_COMMERCE_BASE + perCommerceVal.first,
				perCommerceVal.second, RESOURCE_CAPACITY_OUTPUT_SCOPE_VICINITY);
	}
	pXML->SetVariableListTagPair(FlavorValue(), "Flavors");
	pXML->SetVariableListTagPair(ImprovementFreeSpecialist(), "ImprovementFreeSpecialists");
	pXML->SetVariableListTagPair(BuildingHappinessChanges(), "BuildingHappinessChanges");

	return true;
}
// <advc.310>
bool CvBuildingInfo::m_bEnabledDomesticGreatGeneralRateModifier = true;
bool CvBuildingInfo::m_bEnabledAreaTradeRoutes = true;
bool CvBuildingInfo::m_bEnabledAreaBorderObstacle = true;
void CvBuildingInfo::setDomesticGreatGeneralRateModifierEnabled(bool b) {
	m_bEnabledDomesticGreatGeneralRateModifier = b;
}
void CvBuildingInfo::setAreaTradeRoutesEnabled(bool b) {
	m_bEnabledAreaTradeRoutes = b;
}
void CvBuildingInfo::setAreaBorderObstacleEnabled(bool b) {
	m_bEnabledAreaBorderObstacle = b;
} // </advc.310>

CvBuildingClassInfo::CvBuildingClassInfo() :
m_iMaxGlobalInstances(0),
m_iMaxTeamInstances(0),
m_iMaxPlayerInstances(0),
m_iExtraPlayerInstances(0),
m_iDefaultBuildingIndex(NO_BUILDING),
m_bNoLimit(false),
m_bMonument(false)
{}

// advc: Moved from CvGameCoreUtils, renamed from "limitedWonderClassLimit".
int CvBuildingClassInfo::getLimit() const
{
	int iCount = 0;
	bool bLimited = false;

	int iMax = getMaxGlobalInstances();
	if (iMax != -1)
	{
		iCount += iMax;
		bLimited = true;
	}

	iMax = getMaxTeamInstances();
	if (iMax != -1)
	{
		iCount += iMax;
		bLimited = true;
	}

	iMax = getMaxPlayerInstances();
	if (iMax != -1)
	{
		iCount += iMax;
		bLimited = true;
	}

	return (bLimited ? iCount : -1);
}

bool CvBuildingClassInfo::read(CvXMLLoadUtility* pXML)
{
	if (!base_t::read(pXML))
		return false;

	pXML->GetChildXmlValByName(&m_iMaxGlobalInstances, "iMaxGlobalInstances");
	pXML->GetChildXmlValByName(&m_iMaxTeamInstances, "iMaxTeamInstances");
	pXML->GetChildXmlValByName(&m_iMaxPlayerInstances, "iMaxPlayerInstances");
	pXML->GetChildXmlValByName(&m_iExtraPlayerInstances, "iExtraPlayerInstances");

	pXML->GetChildXmlValByName(&m_bNoLimit, "bNoLimit");
	pXML->GetChildXmlValByName(&m_bMonument, "bMonument");

	pXML->SetVariableListTagPair(VictoryThreshold(), "VictoryThresholds");

	CvString szTextVal;
	pXML->GetChildXmlValByName(szTextVal, "DefaultBuilding");
	m_aszExtraXMLforPass3.push_back(szTextVal);

	return true;
}

bool CvBuildingClassInfo::readPass3()
{
	if (m_aszExtraXMLforPass3.size() < 1)
	{
		FAssert(false);
		return false;
	}

	m_iDefaultBuildingIndex = GC.getInfoTypeForString(m_aszExtraXMLforPass3[0]);
	m_aszExtraXMLforPass3.clear();

	return true;
}

CvSpecialBuildingInfo::CvSpecialBuildingInfo() :
m_eObsoleteTech(NO_TECH),
m_eTechPrereq(NO_TECH),
m_eTechPrereqAnyone(NO_TECH),
m_bValid(false)
{}

bool CvSpecialBuildingInfo::read(CvXMLLoadUtility* pXML)
{
	if (!base_t::read(pXML))
		return false;

	pXML->SetInfoIDFromChildXmlVal(m_eObsoleteTech, "ObsoleteTech");
	pXML->SetInfoIDFromChildXmlVal(m_eTechPrereq, "TechPrereq");
	pXML->SetInfoIDFromChildXmlVal(m_eTechPrereqAnyone, "TechPrereqAnyone");

	pXML->GetChildXmlValByName(&m_bValid, "bValid");

	pXML->SetVariableListTagPair(ProductionTraits(), "ProductionTraits");

	return true;
}

CvVoteSourceInfo::CvVoteSourceInfo() :
	m_iVoteInterval(0),
	m_eFreeSpecialist(NO_SPECIALIST),
	m_eCivic(NO_CIVIC)
{}

const CvWString CvVoteSourceInfo::getPopupText() const
{
	return gDLL->getText(m_szPopupText);
}

const CvWString CvVoteSourceInfo::getSecretaryGeneralText() const
{
	return gDLL->getText(m_szSecretaryGeneralText);
}

bool CvVoteSourceInfo::read(CvXMLLoadUtility* pXML)
{
	if (!base_t::read(pXML))
		return false;

	pXML->GetChildXmlValByName(&m_iVoteInterval, "iVoteInterval");
	pXML->GetChildXmlValByName(m_szPopupText, "PopupText");
	pXML->GetChildXmlValByName(m_szSecretaryGeneralText, "SecretaryGeneralText");

	pXML->SetInfoIDFromChildXmlVal(m_eFreeSpecialist, "FreeSpecialist");
	{
		CvString szTextVal;
		pXML->GetChildXmlValByName(szTextVal, "Civic");
		m_aszExtraXMLforPass3.push_back(szTextVal);
	}
	pXML->SetYieldList(ReligionYield(), "ReligionYields");
	pXML->SetCommerceList(ReligionCommerce(), "ReligionCommerces");

	return true;
}

bool CvVoteSourceInfo::readPass3()
{
	if (m_aszExtraXMLforPass3.size() < 1)
	{
		FAssert(false);
		return false;
	}

	m_eCivic = (CivicTypes)GC.getInfoTypeForString(m_aszExtraXMLforPass3[0]);
	FAssertInfoEnum(m_eCivic); // advc
	m_aszExtraXMLforPass3.clear();

	return true;
}

CvVoteInfo::CvVoteInfo() :
m_iPopulationThreshold(0),
m_iStateReligionVotePercent(0),
m_iTradeRoutes(0),
m_iMinVoters(0),
m_bSecretaryGeneral(false),
m_bVictory(false),
m_bFreeTrade(false),
m_bNoNukes(false),
m_bCityVoting(false),
m_bCivVoting(false),
m_bDefensivePact(false),
m_bOpenBorders(false),
m_bForcePeace(false),
m_bForceNoTrade(false),
m_bForceWar(false),
m_bAssignCity(false)
{}

bool CvVoteInfo::read(CvXMLLoadUtility* pXML)
{
	if (!base_t::read(pXML))
		return false;

	pXML->GetChildXmlValByName(&m_iPopulationThreshold, "iPopulationThreshold");
	pXML->GetChildXmlValByName(&m_iStateReligionVotePercent, "iStateReligionVotePercent");
	pXML->GetChildXmlValByName(&m_iTradeRoutes, "iTradeRoutes");
	pXML->GetChildXmlValByName(&m_iMinVoters, "iMinVoters");

	pXML->GetChildXmlValByName(&m_bSecretaryGeneral, "bSecretaryGeneral");
	pXML->GetChildXmlValByName(&m_bVictory, "bVictory");
	pXML->GetChildXmlValByName(&m_bFreeTrade, "bFreeTrade");
	pXML->GetChildXmlValByName(&m_bNoNukes, "bNoNukes");
	pXML->GetChildXmlValByName(&m_bCityVoting, "bCityVoting");
	pXML->GetChildXmlValByName(&m_bCivVoting, "bCivVoting");
	pXML->GetChildXmlValByName(&m_bDefensivePact, "bDefensivePact");
	pXML->GetChildXmlValByName(&m_bOpenBorders, "bOpenBorders");
	pXML->GetChildXmlValByName(&m_bForcePeace, "bForcePeace");
	pXML->GetChildXmlValByName(&m_bForceNoTrade, "bForceNoTrade");
	pXML->GetChildXmlValByName(&m_bForceWar, "bForceWar");
	pXML->GetChildXmlValByName(&m_bAssignCity, "bAssignCity");

	pXML->SetVariableListTagPair(ForceCivic(), "ForceCivics");
	pXML->SetVariableListTagPair(VoteSourceType(), "DiploVotes");

	return true;
}

CvProjectInfo::CvProjectInfo() :
m_eVictoryPrereq(NO_VICTORY),
m_eTechPrereq(NO_TECH),
m_eAnyoneProjectPrereq(NO_PROJECT),
m_iMaxGlobalInstances(0),
m_iMaxTeamInstances(0),
m_iProductionCost(0),
m_iNukeInterception(0),
m_iTechShare(0),
m_eEveryoneSpecialUnit(NO_SPECIALUNIT),
m_eEveryoneSpecialBuilding(NO_SPECIALBUILDING),
m_iVictoryDelayPercent(0),
m_iSuccessRate(0),
m_bSpaceship(false),
m_bAllowsNukes(false)
{}

const char* CvProjectInfo::getMovieArtDef() const
{
	return m_szMovieArtDef;
}

const TCHAR* CvProjectInfo::getCreateSound() const
{
	return m_szCreateSound;
}

void CvProjectInfo::setCreateSound(const TCHAR* szVal)
{
	m_szCreateSound = szVal;
}

// advc.008e:
bool CvProjectInfo::nameNeedsArticle() const
{
	if(!isLimited())
		return false;
	CvWString szKey = getTextKeyWide();
	// (see comment in CvBuildingInfo::nameNeedsArticle)
	CvWString szText = gDLL->getText(szKey + L"_NA");
	return (szText.compare(L".") != 0);
}

bool CvProjectInfo::read(CvXMLLoadUtility* pXML)
{
	if (!base_t::read(pXML))
		return false;

	pXML->SetInfoIDFromChildXmlVal(m_eVictoryPrereq, "VictoryPrereq");
	pXML->SetInfoIDFromChildXmlVal(m_eTechPrereq, "TechPrereq");

	pXML->GetChildXmlValByName(&m_iMaxGlobalInstances, "iMaxGlobalInstances");
	pXML->GetChildXmlValByName(&m_iMaxTeamInstances, "iMaxTeamInstances");
	pXML->GetChildXmlValByName(&m_iProductionCost, "iCost");
	pXML->GetChildXmlValByName(&m_iNukeInterception, "iNukeInterception");
	pXML->GetChildXmlValByName(&m_iTechShare, "iTechShare");
	FAssertBounds(0, MAX_PLAYERS, m_iTechShare); // advc

	pXML->SetInfoIDFromChildXmlVal(m_eEveryoneSpecialUnit, "EveryoneSpecialUnit");
	pXML->SetInfoIDFromChildXmlVal(m_eEveryoneSpecialBuilding, "EveryoneSpecialBuilding");

	pXML->GetChildXmlValByName(&m_bSpaceship, "bSpaceship");
	pXML->GetChildXmlValByName(&m_bAllowsNukes, "bAllowsNukes");
	pXML->GetChildXmlValByName(m_szMovieArtDef, "MovieDefineTag");

	pXML->SetVariableListTagPair(BonusProductionModifier(), "BonusProductionModifiers");
	pXML->SetVariableListTagPair(VictoryThreshold(), "VictoryThresholds");
	pXML->SetVariableListTagPair(VictoryMinThreshold(), "VictoryMinThresholds");
	/*	<advc.003t> Min threshold defaults to the regular threshold.
		(BtS had handled this in the getter.) */
	if (VictoryThreshold().isAnyNonDefault())
	{
		FOR_EACH_ENUM(Victory)
		{
			VictoryMinThreshold().set(eLoopVictory,
					getVictoryMinThreshold(eLoopVictory) == 0 ?
					getVictoryThreshold(eLoopVictory) :
					getVictoryMinThreshold(eLoopVictory));
		}
	} // </advc.003t>
	pXML->GetChildXmlValByName(&m_iVictoryDelayPercent, "iVictoryDelayPercent");
	pXML->GetChildXmlValByName(&m_iSuccessRate, "iSuccessRate");

	CvString szTextVal;
	pXML->GetChildXmlValByName(szTextVal, "CreateSound");
	setCreateSound(szTextVal);

	return true;
}

bool CvProjectInfo::readPass2(CvXMLLoadUtility* pXML)
{
	pXML->SetVariableListTagPair(ProjectsNeeded(), "PrereqProjects");
	pXML->SetInfoIDFromChildXmlVal(m_eAnyoneProjectPrereq, "AnyonePrereqProject");

	return true;
}
