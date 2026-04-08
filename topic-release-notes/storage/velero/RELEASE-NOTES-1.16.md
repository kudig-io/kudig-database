# velero v1.16 Release Notes

Source: [v1.16.2](https://github.com/vmware-tanzu/velero/releases/tag/v1.16.2)

## v1.16.2

### Download
https://github.com/vmware-tanzu/velero/releases/tag/v1.16.2

### Container Image
`velero/velero:v1.16.2`

### Documentation
https://velero.io/docs/v1.16/

### Upgrading
https://velero.io/docs/v1.16/upgrade-to-1.16/

### All Changes
  * Update "Default Volumes to Fs Backup" to "File System Backup (Default)" (#9105, @shubham-pampattiwar)
  * Fix missing defaultVolumesToFsBackup flag output in Velero describe backup cmd (#9103, @shubham-pampattiwar)
  * Add imagePullSecrets inheritance for VGDP pod and maintenance job. (#9102, @blackpiglet)
  * Fix issue #9077, don't block backup deletion on list VS error (#9101, @Lyndon-Li)
  * Mounted cloud credentials should not be world-readable (#9094, @sseago)
  * Allow for proper tracking of multiple hooks per container (#9060, @sseago)
  * Add BSL status check for backup/restore operations. (#9010, @blackpiglet)
