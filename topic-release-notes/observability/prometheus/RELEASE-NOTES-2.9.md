# prometheus v2.9 Release Notes

Source: [v2.9.2](https://github.com/prometheus/prometheus/releases/tag/v2.9.2)

* [BUGFIX] Make sure subquery range is taken into account for selection #5467
* [BUGFIX] Exhaust every request body before closing it #5166
* [BUGFIX] Cmd/promtool: return errors from rule evaluations #5483
* [BUGFIX] Remote Storage: string interner should not panic in release #5487
* [BUGFIX] Fix memory allocation regression in mergedPostings.Seek tsdb#586