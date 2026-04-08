# falco v0.9 Release Notes

Source: [0.9.0](https://github.com/falcosecurity/falco/releases/tag/0.9.0)

Released 2018-01-18

### Bug Fixes

* Fix driver incompatibility problems with some linux kernel versions that can disable pagefault tracepoints [[#sysdig/1034](https://github.com/draios/sysdig/pull/1034)]
* Fix OSX Build incompatibility with latest version of libcurl [[#291](https://github.com/draios/falco/pull/291)]

### Minor Changes

* Updated the Kubernetes example to provide an additional example: Daemon Set using RBAC and a ConfigMap for configuration. Also expanded the documentation for both the RBAC and non-RBAC examples. [[#309](https://github.com/draios/falco/pull/309)]

### Rule Changes

* Refactor the shell-related rules to reduce false positives. These changes significantly decrease the scope of the rules so they trigger only for shells spawned below specific processes instead of anywhere. [[#301](https://github.com/draios/falco/pull/301)] [[#304](https://github.com/draios/falco/pull/304)]
* Lots of rule changes based on feedback from Sysdig Secure community [[#293](https://github.com/draios/falco/pull/293)] [[#298](https://github.com/draios/falco/pull/298)] [[#300](https://github.com/draios/falco/pull/300)] [[#307](https://github.com/draios/falco/pull/307)] [[#315](https://github.com/draios/falco/pull/315)]