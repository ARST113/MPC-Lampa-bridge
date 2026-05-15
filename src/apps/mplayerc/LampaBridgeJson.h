#pragma once
#include <deque>
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
	CString bridgeSessionId;
	CString bridgeClient;
	CString bridgeMode;
	CString bridgeLocalToken;
	bool bridgeEmitPosition = true;
	int bridgePositionIntervalMs = 1000;
	int bridgeSchemaVersion = 1;
};

namespace LampaBridgeJson {
	bool ParseOpenPayload(const CStringA& body, LampaOpenPayload& payload, CString& error);
	CStringA EscapeJson(const CString& value);
	CStringA BuildEnvelopeJson(int schema, const CStringA& type, const CStringA& client, const CStringA& sessionId, int64_t ts, const CStringA& payloadJson);
	CStringA BuildStateResponse(const LampaBridgeEnvelope* state);
	CStringA BuildEventsResponse(const std::deque<LampaBridgeEnvelope>& events, int limit);
}
