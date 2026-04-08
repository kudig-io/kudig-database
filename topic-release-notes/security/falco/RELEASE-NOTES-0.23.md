# falco v0.23 Release Notes

Source: [0.23.0](https://github.com/falcosecurity/falco/releases/tag/0.23.0)

Released on 2020-18-05

### Major Changes

* BREAKING CHANGE: the falco-driver-loader script now references `falco-probe.o` and `falco-probe.ko` as `falco.o` and `falco.ko` [[#1158](https://github.com/falcosecurity/falco/pull/1158)]
* BREAKING CHANGE: the `falco-driver-loader` script environment variable to use a custom repository to download drivers now uses the `DRIVERS_REPO` environment variable instead of `DRIVER_LOOKUP_URL`. This variable must contain the parent URI containing the following directory structure `/$driver_version$/falco_$target$_$kernelrelease$_$kernelversion$.[ko|o]`. e.g: [[#1160](https://github.com/falcosecurity/falco/pull/1160)]
* new(scripts): options and command-line usage for `falco-driver-loader` [[#1200](https://github.com/falcosecurity/falco/pull/1200)]
* new: ability to specify exact matches when adding rules to Falco engine (only API) [[#1185](https://github.com/falcosecurity/falco/pull/1185)]
* new(docker): add an image that wraps the `falco-driver-loader` with the toolchain [[#1192](https://github.com/falcosecurity/falco/pull/1192)]
* new(docker): add `falcosecurity/falco-no-driver` image [[#1205](https://github.com/falcosecurity/falco/pull/1205)]


### Minor Changes

* update(scripts): improve `falco-driver-loader` output messages [[#1200](https://github.com/falcosecurity/falco/pull/1200)]
* update: containers look for prebuilt drivers on the Drivers Build Grid [[#1158](https://github.com/falcosecurity/falco/pull/1158)]
* update: driver version bump to 96bd9bc560f67742738eb7255aeb4d03046b8045 [[#1190](https://github.com/falcosecurity/falco/pull/1190)]
* update(docker): now `falcosecurity/falco:slim-*` alias to `falcosecurity/falco-no-driver:*` [[#1205](https://github.com/falcosecurity/falco/pull/1205)]
* docs: instructions to run unit tests [[#1199](https://github.com/falcosecurity/falco/pull/1199)]
* docs(examples): move `/examples` to `contrib` repo [[#1191](https://github.com/falcosecurity/falco/pull/1191)]
* update(docker): remove `minimal` image [[#1196](https://github.com/falcosecurity/falco/pull/1196)]
* update(integration): move `/integrations` to `contrib` repo [[#1157](https://github.com/falcosecurity/falco/pull/1157)]
* https://dl.bintray.com/driver/$driver_version$/falco_$target$_$kernelrelease$_$kernelversion$.[ko|o]` [[#1160](https://github.com/falcosecurity/falco/pull/1160)]
* update(docker/event-generator): remove the event-generator from Falco repository [[#1156](https://github.com/falcosecurity/falco/pull/1156)]
* docs(examples): set audit level to metadata for object secrets [[#1153](https://github.com/falcosecurity/falco/pull/1153)]


### Bug Fixes

* fix(scripts): upstream files (prebuilt drivers) for the generic Ubuntu kernel contains "ubuntu-generic" [[#1212](https://github.com/falcosecurity/falco/pull/1212)]
* fix: support Falco driver on Linux kernels 5.6.y [[#1174](https://github.com/falcosecurity/falco/pull/1174)]


### Rule Changes

* rule(Redirect STDOUT/STDIN to Network Connection in Container): correct rule name as per rules naming convention [[#1164](https://github.com/falcosecurity/falco/pull/1164)]
* rule(Redirect STDOUT/STDIN to Network Connection in Container): new rule to detect Redirect stdout/stdin to network connection in container [[#1152](https://github.com/falcosecurity/falco/pull/1152)]
* rule(K8s Secret Created): new rule to track the creation of Kubernetes secrets (excluding kube-system and service account secrets) [[#1151](https://github.com/falcosecurity/falco/pull/1151)]
* rule(K8s Secret Deleted): new rule to track the deletion of Kubernetes secrets (excluding kube-system and service account secrets) [[#1151](https://github.com/falcosecurity/falco/pull/1151)]

### Statistics

| Merged PRs        | Number  |
|-------------------|---------|
| Not user-facing   | 17       |
| Release note      | 18       |
| Total             | 35       |