# prometheus v2.2 Release Notes

Source: [v2.2.1](https://github.com/prometheus/prometheus/releases/tag/v2.2.1)

* [BUGFIX] Fix data loss in TSDB on compaction
* [BUGFIX] Correctly stop timer in remote-write path
* [BUGFIX] Fix deadlock triggered by loading targets page
* [BUGFIX] Fix incorrect buffering of samples on range selection queries
* [BUGFIX] Handle large index files on windows properly