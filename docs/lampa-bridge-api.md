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


## Manual acceptance test: playlist open/select
```powershell
$playlist = @(
  @{ title='Episode 1'; url='http://127.0.0.1:8090/stream/A?x=1&preload#ddd_i=1'; filename='ep1.mkv'; season=1; episode=1 },
  @{ title='Episode 2'; url='http://127.0.0.1:8090/stream/B?x=1&play'; filename='ep2.mkv'; season=1; episode=2 },
  @{ title='Episode 3'; url='http://127.0.0.1:8090/stream/C?x=1&play'; filename='ep3.mkv'; season=1; episode=3 }
)
$open = @{
  url = $playlist[1].url
  title = $playlist[1].title
  playlist_index = 1
  playlist = $playlist
} | ConvertTo-Json -Depth 10

$r = Invoke-RestMethod -Uri http://127.0.0.1:13579/lampa/open -Method POST -ContentType 'application/json' -Body $open
$r
Invoke-RestMethod http://127.0.0.1:13579/lampa/status
Invoke-RestMethod "http://127.0.0.1:13579/state?sid=$($r.session_id)"

# Expected:
# - open.active_index = 1 and open.playlist_size = 3
# - state.lastEvent.payload.windowIndex = 1 and playlistSize = 3
# - visible MPC-BE playlist has all 3 items and starts from selected item
# - MPC-BE UI Next/Prev changes playback and bridge windowIndex
```


## Port behavior
- Bridge runs on MPC-BE Web Server configured port.
- Default in these docs: `13579`.
- Lampa plugin should allow configurable port or try `39677` then fallback to `13579`.

## Additional acceptance tests
- Test B: `POST /lampa/command {"command":"next"}` from item 2 should switch to item 3 (`windowIndex=2`).
- Test C: `POST /lampa/command {"command":"prev"}` from item 3 should switch to item 2 (`windowIndex=1`).
- Test D: open with `playlist_index=2` and `position>10`; verify seek applies only after item 3 becomes active.
