# velero v1.12 Release Notes

Source: [v1.12.4](https://github.com/vmware-tanzu/velero/releases/tag/v1.12.4)

## v1.12.4
### 2024-02-28

### Download
https://github.com/vmware-tanzu/velero/releases/tag/v1.12.4

### Container Image
`velero/velero:v1.12.4`

### Documentation
https://velero.io/docs/v1.12/

### Upgrading
https://velero.io/docs/v1.12/upgrade-to-1.12/

### All changes
* BackupRepositories associated with a BSL are invalidated when BSL is (re-)created. (#7397, @kaovilai)
* Check resource Group Version and Kind is available in cluster before attempting restore to prevent being stuck. (#7337, @kaovilai)
* Make "disable-informer-cache" option false(enabled) by default to keep it consistent with the help message (#7298, @ywk253100)
* Add description markers for dataupload and datadownload CRDs (#7042, @shubham-pampattiwar)
