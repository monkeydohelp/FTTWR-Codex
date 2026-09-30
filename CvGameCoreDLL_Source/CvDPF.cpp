#include "CvGameCoreDLL.h"
#include "CvDPF.h"
#include "CvGame.h"
#include "CvPlayer.h"
#include "CvXMLLoadUtility.h"
#include "BBAILog.h"

#include <cstring>

namespace
{
	const int DPF_SAVE_VERSION = 2;
	const int DPF_LEGACY_SAVE_VERSION = 1;
	const int DPF_MAX_EVENTS = 256;
	const int DPF_MAX_CHOICE_OPTIONS = 8;
	const int DPF_MAX_INSTANCES = 32;
	const int DPF_MAX_ENTRIES = 128;
	const int DPF_MAX_MARKERS = 128;
	const int DPF_MAX_TRACE = 64;
	const int DPF_MAX_SETTLEMENT_WAVES = 16;
	const int DPF_TEST_LINEAR = 1;
	const int DPF_TEST_BLOCKED = 2;
	const int DPF_TEST_FAILED = 3;
	const int DPF_TEST_SHARED_TRAIT = -1;

	enum DPFConditionType
	{
		DPF_CONDITION_TRUE = 0,
		DPF_CONDITION_EVENT_OCCURRED,
		DPF_CONDITION_EVENT_COUNT,
		DPF_CONDITION_VARIABLE_AT_LEAST,
		DPF_CONDITION_STATUS_STAGE,
		DPF_CONDITION_AND,
		DPF_CONDITION_OR,
		DPF_CONDITION_NOT
	};

	enum DPFEffectType
	{
		DPF_EFFECT_INCREMENT_VARIABLE = 1,
		DPF_EFFECT_SET_VARIABLE,
		DPF_EFFECT_SET_MARKER,
		DPF_EFFECT_GRANT_TRAIT,
		DPF_EFFECT_RELEASE_TRAIT,
		DPF_EFFECT_TRACE
	};

	struct DPFCondition
	{
		int type;
		int eventType;
		int variable;
		int value;
		int status;
		int stage;
		std::vector<DPFCondition> children;

		DPFCondition() : type(DPF_CONDITION_TRUE), eventType(0), variable(0),
			value(0), status(0), stage(0) {}
	};

	struct DPFEffect
	{
		int type;
		int id;
		int value;
		int marker;

		DPFEffect() : type(0), id(0), value(0), marker(0) {}
	};
	struct DPFChoiceOption
	{
		int id;
		CvString displayName;
		CvString description;
		int aiScore;
		CvString aiReason;
		int toStatus;
		int toStage;
		std::vector<DPFEffect> effects;

		DPFChoiceOption() : id(0), displayName(""), description(""),
			aiScore(0), aiReason(""), toStatus(-1), toStage(-1) {}
	};


	struct DPFStage
	{
		int id;
		CvString displayName;
		CvString hint;
		int choiceRequestId;
		int choiceExpiryTurns;
		int choiceDefaultOption;
		CvString choiceDisplayName;
		CvString choiceDescription;
		std::vector<DPFChoiceOption> choiceOptions;
		std::vector<DPFEffect> enterEffects;
		std::vector<DPFEffect> exitEffects;
		std::vector<DPFEffect> activeEffects;

		DPFStage() : id(0), displayName(""), hint(""), choiceRequestId(0),
			choiceExpiryTurns(0), choiceDefaultOption(0), choiceDisplayName(""),
			choiceDescription("") {}
	};

	struct DPFTransition
	{
		int id;
		int fromStatus;
		int fromStage;
		int toStatus;
		int toStage;
		int priority;
		DPFCondition condition;
		std::vector<DPFEffect> effects;

		DPFTransition() : id(0), fromStatus(DPF_STATUS_DORMANT), fromStage(0),
			toStatus(DPF_STATUS_DORMANT), toStage(0), priority(0) {}
	};

	struct DPFDefinition
	{
		int id;
		CvString name;
		CvString displayName;
		CvString description;
		int initialStatus;
		int initialStage;
		std::vector<DPFStage> stages;
		std::vector<DPFTransition> transitions;

		DPFDefinition() : id(0), name(""), displayName(""), description(""),
			initialStatus(DPF_STATUS_DORMANT), initialStage(0) {}
	};

	struct DPFEvent
	{
		int type;
		int turn;
		int actor;
		int owner;
		int x;
		int y;
		int object;
		int amount;
		int sequence;

		DPFEvent() : type(0), turn(0), actor(-1), owner(-1), x(-1), y(-1),
			object(-1), amount(1), sequence(0) {}
	};

	struct DPFInstance
	{
		int definitionId;
		int status;
		int stage;
		std::map<int, int> variables;
		std::map<int, int> eventCounts;
		std::set<int> markers;

		DPFInstance() : definitionId(0), status(DPF_STATUS_DORMANT), stage(0) {}
	};
	struct DPFPendingChoice
	{
		bool active;
		int definitionId;
		int requestId;
		int stageId;
		int creationTurn;
		int expiryTurn;
		int defaultOption;
		std::vector<int> optionIds;

		DPFPendingChoice() : active(false), definitionId(-1), requestId(0),
			stageId(-1), creationTurn(0), expiryTurn(0), defaultOption(0) {}
	};


	struct DPFPlayerState
	{
		bool initialized;
		int nextSequence;
		std::vector<DPFEvent> pendingEvents;
		std::map<int, DPFInstance> instances;
		std::map<int, std::set<int> > traitClaims;
		DPFPendingChoice pendingChoice;
		std::vector<CvString> trace;

		DPFPlayerState() : initialized(false), nextSequence(1) {}
	};

	bool g_initialized = false;
	bool g_xmlAttempted = false;
	std::vector<DPFPlayerState> g_players;
	std::map<int, DPFDefinition> g_definitions;

	bool validPlayer(PlayerTypes ePlayer)
	{
		return ePlayer >= 0 && ePlayer < MAX_PLAYERS;
	}

	DPFPlayerState* stateFor(PlayerTypes ePlayer)
	{
		if (!g_initialized || !validPlayer(ePlayer))
			return NULL;
		return &g_players[(int)ePlayer];
	}

	void trace(DPFPlayerState& state, CvString const& szMessage)
	{
		state.trace.push_back(szMessage);
		while ((int)state.trace.size() > DPF_MAX_TRACE)
			state.trace.erase(state.trace.begin());
		logBBAI("%s", szMessage.GetCString());
	}

	DPFCondition conditionEvent(int iEventType)
	{
		DPFCondition result;
		result.type = DPF_CONDITION_EVENT_OCCURRED;
		result.eventType = iEventType;
		return result;
	}

	DPFCondition conditionEventCount(int iEventType, int iMinimum)
	{
		DPFCondition result;
		result.type = DPF_CONDITION_EVENT_COUNT;
		result.eventType = iEventType;
		result.value = iMinimum;
		return result;
	}

	DPFCondition conditionVariable(int iVariable, int iMinimum)
	{
		DPFCondition result;
		result.type = DPF_CONDITION_VARIABLE_AT_LEAST;
		result.variable = iVariable;
		result.value = iMinimum;
		return result;
	}

	DPFCondition conditionAnd(DPFCondition const& first, DPFCondition const& second)
	{
		DPFCondition result;
		result.type = DPF_CONDITION_AND;
		result.children.push_back(first);
		result.children.push_back(second);
		return result;
	}


	int parseIntegerToken(CvString const& szToken, int& iValue)
	{
		char* pEnd = NULL;
		long const iParsed = std::strtol(szToken.c_str(), &pEnd, 10);
		if (pEnd == szToken.c_str() || *pEnd != '\0')
			return false;
		iValue = (int)iParsed;
		return true;
	}

	int statusFromToken(CvString const& szToken)
	{
		if (szToken.CompareNoCase("DORMANT") == 0) return DPF_STATUS_DORMANT;
		if (szToken.CompareNoCase("ACTIVE") == 0) return DPF_STATUS_ACTIVE;
		if (szToken.CompareNoCase("BLOCKED") == 0) return DPF_STATUS_BLOCKED;
		if (szToken.CompareNoCase("FAILED") == 0) return DPF_STATUS_FAILED;
		if (szToken.CompareNoCase("COMPLETED") == 0) return DPF_STATUS_COMPLETED;
		if (szToken.CompareNoCase("WAITING_FOR_CHOICE") == 0) return DPF_STATUS_WAITING_FOR_CHOICE;
		int iValue = 0;
		return parseIntegerToken(szToken, iValue) ? iValue : -1;
	}

	char const* statusName(int iStatus)
	{
		switch (iStatus)
		{
		case DPF_STATUS_DORMANT: return "Not started";
		case DPF_STATUS_ACTIVE: return "In progress";
		case DPF_STATUS_BLOCKED: return "Blocked";
		case DPF_STATUS_FAILED: return "Failed";
		case DPF_STATUS_COMPLETED: return "Completed";
		case DPF_STATUS_WAITING_FOR_CHOICE: return "Choice required";
		default: return "Unknown";
		}
	}

	int eventTypeFromToken(CvString const& szToken)
	{
		if (szToken.CompareNoCase("IMPROVEMENT_COMPLETED") == 0) return DPF_EVENT_IMPROVEMENT_COMPLETED;
		if (szToken.CompareNoCase("FEATURE_REMOVED") == 0) return DPF_EVENT_FEATURE_REMOVED;
		if (szToken.CompareNoCase("TECHNOLOGY_ACQUIRED") == 0) return DPF_EVENT_TECHNOLOGY_ACQUIRED;
		if (szToken.CompareNoCase("CIVIC_CHANGED") == 0) return DPF_EVENT_CIVIC_CHANGED;
		if (szToken.CompareNoCase("TRAIT_CHANGED") == 0) return DPF_EVENT_TRAIT_CHANGED;
		if (szToken.CompareNoCase("CITY_CREATED") == 0) return DPF_EVENT_CITY_CREATED;
		if (szToken.CompareNoCase("CITY_LOST") == 0) return DPF_EVENT_CITY_LOST;
		if (szToken.CompareNoCase("RESOURCE_CHANGED") == 0) return DPF_EVENT_RESOURCE_CHANGED;
		if (szToken.CompareNoCase("TRANSITION_COMMITTED") == 0) return DPF_EVENT_TRANSITION_COMMITTED;
		int iValue = 0;
		return parseIntegerToken(szToken, iValue) ? iValue : -1;
	}

