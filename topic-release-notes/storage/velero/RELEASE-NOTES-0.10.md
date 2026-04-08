# velero v0.10 Release Notes

Source: [v0.10.2](https://github.com/vmware-tanzu/velero/releases/tag/v0.10.2)

### Changes
  * upgrade restic to v0.9.4 & replace --hostname flag with --host (#1156, @skriss)
  * use 'restic stats' instead of 'restic check' to determine if repo exists (#1171, @skriss)
  * Fix concurrency bug in code ensuring restic repository exists (#1235, @skriss)