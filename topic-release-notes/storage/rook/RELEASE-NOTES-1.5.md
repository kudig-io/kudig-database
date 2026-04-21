# rook v1.5 Release Notes

Source: [v1.5.12](https://github.com/rook/rook/releases/tag/v1.5.12)

# Improvements
Rook v1.5.12 is a patch release limited in scope and focusing on small feature additions and bug fixes.

## Ceph
- Fix OSD hostpath to prevent risk of data corruption on restart (#7886, @satoru-takeuchi)
- Double the mon failover timeout (to 20 minutes) during node drain (#7801, @sp98)
- Improve reliability of mon failover when the operator is restarted during failover (#7884, @travisn)
- Allow heap dump generation when logCollector sidecar is not running (#7847, @leseb)
