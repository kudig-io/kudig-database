# prometheus v0.11 Release Notes

Source: [0.11.1](https://github.com/prometheus/prometheus/releases/tag/0.11.1)

Going up to eleven was not enough, so here you'll get v0.11.1 of prometheus/prometheus. This is a bug fix release with a couple of minor fixes and one very critical fix. Upgrading is strongly recommended.
- [BUGFIX] Make series maintenance complete again. (Ever since 0.9.0rc4,
  or commit 0851945, series would not be archived, chunk descriptors would
  not be evicted, and stale head chunks would never be closed. This happened
  due to accidental deletion of a line calling a (well tested :) function.
- [BUGFIX] Do not double count head chunks read from checkpoint on startup.
  Also fix a related but less severe bug in counting chunk descriptors.
- [BUGFIX] Check last time in head chunk for head chunk timeout, not first.
- [CHANGE] Update vendoring due to vendoring changes in client_golang.
- [CLEANUP] Code cleanups.
- [ENHANCEMENT] Limit the number of 'dirty' series counted during checkpointing.

NOTE: This is NOT yet the upcoming release that will change the fingerprinting algorithm.