	int conditionTypeFromToken(CvString const& szToken)
	{
		if (szToken.CompareNoCase("TRUE") == 0) return DPF_CONDITION_TRUE;
		if (szToken.CompareNoCase("EVENT_OCCURRED") == 0) return DPF_CONDITION_EVENT_OCCURRED;
		if (szToken.CompareNoCase("EVENT_COUNT") == 0) return DPF_CONDITION_EVENT_COUNT;
		if (szToken.CompareNoCase("VARIABLE_AT_LEAST") == 0) return DPF_CONDITION_VARIABLE_AT_LEAST;
		if (szToken.CompareNoCase("STATUS_STAGE") == 0) return DPF_CONDITION_STATUS_STAGE;
		if (szToken.CompareNoCase("AND") == 0) return DPF_CONDITION_AND;
		if (szToken.CompareNoCase("OR") == 0) return DPF_CONDITION_OR;
		if (szToken.CompareNoCase("NOT") == 0) return DPF_CONDITION_NOT;
		int iValue = 0;
		return parseIntegerToken(szToken, iValue) ? iValue : -1;
	}

	int effectTypeFromToken(CvString const& szToken)
	{
		if (szToken.CompareNoCase("INCREMENT_VARIABLE") == 0) return DPF_EFFECT_INCREMENT_VARIABLE;
		if (szToken.CompareNoCase("SET_VARIABLE") == 0) return DPF_EFFECT_SET_VARIABLE;
		if (szToken.CompareNoCase("SET_MARKER") == 0) return DPF_EFFECT_SET_MARKER;
		if (szToken.CompareNoCase("GRANT_TRAIT") == 0) return DPF_EFFECT_GRANT_TRAIT;
		if (szToken.CompareNoCase("RELEASE_TRAIT") == 0) return DPF_EFFECT_RELEASE_TRAIT;
		if (szToken.CompareNoCase("TRACE") == 0) return DPF_EFFECT_TRACE;
		int iValue = 0;
		return parseIntegerToken(szToken, iValue) ? iValue : -1;
	}

	bool xmlSetChild(CvXMLLoadUtility& kXML, char const* szTag)
	{
		return gDLL->getXMLIFace()->SetToChildByTagName(kXML.GetXML(), szTag);
	}

	void xmlSetParent(CvXMLLoadUtility& kXML)
	{
		gDLL->getXMLIFace()->SetToParent(kXML.GetXML());
	}

	bool xmlNextSibling(CvXMLLoadUtility& kXML, char const* szTag)
	{
		// Keep the sibling walk identical to the runtime-proven DPF v1 reader.
		// The explicit tag check preserves the cursor contract around comments
		// and unknown siblings.
		FXml* pXML = kXML.GetXML();
		while (gDLL->getXMLIFace()->NextSibling(pXML))
		{
			char szLocatedTag[128] = { 0 };
			if (gDLL->getXMLIFace()->GetLastLocatedNodeTagName(
				pXML, szLocatedTag) && std::strcmp(szLocatedTag, szTag) == 0)
				return true;
		}
		return false;
	}

	bool xmlReadText(CvXMLLoadUtility& kXML, char const* szTag, CvString& szValue)
	{
		if (!xmlSetChild(kXML, szTag))
			return false;
		std::string szRead;
		bool const bResult = gDLL->getXMLIFace()->GetLastNodeValue(
			kXML.GetXML(), szRead);
		xmlSetParent(kXML);
		if (bResult)
			szValue = szRead.c_str();
		return bResult;
	}

	bool xmlReadInt(CvXMLLoadUtility& kXML, char const* szTag, int& iValue)
	{
		if (!xmlSetChild(kXML, szTag))
			return false;
		bool const bResult = gDLL->getXMLIFace()->GetLastNodeValue(
			kXML.GetXML(), &iValue);
		xmlSetParent(kXML);
		return bResult;
	}
	bool xmlReadStatus(CvXMLLoadUtility& kXML, char const* szTag, int& iValue)
	{
		CvString szToken;
		if (!xmlReadText(kXML, szTag, szToken))
			return false;
		iValue = statusFromToken(szToken);
		return iValue >= DPF_STATUS_DORMANT && iValue <= DPF_STATUS_WAITING_FOR_CHOICE;
	}

	bool xmlReadEventType(CvXMLLoadUtility& kXML, char const* szTag, int& iValue)
	{
		CvString szToken;
		if (!xmlReadText(kXML, szTag, szToken))
			return false;
		iValue = eventTypeFromToken(szToken);
		return iValue > 0;
	}

	bool xmlReadEffect(CvXMLLoadUtility& kXML, DPFEffect& kEffect)
	{
	CvString szToken;
	if (!xmlReadText(kXML, "Type", szToken))
		return false;
	kEffect.type = effectTypeFromToken(szToken);
	if (kEffect.type < DPF_EFFECT_INCREMENT_VARIABLE || kEffect.type > DPF_EFFECT_TRACE)
		return false;
	int iId = 0;
	bool const bHasId = xmlReadInt(kXML, "ID", iId);
	CvString szTrait;
	if (xmlReadText(kXML, "TraitType", szTrait))
	{
		iId = GC.getInfoTypeForString(szTrait.c_str(), true);
		if (iId < 0)
			return false;
	}
	kEffect.id = iId;
	if (kEffect.type != DPF_EFFECT_TRACE && !bHasId && szTrait.empty())
		return false;
	if (kEffect.type == DPF_EFFECT_INCREMENT_VARIABLE ||
		kEffect.type == DPF_EFFECT_SET_VARIABLE ||
		kEffect.type == DPF_EFFECT_GRANT_TRAIT ||
		kEffect.type == DPF_EFFECT_RELEASE_TRAIT)
	{
		if (!xmlReadInt(kXML, "Value", kEffect.value))
			return false;
	}
	else xmlReadInt(kXML, "Value", kEffect.value);
	if (!xmlReadInt(kXML, "Marker", kEffect.marker) && kEffect.type == DPF_EFFECT_SET_MARKER)
		kEffect.marker = kEffect.id;
	return true;
	}

	bool xmlReadEffectList(CvXMLLoadUtility& kXML, char const* szContainer,
		std::vector<DPFEffect>& effects)
	{
		if (!xmlSetChild(kXML, szContainer))
			return true;
		if (xmlSetChild(kXML, "Effect"))
		{
			do
			{
				DPFEffect kEffect;
				if (!xmlReadEffect(kXML, kEffect))
				{
					xmlSetParent(kXML);
					xmlSetParent(kXML);
					return false;
				}
				effects.push_back(kEffect);
			} while (xmlNextSibling(kXML, "Effect"));
			xmlSetParent(kXML);
		}
		xmlSetParent(kXML);
		return true;
	}

	bool xmlReadChoiceOption(CvXMLLoadUtility& kXML, DPFChoiceOption& kOption)
	{
		if (!xmlReadInt(kXML, "ID", kOption.id))
			return false;
		if (!xmlReadText(kXML, "DisplayName", kOption.displayName))
			kOption.displayName = CvString::format("Option %d", kOption.id);
		xmlReadText(kXML, "Description", kOption.description);
		xmlReadInt(kXML, "AIScore", kOption.aiScore);
		xmlReadText(kXML, "AIReason", kOption.aiReason);
		if (!xmlReadStatus(kXML, "ToStatus", kOption.toStatus) ||
			!xmlReadInt(kXML, "ToStage", kOption.toStage))
			return false;
		return xmlReadEffectList(kXML, "Effects", kOption.effects);
	}

	bool xmlReadChoice(CvXMLLoadUtility& kXML, DPFStage& kStage)
	{
		if (!xmlSetChild(kXML, "Choice"))
			return true;
		if (!xmlReadInt(kXML, "RequestID", kStage.choiceRequestId))
		{
			xmlSetParent(kXML);
			return false;
		}
		xmlReadInt(kXML, "ExpiryTurns", kStage.choiceExpiryTurns);
		xmlReadInt(kXML, "DefaultOption", kStage.choiceDefaultOption);
		xmlReadText(kXML, "DisplayName", kStage.choiceDisplayName);
		xmlReadText(kXML, "Description", kStage.choiceDescription);
		if (!xmlSetChild(kXML, "Options"))
		{
			xmlSetParent(kXML);
			return false;
		}
		if (xmlSetChild(kXML, "Option"))
		{
			do
			{
				DPFChoiceOption kOption;
				if (!xmlReadChoiceOption(kXML, kOption))
				{
					xmlSetParent(kXML);
					xmlSetParent(kXML);
					return false;
				}
				kStage.choiceOptions.push_back(kOption);
			} while (xmlNextSibling(kXML, "Option"));
			xmlSetParent(kXML);
		}
		xmlSetParent(kXML);
		return !kStage.choiceOptions.empty();
	}

	bool xmlReadCondition(CvXMLLoadUtility& kXML, DPFCondition& kCondition)
	{
	CvString szToken;
	if (!xmlReadText(kXML, "Type", szToken))
		return false;
	kCondition.type = conditionTypeFromToken(szToken);
	if (kCondition.type < DPF_CONDITION_TRUE || kCondition.type > DPF_CONDITION_NOT)
		return false;
	if (kCondition.type == DPF_CONDITION_EVENT_OCCURRED || kCondition.type == DPF_CONDITION_EVENT_COUNT)
	{
		if (!xmlReadEventType(kXML, "EventType", kCondition.eventType))
			return false;
	}
	if (kCondition.type == DPF_CONDITION_EVENT_COUNT || kCondition.type == DPF_CONDITION_VARIABLE_AT_LEAST)
	{
		if (!xmlReadInt(kXML, "Value", kCondition.value))
			return false;
	}
	if (kCondition.type == DPF_CONDITION_VARIABLE_AT_LEAST)
	{
		if (!xmlReadInt(kXML, "Variable", kCondition.variable))
			return false;
	}
	if (kCondition.type == DPF_CONDITION_STATUS_STAGE)
	{
		if (!xmlReadStatus(kXML, "Status", kCondition.status) ||
			!xmlReadInt(kXML, "Stage", kCondition.stage))
			return false;
	}
	if (kCondition.type == DPF_CONDITION_AND || kCondition.type == DPF_CONDITION_OR ||
		kCondition.type == DPF_CONDITION_NOT)
	{
		if (!xmlSetChild(kXML, "Children"))
			return false;
		if (xmlSetChild(kXML, "Condition"))
		{
			do
			{
				DPFCondition kChild;
				if (!xmlReadCondition(kXML, kChild))
				{
					xmlSetParent(kXML);
					xmlSetParent(kXML);
					return false;
				}
				kCondition.children.push_back(kChild);
			} while (xmlNextSibling(kXML, "Condition"));
			xmlSetParent(kXML);
		}
		xmlSetParent(kXML);
		if (kCondition.children.empty())
			return false;
	}
	return true;
	}

