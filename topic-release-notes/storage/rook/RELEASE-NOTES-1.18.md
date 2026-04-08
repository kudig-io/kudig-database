# rook v1.18 Release Notes

Source: [v1.18.10](https://github.com/rook/rook/releases/tag/v1.18.10)

# Improvements
Rook v1.18.10 is a patch release limited in scope and focusing on feature additions and bug fixes to the Ceph operator.

- exporter: Delete orphaned ceph-exporter deployments on reconcile (#17165, @adilGhaffarDev)
- exporter: Add log collector for ceph exporter pod (#16584, @subhamkrai)
- rbac: Remove nodes/proxy rbac grants (#16979, @ibotty)
- osd: Update lockbox key rotation for encrypted OSDs (#17112, @BlaineEXE)
- osd: In cephx key init, don't overwrite key on failure (#17052, @BlaineEXE)
- osd: Find correct osd container in case it is not index 0 (#16969, @kyrbrbik)
- osd: Fix updateExistingOSDs function for cancelled context (#17022, @sp98)
- nfs: Add CephNFS.spec.server.{image,imagePullPolicy} fields (#16982, @jhoblitt)
