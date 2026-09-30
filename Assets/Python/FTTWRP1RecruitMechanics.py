"""Deterministic P1 Recruit-capacity mechanics for ACBase Test.

This module is intentionally independent of Civ4 APIs.  It consumes the flat
descriptor emitted by FTTWRP1SchemaAdapter and evaluates future project
eligibility without mutating the native build queue.  CvCity owns the
authoritative native queue transaction.
"""

MECHANICS_VERSION = "p1-recruit-mechanics-v1"
QUEUE_CONTRACT_VERSION = "p1-recruit-queue-v2"
BASIC_TIER = "BASIC"
NONE_TIER = "NONE"
ORDINARY_FAMILY = "ORDINARY"
EXCEPTIONAL_FAMILY = "EXCEPTIONAL"
ROBOT_FAMILY = "ROBOT"
MUTANT_FEV_FAMILY = "MUTANT_FEV"
REVIEW_FAMILY = "REVIEW"
ACTIVE = "ACTIVE"
FROZEN = "FROZEN"

def _transition_event(status):
	if status == FROZEN:
		return "PROJECT_FROZEN"
	return "PROJECT_UNFROZEN"


def _int_field(text, field, default=0):
	marker = '"' + field + '":'
	index = text.find(marker)
	if index < 0:
		return default
	index += len(marker)
	length = len(text)
	while index < length and text[index] in " \t\r\n\":":
		index += 1
	sign = 1
	if index < length and text[index] == "-":
		sign = -1
		index += 1
	begin = index
	while index < length and text[index] >= "0" and text[index] <= "9":
		index += 1
	if begin == index:
		return default
	try:
		return sign * int(text[begin:index])
	except Exception:
		return default


def _string_field(text, field, default=""):
	marker = '"' + field + '":"'
	index = text.find(marker)
	if index < 0:
		return default
	index += len(marker)
	end = text.find('"', index)
	if end < 0:
		return default
	return text[index:end]


def capability_from_descriptor(descriptor):
	"""Normalize adapter output into the mechanics input contract."""
	text = descriptor or "{}"
	return {
		"owner_player_id": _int_field(text, "owner_player_id"),
		"city_id": _int_field(text, "city_id"),
		"network_component_id": _int_field(text, "network_component_id", -1),
		"human_sources": _int_field(text, "human_sources"),
		"human_occupancy": _int_field(text, "human_occupancy"),
		"human_available": _int_field(text, "human_available"),
		"human_basic_sources": _int_field(text, "human_basic_sources"),
		"human_basic_occupancy": _int_field(text, "human_basic_occupancy"),
		"human_basic_available": _int_field(text, "human_basic_available"),
		"human_advanced_sources": _int_field(text, "human_advanced_sources"),
		"human_advanced_occupancy": _int_field(text, "human_advanced_occupancy"),
		"human_advanced_available": _int_field(text, "human_advanced_available"),
		"human_veteran_sources": _int_field(text, "human_veteran_sources"),
		"human_veteran_occupancy": _int_field(text, "human_veteran_occupancy"),
		"human_veteran_available": _int_field(text, "human_veteran_available"),
		"robot_sources": _int_field(text, "robot_sources"),
		"robot_occupancy": _int_field(text, "robot_occupancy"),
		"robot_available": _int_field(text, "robot_available"),
		"recruit_bonus_identity": _string_field(text, "recruit_bonus_identity", "BONUS_RECRUITS"),
		"human_population_available": _int_field(text, "human_population_available"),
		"population_source": _string_field(text, "population_source", "unknown"),
	}


def invariant_status(capability):
	"""Return a marker-safe invariant result for a native descriptor row."""
	fields = ("human_sources", "human_occupancy", "human_available",
		"robot_sources", "robot_occupancy", "robot_available",
		"human_population_available")
	for field in fields:
		if int(capability.get(field, 0)) < 0:
			return "FAIL", "negative_%s" % field
	return "PASS", "nonnegative_capacity_fields"