	bool xmlReadStage(CvXMLLoadUtility& kXML, DPFStage& kStage)
	{
		if (!xmlReadInt(kXML, "ID", kStage.id))
			return false;
		// Optional presentation metadata is deliberately non-authoritative.
		xmlReadText(kXML, "DisplayName", kStage.displayName);
		xmlReadText(kXML, "Hint", kStage.hint);
		return xmlReadEffectList(kXML, "EnterEffects", kStage.enterEffects) &&
			xmlReadEffectList(kXML, "ExitEffects", kStage.exitEffects) &&
			xmlReadEffectList(kXML, "ActiveEffects", kStage.activeEffects) &&
			xmlReadChoice(kXML, kStage);
	}

	bool xmlReadStageList(CvXMLLoadUtility& kXML, DPFDefinition& kDefinition)
	{
		if (!xmlSetChild(kXML, "Stages"))
			return false;
		if (xmlSetChild(kXML, "Stage"))
		{
			do
			{
				DPFStage kStage;
				if (!xmlReadStage(kXML, kStage))
				{
					xmlSetParent(kXML);
					xmlSetParent(kXML);
					return false;
				}
				kDefinition.stages.push_back(kStage);
			} while (xmlNextSibling(kXML, "Stage"));
			xmlSetParent(kXML);
		}
		xmlSetParent(kXML);
		return !kDefinition.stages.empty();
	}

	bool xmlReadTransition(CvXMLLoadUtility& kXML, DPFTransition& kTransition)
	{
		if (!xmlReadInt(kXML, "ID", kTransition.id) ||
			!xmlReadStatus(kXML, "FromStatus", kTransition.fromStatus) ||
			!xmlReadInt(kXML, "FromStage", kTransition.fromStage) ||
			!xmlReadStatus(kXML, "ToStatus", kTransition.toStatus) ||
			!xmlReadInt(kXML, "ToStage", kTransition.toStage) ||
			!xmlReadInt(kXML, "Priority", kTransition.priority) ||
			!xmlSetChild(kXML, "Condition"))
			return false;
		bool const bCondition = xmlReadCondition(kXML, kTransition.condition);
		xmlSetParent(kXML);
		if (!bCondition)
			return false;
		return xmlReadEffectList(kXML, "Effects", kTransition.effects);
	}

	bool xmlReadTransitionList(CvXMLLoadUtility& kXML, DPFDefinition& kDefinition)
	{
		if (!xmlSetChild(kXML, "Transitions"))
			return false;
		if (xmlSetChild(kXML, "Transition"))
		{
			do
			{
				DPFTransition kTransition;
				if (!xmlReadTransition(kXML, kTransition))
				{
					xmlSetParent(kXML);
					xmlSetParent(kXML);
					return false;
				}
				kDefinition.transitions.push_back(kTransition);
			} while (xmlNextSibling(kXML, "Transition"));
			xmlSetParent(kXML);
		}
		xmlSetParent(kXML);
		return !kDefinition.transitions.empty();
	}

	bool xmlReadDefinition(CvXMLLoadUtility& kXML, DPFDefinition& kDefinition)
	{
		// Civ4 info XML convention places the stable Type token before the
		// numeric ID. Read in that order so named XML content is accepted.
		if (!xmlReadText(kXML, "Type", kDefinition.name) ||
			!xmlReadInt(kXML, "ID", kDefinition.id))
			return false;
		// Fall back to the stable definition type when optional tags are absent.
		kDefinition.displayName = kDefinition.name;
		kDefinition.description = "";
		xmlReadText(kXML, "DisplayName", kDefinition.displayName);
		xmlReadText(kXML, "Description", kDefinition.description);
		kDefinition.initialStatus = DPF_STATUS_DORMANT;
		kDefinition.initialStage = 0;
		xmlReadStatus(kXML, "InitialStatus", kDefinition.initialStatus);
		xmlReadInt(kXML, "InitialStage", kDefinition.initialStage);
		return xmlReadStageList(kXML, kDefinition) &&
			xmlReadTransitionList(kXML, kDefinition);
	}

	bool validCondition(DPFCondition const& kCondition)
	{
		if (kCondition.type < DPF_CONDITION_TRUE || kCondition.type > DPF_CONDITION_NOT)
			return false;
		if ((kCondition.type == DPF_CONDITION_AND || kCondition.type == DPF_CONDITION_OR ||
			kCondition.type == DPF_CONDITION_NOT) && kCondition.children.empty())
			return false;
		for (std::vector<DPFCondition>::const_iterator it = kCondition.children.begin();
			it != kCondition.children.end(); ++it)
		{
			if (!validCondition(*it))
				return false;
		}
		return true;
	}

	bool validDefinition(DPFDefinition const& kDefinition)
	{
		if (kDefinition.id <= 0 || kDefinition.stages.empty() || kDefinition.transitions.empty())
			return false;
		std::set<int> stageIds;
		std::set<int> choiceRequestIds;
		for (std::vector<DPFStage>::const_iterator stageIt = kDefinition.stages.begin();
			stageIt != kDefinition.stages.end(); ++stageIt)
		{
			if (stageIt->id < 0 || !stageIds.insert(stageIt->id).second)
				return false;
			if (stageIt->choiceRequestId > 0)
			{
				if (!choiceRequestIds.insert(stageIt->choiceRequestId).second ||
					stageIt->choiceExpiryTurns < 0 || stageIt->choiceOptions.empty() ||
					(int)stageIt->choiceOptions.size() > DPF_MAX_CHOICE_OPTIONS)
					return false;
				std::set<int> optionIds;
				for (std::vector<DPFChoiceOption>::const_iterator optionIt =
					stageIt->choiceOptions.begin(); optionIt != stageIt->choiceOptions.end(); ++optionIt)
				{
					if (optionIt->id <= 0 || !optionIds.insert(optionIt->id).second ||
						optionIt->toStatus < DPF_STATUS_DORMANT ||
						optionIt->toStatus > DPF_STATUS_WAITING_FOR_CHOICE ||
						stageIds.find(optionIt->toStage) == stageIds.end())
						return false;
				}
				if (stageIt->choiceDefaultOption != 0 &&
					optionIds.find(stageIt->choiceDefaultOption) == optionIds.end())
					return false;
			}
			else if (!stageIt->choiceOptions.empty())
				return false;
		}
		if (stageIds.find(kDefinition.initialStage) == stageIds.end())
			return false;
		std::set<int> transitionIds;
		for (std::vector<DPFTransition>::const_iterator transitionIt = kDefinition.transitions.begin();
			transitionIt != kDefinition.transitions.end(); ++transitionIt)
		{
			if (transitionIt->id <= 0 || !transitionIds.insert(transitionIt->id).second ||
				stageIds.find(transitionIt->fromStage) == stageIds.end() ||
				stageIds.find(transitionIt->toStage) == stageIds.end() ||
				!validCondition(transitionIt->condition))
				return false;
		}
		return true;
	}

	bool loadDefinitionsFromXML(std::map<int, DPFDefinition>& loaded)
	{
		CvXMLLoadUtility kXML;
		if (!kXML.CreateFXml() || !kXML.LoadCivXml(kXML.GetXML(), "xml/DPF/CIV4DPFInfos.xml"))
		{
			if (kXML.GetXML() != NULL)
				kXML.DestroyFXml();
			return false;
		}
		bool bSuccess = gDLL->getXMLIFace()->LocateNode(kXML.GetXML(),
			"Civ4DPFInfos/DPFInfos/DPFInfo");
		if (bSuccess)
		{
			int iDefinitionIndex = 0;
			do
			{
				DPFDefinition kDefinition;
				if (!xmlReadDefinition(kXML, kDefinition))
				{
					logBBAI("DPF XML definition parse failed at index %d", iDefinitionIndex);
					bSuccess = false;
					break;
				}
				if (!validDefinition(kDefinition))
				{
					logBBAI("DPF XML definition invalid id %d at index %d",
						kDefinition.id, iDefinitionIndex);
					bSuccess = false;
					break;
				}
				if (!loaded.insert(std::make_pair(kDefinition.id, kDefinition)).second)
				{
					logBBAI("DPF XML duplicate definition id %d", kDefinition.id);
					bSuccess = false;
					break;
				}
				iDefinitionIndex++;
			} while (xmlNextSibling(kXML, "DPFInfo"));
		}
		else
		{
			logBBAI("DPF XML definition root LocateNode failed");
		}
		kXML.DestroyFXml();
		return bSuccess && !loaded.empty();
	}
	DPFEffect effect(int iType, int iId, int iValue, int iMarker = 0)
	{
		DPFEffect result;
		result.type = iType;
		result.id = iId;
		result.value = iValue;
		result.marker = iMarker;
		return result;
	}

	DPFStage stage(int iId)
	{
		DPFStage result;
		result.id = iId;
		return result;
	}
	DPFChoiceOption choiceOption(int iId, char const* szName, char const* szDescription,
		int iAIScore, int iToStatus, int iToStage)
	{
		DPFChoiceOption result;
		result.id = iId;
		result.displayName = szName;
		result.description = szDescription;
		result.aiScore = iAIScore;
		result.toStatus = iToStatus;
		result.toStage = iToStage;
		return result;
	}


