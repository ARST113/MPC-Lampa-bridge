#include "stdafx.h"
#include "MainFrm.h"
#include "LampaBridge.h"
#include "Version.h"
#include <chrono>

CLampaBridge& CLampaBridge::Instance() { static CLampaBridge s; return s; }
void CLampaBridge::AddCors(CStringA& hdr) const { hdr += "Access-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: GET, POST, OPTIONS\r\nAccess-Control-Allow-Headers: Content-Type\r\nAccess-Control-Allow-Private-Network: true\r\n"; }
bool CLampaBridge::IsLampaPath(const CString& path) const { return path.Left(7).CompareNoCase(L"/lampa/")==0; }
bool CLampaBridge::IsBridgePath(const CString& path) const { return path.CompareNoCase(L"/ping")==0 || path.CompareNoCase(L"/state")==0 || path.CompareNoCase(L"/events")==0 || IsLampaPath(path); }

int64_t CLampaBridge::NowMs() const {
	using namespace std::chrono;
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}
int64_t CLampaBridge::PositionMs(CMainFrame* f) const { return f ? (int64_t)(f->GetPos()/10000) : 0; }
int64_t CLampaBridge::DurationMs(CMainFrame* f) const { return f ? (int64_t)(f->GetDur()/10000) : 0; }
bool CLampaBridge::CheckToken(const CString& token) const { return m_session.localToken.IsEmpty() || token == m_session.localToken; }

CString CLampaBridge::NormalizePlayableUrl(const CString& url) const
{
	CString out = url;
	int frag = out.Find(L"#");
	if (frag >= 0) out = out.Left(frag);
	out.Replace(L"&preload", L"&play");
	out.Replace(L"?preload", L"?play");
	return out;
}

int CLampaBridge::ResolveActiveIndex(const LampaOpenPayload& payload) const
{
	const int candidates[] = { payload.playlistIndex, payload.startIndex, payload.dddIndex, payload.dddStart, payload.index, payload.windowIndex };
	for (int c : candidates) if (c >= 0 && c < (int)m_session.items.size()) return c;
	const CString nurl = NormalizePlayableUrl(payload.url);
	for (size_t i = 0; i < m_session.items.size(); ++i) if (NormalizePlayableUrl(m_session.items[i].url).CompareNoCase(nurl) == 0) return (int)i;
	return 0;
}



void CLampaBridge::SyncActiveFromPlayer(CMainFrame* pMainFrame, const CStringA& reason)
{
	if (!pMainFrame || m_session.items.empty()) return;
	CString cur = NormalizePlayableUrl(pMainFrame->GetCurFileName());
	if (cur.IsEmpty()) return;
	int found = -1;
	for (size_t i = 0; i < m_session.items.size(); ++i) {
		if (NormalizePlayableUrl(m_session.items[i].url).CompareNoCase(cur) == 0) { found = (int)i; break; }
	}
	if (found < 0) {
		CString curName = GetFileName(cur);
		for (size_t i = 0; i < m_session.items.size(); ++i) {
			if (!m_session.items[i].filename.IsEmpty() && m_session.items[i].filename.CompareNoCase(curName) == 0) { found = (int)i; break; }
		}
	}
	if (found < 0) return;
	if (found != m_session.activeIndex) {
		m_session.activeIndex = found;
		auto& it = m_session.items[found];
		m_session.activeUrl = NormalizePlayableUrl(it.url);
		m_session.activeTitle = it.title;
		m_session.activeTimelineHash = it.timelineHash;
		EmitPlaylistItemChanged(pMainFrame, reason);
		if (m_session.pendingNavigation && m_session.pendingTargetIndex == found) {
			m_session.requestedPosition = m_session.pendingSeekPosition;
			m_session.pendingNavigation = false;
			m_session.pendingTargetIndex = -1;
			m_session.pendingSeekPosition = -1.0;
			m_session.pendingSeekTimelineHash.Empty();
			ApplyPendingSeek(pMainFrame);
			EmitPlaybackStateChanged(pMainFrame);
			EmitPositionTick(pMainFrame, true);
		}
	}
}

static CStringA EscA(const CString& v)
{
	return LampaBridgeJson::EscapeJson(v);
}

