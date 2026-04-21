# prometheus v1.7 Release Notes

Source: [v1.7.2](https://github.com/prometheus/prometheus/releases/tag/v1.7.2)

* [BUGFIX] Correctly remove all targets from DNS service discovery if the
  corresponding DNS query succeeds and returns an empty result.
* [BUGFIX] Correctly parse resolution input in expression browser.
* [BUGFIX] Consistently use UTC in the date picker of the expression browser.
* [BUGFIX] Correctly handle multiple ports in Marathon service discovery.
* [BUGFIX] Fix HTML escaping so that HTML templates compile with Go1.9.
* [BUGFIX] Prevent number of remote write shards from going negative.
* [BUGFIX] In the graphs created by the expression browser, render very large
  and small numbers in a readable way.
* [BUGFIX] Fix a rarely occurring iterator issue in varbit encoded chunks.
