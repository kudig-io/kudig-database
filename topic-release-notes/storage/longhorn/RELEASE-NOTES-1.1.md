# longhorn v1.1 Release Notes

Source: [v1.1.3](https://github.com/longhorn/longhorn/releases/tag/v1.1.3)

## Release Note

> Please read the install/upgrade notes before installing/upgrading to this Longhorn version.

**v1.1.3 released!** 🎆

This release introduces bug fixes as described below including security, stability, performance, and so on. Please try it and feedback. Thanks for all the contributions!

## Security Fixes for Vulnerabilities 

- [CVE-2021-36779: Host operations allowed in privileged Longhorn managed pods](https://github.com/longhorn/longhorn/issues/3419)
- [CVE-2021-36780: Unauthorized data access from replicas through vulnerable instance manager pods](https://github.com/longhorn/longhorn/issues/3420)

For more details, see each issue and [security advisories](https://github.com/longhorn/longhorn/security/advisories).

- [CVE-2021-36779](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2021-36779)
- [CVE-2021-36780](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2021-36780)

## Installation

Longhorn supports 3 installation ways including Rancher catalog, Kubectl, and Helm. Follow the installation instructions [here](https://longhorn.io/docs/1.1.3/deploy/install/).

## Upgrade

Follow the upgrade instructions [here](https://longhorn.io/docs/1.1.3/deploy/upgrade/).

## Deprecation & Incompatibilities

No deprecated or incompatible changes are introduced in this release.

## Highlights
  
  - [IMPROVEMENT] Enhance Longhorn data plane on low performance environment (spinning disk, 1GBps network, low CPU etc) ([2206](https://github.com/longhorn/longhorn/issues/2206)) - @keithalucas @khushboo-rancher
  
## Performance
  
  - [BUG] Volume attachment take long time, which may be caused by the replica controller queue being flooded by the backing image events ([3242](https://github.com/longhorn/longhorn/issues/3242)) - @kaxing @shuo-wu
  
## Bugs
  
  - [BUG] Validate S3 input to make sure they don't contain trailing newline or space ([811](https://github.com/longhorn/longhorn/issues/811)) - @kaxing @jenting
  - [BUG] Investigate engine integration test failure - restore_with_frontend: can only restore backup from replica in mode RW, got ERR ([1628](https://github.com/longhorn/longhorn/issues/1628)) - @keithalucas
  - [BUG] High CPU utilization for Longhorn manager sometimes due to exhaustion of all the available sockets because of socket leaking log stream ([2778](https://github.com/longhorn/longhorn/issues/2778)) - @meldafrawi @PhanLe1010
  - [BUG] longhorn-engine image no longer contains longhorn-instance manager ([2796](https://github.com/longhorn/longhorn/issues/2796)) - @innobead @khushboo-rancher
  - [BUG] allow volume migration when volume is degraded (harvester vm) ([2805](https://github.com/longhorn/longhorn/issues/2805)) - @shuo-wu @khushboo-rancher
  - [BUG] Longhorn-csi-plugin pods restart because the Longhorn client 10 secs timeout ([2816](https://github.com/longhorn/longhorn/issues/2816)) - @jenting
  - [BUG] High CPU utilization for Longhorn manager and replica instance manager sometimes due to overwhelming of number of backupstatus in engine ([2818](https://github.com/longhorn/longhorn/issues/2818)) - @joshimoo @khushboo-rancher
  - [BUG] fix instance manager grpc connection leak (VersionGet, ProcessLog, ProcessWatch) ([2824](https://github.com/longhorn/longhorn/issues/2824)) - @joshimoo
  - [BUG] Datastore::GetEngine returns raw cached object which is then modified ([2827](https://github.com/longhorn/longhorn/issues/2827)) - @joshimoo @khushboo-rancher
  - [BUG] Node deletion leads volume to get stuck in attaching state ([2848](https://github.com/longhorn/longhorn/issues/2848)) - @joshimoo @PhanLe1010 @khushboo-rancher
  - [BUG] Replica rebuilding fails and eventually pass after few attempts ([2849](https://github.com/longhorn/longhorn/issues/2849)) - @PhanLe1010 @khushboo-rancher
  - [BUG] Replica rebuilding gets triggered if network bandwidth is restricted below 80mbit ([2882](https://github.com/longhorn/longhorn/issues/2882)) - @keithalucas
  - [BUG] Instance managers keep terminating and getting created while uninstallation is in progress ([2919](https://github.com/longhorn/longhorn/issues/2919)) - @meldafrawi @shuo-wu @khushboo-rancher
  - [BUG] DR volume does not continue to restore after node reboot ([2920](https://github.com/longhorn/longhorn/issues/2920)) - @c3y1huang @shuo-wu
  - [BUG] Power off node during backup volume restore does not continue restore ([2929](https://github.com/longhorn/longhorn/issues/2929)) - @kaxing @c3y1huang @shuo-wu @khushboo-rancher
  - [BUG] Over-provisioning doesn't work properly with values smaller than 200% ([2952](https://github.com/longhorn/longhorn/issues/2952)) - @meldafrawi @PhanLe1010
  - [BUG] Loghorn volumes can be resized to any size and does not respect the over-provisioning limit ([2962](https://github.com/longhorn/longhorn/issues/2962)) - @meldafrawi @PhanLe1010
  - [BUG] Tool tip on Snapshots and Backups List on UI are displayed inconsistently ([2994](https://github.com/longhorn/longhorn/issues/2994)) - @smallteeths @chanow816
  - [BUG] Disk eviction not doing anything ([2995](https://github.com/longhorn/longhorn/issues/2995)) - @kaxing @shuo-wu
  - [BUG] Fail to live upgrade to v1.2.x or v1.1.3 ([3052](https://github.com/longhorn/longhorn/issues/3052)) - @shuo-wu @khushboo-rancher
  - [BUG] Update the default advertised CSI version to csi version 1.2 ([3079](https://github.com/longhorn/longhorn/issues/3079)) - @PhanLe1010 @khushboo-rancher
  - [BUG] fsfreeze race condition ([3125](https://github.com/longhorn/longhorn/issues/3125)) - @joshimoo @khushboo-rancher
  - [BUG] longhorn-ui has a warning message in the browser's console ([3230](https://github.com/longhorn/longhorn/issues/3230)) - @smallteeths @khushboo-rancher
  - [BUG] Instance process is still running when the corresponding engine/replica CR is gone ([3255](https://github.com/longhorn/longhorn/issues/3255)) - @shuo-wu
  - [BUG] can't reset Backup Target Credential Secret  ([3261](https://github.com/longhorn/longhorn/issues/3261)) - @meldafrawi @jenting
  - [BUG] Revision Counter is false by default? ([3308](https://github.com/longhorn/longhorn/issues/3308)) - @smallteeths @khushboo-rancher
  - [BUG] Data loss on doing K3s upgrade with drain ([3350](https://github.com/longhorn/longhorn/issues/3350)) - @PhanLe1010
  - [BUG] [v1.1.3] Longhorn upgrade from v1.1.2 to v1.1.3-rc1 failed ([3297](https://github.com/longhorn/longhorn/issues/3297)) - @PhanLe1010 @khushboo-rancher
  - [BUG] V1 Chart install V1.1.3-rc2 fail ([3356](https://github.com/longhorn/longhorn/issues/3356)) - @PhanLe1010 @chanow816
  - [BUG] Using a backing image created from a YAML file will lead to NPE ([3379](https://github.com/longhorn/longhorn/issues/3379)) - @kaxing @shuo-wu
  
## Misc
  
  - [UI] Notify users about newer stable versions ([3032](https://github.com/longhorn/longhorn/issues/3032)) - @smallteeths @chanow816
  - [IMPROVEMENT] Longhorn v1.1.3 and maximum supported Kubernetes version ([3319](https://github.com/longhorn/longhorn/issues/3319)) - @kaxing @jenting
  
## Contributors

- @PhanLe1010
- @c3y1huang
- @chanow816
- @innobead
- @jenting
- @joshimoo
- @kaxing
- @keithalucas
- @khushboo-rancher
- @meldafrawi
- @shuo-wu
- @smallteeths
