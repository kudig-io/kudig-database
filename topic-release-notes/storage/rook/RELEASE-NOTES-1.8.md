# rook v1.8 Release Notes

Source: [v1.8.10](https://github.com/rook/rook/releases/tag/v1.8.10)

# Improvements
Rook v1.8.10 is a patch release limited in scope and focusing on small feature additions and bug fixes to the Ceph operator.

- core: Improve detection of filesystem properties for disk in use (#10230, @leseb)
- osd: Remove broken argument for upgraded OSDs on PVCs in legacy lvm mode (#10298, @leseb)
- osd: Allow the osd to take two hours to start in case of ceph maintenance (#10250, @travisn)
- operator: Report telemetry 'rook/version' in mon store (#10161, @BlaineEXE)

