#pragma once
#include "rapidjsonHelper.h"
#include "LampaBridgeSession.h"

struct LampaOpenPayload {
	CString url;
	CString title;
	double position = -1.0;
	CString timelineHash;
	double timelineTime = -1.0;
	double timelineDuration = 0.0;
	double timelinePercent = 0.0;
	std::vector<LampaBridgePlaylistItem> playlist;
};

namespace LampaBridgeJson {
	bool ParseOpenPayload(const CStringA& body, LampaOpenPayload& payload, CString& error);
	CStringA EscapeJson(const CString& value);
}
