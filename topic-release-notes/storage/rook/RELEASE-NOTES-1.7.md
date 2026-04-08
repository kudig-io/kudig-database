# rook v1.7 Release Notes

Source: [v1.7.11](https://github.com/rook/rook/releases/tag/v1.7.11)

# Improvements
Rook v1.7.11 is a patch release limited in scope and focusing on small feature additions and bug fixes to the Ceph operator.

- mgr: Update services with the `app=rook-ceph-mgr` label when the active Ceph mgr changes (#9467, @travisn)
- osd: Correct bluestore compression min blob size for ssd (#9582, @subhamkrai)
- build: Update to go v1.16.12 (#9478, @BlaineEXE)