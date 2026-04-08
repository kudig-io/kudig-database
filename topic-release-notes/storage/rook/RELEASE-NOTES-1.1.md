# rook v1.1 Release Notes

Source: [v1.1.9](https://github.com/rook/rook/releases/tag/v1.1.9)

# Improvements

Rook v1.1.9 is a patch release limited in scope and focusing on bug fixes.

## Ceph
- CSI driver handling of upgrade from OCP 4.2 to OCP 4.3 (#4650, @Madhu-1)
- Fix object bucket provisioner when rgw not on port 80 (#4049, @bsperduto)
- Only perform upgrade checks when the Ceph image changes (#4379, @travisn)
