# Fallout XML completeness audit — FTTW-R Test

Date: 2026-09-27  
Target: installed C: `FTTW-R Test`  
Source compared: installed `Fallout-TTW - R`  
Scope boundary: static XML, receiver-schema, text-key, and art-path checks only. No game launch or in-game verification was performed for this pass.

## Method and result

The comparison matched XML files by relative path under `Assets/XML`, indexed direct `Type`-keyed records, then checked source record IDs and non-empty direct child fields against the matching target record. It does not equate schema optionality with a safe DLL default, infer semantic equivalence from a similar tag name, or claim that all same-ID numeric values are identical. Target-only and Fallout-only behavior remains separately identified below.

| Measure | Result |
|---|---:|
| Fallout-R XML files | 115 |
| Target XML files | 223 |
| Same relative XML paths | 81 |
| Common files containing Type-keyed records | 60 |
| Source records in those files | 3,372 |
| Same-ID records in target | 3,105 |
| Fallout source IDs absent from target | 267 |
| Distinct file/field pairs with a non-empty source child absent by the same name | 32 |

The 267 count is not a blanket import list: it contains deferred mechanics, source-only UI/art records, and optional records that lack current target owners, as well as additional content that still needs an explicit support/art review. Missing source art-definition IDs were cross-checked against exact leaf values in the target XML; none of the source-only art IDs in the audited art-definition files has an active target XML reference.

## Changes made in this pass

- Buildings: restored 35 missing nonzero Fallout bonus-yield/commerce effects through ACBase's supported `ResourceCapacityOutputs` representation; added eight source `bPrereqNoVault` flags, Jail's `bSlaveMarket`, and the supported source specialist slots. Existing target specialist counts and target-only `BUILD_ACBASE_SPAWN_TEST` remain intact.
- Units and formations: completed supported Worker/Slave/Slaver fields and added the missing `FORMATION_TYPE_SLAVER` definition. Worker and Slave retain the test-only spawn build. Slave uses its source AI/capture/upgrade/art mappings, `iPrereqPopulation=4`, and no population-capacity cost, preserving its one-use high-work-rate design. Slaver uses `UNITAI_SLAVER`, `UNITCOMBAT_SLAVER`, and the dedicated formation. The explicitly documented ACBase Slaver test prerequisite fixture remains. Restored Deathclaw's `TERRAIN_DECAYED_URBAN_WASTELAND` native entry.
- Other supported data: confirmed all 10 static Fallout civ traits are attached to their target leaders; restored `TRAIT_AGGRESSIVE` pillage-commerce modifiers, Military category attack modifiers, Sentry mission priority, Slavery civic Help/unhealthiness, and missing Slave Help text. Target `bBlitz=1` source semantics are represented with ACBase `iBlitz=-1`; source `iPopulationCost` maps to the target `PopulationCapacityCost` field where applicable. Source `KFM=None` is an empty-path sentinel and is correctly represented by omission.
- Art: copied `CEO_Case_64.dds` from Fallout-R's alternate folder after exact SHA-256 verification; copied the target's existing shared grey-metal texture into the `crossbowman`, `pistoler`, and `survivor_fo` model-relative folders with exact hash matches; copied Fallout-R's `ProcessCulture.dds` to its referenced target path (SHA-256 `F0F795459EAA1774A0272B8B9F6E8170CDCD5FFAF65FFCADDFCAC79AE7DFD004`). The supplemental art manifest now records the resolved CEO texture and 11 residual paths.

All eight changed owning XML files pass their receiving C: XDR validators: BuildingInfos, UnitInfos, FormationInfos, CivilizationInfos, TraitInfos, UnitCategoryInfos, MissionInfos, and CivicInfos. The edited Archid slavery text file parses, has 17 unique tags, and contains exactly one `TXT_KEY_UNIT_SLAVE_HELP` and one `TXT_KEY_CIVIC_SLAVERY_HELP`.

## Source-only record groups still open

| Collection | Missing source IDs | Status |
|---|---:|---|
| EventInfos / EventTriggerInfos | 97 / 54 | Defer the linked Fallout event chains: source uses `PrereqTrait`, which the current receiver XDR does not accept; Dynamic Traits progression is explicitly deferred. |
| ArtDefines_Interface | 35 | Source-only vanilla backgrounds and Dynamic Traits/build-UI records; no target XML owner was found for the missing art IDs. |
| ArtDefines_Unit | 17 | No exact active target XML reference was found for these absent UnitArtInfo IDs. Do not import orphan definitions solely to inflate parity. |
| NewConceptInfos | 17 | Includes the deferred Dynamic Traits concept; the other source Help records need a content/mechanics review before showing them in the target Civilopedia. |
| ProcessInfos | 11 | Ten Fallout tier records plus source `PROCESS_SCIENCE` are absent. ACBase instead defines `PROCESS_RESEARCH`; the Fallout base rates/tech gates differ. Fallout's `ProcessCulture.dds` is present. Vanilla `ProcessWealth.dds` and `ProcessResearch.dds` are indexed in the base-game `Assets/Art0.FPK` and the test XML references their standard paths; they have not been extracted into the mod, and runtime inheritance is not yet verified. The Fallout-specific process records and rates remain open. |
| ArtDefines_Movie / ColorVals | 8 / 6 | Era/vanilla intro movies and source trait/UI colors; not imported with the removed vanilla civs or deferred Dynamic Traits UI. |
| GameOptionInfos | 4 | Two Dynamic Traits options are deferred; `COMPLETE_KILLS` and `FLIPPING_AFTER_CONQUEST` require a receiver-enum/code behavior check before import. |
| MissionInfos | 4 | Fallout adds sentry mission records; its source file fails the current target XDR sequence check at `MissionInfo[48]`. Do not copy raw until the fields are reviewed/mapped. |
| GoodyInfo / PlayerOptionInfos | 3 / 2 | Three random-trait goodies are deferred with Dynamic Traits. Two growth-avoidance player options need an enum/code check before import. |
| ArtDefines_Building / Terrain / Leaderhead / Bonus | 2 / 1 / 1 / 1 | Source-only art IDs have no exact active target XML references; leave out unless a later content import adds owners. |
| YieldInfos / BasicInfos / InterfaceModeInfos | 2 / 1 / 1 | Health/Happiness yield IDs have no target yield type; Platy UI concept and Go-To-Sentry mode need their owning feature review. |