void CLampaBridge::EmitEvent(const CStringA& type, const CStringA& payloadJson, bool asState){
	LampaBridgeEnvelope e; e.schema=m_session.schemaVersion; e.type=type; e.client=UTF8(m_session.client); e.sessionId=UTF8(m_session.sessionId); e.ts=NowMs(); e.payloadJson=payloadJson;
	m_session.store.events.push_back(e); while (m_session.store.events.size()>200) m_session.store.events.pop_front();
	if (asState) { m_session.store.lastState=e; m_session.store.hasLastState=true; }
}
void CLampaBridge::EmitSessionStarted(CMainFrame* pMainFrame){ CStringA p; const CStringA sid = EscA(m_session.sessionId); const CStringA uri = EscA(m_session.activeUrl); const CStringA title = EscA(m_session.activeTitle); const auto& cur = m_session.items[m_session.activeIndex]; p.Format("{\"sessionId\":\"%s\",\"ts\":%lld,\"uri\":\"%s\",\"title\":\"%s\",\"playlistSize\":%d,\"startIndex\":%d,\"startPosition\":%lld,\"currentItem\":{\"uri\":\"%s\",\"title\":\"%s\",\"index\":%d,\"filename\":\"%s\",\"thumbnail\":\"%s\",\"season\":%d,\"episode\":%d,\"timelineHash\":\"%s\"}}", sid.GetString(), NowMs(), uri.GetString(), title.GetString(), (int)m_session.items.size(), m_session.activeIndex, PositionMs(pMainFrame), uri.GetString(), title.GetString(), m_session.activeIndex, EscA(cur.filename).GetString(), EscA(cur.thumbnail).GetString(), cur.season, cur.episode, EscA(cur.timelineHash).GetString()); EmitEvent("session_started", p, true); }
void CLampaBridge::EmitPlaybackStateChanged(CMainFrame* f){
	OAFilterState fs=f?f->GetMediaState():State_Stopped;
	const bool isPlaying = fs == State_Running;
	const bool isBuffering = false;
	CStringA p;
	const CStringA sid = EscA(m_session.sessionId);
	const CStringA title = EscA(m_session.activeTitle);
	const CStringA uri = EscA(m_session.activeUrl);
	p.Format("{\"sessionId\":\"%s\",\"ts\":%lld,\"isPlaying\":%s,\"isBuffering\":%s,\"position\":%lld,\"duration\":%lld,\"windowIndex\":%d,\"playlistSize\":%d,\"title\":\"%s\",\"uri\":\"%s\"}",
		sid.GetString(), NowMs(),
		isPlaying?"true":"false", isBuffering?"true":"false",
		PositionMs(f), DurationMs(f), m_session.activeIndex, (int)m_session.items.size(),
		title.GetString(), uri.GetString());
	EmitEvent("playback_state_changed", p, true);
}
void CLampaBridge::EmitPositionTick(CMainFrame* f, bool force){ if(!m_session.HasActive()) return; if(!m_session.emitPosition && !force) return; int64_t now=NowMs(); if(!force && now-m_session.lastPositionEventTs<m_session.positionIntervalMs) return; m_session.lastPositionEventTs=now; CStringA p; const CStringA sid = EscA(m_session.sessionId); const CStringA uri = EscA(m_session.activeUrl); const CStringA title = EscA(m_session.activeTitle); p.Format("{\"sessionId\":\"%s\",\"ts\":%lld,\"uri\":\"%s\",\"position\":%lld,\"duration\":%lld,\"bufferedPosition\":%lld,\"bufferedPercentage\":0,\"windowIndex\":%d,\"playlistSize\":%d,\"title\":\"%s\",\"reason\":\"tick\"}", sid.GetString(), NowMs(), uri.GetString(), PositionMs(f), DurationMs(f), PositionMs(f), m_session.activeIndex, (int)m_session.items.size(), title.GetString()); EmitEvent("position_tick", p, true); }
void CLampaBridge::EmitSeekCompleted(CMainFrame* pMainFrame, int64_t fromMs, int64_t toMs){ CStringA p; p.Format("{\"fromPosition\":%lld,\"toPosition\":%lld,\"position\":%lld,\"duration\":%lld,\"windowIndex\":%d}", fromMs, toMs, toMs, DurationMs(pMainFrame), m_session.activeIndex); EmitEvent("seek_completed", p, true); }
void CLampaBridge::EmitPlaylistItemChanged(CMainFrame* pMainFrame, const CStringA& reason){ CStringA p; const CStringA uri = EscA(m_session.activeUrl); const CStringA title = EscA(m_session.activeTitle); const auto& cur = m_session.items[m_session.activeIndex]; p.Format("{\"uri\":\"%s\",\"windowIndex\":%d,\"playlistSize\":%d,\"title\":\"%s\",\"reason\":\"%s\",\"position\":%lld,\"duration\":%lld,\"hasPrevious\":%s,\"hasNext\":%s,\"currentItem\":{\"uri\":\"%s\",\"title\":\"%s\",\"index\":%d,\"filename\":\"%s\",\"thumbnail\":\"%s\",\"season\":%d,\"episode\":%d,\"timelineHash\":\"%s\"}}", uri.GetString(), m_session.activeIndex, (int)m_session.items.size(), title.GetString(), reason.GetString(), PositionMs(pMainFrame), DurationMs(pMainFrame), m_session.activeIndex>0?"true":"false", m_session.activeIndex+1<(int)m_session.items.size()?"true":"false", uri.GetString(), title.GetString(), m_session.activeIndex, EscA(cur.filename).GetString(), EscA(cur.thumbnail).GetString(), cur.season, cur.episode, EscA(cur.timelineHash).GetString()); EmitEvent("playlist_item_changed", p, true); }
void CLampaBridge::EmitPlaybackEnded(CMainFrame* pMainFrame){ CStringA p; const CStringA title = EscA(m_session.activeTitle); p.Format("{\"windowIndex\":%d,\"playlistSize\":%d,\"title\":\"%s\",\"position\":%lld,\"duration\":%lld}", m_session.activeIndex, (int)m_session.items.size(), title.GetString(), PositionMs(pMainFrame), DurationMs(pMainFrame)); EmitEvent("playback_ended", p, true); }
void CLampaBridge::EmitSessionFinished(CMainFrame* pMainFrame, const CStringA& endBy){ CStringA p; const CStringA uri = EscA(m_session.activeUrl); const CStringA title = EscA(m_session.activeTitle); p.Format("{\"uri\":\"%s\",\"position\":%lld,\"duration\":%lld,\"endBy\":\"%s\",\"windowIndex\":%d,\"playlistSize\":%d,\"title\":\"%s\"}", uri.GetString(), PositionMs(pMainFrame), DurationMs(pMainFrame), endBy.GetString(), m_session.activeIndex, (int)m_session.items.size(), title.GetString()); EmitEvent("session_finished", p, true); }
void CLampaBridge::EmitError(const CStringA& code, const CStringA& message){ CStringA p; const CStringA escapedCode = LampaBridgeJson::EscapeJson(code); const CStringA escapedMessage = LampaBridgeJson::EscapeJson(message); p.Format("{\"code\":\"%s\",\"message\":\"%s\",\"windowIndex\":%d}", escapedCode.GetString(), escapedMessage.GetString(), m_session.activeIndex); EmitEvent("error", p, false); }

