# rook v1.15 Release Notes

Source: [v1.15.9](https://github.com/rook/rook/releases/tag/v1.15.9)

# Improvements
Rook v1.15.9 is a patch release limited in scope and focusing on feature additions and bug fixes to the Ceph operator.

- ci: Push rook image to repositories quay.io and ghcr.io (#15274, @subhamkrai)
- security: bump go-jose package from 4.0.4 to 4.0.5 (#15456, @dependabot)
- mds: Correct parameters to mds liveness probe (#15424, @parth-gr)
- core: Query env vars instead of polling the operator settings configmap (#15462, @travisn)
- helm: Allow configurable namespace in the cephECBlockPool storage class (#15402, @KarolGongola)
- nfs: Formatting fix for nfs-ganesha config parser (#15393, @BlaineEXE)
