# prometheus v2.48 Release Notes

Source: [v2.48.1](https://github.com/prometheus/prometheus/releases/tag/v2.48.1)

* [BUGFIX] TSDB: Make the wlog watcher read segments synchronously when not tailing. #13224
* [BUGFIX] Agent: Participate in notify calls (fixes slow down in remote write handling introduced in 2.45). #13223