	DPFTransition transition(int iId, int iFromStatus, int iFromStage,
		int iToStatus, int iToStage, int iPriority, DPFCondition const& condition)
	{
		DPFTransition result;
		result.id = iId;
		result.fromStatus = iFromStatus;
		result.fromStage = iFromStage;
		result.toStatus = iToStatus;
		result.toStage = iToStage;
		result.priority = iPriority;
		result.condition = condition;
		return result;
	}
	void ensureDefinitionsLoaded()
	{
		if (!CvDPF::isEnabled() || g_xmlAttempted)
			return;
		g_xmlAttempted = true;
		std::map<int, DPFDefinition> loaded;
		if (loadDefinitionsFromXML(loaded))
		{
			g_definitions.swap(loaded);
			logBBAI("DPF XML definitions loaded (%d definitions)",
				(int)g_definitions.size());
		}
		else
		{
			logBBAI("DPF XML definitions unavailable or invalid; using native ACBase Test fixtures");
		}
	}

	void registerDefinitions()
	{
		g_definitions.clear();

		DPFDefinition linear;
		linear.id = DPF_TEST_LINEAR;
		linear.name = "DPF_TEST_LINEAR";
		linear.initialStatus = DPF_STATUS_DORMANT;
		linear.initialStage = 0;
		DPFStage linearRoot = stage(0);
		DPFStage linearActive = stage(1);
		linearActive.choiceRequestId = 1001;
		linearActive.choiceDisplayName = "Network direction";
		linearActive.choiceDescription = "Choose whether to expand the network or protect it from setbacks.";
		linearActive.choiceDefaultOption = 1;
		linearActive.choiceOptions.push_back(choiceOption(1, "Expand the network",
			"Advance toward the resource reward; greater exposure to setbacks.", 20,
			DPF_STATUS_ACTIVE, 2));
		linearActive.choiceOptions.push_back(choiceOption(2, "Protect the network",
			"Secure the network first, then resume expansion after another improvement.", 10,
			DPF_STATUS_BLOCKED, 4));
		linearActive.enterEffects.push_back(effect(DPF_EFFECT_INCREMENT_VARIABLE, 1, 1));
		linearActive.activeEffects.push_back(effect(DPF_EFFECT_TRACE, 1, 0));
		DPFStage linearDeveloping = stage(2);
		linearDeveloping.enterEffects.push_back(effect(DPF_EFFECT_INCREMENT_VARIABLE, 1, 1));
		DPFStage linearProtected = stage(4);
		DPFStage linearCompleted = stage(3);
		linearCompleted.enterEffects.push_back(effect(DPF_EFFECT_GRANT_TRAIT,
			DPF_TEST_SHARED_TRAIT, 103, 103));
		linear.stages.push_back(linearRoot);
		linear.stages.push_back(linearActive);
		linear.stages.push_back(linearDeveloping);
		linear.stages.push_back(linearProtected);
		linear.stages.push_back(linearCompleted);
		linear.transitions.push_back(transition(101, DPF_STATUS_DORMANT, 0,
			DPF_STATUS_ACTIVE, 1, 100,
			conditionEvent(DPF_EVENT_IMPROVEMENT_COMPLETED)));
		linear.transitions.push_back(transition(102, DPF_STATUS_ACTIVE, 1,
			DPF_STATUS_ACTIVE, 2, 100,
			conditionEvent(DPF_EVENT_FEATURE_REMOVED)));
		linear.transitions.push_back(transition(103, DPF_STATUS_ACTIVE, 2,
			DPF_STATUS_COMPLETED, 3, 100,
			conditionAnd(conditionEventCount(DPF_EVENT_IMPROVEMENT_COMPLETED, 2),
				conditionVariable(1, 2))));
		linear.transitions.push_back(transition(104, DPF_STATUS_BLOCKED, 4,
			DPF_STATUS_ACTIVE, 2, 100,
			conditionEvent(DPF_EVENT_IMPROVEMENT_COMPLETED)));
		g_definitions[linear.id] = linear;

		DPFDefinition blocked;
		blocked.id = DPF_TEST_BLOCKED;
		blocked.name = "DPF_TEST_BLOCKED";
		blocked.initialStatus = DPF_STATUS_DORMANT;
		blocked.initialStage = 0;
		blocked.stages.push_back(stage(0));
		blocked.stages.push_back(stage(1));
		blocked.stages.push_back(stage(2));
		blocked.stages.push_back(stage(3));
		blocked.transitions.push_back(transition(201, DPF_STATUS_DORMANT, 0,
			DPF_STATUS_ACTIVE, 1, 100,
			conditionEvent(DPF_EVENT_IMPROVEMENT_COMPLETED)));
		blocked.transitions.push_back(transition(202, DPF_STATUS_ACTIVE, 1,
			DPF_STATUS_BLOCKED, 2, 100,
			conditionEvent(DPF_EVENT_FEATURE_REMOVED)));
		blocked.transitions.push_back(transition(203, DPF_STATUS_BLOCKED, 2,
			DPF_STATUS_ACTIVE, 1, 100,
			conditionEvent(DPF_EVENT_IMPROVEMENT_COMPLETED)));
		blocked.transitions.push_back(transition(204, DPF_STATUS_ACTIVE, 1,
			DPF_STATUS_COMPLETED, 3, 50,
			conditionEventCount(DPF_EVENT_IMPROVEMENT_COMPLETED, 3)));
		g_definitions[blocked.id] = blocked;

		DPFDefinition failed;
		failed.id = DPF_TEST_FAILED;
		failed.name = "DPF_TEST_FAILED";
		failed.initialStatus = DPF_STATUS_DORMANT;
		failed.initialStage = 0;
		failed.stages.push_back(stage(0));
		failed.stages.push_back(stage(1));
		failed.stages.push_back(stage(2));
		failed.stages.push_back(stage(3));
		failed.transitions.push_back(transition(301, DPF_STATUS_DORMANT, 0,
			DPF_STATUS_ACTIVE, 1, 100,
			conditionEvent(DPF_EVENT_IMPROVEMENT_COMPLETED)));
		failed.transitions.push_back(transition(302, DPF_STATUS_ACTIVE, 1,
			DPF_STATUS_FAILED, 2, 100,
			conditionEvent(DPF_EVENT_FEATURE_REMOVED)));
		failed.transitions.push_back(transition(303, DPF_STATUS_FAILED, 2,
			DPF_STATUS_ACTIVE, 1, 100,
			conditionEvent(DPF_EVENT_IMPROVEMENT_COMPLETED)));
		failed.transitions.push_back(transition(304, DPF_STATUS_ACTIVE, 1,
			DPF_STATUS_COMPLETED, 3, 50,
			conditionEventCount(DPF_EVENT_IMPROVEMENT_COMPLETED, 3)));
		g_definitions[failed.id] = failed;
	}

	void ensurePlayerInitialized(PlayerTypes ePlayer, DPFPlayerState& state)
	{
		if (state.initialized)
			return;
		for (std::map<int, DPFDefinition>::const_iterator it = g_definitions.begin();
			it != g_definitions.end(); ++it)
		{
			DPFInstance instance;
			instance.definitionId = it->second.id;
			instance.status = it->second.initialStatus;
			instance.stage = it->second.initialStage;
			state.instances[it->first] = instance;
		}
		state.initialized = true;
		trace(state, CvString::format("DPF player %d initialized", (int)ePlayer));
	}

	int mapValue(std::map<int, int> const& values, int iKey);
	void applyEffects(PlayerTypes ePlayer, DPFPlayerState& state,
		DPFInstance& instance, std::vector<DPFEffect> const& effects);
	DPFStage const* findStage(DPFDefinition const& definition, int iStage)
	{
		for (std::vector<DPFStage>::const_iterator it = definition.stages.begin();
			it != definition.stages.end(); ++it)
		{
			if (it->id == iStage)
				return &(*it);
		}
		return NULL;
	}
	DPFChoiceOption const* findChoiceOption(DPFStage const& stage, int iOptionId)
	{
		for (std::vector<DPFChoiceOption>::const_iterator it = stage.choiceOptions.begin();
			it != stage.choiceOptions.end(); ++it)
		{
			if (it->id == iOptionId)
				return &(*it);
		}
		return NULL;
	}

	bool pendingHasOption(DPFPendingChoice const& pending, int iOptionId)
	{
		for (std::vector<int>::const_iterator it = pending.optionIds.begin();
			it != pending.optionIds.end(); ++it)
		{
			if (*it == iOptionId)
				return true;
		}
		return false;
	}

	void createPendingChoice(PlayerTypes ePlayer, DPFPlayerState& state,
		DPFDefinition const& definition, DPFInstance& instance,
		DPFStage const& stage, int iTurn)
	{
		if (state.pendingChoice.active || stage.choiceRequestId <= 0 ||
			stage.choiceOptions.empty())
			return;
		DPFPendingChoice pending;
		pending.active = true;
		pending.definitionId = definition.id;
		pending.requestId = stage.choiceRequestId;
		pending.stageId = stage.id;
		pending.creationTurn = iTurn;
		pending.expiryTurn = stage.choiceExpiryTurns > 0 ?
			iTurn + stage.choiceExpiryTurns : 0;
		pending.defaultOption = stage.choiceDefaultOption;
		for (std::vector<DPFChoiceOption>::const_iterator it = stage.choiceOptions.begin();
			it != stage.choiceOptions.end() &&
				(int)pending.optionIds.size() < DPF_MAX_CHOICE_OPTIONS; ++it)
			pending.optionIds.push_back(it->id);
		state.pendingChoice = pending;
		instance.status = DPF_STATUS_WAITING_FOR_CHOICE;
		trace(state, CvString::format("DPF player %d choice request %d def %d stage %d options %d",
			(int)ePlayer, pending.requestId, pending.definitionId, pending.stageId,
			(int)pending.optionIds.size()));
	}

	void applyChoice(PlayerTypes ePlayer, DPFPlayerState& state,
		DPFDefinition const& definition, DPFInstance& instance,
		DPFChoiceOption const& option, int iTurn)
	{
		DPFStage const* oldStage = findStage(definition, instance.stage);
		if (oldStage != NULL)
			applyEffects(ePlayer, state, instance, oldStage->exitEffects);
		instance.status = option.toStatus;
		instance.stage = option.toStage;
		applyEffects(ePlayer, state, instance, option.effects);
		DPFStage const* newStage = findStage(definition, instance.stage);
		if (newStage != NULL)
		{
			applyEffects(ePlayer, state, instance, newStage->enterEffects);
			createPendingChoice(ePlayer, state, definition, instance, *newStage, iTurn);
		}
		trace(state, CvString::format("DPF player %d choice option %d def %d -> status %d stage %d",
			(int)ePlayer, option.id, definition.id, instance.status, instance.stage));
	}

