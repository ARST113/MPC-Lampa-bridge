#pragma once
#include <cstdint>
#include "LampaBridgeSession.h"
#include "LampaBridgeJson.h"

class CMainFrame;

class CLampaBridge {
public:
	static CLampaBridge& Instance();
	void AddCors(CStringA& hdr) const;
	bool IsLampaPath(const CString& path) const;
	bool IsBridgePath(const CString& path) const;
	bool Health(CStringA& body) const;
	bool Open(CMainFrame* pMainFrame, const CStringA& payloadBody, CStringA& body);
	bool Status(CMainFrame* pMainFrame, CStringA& body);
	bool Command(CMainFrame* pMainFrame, const CStringA& payloadBody, CStringA& body);
	bool Close(CMainFrame* pMainFrame, CStringA& body);
	bool Ping(CStringA& body) const;
	bool State(const CString& sid, const CString& token, CMainFrame* pMainFrame, CStringA& body);
	bool Events(const CString& sid, const CString& token, int64_t since, int limit, CStringA& body);
	void ApplyPendingSeek(CMainFrame* pMainFrame);
	void EmitEvent(const CStringA& type, const CStringA& payloadJson, bool asState = true);
	void EmitSessionStarted(CMainFrame* pMainFrame);
	void EmitPlaybackStateChanged(CMainFrame* pMainFrame);
	void EmitPositionTick(CMainFrame* pMainFrame, bool force = false);
	void EmitSeekCompleted(CMainFrame* pMainFrame, int64_t fromMs, int64_t toMs);
	void EmitPlaylistItemChanged(CMainFrame* pMainFrame, const CStringA& reason);
	void EmitPlaybackEnded(CMainFrame* pMainFrame);
	void EmitSessionFinished(CMainFrame* pMainFrame, const CStringA& endBy);
	void EmitError(const CStringA& code, const CStringA& message);
private:
	LampaBridgeSession m_session;
	CString NormalizePlayableUrl(const CString& url) const;
	int ResolveActiveIndex(const LampaOpenPayload& payload) const;
	bool OpenIndex(CMainFrame* pMainFrame, int index);
	void FillStatus(CMainFrame* pMainFrame, CStringA& body);
	bool CheckToken(const CString& token) const;
	int64_t PositionMs(CMainFrame* pMainFrame) const;
	int64_t DurationMs(CMainFrame* pMainFrame) const;
	int64_t NowMs() const;
};
