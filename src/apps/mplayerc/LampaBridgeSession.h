#pragma once

struct LampaBridgePlaylistItem {
	CString title;
	CString url;
	CString timelineHash;
	double position = -1.0;
	double duration = 0.0;
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
	void Clear();
	bool HasActive() const;
};