	int aiChoiceScore(DPFChoiceOption const& option, DPFInstance const& instance)
	{
		int score = option.aiScore;
		if (option.toStatus == DPF_STATUS_ACTIVE)
			score += 2;
		if (option.toStatus == DPF_STATUS_BLOCKED)
			score -= 1;
		if (mapValue(instance.variables, 1) > 0 && option.toStage == 2)
			score += 1;
		return score;
	}

	void resolvePendingChoiceAI(PlayerTypes ePlayer, DPFPlayerState& state, int iTurn)
	{
		if (!state.pendingChoice.active)
			return;
		std::map<int, DPFDefinition>::const_iterator definitionIt =
			g_definitions.find(state.pendingChoice.definitionId);
		std::map<int, DPFInstance>::iterator instanceIt =
			state.instances.find(state.pendingChoice.definitionId);
		if (definitionIt == g_definitions.end() || instanceIt == state.instances.end())
			return;
		DPFStage const* stageIt = findStage(definitionIt->second, state.pendingChoice.stageId);
		if (stageIt == NULL)
			return;
		int bestOption = 0;
		int bestScore = -2147483647;
		for (std::vector<int>::const_iterator pendingIt = state.pendingChoice.optionIds.begin();
			pendingIt != state.pendingChoice.optionIds.end(); ++pendingIt)
		{
			DPFChoiceOption const* option = findChoiceOption(*stageIt, *pendingIt);
			if (option == NULL)
				continue;
			int const score = aiChoiceScore(*option, instanceIt->second);
			CvString reason = option->aiReason.empty() ? CvString("DPF_AI_XML_SCORE") : option->aiReason;
			trace(state, CvString::format("DPF AI choice request %d option %d score %d reason %s",
				state.pendingChoice.requestId, option->id, score, reason.GetCString()));
			if (bestOption == 0 || score > bestScore ||
				(score == bestScore && option->id < bestOption))
			{
				bestOption = option->id;
				bestScore = score;
			}
		}
		if (bestOption == 0)
			return;
		CvString const before = CvDPF::getStateDigest(ePlayer);
		int const requestId = state.pendingChoice.requestId;
		bool const accepted = CvDPF::submitChoice(ePlayer, requestId, bestOption, iTurn);
		CvString const after = CvDPF::getStateDigest(ePlayer);
		trace(state, CvString::format("DPF AI choice result request %d option %d accepted %d before=%s after=%s",
			requestId, bestOption, accepted ? 1 : 0, before.GetCString(), after.GetCString()));
	}

	bool settlePendingChoice(PlayerTypes ePlayer, DPFPlayerState& state, int iTurn)
	{
		if (!state.pendingChoice.active)
			return false;
		if (state.pendingChoice.expiryTurn > 0 && iTurn > state.pendingChoice.expiryTurn)
		{
			int const iDefault = state.pendingChoice.defaultOption;
			int const iRequest = state.pendingChoice.requestId;
			if (iDefault > 0 && pendingHasOption(state.pendingChoice, iDefault))
			{
				trace(state, CvString::format("DPF player %d choice request %d expired; applying default option %d",
					(int)ePlayer, iRequest, iDefault));
				CvDPF::submitChoice(ePlayer, iRequest, iDefault, iTurn);
			}
			else
			{
				std::map<int, DPFInstance>::iterator instanceIt =
					state.instances.find(state.pendingChoice.definitionId);
				if (instanceIt != state.instances.end())
					instanceIt->second.status = DPF_STATUS_BLOCKED;
				trace(state, CvString::format("DPF player %d choice request %d expired without a valid default; blocked",
					(int)ePlayer, iRequest));
				state.pendingChoice = DPFPendingChoice();
				return false;
			}
		}
		if (!state.pendingChoice.active)
			return false;
		if (!GET_PLAYER(ePlayer).isHuman())
			resolvePendingChoiceAI(ePlayer, state, iTurn);
		return state.pendingChoice.active;
	}
	int mapValue(std::map<int, int> const& values, int iKey)
	{
		std::map<int, int>::const_iterator it = values.find(iKey);
		return it == values.end() ? 0 : it->second;
	}

	bool evaluateCondition(DPFCondition const& condition, DPFInstance const& instance,
		std::map<int, int> const& waveEvents)
	{
		switch (condition.type)
		{
		case DPF_CONDITION_TRUE:
			return true;
		case DPF_CONDITION_EVENT_OCCURRED:
			return mapValue(waveEvents, condition.eventType) > 0;
		case DPF_CONDITION_EVENT_COUNT:
			return mapValue(instance.eventCounts, condition.eventType) >= condition.value;
		case DPF_CONDITION_VARIABLE_AT_LEAST:
			return mapValue(instance.variables, condition.variable) >= condition.value;
		case DPF_CONDITION_STATUS_STAGE:
			return instance.status == condition.status && instance.stage == condition.stage;
		case DPF_CONDITION_AND:
			for (std::vector<DPFCondition>::const_iterator it = condition.children.begin();
				it != condition.children.end(); ++it)
			{
				if (!evaluateCondition(*it, instance, waveEvents))
					return false;
			}
			return true;
		case DPF_CONDITION_OR:
			for (std::vector<DPFCondition>::const_iterator it = condition.children.begin();
				it != condition.children.end(); ++it)
			{
				if (evaluateCondition(*it, instance, waveEvents))
					return true;
			}
			return false;
		case DPF_CONDITION_NOT:
			return condition.children.empty() ||
				!evaluateCondition(condition.children[0], instance, waveEvents);
		default:
			return false;
		}
	}

	DPFTransition const* selectTransition(DPFDefinition const& definition,
		DPFInstance const& instance, std::map<int, int> const& waveEvents,
		bool& bAmbiguous)
	{
		DPFTransition const* best = NULL;
		bAmbiguous = false;
		for (std::vector<DPFTransition>::const_iterator it = definition.transitions.begin();
			it != definition.transitions.end(); ++it)
		{
			if (it->fromStatus != instance.status || it->fromStage != instance.stage ||
				!evaluateCondition(it->condition, instance, waveEvents))
				continue;
			if (best == NULL || it->priority > best->priority)
				best = &(*it);
			else if (it->priority == best->priority)
			{
				bAmbiguous = true;
				if (it->id < best->id)
					best = &(*it);
			}
		}
		return best;
	}

	TraitTypes resolveTrait(int iTrait)
	{
		if (iTrait != DPF_TEST_SHARED_TRAIT)
			return (TraitTypes)iTrait;
		return (TraitTypes)GC.getInfoTypeForString("TRAIT_DPF_TEST_SHARED", true);
	}

	void claimTrait(PlayerTypes ePlayer, DPFPlayerState& state,
		TraitTypes eTrait, int iClaim)
	{
		if (eTrait == NO_TRAIT || !validPlayer(ePlayer))
			return;
		std::set<int>& claims = state.traitClaims[(int)eTrait];
		bool const bWasEffective = !claims.empty();
		claims.insert(iClaim);
		if (!bWasEffective && !claims.empty() && GET_PLAYER(ePlayer).isAlive())
			GET_PLAYER(ePlayer).processTraits(1);
	}

	void releaseTrait(PlayerTypes ePlayer, DPFPlayerState& state,
		TraitTypes eTrait, int iClaim)
	{
		std::map<int, std::set<int> >::iterator itClaims =
			state.traitClaims.find((int)eTrait);
		if (itClaims == state.traitClaims.end())
			return;
		itClaims->second.erase(iClaim);
		if (itClaims->second.empty())
		{
			state.traitClaims.erase(itClaims);
			if (validPlayer(ePlayer) && GET_PLAYER(ePlayer).isAlive())
				GET_PLAYER(ePlayer).processTraits(-1);
		}
	}

	void applyEffects(PlayerTypes ePlayer, DPFPlayerState& state,
		DPFInstance& instance, std::vector<DPFEffect> const& effects)
	{
		for (std::vector<DPFEffect>::const_iterator it = effects.begin();
			it != effects.end(); ++it)
		{
			if (it->marker != 0 && instance.markers.find(it->marker) != instance.markers.end())
				continue;
			if (it->marker != 0)
				instance.markers.insert(it->marker);
			switch (it->type)
			{
			case DPF_EFFECT_INCREMENT_VARIABLE:
				instance.variables[it->id] = mapValue(instance.variables, it->id) + it->value;
				break;
			case DPF_EFFECT_SET_VARIABLE:
				instance.variables[it->id] = it->value;
				break;
			case DPF_EFFECT_SET_MARKER:
				instance.markers.insert(it->id);
				break;
			case DPF_EFFECT_GRANT_TRAIT:
				claimTrait(ePlayer, state, resolveTrait(it->id), it->value);
				break;
			case DPF_EFFECT_RELEASE_TRAIT:
				releaseTrait(ePlayer, state, resolveTrait(it->id), it->value);
				break;
			case DPF_EFFECT_TRACE:
				trace(state, CvString::format("DPF player %d active-stage def %d stage %d",
					(int)ePlayer, instance.definitionId, instance.stage));
				break;
			default:
				break;
			}
		}
	}

	void applyTransition(PlayerTypes ePlayer, DPFPlayerState& state,
		DPFDefinition const& definition, DPFInstance& instance,
		DPFTransition const& selected)
	{
		DPFStage const* oldStage = findStage(definition, instance.stage);
		if (oldStage != NULL)
			applyEffects(ePlayer, state, instance, oldStage->exitEffects);
		instance.status = selected.toStatus;
		instance.stage = selected.toStage;
		applyEffects(ePlayer, state, instance, selected.effects);
		DPFStage const* newStage = findStage(definition, instance.stage);
		if (newStage != NULL)
		{
			applyEffects(ePlayer, state, instance, newStage->enterEffects);
			createPendingChoice(ePlayer, state, definition, instance, *newStage,
				GC.getGame().getGameTurn());
		}
		trace(state, CvString::format("DPF player %d transition %d def %d -> status %d stage %d",
			(int)ePlayer, selected.id, definition.id, instance.status, instance.stage));
	}