bool CLampaBridge::Health(CStringA& body) const { body = "{\"ok\":true,\"name\":\"MPC-Lampa-bridge\",\"player\":\"MPC-BE\",\"version\":\"" + CStringA(MPC_VERSION_FULL_STR) + "\"}"; return true; }
bool CLampaBridge::Ping(CStringA& body) const { body = "{\"ok\":true,\"service\":\"dddplayer-local-bridge\"}"; return true; }
bool CLampaBridge::State(const CString& sid, const CString& token, CMainFrame* pMainFrame, CStringA& body){ if (sid != m_session.sessionId || !CheckToken(token)) { body = "{\"ok\":false,\"error\":\"forbidden\"}"; return true; } SyncActiveFromPlayer(pMainFrame, "state_poll"); EmitPositionTick(pMainFrame, false); body = LampaBridgeJson::BuildStateResponse(m_session, PositionMs(pMainFrame), DurationMs(pMainFrame), pMainFrame && pMainFrame->GetMediaState()==State_Running, m_session.store.hasLastState ? &m_session.store.lastState : nullptr); return true; }
bool CLampaBridge::Events(const CString& sid, const CString& token, int64_t since, int limit, CStringA& body){ if (sid != m_session.sessionId || !CheckToken(token)) { body = "{\"ok\":false,\"error\":\"forbidden\"}"; return true; } if(limit<=0) limit=50; if(limit>200) limit=200; std::deque<LampaBridgeEnvelope> filtered; for (const auto& e : m_session.store.events) if (e.ts > since) filtered.push_back(e); body = LampaBridgeJson::BuildEventsResponse(filtered, limit); return true; }