def make_project(project_id, queue_index, progress, requires_recruit=False,
		recruit_tier=NONE_TIER, human_capacity_cost=0, robot_capacity_cost=0,
		population_cost=0, status=ACTIVE, project_family=""):
	"""Create a serializable project record for deterministic queue tests."""
	if not project_family:
		if requires_recruit:
			project_family = EXCEPTIONAL_FAMILY
		elif int(robot_capacity_cost) > 0:
			project_family = ROBOT_FAMILY
		else:
			project_family = ORDINARY_FAMILY
	return {
		"project_id": project_id,
		"queue_index": int(queue_index),
		"progress": int(progress),
		"status": status,
		"requires_recruit": bool(requires_recruit),
		"recruit_tier": recruit_tier,
		"human_capacity_cost": int(human_capacity_cost),
		"robot_capacity_cost": int(robot_capacity_cost),
		"population_cost": int(population_cost),
		"project_family": project_family,
	}


def evaluate_project(project, capability):
	"""Evaluate one project while preserving progress and queue metadata."""
	result = dict(project)
	blockers = []
	family = project.get("project_family", ORDINARY_FAMILY)
	if family in (MUTANT_FEV_FAMILY, REVIEW_FAMILY):
		blockers.append("unsupported_project_family")
	elif family == EXCEPTIONAL_FAMILY and not project.get("requires_recruit", False):
		blockers.append("exceptional_requires_recruit")
	elif family == ROBOT_FAMILY and int(project.get("human_capacity_cost", 0)) > 0:
		blockers.append("robot_family_human_capacity")
	elif family == ORDINARY_FAMILY and (
		int(project.get("human_capacity_cost", 0)) > 0 or
		int(project.get("robot_capacity_cost", 0)) > 0):
		blockers.append("ordinary_family_capacity_cost")
	if int(project.get("population_cost", 0)) > int(capability.get("human_population_available", 0)):
		blockers.append("population_capacity")
	if int(project.get("robot_capacity_cost", 0)) > int(capability.get("robot_available", 0)):
		blockers.append("robot_capacity")
	if project.get("requires_recruit", False):
		if project.get("recruit_tier", NONE_TIER) != BASIC_TIER:
			blockers.append("unsupported_recruit_tier")
		if int(project.get("human_capacity_cost", 0)) > int(capability.get("human_available", 0)):
			blockers.append("human_recruit_capacity")
	elif int(project.get("human_capacity_cost", 0)) > 0:
		blockers.append("human_capacity_requires_recruit")
	result["blockers"] = blockers
	result["requirements_met"] = len(blockers) == 0
	result["status"] = ACTIVE
	if len(blockers) != 0:
		result["status"] = FROZEN
	result["freeze_reason"] = ""
	if blockers:
		result["freeze_reason"] = blockers[0]
	return result


def evaluate_queue(projects, capability, restore_priority=False):
	"""Evaluate and order projects without losing frozen progress."""
	evaluated = []
	restored = []
	for project in projects:
		row = evaluate_project(project, capability)
		evaluated.append(row)
		if project.get("status") == FROZEN and row["status"] == ACTIVE:
			restored.append(row)
	evaluated.sort(key=lambda row: int(row.get("queue_index", 0)))
	restored.sort(key=lambda row: int(row.get("queue_index", 0)))
	if restore_priority:
		restored_ids = set([row["project_id"] for row in restored])
		ordered = restored + [row for row in evaluated if row["project_id"] not in restored_ids]
	else:
		ordered = evaluated
	return {
		"mechanics_version": MECHANICS_VERSION,
		"restore_priority": bool(restore_priority),
		"projects": evaluated,
		"ordered_project_ids": [row["project_id"] for row in ordered],
		"restored_project_ids": [row["project_id"] for row in restored],
	}