	void readMap(FDataStreamBase* pStream, std::map<int, int>& values)
	{
		int iCount = 0;
		pStream->Read(&iCount);
		if (iCount < 0 || iCount > DPF_MAX_ENTRIES)
		{
			FAssertMsg(false, "Invalid DPF map count");
			return;
		}
		for (int i = 0; i < iCount; i++)
		{
			int iKey = 0, iValue = 0;
			pStream->Read(&iKey);
			pStream->Read(&iValue);
			values[iKey] = iValue;
		}
	}

	void writeMap(FDataStreamBase* pStream, std::map<int, int> const& values)
	{
		pStream->Write((int)values.size());
		for (std::map<int, int>::const_iterator it = values.begin(); it != values.end(); ++it)
		{
			pStream->Write(it->first);
			pStream->Write(it->second);
		}
	}

	void readMarkers(FDataStreamBase* pStream, std::set<int>& markers)
	{
		int iCount = 0;
		pStream->Read(&iCount);
		if (iCount < 0 || iCount > DPF_MAX_MARKERS)
		{
			FAssertMsg(false, "Invalid DPF marker count");
			return;
		}
		for (int i = 0; i < iCount; i++)
		{
			int iMarker = 0;
			pStream->Read(&iMarker);
			markers.insert(iMarker);
		}
	}

	void writeMarkers(FDataStreamBase* pStream, std::set<int> const& markers)
	{
		pStream->Write((int)markers.size());
		for (std::set<int>::const_iterator it = markers.begin(); it != markers.end(); ++it)
			pStream->Write(*it);
	}
}

void CvDPF::initStatics()
{
	g_xmlAttempted = false;
	registerDefinitions();
	g_players.assign(MAX_PLAYERS, DPFPlayerState());
	g_initialized = true;
}

void CvDPF::freeStatics()
{
	g_players.clear();
	g_xmlAttempted = false;
	g_definitions.clear();
	g_initialized = false;
}

void CvDPF::resetPlayer(PlayerTypes ePlayer)
{
	DPFPlayerState* state = stateFor(ePlayer);
	if (state != NULL)
		*state = DPFPlayerState();
}

bool CvDPF::isEnabled()
{
	return GC.getModName().getName() != NULL &&
		std::strcmp(GC.getModName().getName(), "ACBase Test") == 0;
}

void CvDPF::queueEvent(PlayerTypes eTarget, DPFEventType eType, int iTurn,
	PlayerTypes eActor, int iX, int iY, int iObject, int iAmount)
{
	if (!isEnabled() || !validPlayer(eTarget))
		return;
	DPFPlayerState* state = stateFor(eTarget);
	if (state == NULL)
		return;
	if ((int)state->pendingEvents.size() >= DPF_MAX_EVENTS)
	{
		trace(*state, CvString::format("DPF player %d event queue full", (int)eTarget));
		return;
	}
	DPFEvent event;
	event.type = (int)eType;
	event.turn = iTurn;
	event.actor = (int)eActor;
	event.owner = (int)eTarget;
	event.x = iX;
	event.y = iY;
	event.object = iObject;
	event.amount = std::max(1, iAmount);
	event.sequence = state->nextSequence++;
	state->pendingEvents.push_back(event);
}

void CvDPF::queueImprovementCompleted(PlayerTypes eActor, PlayerTypes eOwner,
	int iX, int iY, int iImprovement, int iAmount)
{
	PlayerTypes eTarget = eOwner != NO_PLAYER ? eOwner : eActor;
	queueEvent(eTarget, DPF_EVENT_IMPROVEMENT_COMPLETED,
		GC.getGame().getGameTurn(), eActor, iX, iY, iImprovement, iAmount);
}

void CvDPF::queueFeatureRemoved(PlayerTypes eActor, PlayerTypes eOwner,
	int iX, int iY, int iFeature, int iAmount)
{
	PlayerTypes eTarget = eOwner != NO_PLAYER ? eOwner : eActor;
	queueEvent(eTarget, DPF_EVENT_FEATURE_REMOVED,
		GC.getGame().getGameTurn(), eActor, iX, iY, iFeature, iAmount);
}

void CvDPF::queueTechnologyAcquired(PlayerTypes eActor, PlayerTypes eOwner,
	int iTechnology, int iAmount)
{
	PlayerTypes eTarget = eOwner != NO_PLAYER ? eOwner : eActor;
	queueEvent(eTarget, DPF_EVENT_TECHNOLOGY_ACQUIRED,
		GC.getGame().getGameTurn(), eActor, -1, -1, iTechnology, iAmount);
}

void CvDPF::queueCivicChanged(PlayerTypes eActor, PlayerTypes eOwner,
	int iCivic, int iAmount)
{
	PlayerTypes eTarget = eOwner != NO_PLAYER ? eOwner : eActor;
	queueEvent(eTarget, DPF_EVENT_CIVIC_CHANGED,
		GC.getGame().getGameTurn(), eActor, -1, -1, iCivic, iAmount);
}

void CvDPF::queueTraitChanged(PlayerTypes eActor, PlayerTypes eOwner,
	int iTrait, int iAmount)
{
	PlayerTypes eTarget = eOwner != NO_PLAYER ? eOwner : eActor;
	queueEvent(eTarget, DPF_EVENT_TRAIT_CHANGED,
		GC.getGame().getGameTurn(), eActor, -1, -1, iTrait, iAmount);
}

void CvDPF::queueCityCreated(PlayerTypes eActor, PlayerTypes eOwner,
	int iX, int iY, int iCity, int iAmount)
{
	PlayerTypes eTarget = eOwner != NO_PLAYER ? eOwner : eActor;
	queueEvent(eTarget, DPF_EVENT_CITY_CREATED,
		GC.getGame().getGameTurn(), eActor, iX, iY, iCity, iAmount);
}

void CvDPF::queueCityLost(PlayerTypes eActor, PlayerTypes eOwner,
	int iX, int iY, int iCity, int iAmount)
{
	PlayerTypes eTarget = eOwner != NO_PLAYER ? eOwner : eActor;
	queueEvent(eTarget, DPF_EVENT_CITY_LOST,
		GC.getGame().getGameTurn(), eActor, iX, iY, iCity, iAmount);
}

void CvDPF::queueResourceChanged(PlayerTypes eActor, PlayerTypes eOwner,
	int iResource, int iAmount)
{
	PlayerTypes eTarget = eOwner != NO_PLAYER ? eOwner : eActor;
	queueEvent(eTarget, DPF_EVENT_RESOURCE_CHANGED,
		GC.getGame().getGameTurn(), eActor, -1, -1, iResource, iAmount);
}

void CvDPF::queueTransitionCommitted(PlayerTypes eActor, PlayerTypes eOwner,
	int iTransition, int iAmount)
{
	PlayerTypes eTarget = eOwner != NO_PLAYER ? eOwner : eActor;
	queueEvent(eTarget, DPF_EVENT_TRANSITION_COMMITTED,
		GC.getGame().getGameTurn(), eActor, -1, -1, iTransition, iAmount);
}

void CvDPF::settlePlayer(PlayerTypes ePlayer, int iTurn)
{
	if (!isEnabled())
		return;
	ensureDefinitionsLoaded();
	DPFPlayerState* state = stateFor(ePlayer);
	if (state == NULL)
		return;
	ensurePlayerInitialized(ePlayer, *state);
	if (settlePendingChoice(ePlayer, *state, iTurn))
	{
		trace(*state, CvString::format("DPF player %d waiting for choice request %d",
			(int)ePlayer, state->pendingChoice.requestId));
		return;
	}
	std::vector<DPFEvent> snapshot = state->pendingEvents;
	std::map<int, int> waveEvents;
	for (std::vector<DPFEvent>::const_iterator eventIt = snapshot.begin();
		eventIt != snapshot.end(); ++eventIt)
	{
		waveEvents[eventIt->type] = mapValue(waveEvents, eventIt->type) + eventIt->amount;
		for (std::map<int, DPFInstance>::iterator instanceIt = state->instances.begin();
			instanceIt != state->instances.end(); ++instanceIt)
		{
			DPFInstance& instance = instanceIt->second;
			instance.eventCounts[eventIt->type] = mapValue(instance.eventCounts,
				eventIt->type) + eventIt->amount;
		}
	}

	for (int iWave = 0; iWave < DPF_MAX_SETTLEMENT_WAVES; iWave++)
	{
		bool bChanged = false;
		for (std::map<int, DPFInstance>::iterator instanceIt = state->instances.begin();
			instanceIt != state->instances.end(); ++instanceIt)
		{
			std::map<int, DPFDefinition>::const_iterator definitionIt =
				g_definitions.find(instanceIt->first);
			if (definitionIt == g_definitions.end())
				continue;
			bool bAmbiguous = false;
			DPFTransition const* selected = selectTransition(definitionIt->second,
				instanceIt->second, waveEvents, bAmbiguous);
			if (selected == NULL)
				continue;
			if (bAmbiguous)
				trace(*state, CvString::format("DPF player %d deterministic priority tie def %d",
					(int)ePlayer, instanceIt->first));
			applyTransition(ePlayer, *state, definitionIt->second,
				instanceIt->second, *selected);
			bChanged = true;
		}
		if (!bChanged)
			break;
		if (iWave == DPF_MAX_SETTLEMENT_WAVES - 1)
			trace(*state, CvString::format("DPF player %d settlement wave limit at turn %d",
				(int)ePlayer, iTurn));
	}

	if (state->pendingChoice.active && settlePendingChoice(ePlayer, *state, iTurn))
	{
		state->pendingEvents.clear();
		trace(*state, CvString::format("DPF player %d waiting for choice request %d after settlement",
			(int)ePlayer, state->pendingChoice.requestId));
		return;
	}

	// Active-stage effects run only after the transition waves have stabilized.
	for (std::map<int, DPFInstance>::iterator instanceIt = state->instances.begin();
		instanceIt != state->instances.end(); ++instanceIt)
	{
		std::map<int, DPFDefinition>::const_iterator definitionIt =
			g_definitions.find(instanceIt->first);
		if (definitionIt == g_definitions.end())
			continue;
		DPFStage const* currentStage = findStage(definitionIt->second, instanceIt->second.stage);
		if (currentStage != NULL && instanceIt->second.status == DPF_STATUS_ACTIVE)
			applyEffects(ePlayer, *state, instanceIt->second, currentStage->activeEffects);
	}
	state->pendingEvents.clear();
	trace(*state, CvDPF::getStateDigest(ePlayer));
}

