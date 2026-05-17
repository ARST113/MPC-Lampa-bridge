#include "stdafx.h"
#include "LampaBridgeJson.h"

using namespace rapidjson;

static CString Utf8ToWide(const CStringA& value)
{
	if (value.IsEmpty()) return CString();
	int len = MultiByteToWideChar(CP_UTF8, 0, value.GetString(), -1, nullptr, 0);
	if (len <= 0) return CString();
	CString result;
	LPWSTR out = result.GetBuffer(len);
	MultiByteToWideChar(CP_UTF8, 0, value.GetString(), -1, out, len);
	result.ReleaseBuffer();
	return result;
}

static CStringA WideToUtf8(const CString& value)
{
	if (value.IsEmpty()) return CStringA();
	int len = WideCharToMultiByte(CP_UTF8, 0, value.GetString(), -1, nullptr, 0, nullptr, nullptr);
	if (len <= 0) return CStringA();
	CStringA result;
	LPSTR out = result.GetBuffer(len);
	WideCharToMultiByte(CP_UTF8, 0, value.GetString(), -1, out, len, nullptr, nullptr);
	result.ReleaseBuffer();
	return result;
}


static CStringA JsonValueOrAlias(const rapidjson::Value& obj, const char* k, const char* a)
{
	CStringA v;
	getJsonValue(obj, k, v);
	if (v.IsEmpty() && a) {
		getJsonValue(obj, a, v);
	}
	return v;
}

static double GetNumberOrDefault(const Value& obj, const char* key, double def)
{
	if (const auto it = obj.FindMember(key); it != obj.MemberEnd()) {
		if (it->value.IsNumber()) return it->value.GetDouble();
	}
	return def;
}

static CString JsonStringOrAlias(const Value& obj, std::initializer_list<const char*> keys)
{
	CStringA tmp;
	for (const char* key : keys) {
		getJsonValue(obj, key, tmp);
		if (!tmp.IsEmpty()) return Utf8ToWide(tmp);
	}
	return CString();
}

static int GetIntOrDefault(const Value& obj, const char* key, int def)
{
	if (const auto it = obj.FindMember(key); it != obj.MemberEnd() && it->value.IsInt()) return it->value.GetInt();
	if (const auto it = obj.FindMember(key); it != obj.MemberEnd() && it->value.IsNumber()) return (int)it->value.GetDouble();
	return def;
}

bool LampaBridgeJson::ParseOpenPayload(const CStringA& body, LampaOpenPayload& payload, CString& error)
{
	Document d;
	if (d.Parse(body).HasParseError() || !d.IsObject()) {
		error = L"invalid json";
		return false;
	}
	payload.url = JsonStringOrAlias(d, {"url", "uri", "src", "path"});
	payload.title = JsonStringOrAlias(d, {"title", "name", "filename", "file_name"});
	payload.position = GetNumberOrDefault(d, "position", -1.0);
	payload.playlistIndex = GetIntOrDefault(d, "playlist_index", -1);
	payload.startIndex = GetIntOrDefault(d, "start_index", -1);
	payload.dddIndex = GetIntOrDefault(d, "ddd_i", -1);
	payload.dddStart = GetIntOrDefault(d, "ddd_start", -1);
	payload.index = GetIntOrDefault(d, "index", -1);
	payload.windowIndex = GetIntOrDefault(d, "windowIndex", -1);
	CStringA tmp;
	getJsonValue(d, "timeline_hash", tmp); payload.timelineHash = Utf8ToWide(tmp);
	if (const Value* timeline = GetJsonObject(d, "timeline")) {
		getJsonValue(*timeline, "hash", tmp); if (payload.timelineHash.IsEmpty()) payload.timelineHash = Utf8ToWide(tmp);
		payload.timelineTime = GetNumberOrDefault(*timeline, "time", -1.0);
		payload.timelineDuration = GetNumberOrDefault(*timeline, "duration", 0.0);
		payload.timelinePercent = GetNumberOrDefault(*timeline, "percent", 0.0);
	}
	if (const Value* playlist = GetJsonArray(d, "playlist")) {
		for (const auto& item : playlist->GetArray()) {
			if (!item.IsObject()) continue;
			LampaBridgePlaylistItem p;
			p.url = JsonStringOrAlias(item, {"url", "uri", "src", "path"});
			if (p.url.IsEmpty()) continue;
			p.title = JsonStringOrAlias(item, {"title", "name", "filename", "file_name"});
			p.filename = JsonStringOrAlias(item, {"filename", "file_name", "name", "title"});
			p.thumbnail = JsonStringOrAlias(item, {"thumbnail", "img"});
			getJsonValue(item, "timeline_hash", tmp); p.timelineHash = Utf8ToWide(tmp);
			if (const Value* itemTimeline = GetJsonObject(item, "timeline")) {
				getJsonValue(*itemTimeline, "hash", tmp); if (p.timelineHash.IsEmpty()) p.timelineHash = Utf8ToWide(tmp);
				p.position = GetNumberOrDefault(*itemTimeline, "time", -1.0);
				p.duration = GetNumberOrDefault(*itemTimeline, "duration", 0.0);
				p.timelinePercent = GetNumberOrDefault(*itemTimeline, "percent", 0.0);
			}
			p.position = GetNumberOrDefault(item, "position", p.position);
			p.season = GetIntOrDefault(item, "season", GetIntOrDefault(item, "season_number", GetIntOrDefault(item, "s", -1)));
			p.episode = GetIntOrDefault(item, "episode", GetIntOrDefault(item, "episode_number", GetIntOrDefault(item, "e", -1)));
			payload.playlist.emplace_back(std::move(p));
		}
	}
	payload.bridgeSessionId = Utf8ToWide(JsonValueOrAlias(d, "bridge_session_id", "ddd_sid"));
	payload.bridgeClient = Utf8ToWide(JsonValueOrAlias(d, "bridge_client", "ddd_client"));
	payload.bridgeMode = Utf8ToWide(JsonValueOrAlias(d, "bridge_mode", "ddd_mode"));
	payload.bridgeLocalToken = Utf8ToWide(JsonValueOrAlias(d, "bridge_local_token", "ddd_token"));
	if (const auto it = d.FindMember("bridge_emit_position"); it != d.MemberEnd() && it->value.IsBool()) payload.bridgeEmitPosition = it->value.GetBool();
	payload.bridgePositionIntervalMs = (int)GetNumberOrDefault(d, "bridge_position_interval_ms", 1000.0);
	payload.bridgeSchemaVersion = (int)GetNumberOrDefault(d, "bridge_schema_version", 1.0);
	if (payload.bridgePositionIntervalMs <= 0) payload.bridgePositionIntervalMs = 1000;
	if (payload.bridgeSchemaVersion <= 0) payload.bridgeSchemaVersion = 1;
	if (payload.url.IsEmpty() && !payload.playlist.empty()) payload.url = payload.playlist.front().url;
	if (payload.url.IsEmpty()) { error = L"url is empty"; return false; }
	return true;
}

