# rook v0.9 Release Notes

Source: [v0.9.3](https://github.com/rook/rook/releases/tag/v0.9.3)

Rook v0.9.3 is a patch release limited in scope and focusing on bug fixes.

## Improvements

### Cassandra
- Fix the mount point for the PVs (#2443, @yanniszark)

### Ceph
- Improve mon failover cleanup and operator restart during failover (#2262 #2570, @travisn)
- Enable host ipc for osd encryption (#923, @noahdesu)  
- Add missing "host path requires privileged" setting to the helm chart (#2735, @galexrt)