bool CvDPF::hasTraitClaim(PlayerTypes ePlayer, TraitTypes eTrait)
{
	DPFPlayerState* state = stateFor(ePlayer);
	if (state == NULL)
		return false;
	std::map<int, std::set<int> >::const_iterator it = state->traitClaims.find((int)eTrait);
	return it != state->traitClaims.end() && !it->second.empty();
}

CvString CvDPF::getStateDigest(PlayerTypes ePlayer)
{
	DPFPlayerState* state = stateFor(ePlayer);
	if (state == NULL)
		return CvString::format("DPF player %d unavailable", (int)ePlayer);
	std::ostringstream result;
	result << "DPF player " << (int)ePlayer << " turn " << GC.getGame().getGameTurn();
	for (std::map<int, DPFInstance>::const_iterator it = state->instances.begin();
		it != state->instances.end(); ++it)
	{
		result << " [" << it->first << ":" << it->second.status << "/" << it->second.stage;
		result << ",v1=" << mapValue(it->second.variables, 1) << "]";
	}
	if (state->pendingChoice.active)
	{
		result << " pending=" << state->pendingChoice.requestId
			<< "/" << state->pendingChoice.definitionId
			<< "/" << state->pendingChoice.stageId
			<< "@" << state->pendingChoice.creationTurn
			<< "~" << state->pendingChoice.expiryTurn
			<< " default=" << state->pendingChoice.defaultOption << " options=";
		for (std::vector<int>::const_iterator optionIt = state->pendingChoice.optionIds.begin();
			optionIt != state->pendingChoice.optionIds.end(); ++optionIt)
		{
			if (optionIt != state->pendingChoice.optionIds.begin())
				result << ",";
			result << *optionIt;
		}
	}
	else
		result << " pending=none";
	return CvString(result.str());
}

CvString CvDPF::getInstanceDisplayName(PlayerTypes ePlayer, int iDefinition)
{
	if (!isEnabled())
		return CvString("");
	ensureDefinitionsLoaded();
	std::map<int, DPFDefinition>::const_iterator it = g_definitions.find(iDefinition);
	return it == g_definitions.end() ? CvString("") :
		(it->second.displayName.empty() ? it->second.name : it->second.displayName);
}

CvString CvDPF::getInstanceDescription(PlayerTypes ePlayer, int iDefinition)
{
	if (!isEnabled())
		return CvString("");
	ensureDefinitionsLoaded();
	std::map<int, DPFDefinition>::const_iterator it = g_definitions.find(iDefinition);
	return it == g_definitions.end() ? CvString("") : it->second.description;
}

CvString CvDPF::getInstanceStatusName(PlayerTypes ePlayer, int iDefinition)
{
	return CvString(statusName(getInstanceStatus(ePlayer, iDefinition)));
}

CvString CvDPF::getInstanceStageName(PlayerTypes ePlayer, int iDefinition)
{
	if (!isEnabled())
		return CvString("");
	ensureDefinitionsLoaded();
	std::map<int, DPFDefinition>::const_iterator definitionIt = g_definitions.find(iDefinition);
	if (definitionIt == g_definitions.end())
		return CvString("");
	int const iStage = getInstanceStage(ePlayer, iDefinition);
	DPFStage const* stageIt = findStage(definitionIt->second, iStage);
	return stageIt != NULL && !stageIt->displayName.empty() ? stageIt->displayName :
		CvString::format("Stage %d", iStage);
}

CvString CvDPF::getInstanceNextHint(PlayerTypes ePlayer, int iDefinition)
{
	if (!isEnabled())
		return CvString("");
	ensureDefinitionsLoaded();
	std::map<int, DPFDefinition>::const_iterator definitionIt = g_definitions.find(iDefinition);
	if (definitionIt == g_definitions.end())
		return CvString("");
	int const iStage = getInstanceStage(ePlayer, iDefinition);
	DPFStage const* stageIt = findStage(definitionIt->second, iStage);
	if (stageIt != NULL && !stageIt->hint.empty())
		return stageIt->hint;
	switch (getInstanceStatus(ePlayer, iDefinition))
	{
	case DPF_STATUS_DORMANT: return CvString("Complete the first required action to begin.");
	case DPF_STATUS_ACTIVE: return CvString("Continue the progression actions.");
	case DPF_STATUS_BLOCKED: return CvString("Resolve the blocking condition before continuing.");
	case DPF_STATUS_FAILED: return CvString("Recover from the setback to continue.");
	case DPF_STATUS_COMPLETED: return CvString("Progression complete.");
	case DPF_STATUS_WAITING_FOR_CHOICE: return CvString("Choose an option to continue.");
	default: return CvString("");
	}
}

bool CvDPF::hasPendingChoice(PlayerTypes ePlayer)
{
	if (!isEnabled())
		return false;
	DPFPlayerState* state = stateFor(ePlayer);
	return state != NULL && state->pendingChoice.active;
}

int CvDPF::getPendingChoiceDefinitionId(PlayerTypes ePlayer)
{
	DPFPlayerState* state = stateFor(ePlayer);
	return state != NULL && state->pendingChoice.active ? state->pendingChoice.definitionId : -1;
}

int CvDPF::getPendingChoiceRequestId(PlayerTypes ePlayer)
{
	DPFPlayerState* state = stateFor(ePlayer);
	return state != NULL && state->pendingChoice.active ? state->pendingChoice.requestId : 0;
}

int CvDPF::getPendingChoiceCreationTurn(PlayerTypes ePlayer)
{
	DPFPlayerState* state = stateFor(ePlayer);
	return state != NULL && state->pendingChoice.active ? state->pendingChoice.creationTurn : 0;
}

int CvDPF::getPendingChoiceExpiryTurn(PlayerTypes ePlayer)
{
	DPFPlayerState* state = stateFor(ePlayer);
	return state != NULL && state->pendingChoice.active ? state->pendingChoice.expiryTurn : 0;
}

int CvDPF::getPendingChoiceDefaultOption(PlayerTypes ePlayer)
{
	DPFPlayerState* state = stateFor(ePlayer);
	return state != NULL && state->pendingChoice.active ? state->pendingChoice.defaultOption : 0;
}

int CvDPF::getPendingChoiceOptionCount(PlayerTypes ePlayer)
{
	DPFPlayerState* state = stateFor(ePlayer);
	return state != NULL && state->pendingChoice.active ? (int)state->pendingChoice.optionIds.size() : 0;
}

int CvDPF::getPendingChoiceOptionIdAt(PlayerTypes ePlayer, int iIndex)
{
	DPFPlayerState* state = stateFor(ePlayer);
	if (state == NULL || !state->pendingChoice.active || iIndex < 0 ||
		iIndex >= (int)state->pendingChoice.optionIds.size())
		return -1;
	return state->pendingChoice.optionIds[iIndex];
}

CvString CvDPF::getPendingChoiceOptionName(PlayerTypes ePlayer, int iOptionId)
{
	DPFPlayerState* state = stateFor(ePlayer);
	if (!isEnabled() || state == NULL || !state->pendingChoice.active ||
		!pendingHasOption(state->pendingChoice, iOptionId))
		return CvString("");
	ensureDefinitionsLoaded();
	std::map<int, DPFDefinition>::const_iterator definitionIt =
		g_definitions.find(state->pendingChoice.definitionId);
	if (definitionIt == g_definitions.end())
		return CvString::format("Option %d", iOptionId);
	DPFStage const* stageIt = findStage(definitionIt->second, state->pendingChoice.stageId);
	DPFChoiceOption const* option = stageIt == NULL ? NULL :
		findChoiceOption(*stageIt, iOptionId);
	return option == NULL || option->displayName.empty() ?
		CvString::format("Option %d", iOptionId) : option->displayName;
}

CvString CvDPF::getPendingChoiceOptionDescription(PlayerTypes ePlayer, int iOptionId)
{
	DPFPlayerState* state = stateFor(ePlayer);
	if (!isEnabled() || state == NULL || !state->pendingChoice.active ||
		!pendingHasOption(state->pendingChoice, iOptionId))
		return CvString("");
	ensureDefinitionsLoaded();
	std::map<int, DPFDefinition>::const_iterator definitionIt =
		g_definitions.find(state->pendingChoice.definitionId);
	if (definitionIt == g_definitions.end())
		return CvString("");
	DPFStage const* stageIt = findStage(definitionIt->second, state->pendingChoice.stageId);
	DPFChoiceOption const* option = stageIt == NULL ? NULL :
		findChoiceOption(*stageIt, iOptionId);
	return option == NULL ? CvString("") : option->description;
}

bool CvDPF::submitChoice(PlayerTypes ePlayer, int iRequestId, int iOptionId,
	int iTurn)
{
	if (!isEnabled())
		return false;
	ensureDefinitionsLoaded();
	DPFPlayerState* state = stateFor(ePlayer);
	if (state == NULL || !state->pendingChoice.active ||
		state->pendingChoice.requestId != iRequestId ||
		!pendingHasOption(state->pendingChoice, iOptionId))
		return false;
	std::map<int, DPFDefinition>::const_iterator definitionIt =
		g_definitions.find(state->pendingChoice.definitionId);
	std::map<int, DPFInstance>::iterator instanceIt = state->instances.find(
		state->pendingChoice.definitionId);
	if (definitionIt == g_definitions.end() || instanceIt == state->instances.end())
		return false;
	DPFStage const* stageIt = findStage(definitionIt->second, state->pendingChoice.stageId);
	DPFChoiceOption const* option = stageIt == NULL ? NULL :
		findChoiceOption(*stageIt, iOptionId);
	if (option == NULL)
		return false;
	state->pendingChoice = DPFPendingChoice();
	applyChoice(ePlayer, *state, definitionIt->second, instanceIt->second, *option, iTurn);
	trace(*state, CvString::format("DPF player %d accepted choice request %d option %d",
		(int)ePlayer, iRequestId, iOptionId));
	return true;
}

int CvDPF::getInstanceCount(PlayerTypes ePlayer)
{
	if (!isEnabled())
		return 0;
	ensureDefinitionsLoaded();
	DPFPlayerState* state = stateFor(ePlayer);
	return state == NULL ? 0 : (int)state->instances.size();
}

