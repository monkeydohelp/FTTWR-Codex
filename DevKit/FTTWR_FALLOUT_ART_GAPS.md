# FTTW-R Test Fallout art port: unresolved dependencies

Audit decision (updated 2026-09-26): preserve Fallout's named art paths and fill a missing path from another location only when the candidate file is byte-for-byte identical (SHA-256 match). Do not choose visually similar but different files by name alone; leave ambiguous variants unresolved for targeted review. Paths that have no safe alternate remain documented gaps.

Scope: the 933-dependency counts and `FTTWR_FalloutArtPortManifest.json` describe the E: candidate's baseline art audit, reconciled with the alternate-path pass. The separate UnitArtStyle repair manifest covers 16 definitions and their added dependencies. On 2026-09-26 the art-definition XML and art assets were synchronized to the installed C: test mod; the broader Fallout game-content batch remains E-only.

## Audit snapshot

- The art-definition import added 196 previously missing records — 77 building, 10 civilization, 10 leaderhead, and 99 unit definitions. A later cross-reference pass found 16 additional active unit-art IDs; their definitions have now been imported and synchronized as described below.
- Corrected recursive path audit: 933 dependencies, resolving model-relative texture basenames separately for each model directory.
- Imported 705 available Fallout loose assets into the candidate in two passes (580 initial + 125 additional model-relative dependencies), totaling 57,522,508 bytes.
- Exact path matches in the installed BtS base-game `Assets/Art0.FPK`: 107; 99 were already referenced by pre-import target art XML. Twenty other dependencies are supplied as loose base-game files.
- The original 2026-09-26 pre-fallback snapshot had 76 missing paths — 1 interface, 7 building, 27 leaderhead, and 41 unit dependencies. A same-basename alternate-location pass has since supplied 47 of those paths only where the source and destination SHA-256 hashes match; 29 paths remain from that snapshot (4 with conflicting variants and 25 with no loose same-basename candidate).
- The reconciled old-path list is the `unresolved` array in `FTTWR_FalloutArtPortManifest.json`; successful exact-hash alternate copies are recorded separately in `alternate_location_copies`. The new 16-unit-art-definition repair has its own supplemental manifest because it introduces a separate set of dependencies.
- The earlier first-pass table below came from an incomplete basename-deduplication pass and is retained only as audit history; the original 76-path table is also historical. The reconciled main manifest now has 29 unresolved paths. Re-run the preview after any art/source changes.

The tables below preserve the original 76-path inventory as audit history, not as a statement that all 76 are still missing. Files later supplied from alternate locations retain Fallout's original XML paths. Owners identify the imported art-definition records that refer to each path.

## Newly detected XML art-type gaps (2026-09-26)

The initial typed-reference audit found 51 unresolved `EarlyArtDefineTag`/`MiddleArtDefineTag`/`LateArtDefineTag` values in imported unit-art-style XML. They referred to 16 distinct `UnitArtInfo` types that were defined in Fallout's `CIV4ArtDefines_Unit.xml` but missing from the candidate. Those definitions are now present in both E: and C:.

- Vault style: `ART_DEF_UNIT_VAULT_GECK`, `ART_DEF_UNIT_VAULT_SETTLER_MALE`, `ART_DEF_UNIT_VAULT_SURVIVOR`, `ART_DEF_UNIT_VAULT_TOWN_WATCH`, `ART_DEF_UNIT_VAULT_WARRIOR`, `ART_DEF_UNIT_VAULT_WORKER`.
- Legion style: `ART_DEF_UNIT_LEGIONWARLORD`.
- Super Mutant style: `ART_DEF_UNIT_SUPER_MUTANTS_CEO_1` through `ART_DEF_UNIT_SUPER_MUTANTS_CEO_7`, `ART_DEF_UNIT_SUPER_MUTANTS_FEV`, `ART_DEF_UNIT_SUPER_MUTANTS_UNARMED`.

These were missing XML art definitions, not missing texture/model files, and were separate from the original 76-path snapshot. On 2026-09-26 the 16 source definitions were imported into the E: candidate, their 38 directly available dependencies were copied, and byte-identical alternate-location files were staged. The supplemental manifest `FTTWR_FalloutArtPortManifest-UnitArtStyleRepair-20260926.json` records the resulting 12 unresolved dependencies and exact owners. The art XML was synced to C: after validation against the receiving art schema. A refreshed typed-reference audit found no unresolved art references; its 23 remaining `CivicOptionType` reports are expected base-game references because all five distinct IDs are defined in the base BtS `CIV4CivicOptionInfos.xml`.

