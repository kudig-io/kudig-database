# rook v1.0 Release Notes

Source: [v1.0.6](https://github.com/rook/rook/releases/tag/v1.0.6)

Rook v1.0.6 is a patch release limited in scope and focusing on bug fixes.

# Improvements

## Ceph
- Set public-addr flag for MGR (#3136, @galexrt)
- Remove the 20GB default for OSD db size and allow ceph-volume to use all available space (#3448, @travisn)
- Correctly set osd mem target for init-ed clusters (#3638, @odinuge)  
- Properly propagate errors when deleting mds deployment (#3641, @odinuge) 
