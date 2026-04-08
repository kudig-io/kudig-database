# prometheus v2.3 Release Notes

Source: [v2.3.2](https://github.com/prometheus/prometheus/releases/tag/v2.3.2)

* [BUGFIX] Fix various tsdb bugs #4369
* [BUGFIX] Reorder startup and shutdown to prevent panics. #4321
* [BUGFIX] Exit with non-zero code on error #4296
* [BUGFIX] discovery/kubernetes/ingress: fix scheme discovery #4329
* [BUGFIX] Fix race in zookeeper sd #4355
* [BUGFIX] Better timeout handling in promql #4291 #4300
* [BUGFIX] Propogate errors when selecting series from the tsdb #4136