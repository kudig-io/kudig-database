# rook v1.11 Release Notes

Source: [v1.11.11](https://github.com/rook/rook/releases/tag/v1.11.11)

# Improvements
Rook v1.11.11 is a patch release limited in scope and focusing on feature additions and bug fixes to the Ceph operator.

- object: Unique username for OBC even when preceding OBC was retained (#12884, @haslersn)
- object: Avoid creating same bucket for two different OBC (#12804, @thotz)
- csi: Add csi pods to the list to force delete pod on an unavailable node (#12681, @Madhu-1)
- operator: Fix formatting of some logger methods (#12666, @polyedre)
