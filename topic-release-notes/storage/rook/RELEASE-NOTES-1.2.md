# rook v1.2 Release Notes

Source: [v1.2.7](https://github.com/rook/rook/releases/tag/v1.2.7)

# Improvements

Rook v1.2.7 is a patch release limited in scope and focusing on bug fixes.

## Ceph

- Apply the expected lower PG count for rgw metadata pools (#5091, @travisn)
- Reject devices smaller than 5GiB for OSDs (#5089, @leseb)
- Add extra check for filesystem to skip boot volumes for OSD configuration (#5022, @leseb)
- Avoid duplication of mon pod anti-affinity (#4998, @travisn)
- Update service monitor definition during upgrade (#5078, @umangachapagain)
- Resizer container fix due to misinterpretation of the cephcsi version (#5073, @Madhu-1)
- Set ResourceVersion for Prometheus rules (#4528, @galexrt)
- Upgrade doc clarification for RBAC related to the helm chart (#5054, @PCatinean)