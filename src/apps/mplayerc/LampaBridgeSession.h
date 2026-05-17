#pragma once
#include <cstdint>
#include <deque>
#include <vector>

struct LampaBridgeEnvelope {
	int schema = 1;
	CStringA type;
	CStringA client;
	CStringA sessionId;
	int64_t ts = 0;
	CStringA payloadJson;
};

struct LampaBridgeStore {
	bool hasLastState = false;
	LampaBridgeEnvelope lastState;
	std::deque<LampaBridgeEnvelope> events;
};

struct LampaBridgePlaylistItem {
	CString title;
	CString url;
	CString timelineHash;
	CString thumbnail;
	CString filename;
	int season = -1;
	int episode = -1;
	double position = -1.0;
	double duration = 0.0;
	double timelinePercent = 0.0;
};

struct LampaBridgeSession {
	CString sessionId;
	std::vector<LampaBridgePlaylistItem> items;
	int activeIndex = 0;
	CString activeUrl;
	CString activeTitle;
	CString activeTimelineHash;
	double requestedPosition = -1.0;
	double timelineDuration = 0.0;
	double timelinePercent = 0.0;
	CString client = L"lampa";
	CString bridgeMode = L"local";
	CString localToken;
	bool emitPosition = true;
	int positionIntervalMs = 1000;
	int schemaVersion = 1;
	LampaBridgeStore store;
	int64_t lastPositionEventTs = 0;
	CString lastPlaybackState;
	bool lastIsPlaying = false;
	bool lastIsBuffering = false;
	bool pendingNavigation = false;
	int pendingTargetIndex = -1;
	double pendingSeekPosition = -1.0;
	CString pendingSeekTimelineHash;
	void Clear();
	bool HasActive() const;
};
