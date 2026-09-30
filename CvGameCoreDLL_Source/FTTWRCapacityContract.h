#pragma once

#ifndef FTTWR_CAPACITY_CONTRACT_H
#define FTTWR_CAPACITY_CONTRACT_H

// FTTWR-DLL-ACB-001: shared AdvCiv contract identifiers.  These values are
// intentionally stable, bounded, and deterministic for save/multiplayer
// compatibility; resource identities resolve through XML BonusTypes.
// XML-declared capacity costs are deliberately bounded in the first generic
// schema.  The bound keeps the native unit/lease ABI simple while allowing a
// unit to consume several independent resource ledgers.
enum
{
	FTTWR_PROJECT_FAMILY_NONE = -1,
	FTTWR_PROJECT_FAMILY_ORDINARY = 0,
	FTTWR_PROJECT_FAMILY_EXCEPTIONAL = 1,
	FTTWR_PROJECT_FAMILY_ROBOT = 2,
	FTTWR_PROJECT_FAMILY_MUTANT_FEV = 3,
	FTTWR_PROJECT_FAMILY_REVIEW = 4,
	FTTWR_RECRUIT_TIER_NONE = 0,
	FTTWR_RECRUIT_TIER_BASIC = 1,
	FTTWR_RECRUIT_TIER_ADVANCED = 2,
	FTTWR_RECRUIT_TIER_VETERAN = 3,
	FTTWRP1_MAX_CAPACITY_COSTS = 4,
	// Building output rules use the same XML-resolved bonus identities as
	// unit capacity costs.  Keep the first candidate bounded and cache-safe;
	// adding a new resource does not require a new native enum entry.
	FTTWRP1_LEGACY_MAX_BONUS_OUTPUTS = 8,
	FTTWRP1_MAX_BONUS_OUTPUTS = 16,
	FTTWR_BONUS_OUTPUT_NONE = 0,
	FTTWR_BONUS_OUTPUT_UNIT_EXPERIENCE = 1,
	// ResourceCapacity output channels are encoded in the existing output-type
	// integer so the XML-cache record remains append-free and save-compatible
	// with the first candidate.  The low values are reserved for legacy/native
	// channels; typed yield/commerce channels occupy disjoint ranges.
	RESOURCE_CAPACITY_OUTPUT_YIELD_BASE = 1000,
	RESOURCE_CAPACITY_OUTPUT_COMMERCE_BASE = 2000,
	RESOURCE_CAPACITY_OUTPUT_INDEX_LIMIT = 100,
	FTTWR_UNIT_INFO_SAVE_FLAG = 0x00000001,
	FTTWR_BUILDING_INFO_SAVE_FLAG = 0x00000001,
	// Optional compatibility maps are appended only when this bit is present;
	// caches written before the compatibility port remain readable.
	FTTWR_BUILDING_INFO_COMPATIBILITY_FLAG = 0x00000002,
	// The normalized output array carries an explicit scope.  This bit marks
	// caches written after legacy XML was folded into that single array.
	RESOURCE_CAPACITY_OUTPUT_SCOPE_SAVE_FLAG = 0x00000004,
	// Later cache records widen the scoped output array from eight to sixteen;
	// old records retain their eight-slot layout and remain readable.
	RESOURCE_CAPACITY_OUTPUT_SIZE_SAVE_FLAG = 0x00001000,
	CIVIC_PREREQ_INFO_SAVE_FLAG = 0x00000008,
	// Appended CivicInfo specialist health/happiness maps are present only when
	// this bit is set, so caches written before the extension remain readable.
	CIVIC_SPECIALIST_OUTPUT_INFO_SAVE_FLAG = 0x00002000,
	// Vault settlement metadata is appended to unit/building XML caches only
	// when this bit is present, preserving compatibility with older caches.
	VAULT_PREREQ_INFO_SAVE_FLAG = 0x00000010,
	// Building political-authority/religion founding metadata is appended only
	// when this bit is present, preserving compatibility with older caches.
	FOUND_RELIGION_INFO_SAVE_FLAG = 0x00000020,
	// Building workable-radius metadata is appended only when this bit is
	// present, preserving compatibility with older XML-info caches.
	WORKABLE_RADIUS_INFO_SAVE_FLAG = 0x00000040,
	// Building culture-flip-resistance metadata is appended only when the bit
	// is present, preserving compatibility with older XML-info caches.
	CULTURE_FLIP_RESISTANCE_INFO_SAVE_FLAG = 0x00000080,
	// Promotion-level air/ranged/collateral ceiling metadata is appended only
	// when this bit is present, preserving compatibility with older XML-info
	// caches.
	PROMOTION_COMBAT_LIMIT_INFO_SAVE_FLAG = 0x00000100,
	// The ranged-strength percentage modifier is a later optional append to the
	// promotion record; older combat-limit caches therefore remain readable.
	PROMOTION_RANGED_STRENGTH_PERCENT_INFO_SAVE_FLAG = 0x00000200,
	// Promotion KeepFeatureBuilds entries are appended as deferred BuildType
	// names, preserving compatibility with caches written before that mechanic.
	PROMOTION_BUILD_LEAVE_FEATURE_INFO_SAVE_FLAG = 0x00000400,
	// The generic ranged-range promotion modifier is appended to promotion
	// cache records only when this bit is present.
	PROMOTION_RANGED_COMBAT_RANGE_INFO_SAVE_FLAG = 0x00004000,
	// Building ResourceCapacity costs are appended to building-info caches
	// only when this bit is present; older XML caches receive zero-cost defaults.
	BUILDING_RESOURCE_CAPACITY_COST_INFO_SAVE_FLAG = 0x00000800,
	CIVILIZATION_VAULT_INFO_SAVE_FLAG = 0x00000010
};

