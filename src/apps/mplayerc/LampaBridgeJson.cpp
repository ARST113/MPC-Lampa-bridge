#include "stdafx.h"
#include "LampaBridgeJson.h"

using namespace rapidjson;



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

bool LampaBridgeJson::ParseOpenPayload(const CStringA& body, LampaOpenPayload& payload, CString& error)
{
	Document d;
	if (d.Parse(body).HasParseError() || !d.IsObject()) {
		error = L"invalid json";
		return false;
	}
	CStringA tmp;
	getJsonValue(d, "url", tmp); payload.url = UTF8To16(tmp);
	getJsonValue(d, "title", tmp); payload.title = UTF8To16(tmp);
	payload.position = GetNumberOrDefault(d, "position", -1.0);
	getJsonValue(d, "timeline_hash", tmp); payload.timelineHash = UTF8To16(tmp);
	if (const Value* timeline = GetJsonObject(d, "timeline")) {
		getJsonValue(*timeline, "hash", tmp); if (payload.timelineHash.IsEmpty()) payload.timelineHash = UTF8To16(tmp);
		payload.timelineTime = GetNumberOrDefault(*timeline, "time", -1.0);
		payload.timelineDuration = GetNumberOrDefault(*timeline, "duration", 0.0);
		payload.timelinePercent = GetNumberOrDefault(*timeline, "percent", 0.0);
	}
	if (const Value* playlist = GetJsonArray(d, "playlist")) {
		for (const auto& item : playlist->GetArray()) {
			if (!item.IsObject()) continue;
			LampaBridgePlaylistItem p;
			getJsonValue(item, "url", tmp); p.url = UTF8To16(tmp);
			if (p.url.IsEmpty()) continue;
			getJsonValue(item, "title", tmp); p.title = UTF8To16(tmp);
			getJsonValue(item, "timeline_hash", tmp); p.timelineHash = UTF8To16(tmp);
			p.position = GetNumberOrDefault(item, "position", -1.0);
			payload.playlist.emplace_back(std::move(p));
		}
	}
	payload.bridgeSessionId = UTF8To16(JsonValueOrAlias(d, "bridge_session_id", "ddd_sid"));
	payload.bridgeClient = UTF8To16(JsonValueOrAlias(d, "bridge_client", "ddd_client"));
	payload.bridgeMode = UTF8To16(JsonValueOrAlias(d, "bridge_mode", "ddd_mode"));
	payload.bridgeLocalToken = UTF8To16(JsonValueOrAlias(d, "bridge_local_token", "ddd_token"));
	if (const auto it = d.FindMember("bridge_emit_position"); it != d.MemberEnd() && it->value.IsBool()) payload.bridgeEmitPosition = it->value.GetBool();
	payload.bridgePositionIntervalMs = (int)GetNumberOrDefault(d, "bridge_position_interval_ms", 1000.0);
	payload.bridgeSchemaVersion = (int)GetNumberOrDefault(d, "bridge_schema_version", 1.0);
	if (payload.bridgePositionIntervalMs <= 0) payload.bridgePositionIntervalMs = 1000;
	if (payload.bridgeSchemaVersion <= 0) payload.bridgeSchemaVersion = 1;
	if (payload.url.IsEmpty()) { error = L"url is empty"; return false; }
	return true;
}

CStringA LampaBridgeJson::EscapeJson(const CString& value)
{
	CStringA s = UTF8(value);
	s.Replace("\\", "\\\\");
	s.Replace("\"", "\\\"");
	s.Replace("\r", "");
	s.Replace("\n", "\\n");
	return s;
}


CStringA LampaBridgeJson::BuildEnvelopeJson(int schema, const CStringA& type, const CStringA& client, const CStringA& sessionId, int64_t ts, const CStringA& payloadJson)
{
	CStringA out;
	out.Format("{\"schema\":%d,\"type\":\"%s\",\"client\":\"%s\",\"sessionId\":\"%s\",\"ts\":%lld,\"payload\":%s}", schema, type.GetString(), client.GetString(), sessionId.GetString(), ts, payloadJson.IsEmpty()?"{}":payloadJson.GetString());
	return out;
}

CStringA LampaBridgeJson::BuildStateResponse(const LampaBridgeEnvelope* state)
{
	if (!state) return "{\"ok\":true,\"state\":null}";
	return "{\"ok\":true,\"state\":" + BuildEnvelopeJson(state->schema, state->type, state->client, state->sessionId, state->ts, state->payloadJson) + "}";
}

CStringA LampaBridgeJson::BuildEventsResponse(const std::deque<LampaBridgeEnvelope>& events, int limit)
{
	CStringA body = "{\"ok\":true,\"events\":[";
	int count = 0;
	for (auto it = events.rbegin(); it != events.rend() && count < limit; ++it, ++count) {
		if (count) body += ",";
		body += BuildEnvelopeJson(it->schema, it->type, it->client, it->sessionId, it->ts, it->payloadJson);
	}
	body += "]}";
	return body;
}