## Superseded first-pass detail (46-entry scan)

| Missing path | Referencing records |
|---|---|
| `Art/Interface/Buttons/Warlords_Atlas_1.dds` | `ART_DEF_BUILDING_ADVANCED_FEV_FACTORY`, `ART_DEF_BUILDING_ADVANCED_FEV_LAB`, `ART_DEF_BUILDING_ADVANCED_ROBOT_FACTORY`, `ART_DEF_BUILDING_CASH_PRINTER`, `ART_DEF_BUILDING_FEV_FACTORY`, `ART_DEF_BUILDING_GARAGE`, `ART_DEF_BUILDING_MILITARY_BASE`, `ART_DEF_BUILDING_MOTOR_POOL`, `ART_DEF_BUILDING_POLLING_STATION`, `ART_DEF_BUILDING_PRIME_ROBOT_FACILITY`, `ART_DEF_BUILDING_PROSPECTORS_GUILD`, `ART_DEF_BUILDING_TRANSPORT_COMPANY` |

## Building models

| Missing path | Referencing records |
|---|---|
| `Art/Structures/Buildings/German_Assembly_Plant/German_Assembly_Plant.nif` | `ART_DEF_BUILDING_ADVANCED_FEV_FACTORY`, `ART_DEF_BUILDING_ADVANCED_ROBOT_FACTORY`, `ART_DEF_BUILDING_FEV_FACTORY`, `ART_DEF_BUILDING_PRIME_ROBOT_FACILITY` |
| `Art/Structures/Buildings/Mali Mint/Mali_mint.nif` | `ART_DEF_BUILDING_CASH_PRINTER` |
| `Art/Structures/Buildings/Military_Academy/Military_Academy.nif` | `ART_DEF_BUILDING_MILITARY_BASE` |
| `Art/Structures/Buildings/Roman Forum/Forum.nif` | `ART_DEF_BUILDING_POLLING_STATION` |
| `Art/Structures/Buildings/Russian Research Institute/Research_Institute.nif` | `ART_DEF_BUILDING_ADVANCED_FEV_LAB` |
| `Art/Structures/Buildings/Stable/Stable.nif` | `ART_DEF_BUILDING_GARAGE`, `ART_DEF_BUILDING_MOTOR_POOL` |
| `Art/Structures/Buildings/Yert_ger/Yurt_Ger.nif` | `ART_DEF_BUILDING_PROSPECTORS_GUILD`, `ART_DEF_BUILDING_TRANSPORT_COMPANY` |

## Leaderhead assets

| Missing path | Referencing records |
|---|---|
| `Art/Leaderheads/Tandi/hatshepsut_noshader.nif` | `ART_DEF_LEADER_TANDI_WILLIAMS` |
| `Art/Leaderheads/John/Shaka_NONSHADER.dds` | `ART_DEF_LEADER_JOHN_MAXSON` |
| `Art/Leaderheads/John/Augustus_NONSHADER.dds` | `ART_DEF_LEADER_JOHN_MAXSON` |
| `Art/Leaderheads/John/ragnar_nonshader.dds` | `ART_DEF_LEADER_JOHN_MAXSON` |
| `Art/Leaderheads/President/Gilgamesh_Backgrnd_BG_Parent_Background.kf` | `ART_DEF_LEADER_PRESIDENT_RICHARD_RICHARDSON` |
| `Art/Leaderheads/Tandi/Boudica.nif` | `ART_DEF_LEADER_TANDI_WILLIAMS` |
| `Art/Leaderheads/Tandi/Boudica_DIFF.dds` | `ART_DEF_LEADER_TANDI_WILLIAMS` |
| `Art/Leaderheads/Tandi/Boudica_ENV.dds` | `ART_DEF_LEADER_TANDI_WILLIAMS` |
| `Art/Leaderheads/Tandi/Boudica_NORM.dds` | `ART_DEF_LEADER_TANDI_WILLIAMS` |
| `Art/Leaderheads/Tandi/Boudica_SPEC.dds` | `ART_DEF_LEADER_TANDI_WILLIAMS` |
| `Art/Leaderheads/Tandi/Boudica_EMSK.dds` | `ART_DEF_LEADER_TANDI_WILLIAMS` |
| `art/LeaderHeads/Caesar/HairBase_hl.dds` | `ART_DEF_LEADER_CAESAR` |
| `art/LeaderHeads/Caesar/HairBase_n.dds` | `ART_DEF_LEADER_CAESAR` |
| `art/LeaderHeads/John/shaka.nif` | `ART_DEF_LEADER_JOHN_MAXSON` |
| `art/LeaderHeads/harold/FDR_ENV.dds` | `ART_DEF_LEADER_HAROLD` |
| `art/leaderheads/Bishop/winston_churchill.nif` | `ART_DEF_LEADER_JOHN_BISHOP` |
| `Art/Leaderheads/John/Shaka_Sky_BG.dds` | `ART_DEF_LEADER_JOHN_MAXSON` |
| `art/LeaderHeads/Elder/Isabella_Sky.dds` | `ART_DEF_LEADER_GREAT_ELDER` |
| `art/LeaderHeads/harold/FDR_DIFF.dds` | `ART_DEF_LEADER_HAROLD` |
| `art/LeaderHeads/harold/FDR_NRML.dds` | `ART_DEF_LEADER_HAROLD` |
| `art/LeaderHeads/harold/FDR_SPEC.dds` | `ART_DEF_LEADER_HAROLD` |
| `art/LeaderHeads/harold/FDR_ENV_MASK.dds` | `ART_DEF_LEADER_HAROLD` |
| `art/LeaderHeads/harold/E.wAV` | `ART_DEF_LEADER_HAROLD` |
| `art/leaderheads/Bishop/Alexander_BG_Sky.dds` | `ART_DEF_LEADER_JOHN_BISHOP` |

