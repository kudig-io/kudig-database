# rook v0.8 Release Notes

Source: [v0.8.3](https://github.com/rook/rook/releases/tag/v0.8.3)

Rook v0.8.3 is a patch release limited in scope and focusing on bug fixes.

## Improvements
- OSD can now be configured in K8s clusters where the [hostname label is different from the node name](https://github.com/rook/rook/issues/2148)  (@travisn)
- Fix regression in v0.8.2 that caused [PVCs to fail](https://github.com/rook/rook/issues/2149) to be mounted in some clusters due to unexpected logging (@rootfs)