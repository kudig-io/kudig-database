# rook v1.10 Release Notes

Source: [v1.10.13](https://github.com/rook/rook/releases/tag/v1.10.13)

# Improvements
Rook v1.10.13 is a patch release limited in scope and focusing on feature additions and bug fixes to the Ceph operator.

- osd: Handle global or node-local device class configuration correctly (#11966, @satoru-takeuchi)
- manifest: Add missing quote (#11880, @DjVinnii)
- object: Make OBC genUserID unique across clusters (#11665, @BlaineEXE)
- file: Check if the filesystem exists before checking dependencies (#11221, @zhucan)
- core: On crash pod ensure rook version label is not set (#11760, @gaord)