## Unit assets

| Missing path | Referencing records |
|---|---|
| `Art/Units/Enclavepowerarmour/Environment_FX_GreyMetal.dds` | `ART_DEF_UNIT_ADVANCED_POWER_ARMOR`, `ART_DEF_UNIT_ADVANCED_POWER_ARMOR_WITH_PLASMA_RIFLE` |
| `Art/Units/Plasmathrower/Flame Thrower3a.nif` | `ART_DEF_UNIT_PLASMA_TROOPER` |
| `Art/Units/Plasmathrower/Flame Thrower3.nif` | `ART_DEF_UNIT_PLASMA_TROOPER` |
| `Art/Units/Slave/Environment_Light-1.dds` | `ART_DEF_UNIT_SLAVE` |
| `Art/Units/legion/praetorian_weapons_gloss_64.dds` | `ART_DEF_UNIT_LEGIONARY` |
| `Art/Units/plasmamortar/Environment_FX_Bronze.dds` | `ART_DEF_UNIT_PLASMA_MORTAR` |
| `Art/Units/sheriff/Cutlass_01_GLOW.dds` | `ART_DEF_UNIT_CENTAUR`, `ART_DEF_UNIT_DOCTOR`, `ART_DEF_UNIT_MEDIC`, `ART_DEF_UNIT_SHERIFF` |
| `Art/Units/survivor_gun/Commando_MD_DieA.kf` | `ART_DEF_UNIT_SLAVER` |
| `Art/Units/survivor_gun/Commando_MD_DieB.kf` | `ART_DEF_UNIT_SLAVER` |
| `Art/Units/survivor_gun/Commando_MD_DieC.kf` | `ART_DEF_UNIT_SLAVER` |
| `Art/Units/survivor_gun/environment_fx_greymetal.dds` | `ART_DEF_UNIT_SLAVER` |
| `Art/Units/robots/libertyprime/Commando_Head_64.dds` | `ART_DEF_UNIT_LIBERTY_PRIME` |
| `Art/Units/survivor/Inca_Quechua_Weapons.dds` | `ART_DEF_UNIT_SUPER_MUTANT_UNARMED` |
| `Art/Units/survivor/Inca_Quechua_128.dds` | `ART_DEF_UNIT_SUPER_MUTANT_UNARMED` |

## Residual path triage after the 2026-09-26 fallback pass