bool CLampaBridge::OpenIndex(CMainFrame* pMainFrame, int index){ if(!pMainFrame||index<0||index>=(int)m_session.items.size()) return false; std::list<CString> cmdln; for (const auto& item : m_session.items) { CString clean = NormalizePlayableUrl(item.url); if (!clean.IsEmpty()) cmdln.emplace_back(clean);} if (cmdln.empty()) return false; int len=0; for(auto&s:cmdln){len+=(s.GetLength()+1)*sizeof(WCHAR);} std::unique_ptr<BYTE[]> buff(new BYTE[4+len]); BYTE* p=buff.get(); *(DWORD*)p=(DWORD)cmdln.size(); p+=4; for(auto&s:cmdln){int l=(s.GetLength()+1)*sizeof(WCHAR); memcpy(p,(LPCWSTR)s,l); p+=l;} COPYDATASTRUCT cds{0x6ABE51,(DWORD)(p-buff.get()),buff.get()}; pMainFrame->SendMessageW(WM_COPYDATA,0,(LPARAM)&cds); auto& it=m_session.items[index]; m_session.activeIndex=index; m_session.activeUrl=NormalizePlayableUrl(it.url); m_session.activeTitle=it.title; m_session.activeTimelineHash=it.timelineHash; m_session.requestedPosition=it.position; m_session.timelinePercent=it.timelinePercent; m_session.timelineDuration=it.duration; return true; }

bool CLampaBridge::Open(CMainFrame* pMainFrame, const CStringA& payloadBody, CStringA& body){ LampaOpenPayload p; CString err; if(!LampaBridgeJson::ParseOpenPayload(payloadBody,p,err)){ body="{\"ok\":false,\"error\":\""+LampaBridgeJson::EscapeJson(err)+"\"}"; return true; } m_session.Clear(); if(!p.bridgeSessionId.IsEmpty()) m_session.sessionId=p.bridgeSessionId; else m_session.sessionId.Format(L"%08x", GetTickCount()); m_session.client = p.bridgeClient.IsEmpty()?L"lampa":p.bridgeClient; if(!p.bridgeMode.IsEmpty()) m_session.bridgeMode=p.bridgeMode; m_session.localToken=p.bridgeLocalToken; m_session.emitPosition=p.bridgeEmitPosition; m_session.positionIntervalMs=p.bridgePositionIntervalMs; m_session.schemaVersion=p.bridgeSchemaVersion; m_session.timelineDuration=p.timelineDuration; m_session.timelinePercent=p.timelinePercent; if(p.playlist.empty()){ LampaBridgePlaylistItem i; i.url=NormalizePlayableUrl(p.url); i.title=p.title; i.position=p.position; i.timelineHash=p.timelineHash; m_session.items.push_back(i);} else { m_session.items=p.playlist; for (auto& item : m_session.items) item.url = NormalizePlayableUrl(item.url); } p.url = NormalizePlayableUrl(p.url); int active=ResolveActiveIndex(p); if(!OpenIndex(pMainFrame, active)){ body="{\"ok\":false,\"error\":\"open failed\"}"; return true;} if(m_session.requestedPosition<=10 && p.timelineTime>10) m_session.requestedPosition=p.timelineTime; m_session.pendingNavigation=true; m_session.pendingTargetIndex=active; m_session.pendingSeekPosition=m_session.requestedPosition; SyncActiveFromPlayer(pMainFrame, "open_selected"); if(!m_session.pendingNavigation) ApplyPendingSeek(pMainFrame); EmitSessionStarted(pMainFrame); EmitPlaybackStateChanged(pMainFrame); EmitPositionTick(pMainFrame, true); const CStringA sid = EscA(m_session.sessionId); const CStringA title = EscA(m_session.activeTitle); body.Format("{\"ok\":true,\"session_id\":\"%s\",\"active_index\":%d,\"playlist_size\":%d,\"title\":\"%s\"}", sid.GetString(), m_session.activeIndex, (int)m_session.items.size(), title.GetString()); return true; }

void CLampaBridge::ApplyPendingSeek(CMainFrame* pMainFrame){ if(!pMainFrame||m_session.requestedPosition<=10) return; OAFilterState fs=pMainFrame->GetMediaState(); double dur=(double)pMainFrame->GetDur()/10000000.0; if(fs==State_Stopped||dur<=0.0) return; if(m_session.timelinePercent>=90){ m_session.requestedPosition=-1.0; return; } double pos=m_session.requestedPosition; if(pos>dur-15.0) pos=dur-15.0; if(pos>1.0){ pMainFrame->SeekTo((REFERENCE_TIME)(pos*10000000.0)); } m_session.requestedPosition=-1.0; }

