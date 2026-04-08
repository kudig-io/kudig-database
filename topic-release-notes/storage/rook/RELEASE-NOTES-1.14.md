# rook v1.14 Release Notes

Source: [v1.14.12](https://github.com/rook/rook/releases/tag/v1.14.12)

# Improvements
Rook v1.14.12 is a patch release limited in scope and focusing on feature additions and bug fixes to the Ceph operator.

- object: Also use system certs for validating RGW cert (#14835, @BlaineEXE)
- osd: mount /run/udev in the init container for ceph-volume activate (#14901, @guits)
- core: Define empty securityContext for pods to fix CIS 5.7.3 (#14823, @prazumovsky)
- csi: Disable fencing in Rook (#14831, @Madhu-1)
- mds: Fix liveness probe timeout (#14798, @BlaineEXE)