// Unit-info caches append optional fields only when their bit is present.  A
// cache written before population capacity existed therefore remains readable
// and receives the neutral zero-cost default.
enum
{
	FTTWR_UNIT_INFO_POPULATION_FLAG = 0x00000002,
	FTTWR_UNIT_INFO_RANGED_LIMIT_FLAG = 0x00000004
};

// Public mechanic-named aliases for output channels.  The FTTWR-prefixed
// values above are retained only as binary/source provenance for the staged
// candidate; XML and UI code should use ResourceCapacity names.
enum
{
	RESOURCE_CAPACITY_OUTPUT_NONE = FTTWR_BONUS_OUTPUT_NONE,
	RESOURCE_CAPACITY_OUTPUT_UNIT_EXPERIENCE = FTTWR_BONUS_OUTPUT_UNIT_EXPERIENCE,
	RESOURCE_CAPACITY_OUTPUT_HEALTH = 2,
	RESOURCE_CAPACITY_OUTPUT_HAPPINESS = 3,
	RESOURCE_CAPACITY_OUTPUT_DOMAIN_NONE = 0,
	RESOURCE_CAPACITY_OUTPUT_DOMAIN_UNIT_EXPERIENCE = 1,
	RESOURCE_CAPACITY_OUTPUT_DOMAIN_YIELD = 2,
	RESOURCE_CAPACITY_OUTPUT_DOMAIN_COMMERCE = 3,
	RESOURCE_CAPACITY_OUTPUT_DOMAIN_HEALTH = 4,
	RESOURCE_CAPACITY_OUTPUT_DOMAIN_HAPPINESS = 5,
	// Scope is part of the generic output rule, not a mod-specific field.
	// CITY preserves the normal connected/available-copy behavior; VICINITY
	// preserves the legacy nearby-bonus behavior without consuming a copy.
	RESOURCE_CAPACITY_OUTPUT_SCOPE_CITY = 0,
	RESOURCE_CAPACITY_OUTPUT_SCOPE_VICINITY = 1
};

// These helpers intentionally use only integer ranges so this contract can be
// included by the XML-info classes before the generated enum-count macros are
// available.  The decoded index is the native YieldTypes or CommerceTypes
// value for the corresponding domain.
inline int resourceCapacityOutputDomain(int iOutputType)
{
	if (iOutputType == RESOURCE_CAPACITY_OUTPUT_UNIT_EXPERIENCE)
		return RESOURCE_CAPACITY_OUTPUT_DOMAIN_UNIT_EXPERIENCE;
	if (iOutputType == RESOURCE_CAPACITY_OUTPUT_HEALTH)
		return RESOURCE_CAPACITY_OUTPUT_DOMAIN_HEALTH;
	if (iOutputType == RESOURCE_CAPACITY_OUTPUT_HAPPINESS)
		return RESOURCE_CAPACITY_OUTPUT_DOMAIN_HAPPINESS;
	if (iOutputType >= RESOURCE_CAPACITY_OUTPUT_YIELD_BASE &&
		iOutputType < RESOURCE_CAPACITY_OUTPUT_YIELD_BASE +
		RESOURCE_CAPACITY_OUTPUT_INDEX_LIMIT)
		return RESOURCE_CAPACITY_OUTPUT_DOMAIN_YIELD;
	if (iOutputType >= RESOURCE_CAPACITY_OUTPUT_COMMERCE_BASE &&
		iOutputType < RESOURCE_CAPACITY_OUTPUT_COMMERCE_BASE +
		RESOURCE_CAPACITY_OUTPUT_INDEX_LIMIT)
		return RESOURCE_CAPACITY_OUTPUT_DOMAIN_COMMERCE;
	return RESOURCE_CAPACITY_OUTPUT_DOMAIN_NONE;
}

inline int resourceCapacityOutputIndex(int iOutputType)
{
	if (resourceCapacityOutputDomain(iOutputType) ==
		RESOURCE_CAPACITY_OUTPUT_DOMAIN_YIELD)
		return iOutputType - RESOURCE_CAPACITY_OUTPUT_YIELD_BASE;
	if (resourceCapacityOutputDomain(iOutputType) ==
		RESOURCE_CAPACITY_OUTPUT_DOMAIN_COMMERCE)
		return iOutputType - RESOURCE_CAPACITY_OUTPUT_COMMERCE_BASE;
	return -1;
}

#endif
