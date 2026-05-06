#pragma once
#include "LampaBridgeSession.h"
#include "LampaBridgeJson.h"

class CMainFrame;

class CLampaBridge {
public:
	static CLampaBridge& Instance();
	void AddCors(CStringA& hdr) const;
	bool IsLampaPath(const CString& path) const;
	bool Health(CStringA& body) const;
	bool Open(CMainFrame* pMainFrame, const CStringA& payloadBody, CStringA& body);
	bool Status(CMainFrame* pMainFrame, CStringA& body);
	bool Command(CMainFrame* pMainFrame, const CStringA& payloadBody, CStringA& body);
	bool Close(CMainFrame* pMainFrame, CStringA& body);
	void ApplyPendingSeek(CMainFrame* pMainFrame);
private:
	LampaBridgeSession m_session;
	bool OpenIndex(CMainFrame* pMainFrame, int index);
	void FillStatus(CMainFrame* pMainFrame, CStringA& body);
};
