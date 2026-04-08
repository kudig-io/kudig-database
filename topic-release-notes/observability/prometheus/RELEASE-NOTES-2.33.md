# prometheus v2.33 Release Notes

Source: [v2.33.5](https://github.com/prometheus/prometheus/releases/tag/v2.33.5)

The binaries published with this release are built with Go1.17.8 to avoid [CVE-2022-24921](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2022-24921).

* [BUGFIX] Remote-write: Fix deadlock between adding to queue and getting batch. #10395
