# ACBase Test candidate-only bridge overlay for the deterministic P1 Recruit
# mechanics module.  The native descriptor is read once per city and remains
# authoritative; this module adds normalized fields for AI/UI diagnostics.
# Python 2.4-compatible: no json module or newer syntax.
import os

import FTTWRRuntimeCapabilityBridgeBase as _base
from FTTWRP1SchemaAdapter import adapt_descriptor
from FTTWRP1RecruitMechanics import snapshot_fields


def _identity():
	if _base._IDENTITY_WRITTEN:
		return
	_base._IDENTITY_WRITTEN = True
	_base._mark("FTTWR_RUNTIME_IDENTITY", {
		"hook_version": 1,
		"bridge_version": "acbase-test-recruit-queue-v1",
		"contract_schema": "fttwr-recruit-capacity-p1-0.1",
		"mapping_version": "p1-manpower-v3",
		"formula_version": "connected-tier-copy-count-plus-native-commit-v3",
		"descriptor_adapter": "p1-manpower-flat-v2",
		"raw_descriptor_schema": "p1-manpower-v1-nested",
		"mechanics_version": "p1-recruit-mechanics-v1",
		"mechanics_contract": "FTTWR_RECRUIT_CAPACITY_V1",
		"candidate_mod": os.environ.get("FTTW_DEVKIT_MOD", ""),
		"native_api": "getResurrectionManpowerDescriptor",
		"queue_integration": "native-combined-ledger-reservation-v1",
		"process_id": os.getpid(),
		"executable": "Civ4BeyondSword.exe",
	})


def _row(city, player_id):
	status, value = _base._descriptor(city)
	mechanics = {}
	if status == "ok":
		try:
			value = adapt_descriptor(value, city, player_id)
			mechanics = snapshot_fields(value)
		except Exception:
			status = "error"
			value = "{}"
	return _base._object({
		"player_id": player_id,
		"city_id": city.getID(),
		"city_name": city.getName(),
		"consumer": "native-api+recruit-mechanics",
		"api_status": status,
		"descriptor": value,
		"mechanics_version": mechanics.get("mechanics_version", "p1-recruit-mechanics-v1"),
		"mechanics_contract": mechanics.get("mechanics_contract", "FTTWR_RECRUIT_CAPACITY_V1"),
		"human_capacity": mechanics.get("human_capacity", 0),
		"human_occupancy": mechanics.get("human_occupancy", 0),
		"human_available": mechanics.get("human_available", 0),
		"human_basic_capacity": mechanics.get("human_basic_capacity", 0),
		"human_basic_occupancy": mechanics.get("human_basic_occupancy", 0),
		"human_basic_available": mechanics.get("human_basic_available", 0),
		"human_advanced_capacity": mechanics.get("human_advanced_capacity", 0),
		"human_advanced_occupancy": mechanics.get("human_advanced_occupancy", 0),
		"human_advanced_available": mechanics.get("human_advanced_available", 0),
		"human_veteran_capacity": mechanics.get("human_veteran_capacity", 0),
		"human_veteran_occupancy": mechanics.get("human_veteran_occupancy", 0),
		"human_veteran_available": mechanics.get("human_veteran_available", 0),
		"robot_capacity": mechanics.get("robot_capacity", 0),
		"robot_occupancy": mechanics.get("robot_occupancy", 0),
		"robot_available": mechanics.get("robot_available", 0),
		"population_available": mechanics.get("population_available", 0),
		"population_source": mechanics.get("population_source", "unknown"),
		"descriptor_invariant_status": mechanics.get("descriptor_invariant_status", "FAIL"),
		"descriptor_invariant_reason": mechanics.get("descriptor_invariant_reason", "adapter-error"),
		"frozen_project_count": mechanics.get("frozen_project_count", 0),
		"restore_priority_default": mechanics.get("restore_priority_default", False),
	})


# The base snapshot resolves these names in its own module globals.  Keeping
# the lifecycle in the base guarantees that the fixture and de-duplication
# policy are shared by the native and normalized telemetry paths.
_base._identity = _identity
_base._row = _row


def on_game_start():
	_base.on_game_start()


def on_load_game():
	_base.on_load_game()


def on_end_player_turn(game_turn, player_id):
	_base.on_end_player_turn(game_turn, player_id)
