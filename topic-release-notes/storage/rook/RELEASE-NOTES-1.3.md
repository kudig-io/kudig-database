# rook v1.3 Release Notes

Source: [v1.3.11](https://github.com/rook/rook/releases/tag/v1.3.11)

# Improvements

Rook v1.3.11 is a patch release limited in scope to a single bug fix.

## Ceph
- The Ceph-CSI driver was being unexpectedly removed by the garbage collector in some clusters. For more details to apply a fix during the upgrade to this patch release, see [these steps](https://github.com/rook/rook/issues/6162#issuecomment-691273679). (#6162, @Madhu-1)