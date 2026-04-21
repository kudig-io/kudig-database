# prometheus v2.17 Release Notes

Source: [v2.17.2](https://github.com/prometheus/prometheus/releases/tag/v2.17.2)

* [BUGFIX] Federation: Register federation metrics #7081
* [BUGFIX] PromQL: Fix panic in parser error handling #7132
* [BUGFIX] Rules: Fix reloads hanging when deleting a rule group that is being evaluated #7138
* [BUGFIX] TSDB: Fix a memory leak when prometheus starts with an empty TSDB WAL #7135
* [BUGFIX] TSDB: Make isolation more robust to panics in web handlers #7129 #7136
