# ACBase Test DPF player-facing content.
#
# This is presentation metadata only.  Native DPF state, event ingestion,
# transition selection, and effects remain authoritative in CvDPF.  The table
# gives the current disposable fixture names and the player-facing hint for
# each stage while the XML/DLL presentation query surface is being expanded.
# Python 2.4-compatible: no comprehensions, decorators, or newer libraries.

_PROGRESSIONS = {
    1: {
        "name": u"Resource Network",
        "description": u"Build improvements to expand or protect a local resource network; completing it grants a persistent trait.",
        "event_1": u"Improvements",
        "event_2": u"Features lost",
        "reward_variables": {
            2: u"Network capacity",
            3: u"Protection charges",
            4: u"Network exposed",
        },
        "stages": {
            0: {"status": u"Not started", "hint": u"Complete an improvement to begin."},
            1: {"status": u"Developing", "hint": u"Remove a feature to advance."},
            2: {"status": u"Expanding", "hint": u"Complete another improvement to increase the network reward."},
            3: {"status": u"Network reward active", "hint": u"The persistent network trait reward is active."},
        },
    },
    2: {
        "name": u"Network Protection",
        "description": u"A feature loss can block progress; improvements restore it.",
        "event_1": u"Improvements",
        "event_2": u"Features lost",
        "stages": {
            0: {"status": u"Not started", "hint": u"Complete an improvement to begin."},
            1: {"status": u"Protected", "hint": u"Keep improving to finish this track."},
            2: {"status": u"Blocked", "hint": u"Complete an improvement to recover."},
            3: {"status": u"Protection complete", "hint": u"Protection reward is complete."},
        },
    },
    3: {
        "name": u"Setback Recovery",
        "description": u"A feature loss can fail progress; improvements allow recovery.",
        "event_1": u"Improvements",
        "event_2": u"Features lost",
        "stages": {
            0: {"status": u"Not started", "hint": u"Complete an improvement to begin."},
            1: {"status": u"Advancing", "hint": u"Continue improving this track."},
            2: {"status": u"Failed", "hint": u"Complete an improvement to recover."},
            3: {"status": u"Recovery complete", "hint": u"Recovery reward is complete."},
        },
    },
}


def get_progression(definition_id):
    """Return immutable-by-convention presentation metadata for one ID."""
    return _PROGRESSIONS.get(definition_id, None)


def get_stage(definition_id, stage_id):
    """Return stage presentation metadata, with a safe fallback."""
    progression = get_progression(definition_id)
    if progression is None:
        return None
    return progression.get("stages", {}).get(stage_id, None)