CStringA LampaBridgeJson::EscapeJson(const CStringA& value)
{
	CStringA s = value;
	s.Replace("\\", "\\\\");
	s.Replace("\"", "\\\"");
	s.Replace("\r", "");
	s.Replace("\n", "\\n");
	return s;
}


CStringA LampaBridgeJson::EscapeJson(const CString& value)
{
	return EscapeJson(WideToUtf8(value));
}

CStringA LampaBridgeJson::BuildEnvelopeJson(int schema, const CStringA& type, const CStringA& client, const CStringA& sessionId, int64_t ts, const CStringA& payloadJson)
{
	CStringA out;
	out.Format("{\"schema\":%d,\"type\":\"%s\",\"client\":\"%s\",\"sessionId\":\"%s\",\"ts\":%lld,\"payload\":%s}", schema, EscapeJson(type).GetString(), EscapeJson(client).GetString(), EscapeJson(sessionId).GetString(), ts, payloadJson.IsEmpty()?"{}":payloadJson.GetString());
	return out;
}

CStringA LampaBridgeJson::BuildStateResponse(const LampaBridgeSession& session, int64_t positionMs, int64_t durationMs, bool active, const LampaBridgeEnvelope* state)
{
	if (!session.HasActive()) {
		return "{\"ok\":true,\"state\":null}";
	}

	const auto& cur = session.items[(size_t)session.activeIndex];
	CStringA lastEvent = state ? BuildEnvelopeJson(state->schema, state->type, state->client, state->sessionId, state->ts, state->payloadJson) : "null";
	CStringA out;

	out.Format(
		"{\"ok\":true,\"state\":{"
		"\"sessionId\":\"%s\","
		"\"active\":%s,"
		"\"position\":%lld,"
		"\"duration\":%lld,"
		"\"uri\":\"%s\","
		"\"title\":\"%s\","
		"\"windowIndex\":%d,"
		"\"playlistSize\":%d,"
		"\"currentItem\":{"
			"\"uri\":\"%s\","
			"\"url\":\"%s\","
			"\"title\":\"%s\","
			"\"filename\":\"%s\","
			"\"thumbnail\":\"%s\","
			"\"season\":%d,"
			"\"episode\":%d,"
			"\"timelineHash\":\"%s\""
		"},"
		"\"lastEvent\":%s"
		"}}",
		EscapeJson(session.sessionId).GetString(),
		active ? "true" : "false",
		positionMs,
		durationMs,
		EscapeJson(session.activeUrl).GetString(),
		EscapeJson(session.activeTitle).GetString(),
		session.activeIndex,
		(int)session.items.size(),
		EscapeJson(session.activeUrl).GetString(),
		EscapeJson(session.activeUrl).GetString(),
		EscapeJson(session.activeTitle).GetString(),
		EscapeJson(cur.filename).GetString(),
		EscapeJson(cur.thumbnail).GetString(),
		cur.season,
		cur.episode,
		EscapeJson(cur.timelineHash).GetString(),
		lastEvent.GetString());

	return out;
}

CStringA LampaBridgeJson::BuildEventsResponse(const std::deque<LampaBridgeEnvelope>& events, int limit)
{
	CStringA body = "{\"ok\":true,\"events\":[";
	int count = 0;
	int start = (int)events.size() - limit;
	if (start < 0) start = 0;
	for (int i = start; i < (int)events.size(); ++i) {
		if (count++) body += ",";
		const auto& e = events[(size_t)i];
		body += BuildEnvelopeJson(e.schema, e.type, e.client, e.sessionId, e.ts, e.payloadJson);
	}
	body += "]}";
	return body;
}
