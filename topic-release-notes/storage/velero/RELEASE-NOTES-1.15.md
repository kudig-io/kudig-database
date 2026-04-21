# velero v1.15 Release Notes

Source: [v1.15.2](https://github.com/vmware-tanzu/velero/releases/tag/v1.15.2)

## v1.15.2

### Download
https://github.com/vmware-tanzu/velero/releases/tag/v1.15.2

### Container Image
`velero/velero:v1.15.2`

### Documentation
https://velero.io/docs/v1.15/

### Upgrading
https://velero.io/docs/v1.15/upgrade-to-1.15/

### All Changes
* fix(pkg/repository/maintenance): don't panic when there's no container statuses (#8568, @mcluseau)
* Don't include excluded items in ItemBlocks (#8585, @kaovilai)
* Check the PVB status via podvolume Backupper rather than calling API server to avoid API server issue (#8596, @ywk253100)