def reconcile_queue(projects, capability, restore_priority=False, active_project_id=""):
	"""Reconcile eligibility without mutating progress or queue position."""
	previous = {}
	for project in projects:
		previous[project.get("project_id")] = project
	evaluated = evaluate_queue(projects, capability, restore_priority)
	rows_by_id = {}
	notifications = []
	for row in evaluated["projects"]:
		project_id = row.get("project_id")
		rows_by_id[project_id] = row
		old = previous.get(project_id, {})
		old_status = old.get("status", ACTIVE)
		new_status = row.get("status", ACTIVE)
		if old_status != new_status:
			notifications.append({
				"event": _transition_event(new_status),
				"project_id": project_id,
				"queue_index": int(row.get("queue_index", 0)),
				"reason": row.get("freeze_reason", ""),
			})
		if new_status == FROZEN and old_status != FROZEN:
			row["status_transition"] = "FROZEN"
		elif new_status == ACTIVE and old_status == FROZEN:
			row["status_transition"] = "UNFROZEN"
		else:
			row["status_transition"] = "UNCHANGED"
	active_id = active_project_id or ""
	active_row = rows_by_id.get(active_id)
	if restore_priority and evaluated["restored_project_ids"]:
		active_id = evaluated["restored_project_ids"][0]
	elif active_row is None or active_row.get("status") != ACTIVE:
		active_id = ""
		for row in evaluated["projects"]:
			if row.get("status") == ACTIVE:
				active_id = row.get("project_id")
				break
	suspended = []
	for row in evaluated["projects"]:
		if row.get("status") == FROZEN:
			suspended.append(row.get("project_id"))
	return {
		"mechanics_version": MECHANICS_VERSION,
		"queue_contract_version": QUEUE_CONTRACT_VERSION,
		"restore_priority": bool(restore_priority),
		"projects": evaluated["projects"],
		"ordered_project_ids": evaluated["ordered_project_ids"],
		"restored_project_ids": evaluated["restored_project_ids"],
		"suspended_project_ids": suspended,
		"active_project_id": active_id,
		"active_project_changed": active_id != (active_project_id or ""),
		"notifications": notifications,
	}


def queue_snapshot_fields(queue_state):
	"""Return bounded queue telemetry for the UI/marker bridge."""
	state = queue_state or {}
	return {
		"queue_contract_version": state.get("queue_contract_version", QUEUE_CONTRACT_VERSION),
		"active_project_id": state.get("active_project_id", ""),
		"active_project_changed": bool(state.get("active_project_changed", False)),
		"frozen_project_count": len(state.get("suspended_project_ids", [])),
		"restored_project_count": len(state.get("restored_project_ids", [])),
		"notification_count": len(state.get("notifications", [])),
		"restore_priority": bool(state.get("restore_priority", False)),
	}


def snapshot_fields(descriptor):
	"""Return compact capability and invariant fields for a marker/UI row."""
	capability = capability_from_descriptor(descriptor)
	invariant, invariant_reason = invariant_status(capability)
	return {
		"mechanics_version": MECHANICS_VERSION,
		"mechanics_contract": "FTTWR_RECRUIT_CAPACITY_V1",
		"human_capacity": capability["human_sources"],
		"human_occupancy": capability["human_occupancy"],
		"human_available": capability["human_available"],
		"human_basic_capacity": capability["human_basic_sources"],
		"human_basic_occupancy": capability["human_basic_occupancy"],
		"human_basic_available": capability["human_basic_available"],
		"human_advanced_capacity": capability["human_advanced_sources"],
		"human_advanced_occupancy": capability["human_advanced_occupancy"],
		"human_advanced_available": capability["human_advanced_available"],
		"human_veteran_capacity": capability["human_veteran_sources"],
		"human_veteran_occupancy": capability["human_veteran_occupancy"],
		"human_veteran_available": capability["human_veteran_available"],
		"robot_capacity": capability["robot_sources"],
		"robot_occupancy": capability["robot_occupancy"],
		"robot_available": capability["robot_available"],
		"population_available": capability["human_population_available"],
		"population_source": capability["population_source"],
		"descriptor_invariant_status": invariant,
		"descriptor_invariant_reason": invariant_reason,
		"frozen_project_count": 0,
		"restore_priority_default": False,
	}
