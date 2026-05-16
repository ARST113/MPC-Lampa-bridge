#include "stdafx.h"
#include "LampaBridgeSession.h"

void LampaBridgeSession::Clear()
{
	sessionId.Empty();
	items.clear();
	activeIndex = 0;
	activeUrl.Empty();
	activeTitle.Empty();
	activeTimelineHash.Empty();
	requestedPosition = -1.0;
	timelineDuration = 0.0;
	timelinePercent = 0.0;
	client = L"lampa";
	bridgeMode = L"local";
	localToken.Empty();
	emitPosition = true;
	positionIntervalMs = 1000;
	schemaVersion = 1;
	store = LampaBridgeStore{};
	lastPositionEventTs = 0;
	lastPlaybackState.Empty();
	lastIsPlaying = false;
	lastIsBuffering = false;
	pendingNavigation = false;
	pendingTargetIndex = -1;
	pendingSeekPosition = -1.0;
	pendingSeekTimelineHash.Empty();
}

bool LampaBridgeSession::HasActive() const
{
	return !items.empty() && activeIndex >= 0 && activeIndex < (int)items.size();
}
