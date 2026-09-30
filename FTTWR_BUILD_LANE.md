# FTTW-R Test lane

This folder is the isolated development lane for Fallout-TTW Reimagined.

The 2026-09-23 baseline was promoted from the runtime-tested `ACBase Blank`
content. The active DLL hash is recorded in `FTTWR_BUILD_LANE.json`. The synchronized C:
source tree was rebuilt for the 2026-09-25 Fallout terrain/resource/improvement/map/
pillage/sea-bridge batch and again on 2026-09-26 for the civic/trait pillage and
race/category production schema extensions. Fallout-specific content integration
remains in progress; the live `Fallout-TTW - R` source remains untouched and is a
read-only reference.

On 2026-09-29, ProcessInfo loading was moved into pre-menu initialization after
yield and commerce definitions so Processes are available in the opening-menu
Civilopedia. The VC7.1 Assert DLL was rebuilt with compact line-number debug info
and deployed; the process page still needs human runtime verification.

Experimental Fallout mechanics, content merges, UI work, and destabilising
native changes belong here first. A change may move to `FTTW-R` only after
the applicable static, XML, build-provenance, preflight, runtime, save/load,
and multiplayer gates pass.

The folder, INI, theme path, and Python mod name are intentionally identical:
`FTTW-R Test`.