int CvDPF::getInstanceDefinitionIdAt(PlayerTypes ePlayer, int iIndex)
{
	if (!isEnabled() || iIndex < 0)
		return -1;
	DPFPlayerState* state = stateFor(ePlayer);
	if (state == NULL || iIndex >= (int)state->instances.size())
		return -1;
	std::map<int, DPFInstance>::const_iterator it = state->instances.begin();
	for (int i = 0; i < iIndex; i++)
		++it;
	return it->first;
}

int CvDPF::getInstanceStatus(PlayerTypes ePlayer, int iDefinition)
{
	if (!isEnabled())
		return -1;
	ensureDefinitionsLoaded();
	DPFPlayerState* state = stateFor(ePlayer);
	if (state == NULL)
		return -1;
	std::map<int, DPFInstance>::const_iterator it = state->instances.find(iDefinition);
	return it == state->instances.end() ? -1 : it->second.status;
}

int CvDPF::getInstanceStage(PlayerTypes ePlayer, int iDefinition)
{
	if (!isEnabled())
		return -1;
	ensureDefinitionsLoaded();
	DPFPlayerState* state = stateFor(ePlayer);
	if (state == NULL)
		return -1;
	std::map<int, DPFInstance>::const_iterator it = state->instances.find(iDefinition);
	return it == state->instances.end() ? -1 : it->second.stage;
}

int CvDPF::getInstanceVariable(PlayerTypes ePlayer, int iDefinition, int iVariable)
{
	if (!isEnabled())
		return 0;
	ensureDefinitionsLoaded();
	DPFPlayerState* state = stateFor(ePlayer);
	if (state == NULL)
		return 0;
	std::map<int, DPFInstance>::const_iterator instanceIt = state->instances.find(iDefinition);
	return instanceIt == state->instances.end() ? 0 : mapValue(instanceIt->second.variables, iVariable);
}

int CvDPF::getInstanceEventCount(PlayerTypes ePlayer, int iDefinition, int iEventType)
{
	if (!isEnabled())
		return 0;
	ensureDefinitionsLoaded();
	DPFPlayerState* state = stateFor(ePlayer);
	if (state == NULL)
		return 0;
	std::map<int, DPFInstance>::const_iterator instanceIt = state->instances.find(iDefinition);
	return instanceIt == state->instances.end() ? 0 : mapValue(instanceIt->second.eventCounts, iEventType);
}

int CvDPF::getPendingEventCount(PlayerTypes ePlayer)
{
	if (!isEnabled())
		return 0;
	DPFPlayerState* state = stateFor(ePlayer);
	return state == NULL ? 0 : (int)state->pendingEvents.size();
}

int CvDPF::getTraceCount(PlayerTypes ePlayer)
{
	if (!isEnabled())
		return 0;
	DPFPlayerState* state = stateFor(ePlayer);
	return state == NULL ? 0 : (int)state->trace.size();
}

CvString CvDPF::getTraceEntry(PlayerTypes ePlayer, int iIndex)
{
	if (!isEnabled())
		return CvString();
	DPFPlayerState* state = stateFor(ePlayer);
	if (state == NULL || iIndex < 0 || iIndex >= (int)state->trace.size())
		return CvString();
	return state->trace[iIndex];
}

void CvDPF::readPlayer(PlayerTypes ePlayer, FDataStreamBase* pStream)
{
	DPFPlayerState* state = stateFor(ePlayer);
	int iVersion = 0, iEnabled = 0;
	pStream->Read(&iVersion);
	pStream->Read(&iEnabled);
	if (state == NULL || (iVersion != DPF_SAVE_VERSION && iVersion != DPF_LEGACY_SAVE_VERSION) || iEnabled == 0)
	{
		if (state != NULL)
			*state = DPFPlayerState();
		return;
	}
	*state = DPFPlayerState();
	state->nextSequence = 1;
	pStream->Read(&state->nextSequence);
	int iInstanceCount = 0;
	pStream->Read(&iInstanceCount);
	if (iInstanceCount < 0 || iInstanceCount > DPF_MAX_INSTANCES)
	{
		FAssertMsg(false, "Invalid DPF instance count");
		return;
	}
	for (int i = 0; i < iInstanceCount; i++)
	{
		DPFInstance instance;
		pStream->Read(&instance.definitionId);
		pStream->Read(&instance.status);
		pStream->Read(&instance.stage);
		readMap(pStream, instance.variables);
		readMap(pStream, instance.eventCounts);
		readMarkers(pStream, instance.markers);
		state->instances[instance.definitionId] = instance;
	}
	int iClaimCount = 0;
	pStream->Read(&iClaimCount);
	if (iClaimCount < 0 || iClaimCount > DPF_MAX_ENTRIES)
	{
		FAssertMsg(false, "Invalid DPF trait claim count");
		return;
	}
	for (int iClaim = 0; iClaim < iClaimCount; iClaim++)
	{
		int iTrait = 0, iClaims = 0;
		pStream->Read(&iTrait);
		pStream->Read(&iClaims);
		if (iClaims < 0 || iClaims > DPF_MAX_ENTRIES)
		{
			FAssertMsg(false, "Invalid DPF provenance count");
			return;
		}
		for (int i = 0; i < iClaims; i++)
		{
			int iClaimId = 0;
			pStream->Read(&iClaimId);
			state->traitClaims[iTrait].insert(iClaimId);
		}
	}
	int iEventCount = 0;
	pStream->Read(&iEventCount);
	if (iEventCount < 0 || iEventCount > DPF_MAX_EVENTS)
	{
		FAssertMsg(false, "Invalid DPF pending event count");
		return;
	}
	for (int i = 0; i < iEventCount; i++)
	{
		DPFEvent event;
		pStream->Read(&event.type);
		pStream->Read(&event.turn);
		pStream->Read(&event.actor);
		pStream->Read(&event.owner);
		pStream->Read(&event.x);
		pStream->Read(&event.y);
		pStream->Read(&event.object);
		pStream->Read(&event.amount);
		pStream->Read(&event.sequence);
		state->pendingEvents.push_back(event);
	}
	if (iVersion >= DPF_SAVE_VERSION)
	{
		int iPendingActive = state->pendingChoice.active ? 1 : 0;
		pStream->Read(&iPendingActive);
		if (iPendingActive != 0 && iPendingActive != 1)
		{
			FAssertMsg(false, "Invalid DPF pending choice active flag");
			return;
		}
		if (iPendingActive != 0)
		{
			state->pendingChoice.active = true;
			pStream->Read(&state->pendingChoice.definitionId);
			pStream->Read(&state->pendingChoice.requestId);
			pStream->Read(&state->pendingChoice.stageId);
			pStream->Read(&state->pendingChoice.creationTurn);
			pStream->Read(&state->pendingChoice.expiryTurn);
			pStream->Read(&state->pendingChoice.defaultOption);
			int iOptionCount = 0;
			pStream->Read(&iOptionCount);
			if (iOptionCount < 0 || iOptionCount > DPF_MAX_CHOICE_OPTIONS)
			{
				FAssertMsg(false, "Invalid DPF pending choice option count");
				return;
			}
			std::set<int> optionIds;
			for (int i = 0; i < iOptionCount; i++)
			{
				int iOptionId = 0;
				pStream->Read(&iOptionId);
				if (iOptionId <= 0 || !optionIds.insert(iOptionId).second)
				{
					FAssertMsg(false, "Invalid DPF pending choice option ID");
					return;
				}
				state->pendingChoice.optionIds.push_back(iOptionId);
			}
			if (state->pendingChoice.optionIds.empty())
			{
				FAssertMsg(false, "DPF pending choice has no options");
				return;
			}
		}
	}
	state->initialized = true;
}

void CvDPF::writePlayer(PlayerTypes ePlayer, FDataStreamBase* pStream)
{
	DPFPlayerState* state = stateFor(ePlayer);
	pStream->Write(DPF_SAVE_VERSION);
	pStream->Write(isEnabled() ? 1 : 0);
	if (!isEnabled() || state == NULL)
		return;
	pStream->Write(state->nextSequence);
	pStream->Write((int)state->instances.size());
	for (std::map<int, DPFInstance>::const_iterator it = state->instances.begin();
		it != state->instances.end(); ++it)
	{
		DPFInstance const& instance = it->second;
		pStream->Write(instance.definitionId);
		pStream->Write(instance.status);
		pStream->Write(instance.stage);
		writeMap(pStream, instance.variables);
		writeMap(pStream, instance.eventCounts);
		writeMarkers(pStream, instance.markers);
	}
	pStream->Write((int)state->traitClaims.size());
	for (std::map<int, std::set<int> >::const_iterator it = state->traitClaims.begin();
		it != state->traitClaims.end(); ++it)
	{
		pStream->Write(it->first);
		pStream->Write((int)it->second.size());
		for (std::set<int>::const_iterator claimIt = it->second.begin();
			claimIt != it->second.end(); ++claimIt)
			pStream->Write(*claimIt);
	}
	pStream->Write((int)state->pendingEvents.size());
	for (std::vector<DPFEvent>::const_iterator it = state->pendingEvents.begin();
		it != state->pendingEvents.end(); ++it)
	{
		pStream->Write(it->type);
		pStream->Write(it->turn);
		pStream->Write(it->actor);
		pStream->Write(it->owner);
		pStream->Write(it->x);
		pStream->Write(it->y);
		pStream->Write(it->object);
		pStream->Write(it->amount);
		pStream->Write(it->sequence);
	}
	int const iPendingActive = state->pendingChoice.active ? 1 : 0;
	pStream->Write(iPendingActive);
	if (iPendingActive != 0)
	{
		pStream->Write(state->pendingChoice.definitionId);
		pStream->Write(state->pendingChoice.requestId);
		pStream->Write(state->pendingChoice.stageId);
		pStream->Write(state->pendingChoice.creationTurn);
		pStream->Write(state->pendingChoice.expiryTurn);
		pStream->Write(state->pendingChoice.defaultOption);
		int const iOptionCount = std::min((int)state->pendingChoice.optionIds.size(),
			DPF_MAX_CHOICE_OPTIONS);
		pStream->Write(iOptionCount);
		for (int i = 0; i < iOptionCount; i++)
			pStream->Write(state->pendingChoice.optionIds[i]);
	}
}

bool dpfHasTraitClaim(PlayerTypes ePlayer, TraitTypes eTrait)
{
	return CvDPF::isEnabled() && CvDPF::hasTraitClaim(ePlayer, eTrait);
}