- Four paths in the original audit had same-basename candidates with differing hashes and were intentionally left untouched: `Art/Units/militia/cyborg_128.dds`, `Art/Units/power armor/WWAntiTankInfantry.nif`, `art/LeaderHeads/harold/eye_shadow.dds`, and `art/LeaderHeads/supermutant/eyeshadow.dds`. Their candidate variants need visual/model-context review before choosing.
- The other 25 paths still have no same-basename candidate in the scanned loose Fallout/base/BtS asset folders. A separate basename search of the base-game FPK did find alternate packed paths for `Settler_child_128.dds`, both `Inca_Quechua` textures, `Isabella_Sky.dds`, `Alexander_BG_Sky.dds`, and generic leader `eye_shadow.dds`/`eyeshadow.dds`/`gloss.dds` names. These are candidate archive entries, not resolved assets: each requires extracting the exact entry and validating the owning model/texture match. The main manifest retains the exact Fallout destination paths and owners.
- The installed `Art0.FPK` is format version 4. No compatible extractor was found in the local game or candidate tool folders during this pass; the locally reviewed community format write-up describes version 2 and is not sufficient to extract this archive safely. No packed-file contents were copied.
- After the 2026-09-27 reconciliation, 11 dependencies from the UnitArtStyle repair remain unresolved: `Art/Units/VAULT/FinalFrontier_Damaged_State_All.dds`, `Scout_DIFF.dds`, `Scout_GLOSS.dds`, `Settler_child_128.dds`, and `gloss.dds`; `Art/Units/Warlord_Ancient/Warlord_Ancient.kfm`; `Art/Units/legion/Warlord_Ancient_Gloss.dds`; and `Inca_Quechua_128.dds`/`Inca_Quechua_Weapons.dds` under both `Art/Units/survivor/` and `Art/Units/townwatch/`. `Art/Units/CEO/Logos/CEO_Case_64.dds` is resolved from Fallout-R's alternate `Assets/Art/Units/CEO/CEO_Case_64.dds` location; source and destination SHA-256 are identical. Exact owners and hash are in the supplemental manifest.
- The survivor Inca texture pair occurs in both audit scopes, so the two manifests currently contain 40 unresolved entries but 38 unique expected file paths.

## 2026-09-27 — XML-port art closure check

- The imported `ART_DEF_UNIT_SLAVER` has a complete primary path set in the test mod: it uses the existing Fallout `raider.dds` button, `survivor_gun/survivor gun.nif`, `commando g.kfm`, and the same NIF as SHADERNIF. No distinct Slaver button art is referenced by Fallout's art definition. Three secondary animation clips named by the model are not present in the target or either loose Fallout source folder: `Commando_MD_DieA.kf`, `Commando_MD_DieB.kf`, and `Commando_MD_DieC.kf`. Related Fade/Ranged clips were not substituted.
- The Slave uses Fallout's `slave.dds` button and `Slave/Worker.nif` plus `Worker_FX.nif`; those files match the Fallout-R source. Its `Worker/Worker.kfm` is a base-BtS dependency, and its target `Slave/Environment_Light-1.dds` is already supplied by the exact-hash alternate-path copy recorded in the supplemental manifest.
- Four additional path closures were made for already-imported art definitions: `CEO/Logos/CEO_Case_64.dds` was copied from Fallout-R's shallower `CEO/CEO_Case_64.dds` path after SHA-256 verification; the same existing target greymetal environment texture was copied into the `crossbowman`, `pistoler`, and `survivor_fo` art directories, with all three destination hashes matching the source texture exactly. These copies preserve the paths embedded in their models.
- `Assets/Art/Interface/Buttons/Process/ProcessCulture.dds` was also copied from the same Fallout-R path to satisfy the existing `PROCESS_CULTURE` button reference; its SHA-256 is `F0F795459EAA1774A0272B8B9F6E8170CDCD5FFAF65FFCADDFCAC79AE7DFD004`. The standard `ProcessWealth.dds` and `ProcessResearch.dds` button assets are indexed in the vanilla base-game `Assets/Art0.FPK`; the vanilla XML confirms the exact paths referenced by the test XML. They are not loose in the test mod and were not extracted from the archive. This resolves the asset-location question, but actual game VFS loading/rendering remains unverified. These two buttons are not part of the 38 Fallout art-definition dependency paths above; the missing Fallout-specific tiered process records remain a separate XML/content gap.
- This review closes those specific loose-file paths, not the global art audit. The main manifest still has 29 unavailable dependencies and the supplemental manifest has 11 (38 unique paths after overlap). Do not replace them with lookalikes; archive-contained fallback candidates remain unverified.

## Verification boundary

The pre-fallback 933-dependency audit's 76 missing paths were reconciled on 2026-09-26: 47 are now present at their original Fallout-relative paths from SHA-256-identical alternate locations, while 29 remain. The separate unit-art-style repair added 16 unit-art definitions and staged 38 source dependencies plus 14 hash-verified alternate files (the original 13 plus `CEO_Case_64.dds`); 11 dependencies remain unresolved. The full baseline manifest and the separate repair manifest preserve their distinct scopes. Four art-definition XML files pass the C: receiving art XDR structure/order check. The corrected archive root is the outer BtS install directory containing `Assets/Art0.FPK`; the previously used nested Beyond the Sword folder is a separate expansion root. These static checks do not prove in-game rendering. Remaining art files and render status are open until the missing dependencies are supplied or intentionally remapped and visually checked.
