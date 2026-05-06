#include "stdafx.h"
#include "LampaBridgeJson.h"

using namespace rapidjson;

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
