# velero v0.5 Release Notes

Source: [v0.5.1](https://github.com/vmware-tanzu/velero/releases/tag/v0.5.1)

A few bugs squeaked their way in to v0.5.0. This release hopefully squashes them!

Bugs fixed:
- Headless services were incorrectly restored as cluster-ip services (backport of #171, @nrb)
- Label selectors were not being honored when listing backups, schedules, and restores (backport of #169, @nrb)
- Restore namespace mappings were inadvertently broken (backport of #179, @skriss)
- Namespace objects were not always included in the backup (backport of #182, @dgoodwin and @ncdc)
- Namespace objects that should have been excluded were incorrectly included (backport of #182, @dgoodwin and @ncdc)

Binary checksums:
```
0a764974a633a640af86f3162711b7539b0e442232de76d13317364011b004ed  ark-darwin-amd64.tar.gz
78f7eea1a5886e4ca883b76a95dc7e8fe8df9b2f4504c2a146ac9cb26cc640a1  ark-linux-amd64.tar.gz
c2a9ff6910b687179125d6cdc1f36eaebcaca15cb3a2c6e6c158f7d36cd225b3  ark-linux-arm64.tar.gz
da6e73086742b94804bc62fcc3c84c3a0fbdd8596bedee6d214c9840a76a5b11  ark-linux-arm.tar.gz
15918c07bdafc8947ee8f690294fccd32108a2840a67e000c794aec63133c844  ark-windows-amd64.tar.gz
4b9aaac97f21f92c6916cb44d95b7b32476f0a5255f7d29e7670ff3e4ba4b95e  CHECKSUM
```
