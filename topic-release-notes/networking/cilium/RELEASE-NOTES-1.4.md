# cilium v1.4 Release Notes

Source: [v1.4.10](https://github.com/cilium/cilium/releases/tag/v1.4.10)

Summary of Changes
------------------

**Important Bug Fixes**

* Envoy is updated to release 1.12.2, including important security fixes (#9742, @jrajahalme)
  * Fixes CVE-2019-18801, CVE-1019-18802, CVE-1019-18838
  * For more information, see [Envoy 1.12.2 Release Notes](https://groups.google.com/forum/#!topic/envoy-announce/BjgUTDTKAu8)

**Misc**

* bugtool: add cilium node list output (#9474, @ianvernon)


Changes
-------

```
   Ian Vernon (1):
         bugtool: add `cilium node list` output

   Jarno Rajahalme (8):
         Envoy: Do not configure policy name
         envoy: Update to the latest API
         Dockerfile: Use latest Envoy image
         envoy: Update image for Envoy CVEs 2019-10-08
         envoy: Update to release 1.12 with Cilium TLS support
         envoy: Update to release 1.12.1
         Dockerfile: Use Envoy image that always resumes NPDS
         envoy: Update to 1.12.2
```
```