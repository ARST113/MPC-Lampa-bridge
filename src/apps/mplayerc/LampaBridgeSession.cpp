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
}

bool LampaBridgeSession::HasActive() const
{
	return !items.empty() && activeIndex >= 0 && activeIndex < (int)items.size();
}
