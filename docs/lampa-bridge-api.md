# Lampa Bridge API (MVP)

Base URL: `http://127.0.0.1:13579`

## Endpoints
- `GET /lampa/health`
- `POST /lampa/open`
- `GET /lampa/status`
- `POST /lampa/command`
- `POST /lampa/close`
- `OPTIONS /lampa/*`

All endpoints return JSON and include CORS headers for localhost plugin calls.

## Before testing
1. Enable MPC-BE Web Server in settings.
2. Set port to 13579 or use the configured port in all requests.
3. Keep localhost-only mode enabled for safety.
4. Verify /lampa/health before testing /lampa/open.


## Open payload
See project task contract: root `url/title/position`, optional `timeline`, optional `playlist[]`.
If playlist is empty, bridge creates one-item playlist from root fields.

## Status response
Contains:
- opened/state/time/duration/percent
- active_index
- title/url/timeline_hash

## Command payload
`{ "command": "play|pause|stop|seek|next|prev" }`
For seek: `{ "command": "seek", "time": 120 }`

## PowerShell examples
```powershell
Invoke-RestMethod http://127.0.0.1:13579/lampa/health
```

```powershell
$body = @{ url='http://example.com/video.mp4'; title='Test'; position=0; timeline_hash='test_1'; playlist=@(@{title='Test';url='http://example.com/video.mp4';position=0;timeline_hash='test_1'}) } | ConvertTo-Json -Depth 10
Invoke-RestMethod -Uri http://127.0.0.1:13579/lampa/open -Method POST -ContentType 'application/json' -Body $body
```

```powershell
Invoke-RestMethod http://127.0.0.1:13579/lampa/status
```

```powershell
$seek = @{ command='seek'; time=120 } | ConvertTo-Json
Invoke-RestMethod -Uri http://127.0.0.1:13579/lampa/command -Method POST -ContentType 'application/json' -Body $seek
```

```powershell
Invoke-RestMethod -Uri http://127.0.0.1:13579/lampa/close -Method POST
```

## Future plan
- Lampa plugin persistence of timeline hash/time.
- Optional subtitles wiring and header/proxy enrichment.
