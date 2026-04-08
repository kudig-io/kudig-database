# falco v0.43 Release Notes

Source: [0.43.0](https://github.com/falcosecurity/falco/releases/tag/0.43.0)

[![LIBS](https://img.shields.io/badge/LIBS-0.23.1-yellow)](https://github.com/falcosecurity/libs/releases/tag/0.23.1)
[![DRIVER](https://img.shields.io/badge/DRIVER-9.1.0+driver-yellow)](https://github.com/falcosecurity/libs/releases/tag/9.1.0+driver)

| Packages | Download                                                                                                                                               |
| -------- | ------------------------------------------------------------------------------------------------------------------------------------------------------ |
| rpm-x86_64      | [![rpm](https://img.shields.io/badge/Falco-0.43.0-%2300aec7?style=flat-square)](https://download.falco.org/packages/rpm/falco-0.43.0-x86_64.rpm)        |
| deb-x86_64      | [![deb](https://img.shields.io/badge/Falco-0.43.0-%2300aec7?style=flat-square)](https://download.falco.org/packages/deb/stable/falco-0.43.0-x86_64.deb) |
| tgz-x86_64      | [![tgz](https://img.shields.io/badge/Falco-0.43.0-%2300aec7?style=flat-square)](https://download.falco.org/packages/bin/x86_64/falco-0.43.0-x86_64.tar.gz) |
| tgz-static-x86_64      | [![tgz-static](https://img.shields.io/badge/Falco-0.43.0-%2300aec7?style=flat-square)](https://download.falco.org/packages/bin/x86_64/falco-0.43.0-static-x86_64.tar.gz) |
| rpm-aarch64      | [![rpm](https://img.shields.io/badge/Falco-0.43.0-%2300aec7?style=flat-square)](https://download.falco.org/packages/rpm/falco-0.43.0-aarch64.rpm)        |
| deb-aarch64      | [![deb](https://img.shields.io/badge/Falco-0.43.0-%2300aec7?style=flat-square)](https://download.falco.org/packages/deb/stable/falco-0.43.0-aarch64.deb) |
| tgz-aarch64      | [![tgz](https://img.shields.io/badge/Falco-0.43.0-%2300aec7?style=flat-square)](https://download.falco.org/packages/bin/aarch64/falco-0.43.0-aarch64.tar.gz) |

| Images                                                                    |
|---------------------------------------------------------------------------|
| `docker pull docker.io/falcosecurity/falco:0.43.0`                      |
| `docker pull public.ecr.aws/falcosecurity/falco:0.43.0`                 |
| `docker pull docker.io/falcosecurity/falco-driver-loader:0.43.0`        |
| `docker pull docker.io/falcosecurity/falco-driver-loader:0.43.0-buster` |
| `docker pull docker.io/falcosecurity/falco:0.43.0-debian`               |

## v0.43.0

Released on 2026-01-28

### Breaking Changes :warning:

* fix(userspace)!: show source config path only in debug builds [[#3787](https://github.com/falcosecurity/falco/pull/3787)] - [@leogr](https://github.com/leogr)



### Minor Changes

* chore: [NOTICE] The GPG key used to sign DEB/RPM packages has been rotated, and all existing packages have been re-signed. New key fingerprint: `478B2FBBC75F4237B731DA4365106822B35B1B1F` [[#3753](https://github.com/falcosecurity/falco/pull/3753)] - [@leogr](https://github.com/leogr)
* chore(userspace): deprecate `--gvisor-generate-config` CLI option [[#3784](https://github.com/falcosecurity/falco/pull/3784)] - [@ekoops](https://github.com/ekoops)
* docs: add deprecation notice for legacy eBPF in pkg install dialog [[#3786](https://github.com/falcosecurity/falco/pull/3786)] - [@ekoops](https://github.com/ekoops)
* chore(scripts/falcoctl): increase follow interval to 1 week [[#3757](https://github.com/falcosecurity/falco/pull/3757)] - [@leogr](https://github.com/leogr)
* docs: add deprecation notice for legacy eBPF, gVisor and gRPC usage [[#3763](https://github.com/falcosecurity/falco/pull/3763)] - [@ekoops](https://github.com/ekoops)
* chore(userspace): deprecate legacy eBPF probe, gVisor engine and gRPC [[#3763](https://github.com/falcosecurity/falco/pull/3763)] - [@ekoops](https://github.com/ekoops)
* chore(engine): emit warning when the deprecated `evt.latency` field family is used in a rule condition or output [[#3744](https://github.com/falcosecurity/falco/pull/3744)] - [@irozzo-1A](https://github.com/irozzo-1A)


### Bug Fixes

* fix: prevent null pointer crash on `popen()` failure in output_program [[#3722](https://github.com/falcosecurity/falco/pull/3722)] - [@vietcgi](https://github.com/vietcgi)
* fix: correct falcoctl.yaml path in debian conffiles [[#3745](https://github.com/falcosecurity/falco/pull/3745)] - [@leogr](https://github.com/leogr)



### Non user-facing changes

* revert: chore(.github): temporary action for GPG key rotation [[#3766](https://github.com/falcosecurity/falco/pull/3766)] - [@leogr](https://github.com/leogr)
* chore(cmake): bump falcoctl dependency version to `0.12.2` [[#3790](https://github.com/falcosecurity/falco/pull/3790)] - [@ekoops](https://github.com/ekoops)
* chore(cmake): bump falcoctl dependency version to `0.12.1` [[#3777](https://github.com/falcosecurity/falco/pull/3777)] - [@ekoops](https://github.com/ekoops)
* chore(cmake): bump container plugin version to `0.6.1` [[#3780](https://github.com/falcosecurity/falco/pull/3780)] - [@ekoops](https://github.com/ekoops)
* fix(userspace/engine): missing closing quote in deprecated field warning [[#3779](https://github.com/falcosecurity/falco/pull/3779)] - [@leogr](https://github.com/leogr)
* chore(.github): Put back gpg key rotation workflow [[#3772](https://github.com/falcosecurity/falco/pull/3772)] - [@irozzo-1A](https://github.com/irozzo-1A)
* chore(cmake): bump libs/drivers to `0.23.1`/`9.1.0+driver` [[#3769](https://github.com/falcosecurity/falco/pull/3769)] - [@ekoops](https://github.com/ekoops)
* chore(cmake): bump container plugin version to 0.6.0 [[#3768](https://github.com/falcosecurity/falco/pull/3768)] - [@irozzo-1A](https://github.com/irozzo-1A)
* docs(proposals): add proposal for legacy probe, gVisor engine and gRPC output deprecation [[#3755](https://github.com/falcosecurity/falco/pull/3755)] - [@ekoops](https://github.com/ekoops)
* chore(cmake): bump libs/drivers to `0.23.0`/`9.1.0+driver` [[#3760](https://github.com/falcosecurity/falco/pull/3760)] - [@ekoops](https://github.com/ekoops)
* update(cmake): update libs and driver to latest master [[#3754](https://github.com/falcosecurity/falco/pull/3754)] - [@github-actions[bot]](https://github.com/apps/github-actions)
* fix(metrics): Add null check for state.outputs in metrics collection [[#3740](https://github.com/falcosecurity/falco/pull/3740)] - [@adduali1310](https://github.com/adduali1310)
* chore(cmake): bump libs to `0.23.0-rc2` [[#3759](https://github.com/falcosecurity/falco/pull/3759)] - [@ekoops](https://github.com/ekoops)
* chore(cmake): bump libs/drivers to `0.23.0-rc1`/`9.1.0-rc1+driver` [[#3758](https://github.com/falcosecurity/falco/pull/3758)] - [@ekoops](https://github.com/ekoops)
* fix(ci): revert changes to mitigate rate-limitar change [[#3752](https://github.com/falcosecurity/falco/pull/3752)] - [@irozzo-1A](https://github.com/irozzo-1A)
* update(cmake): update libs and driver to latest master [[#3723](https://github.com/falcosecurity/falco/pull/3723)] - [@github-actions[bot]](https://github.com/apps/github-actions)
* Reduce image size [[#3746](https://github.com/falcosecurity/falco/pull/3746)] - [@jfcoz](https://github.com/jfcoz)
* docs(RELEASE.md): specify target branch association upon release creation [[#3717](https://github.com/falcosecurity/falco/pull/3717)] - [@ekoops](https://github.com/ekoops)
* docs(RELEASE.md): fix `rn2md` cmd generating changelogs [[#3709](https://github.com/falcosecurity/falco/pull/3709)] - [@ekoops](https://github.com/ekoops)
* docs(RELEASE.md): fix PRs filtering expr for checking release notes [[#3708](https://github.com/falcosecurity/falco/pull/3708)] - [@ekoops](https://github.com/ekoops)
* docs(RELEASE.md): fix PRs filtering expression text [[#3707](https://github.com/falcosecurity/falco/pull/3707)] - [@ekoops](https://github.com/ekoops)

### Statistics

|   MERGED PRS    | NUMBER |
|-----------------|--------|
| Not user-facing |     21 |
| Release note    |     11 |
| Total           |     32 |

#### Release Manager @ekoops