## Differing fields: mappings and genuine gaps

The raw same-name field scan reports source children absent in the target even where a receiver alias is already used. These are mapped or understood:

- Building `BonusYieldChanges`, `BonusCommerceChanges`, `VicinityBonusYieldChanges`, and `VicinityBonusCommerceChanges` are represented by the receiver's `ResourceCapacityOutputs`; the 35 missing nonzero effects noted above were added.
- Civilization `TraitType` values are carried by each faction's static leader trait mapping; all 10 source mappings resolve to the intended target leader.
- Unit `iPopulationCost` maps to `PopulationCapacityCost`; promotion `bBlitz=1` maps to receiver `iBlitz=-1`; `iAIPerEraModifier` is the source name for the receiver's renamed `iAIHandicapIncrementTurns` field. The source Noble value for the former is neutral zero.
- The Taoist shrine movie's `KFM=None` has no runtime file path and is equivalent to omitting the empty sentinel.

The following are not safe to treat as complete by same-name matching:

- Dynamic Traits source fields (`TraitClass`, `bPermanent`, `iLevel`, `ParentTrait`, `NextTrait`, `RemoveTraits`, and civic `DisableTraits`) remain deferred as directed. Chosen One progression traits also contain capital/maintenance modifiers whose effects are part of that deferred progression.
- Source global pillage modifiers on culture-level, era, world-size, and handicap records (`iPillageCultureModifier`, `iPillageCommercePercent`, `iPillageYieldPercent`, `iAIPillageCommercePercent`) are not covered by the already-added civic/trait `PillageCommerceModifiers` and no-pillage/capital flags. Several source values are nonzero; these require separate receiver-mechanics review. The prior approved receiver work supports the civic/trait arrays/flags and race/category unit-production arrays, not these global fields.
- Leader `Image` and religion/corporation `iTGAIndex` are source-only presentation metadata with no matching receiver schema/reader field. Do not copy their art/text paths as if ACBase consumed them; the leader ArtDefine buttons and active religion/corporation Buttons remain the supported display route.
- Route `bSeaTunnel` is absent from the receiver but all source values are false, so no nonzero behavior is lost in the current port.

This inventory is a cross-mod completeness checkpoint, not a claim that all Fallout XML content is ported or that every same-ID numeric value has been reconciled. In particular, several core process rates differ from Fallout (target Wealth/Research/Culture are 50%; Fallout's corresponding base records use 10%/10%/5% and different tech availability). Those existing target values are left visible for an explicit balance decision rather than silently overwritten.

## Art closure and remaining dependencies

- `ART_DEF_UNIT_SLAVER` uses Fallout's existing `raider.dds` icon (no distinct Slaver icon is referenced), `survivor_gun/survivor gun.nif`, `commando g.kfm`, and its NIF as SHADERNIF; all primary paths exist in the test mod. The three secondary `Commando_MD_DieA.kf`, `DieB.kf`, and `DieC.kf` clips remain unavailable in the target and both loose Fallout source folders; no Fade/Ranged animation was substituted.
- `ART_DEF_UNIT_SLAVE` uses the Fallout `slave.dds` button, `Slave/Worker.nif`, and `Worker_FX.nif`; the Fallout-specific button/model files are present. Its `Worker/Worker.kfm` is supplied by base BtS, and the target's `Slave/Environment_Light-1.dds` is already the exact-hash alternate-path copy.
- The main art manifest still has 29 missing dependency paths; the unit-style supplement has 11, with two paths overlapping, for 38 unique art-definition dependencies. All listed paths remain absent at their target destination. The unresolved, visually different variants remain untouched. See `DevKit/FTTWR_FALLOUT_ART_GAPS.md` and both manifests for owners and candidate archive paths.
- Existing XML formatting guidance is in `FTTWR_XML_PORT_VALIDATION.md` under “XML formatting standard”: preserve vanilla BtS line breaks, indentation, compact record spacing, encoding, and comments; do not serialize whole collections as one line.

Backups for the XML changes are in `DevKit/Backups/20260927-xml-completeness-01` through `-06`; pre-audit docs/manifests are in `DevKit/Backups/20260927-xml-completeness-audit-01/`. C: test only; E: parity is not claimed. No runtime launch was performed.
