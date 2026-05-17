#pragma once
#include <cstdint>
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
	int playlistIndex = -1;
	int startIndex = -1;
	int dddIndex = -1;
	int dddStart = -1;
	int index = -1;
	int windowIndex = -1;
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
	CStringA EscapeJson(const CStringA& value);
	CStringA EscapeJson(const CString& value);
	CStringA BuildEnvelopeJson(int schema, const CStringA& type, const CStringA& client, const CStringA& sessionId, int64_t ts, const CStringA& payloadJson);
	CStringA BuildStateResponse(const LampaBridgeSession& session, int64_t positionMs, int64_t durationMs, bool active, const LampaBridgeEnvelope* state);
	CStringA BuildEventsResponse(const std::deque<LampaBridgeEnvelope>& events, int limit);
}