void CLampaBridge::FillStatus(CMainFrame* pMainFrame, CStringA& body){ OAFilterState fs = pMainFrame ? pMainFrame->GetMediaState() : State_Stopped; const char* state="unknown"; if(fs==State_Running) state="playing"; else if(fs==State_Paused) state="paused"; else if(fs==State_Stopped) state="stopped"; double t=pMainFrame?(double)pMainFrame->GetPos()/10000000.0:0; double d=pMainFrame?(double)pMainFrame->GetDur()/10000000.0:0; int percent=(d>0)?(int)(t*100.0/d):0; if(!m_session.HasActive()){ body="{\"ok\":true,\"opened\":false,\"state\":\"stopped\",\"time\":0,\"duration\":0,\"percent\":0,\"active_index\":-1,\"playlist_size\":0,\"title\":\"\",\"url\":\"\",\"timeline_hash\":\"\"}"; return;} const CStringA title = EscA(m_session.activeTitle); const CStringA url = EscA(m_session.activeUrl); const CStringA th = EscA(m_session.activeTimelineHash); body.Format("{\"ok\":true,\"opened\":true,\"state\":\"%s\",\"time\":%.3f,\"duration\":%.3f,\"percent\":%d,\"active_index\":%d,\"playlist_size\":%d,\"title\":\"%s\",\"url\":\"%s\",\"timeline_hash\":\"%s\"}", state,t,d,percent,m_session.activeIndex,(int)m_session.items.size(),title.GetString(),url.GetString(),th.GetString()); }
bool CLampaBridge::Status(CMainFrame* pMainFrame, CStringA& body){ SyncActiveFromPlayer(pMainFrame, "ui_switch"); ApplyPendingSeek(pMainFrame); EmitPositionTick(pMainFrame, false); FillStatus(pMainFrame, body); return true; }

bool CLampaBridge::Command(CMainFrame* pMainFrame, const CStringA& payloadBody, CStringA& body){ rapidjson::Document d; if(d.Parse(payloadBody).HasParseError()||!d.IsObject()){ body="{\"ok\":false,\"error\":\"invalid json\"}"; return true;} CStringA cmd; getJsonValue(d,"command",cmd); cmd.MakeLower(); if(cmd=="play"){ pMainFrame->SendMessageW(WM_COMMAND,ID_PLAY_PLAY); EmitPlaybackStateChanged(pMainFrame);} else if(cmd=="pause"){ pMainFrame->SendMessageW(WM_COMMAND,ID_PLAY_PAUSE); EmitPlaybackStateChanged(pMainFrame);} else if(cmd=="stop"){ pMainFrame->SendMessageW(WM_COMMAND,ID_PLAY_STOP); EmitPlaybackStateChanged(pMainFrame); EmitSessionFinished(pMainFrame, "user");} else if(cmd=="seek"){ double tm=0; if(d.HasMember("time")&&d["time"].IsNumber()) tm=d["time"].GetDouble(); int64_t from=PositionMs(pMainFrame); pMainFrame->SeekTo((REFERENCE_TIME)(tm*10000000.0)); EmitSeekCompleted(pMainFrame, from, (int64_t)(tm*1000.0)); EmitPositionTick(pMainFrame, true);} else if(cmd=="next"&&m_session.activeIndex+1<(int)m_session.items.size()){ m_session.pendingNavigation=true; m_session.pendingTargetIndex=m_session.activeIndex+1; m_session.pendingSeekPosition=m_session.items[m_session.pendingTargetIndex].position; m_session.pendingSeekTimelineHash=m_session.items[m_session.pendingTargetIndex].timelineHash; pMainFrame->PostMessageW(WM_COMMAND, ID_NAVIGATE_SKIPFORWARD); SyncActiveFromPlayer(pMainFrame, "next"); } else if(cmd=="prev"&&m_session.activeIndex>0){ m_session.pendingNavigation=true; m_session.pendingTargetIndex=m_session.activeIndex-1; m_session.pendingSeekPosition=m_session.items[m_session.pendingTargetIndex].position; m_session.pendingSeekTimelineHash=m_session.items[m_session.pendingTargetIndex].timelineHash; pMainFrame->PostMessageW(WM_COMMAND, ID_NAVIGATE_SKIPBACK); SyncActiveFromPlayer(pMainFrame, "prev"); } FillStatus(pMainFrame, body); return true; }

bool CLampaBridge::Close(CMainFrame* pMainFrame, CStringA& body){ EmitSessionFinished(pMainFrame, "user"); if(pMainFrame) pMainFrame->SendMessageW(WM_COMMAND,ID_PLAY_STOP); m_session.items.clear(); m_session.activeIndex=0; m_session.activeUrl.Empty(); m_session.activeTitle.Empty(); m_session.activeTimelineHash.Empty(); body="{\"ok\":true}"; return true; }
