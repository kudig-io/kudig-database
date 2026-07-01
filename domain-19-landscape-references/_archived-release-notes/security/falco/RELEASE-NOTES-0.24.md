---
title: falco v0.24 Release Notes
description: falco v0.24 Release Notes — Kubernetes 生产运维知识库
summary: falco v0.24 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- apiserver
- docker
- falco
- job
- ebpf
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- falco v0.24 Release Notes 是什么
- 如何 falco v0.24 Release Notes
trigger_keywords:
- falco
- v0.24
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- ebpf-basics
- logging-basics
---



# [[Falco|falco]] v0.24 Release Notes

Source: [0.24.0](https://github.com/falcosecurity/falco/releases/tag/0.24.0)

Released on 2020-16-07

### Major Changes

* BREAKING CHANGE: --stats_interval is now --stats-interval [[#1308](https://github.com/falcosecurity/falco/pull/1308)]
* BREAKING CHANGE: server streaming [[gRPC|gRPC]] outputs method is now `falco.outputs.[[Service|service]]/get` [[#1241](https://github.com/falcosecurity/falco/pull/1241)]
* new: auto threadiness for gRPC server [[#1271](https://github.com/falcosecurity/falco/pull/1271)]
* new: new bi-directional async streaming gRPC outputs (`falco.outputs.service/sub`)  [[#1241](https://github.com/falcosecurity/falco/pull/1241)]
* new: unix socket for the gRPC server [[#1217](https://github.com/falcosecurity/falco/pull/1217)]
* new: Falco now supports userspace instrumentation with the -u flag [[#1195](https://github.com/falcosecurity/falco/pull/1195)]


### Minor Changes

* update: driver version is 85c88952b018fdbce2464222c3303229f5bfcfad now [[#1305](https://github.com/falcosecurity/falco/pull/1305)]
* update: `SKIP_MODULE_LOAD` renamed to `SKIP_DRIVER_LOADER` [[#1297](https://github.com/falcosecurity/falco/pull/1297)]
* docs: add leogr to OWNERS [[#1300](https://github.com/falcosecurity/falco/pull/1300)]
* update: default threadiness to 0 ("auto" behavior) [[#1271](https://github.com/falcosecurity/falco/pull/1271)]
* update: k8s audit endpoint now defaults to /k8s-audit everywhere [[#1292](https://github.com/falcosecurity/falco/pull/1292)]
* update(falco.yaml): `webserver.k8s_audit_endpoint` default value changed from `/k8s_audit` to `/k8s-audit` [[#1261](https://github.com/falcosecurity/falco/pull/1261)]
* docs(test): instructions to run regression test suites locally [[#1234](https://github.com/falcosecurity/falco/pull/1234)]


### Bug Fixes

* fix: --stats-interval correctly accepts values >= 999 (ms) [[#1308](https://github.com/falcosecurity/falco/pull/1308)]
* fix: make the eBPF driver build work on CentOS 8 [[#1301](https://github.com/falcosecurity/falco/pull/1301)]
* fix(userspace/falco): correct options handling for `buffered_output: false` which was not honored for the `stdout` output [[#1296](https://github.com/falcosecurity/falco/pull/1296)]
* fix(userspace/falco): honor -M also when using a trace file [[#1245](https://github.com/falcosecurity/falco/pull/1245)]
* fix: high CPU usage when using server streaming gRPC outputs [[#1241](https://github.com/falcosecurity/falco/pull/1241)]
* fix: missing newline from some log messages (eg., token bucket depleted) [[#1257](https://github.com/falcosecurity/falco/pull/1257)]


### Rule Changes

* rule(Container Drift Detected (chmod)): disabled by default [[#1316](https://github.com/falcosecurity/falco/pull/1316)]
* rule(Container Drift Detected (open+create)): disabled by default [[#1316](https://github.com/falcosecurity/falco/pull/1316)]
* rule(Write below etc): allow snapd to write its unit files [[#1289](https://github.com/falcosecurity/falco/pull/1289)]
* rule(macro remote_file_copy_procs): fix reference to remote_file_copy_binaries [[#1224](https://github.com/falcosecurity/falco/pull/1224)]
* rule(list allowed_k8s_users): whitelisted kube-apiserver-healthcheck user created by kops >= 1.17.0 for the kube-apiserver-healthcheck sidecar [[#1286](https://github.com/falcosecurity/falco/pull/1286)]
* rule(Change thread namespace): Allow `protokube`, `dockerd`, `tini` and `aws` binaries to change thread namespace. [[#1222](https://github.com/falcosecurity/falco/pull/1222)]
* rule(macro exe_running_docker_save): to filter out cmdlines containing `/var/run/docker`. [[#1222](https://github.com/falcosecurity/falco/pull/1222)]
* rule(macro user_known_cron_jobs): new macro to be overridden to list known cron jobs   [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(Schedule Cron Jobs): exclude known cron jobs  [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(macro user_known_update_package_registry): new macro to be overridden to list known package registry update  [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(Update Package Registry): exclude known package registry update [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(macro user_known_read_ssh_information_activities): new macro to be overridden to list known activities that read SSH info  [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(Read ssh information): do not throw for activities known to read SSH info [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(macro user_known_read_sensitive_files_activities): new macro to be overridden to list activities known to read sensitive files [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(Read sensitive file trusted after startup): do not throw for activities known to read sensitive files [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(Read sensitive file untrusted): do not throw for activities known to read sensitive files [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(macro user_known_write_rpm_database_activities): new macro to be overridden to list activities known to write RPM database [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(Write below rpm database): do not throw for activities known to write RPM database [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(macro user_known_db_spawned_processes): new macro to be overridden to list processes known to spawn DB [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(DB program spawned process): do not throw for processes known to spawn DB [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(macro user_known_modify_bin_dir_activities): new macro to be overridden to list activities known to modify bin directories [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(Modify binary dirs): do not throw for activities known to modify bin directories [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(macro user_known_mkdir_bin_dir_activities): new macro to be overridden to list activities known to create directories below bin directories [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(Mkdir binary dirs): do not throw for activities known to create directories below bin directories [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(macro user_known_system_user_login): new macro to exclude known system user logins [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(System user interactive): do not throw for known system user logins [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(macro user_known_user_management_activities): new macro to be overridden to list activities known to do user managements activities [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(User mgmt binaries): do not throw for activities known to do user managements activities [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(macro user_known_create_files_below_dev_activities): new macro to be overridden to list activities known to create files below dev [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(Create files below dev): do not throw for activities known to create files below dev [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(macro user_known_contact_k8s_api_server_activities): new macro to be overridden to list activities known to contact Kubernetes API server [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(Contact K8S API Server From Container): do not throw for activities known to contact Kubernetes API server [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(macro user_known_network_tool_activities): new macro to be overridden to list activities known to spawn/use network tools [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(Launch Suspicious Network Tool in Container): do not throw for activities known to spawn/use network tools [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(macro user_known_remove_data_activities): new macro to be overridden to list activities known to perform data remove commands [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(Remove Bulk Data from Disk): do not throw for activities known to perform data remove commands [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(macro user_known_create_hidden_file_activities): new macro to be overridden to list activities known to create hidden files [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(Create Hidden Files or Directories): do not throw for activities known to create hidden files [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(macro user_known_stand_streams_redirect_activities): new macro to be overridden to list activities known to redirect stream to network connection (in containers) [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(Redirect STDOUT/STDIN to Network Connection in Container): do not throw for activities known to redirect stream to network connection (in containers) [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(macro user_known_container_drift_activities): new macro to be overridden to list activities known to create executables in containers [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(Container Drift Detected (chmod)): do not throw for activities known to give execution permissions to files in containers [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(Container Drift Detected (open+create)): do not throw for activities known to create executables in containers [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(macro user_known_node_port_service): do not throw for services known to start with a NopePort service type (k8s) [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(Create NodePort Service): do not throw for services known to start with a NopePort service type (k8s) [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(macro user_known_exec_pod_activities): do not throw for activities known to attach/exec to a pod (k8s) [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(Attach/Exec Pod): do not throw for activities known to attach/exec to a pod (k8s) [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(macro trusted_pod): defines trusted pods by an image list  [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(Pod Created in Kube Namespace): do not throw for trusted pods [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(macro trusted_sa): define trusted ServiceAccount [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(Service Account Created in Kube Namespace): do not throw for trusted ServiceAccount [[#1294](https://github.com/falcosecurity/falco/pull/1294)]
* rule(list network_tool_binaries): add zmap to the list [[#1284](https://github.com/falcosecurity/falco/pull/1284)]
* rule(macro root_dir): correct macro to exactly match the `/root` dir and not other with just `/root` as a prefix [[#1279](https://github.com/falcosecurity/falco/pull/1279)]
* rule(macro user_expected_terminal_shell_in_container_conditions): allow whitelisting terminals in containers under specific conditions [[#1154](https://github.com/falcosecurity/falco/pull/1154)]
* rule(macro user_known_write_below_binary_dir_activities): allow writing to a binary dir in some conditions [[#1260](https://github.com/falcosecurity/falco/pull/1260)]
* rule(macro trusted_logging_images): Add addl fluentd image [[#1230](https://github.com/falcosecurity/falco/pull/1230)]
* rule(macro trusted_logging_images): Let azure-npm image write to /var/log [[#1230](https://github.com/falcosecurity/falco/pull/1230)]
* rule(macro lvprogs_writing_conf): Add lvs as a lvm program [[#1230](https://github.com/falcosecurity/falco/pull/1230)]
* rule(macro user_known_k8s_client_container): Allow hcp-tunnelfront to run kubectl in containers [[#1230](https://github.com/falcosecurity/falco/pull/1230)]
* rule(list allowed_k8s_users): Add vertical pod autoscaler as known k8s users [[#1230](https://github.com/falcosecurity/falco/pull/1230)]
* rule(Anonymous Request Allowed): update to checking auth decision equals to allow [[#1267](https://github.com/falcosecurity/falco/pull/1267)]
* rule(Container Drift Detected (chmod)): new rule to detect if an existing file get exec permissions in a container [[#1254](https://github.com/falcosecurity/falco/pull/1254)]
* rule(Container Drift Detected (open+create)): new rule to detect if a new file with execution permission is created in a container [[#1254](https://github.com/falcosecurity/falco/pull/1254)]
* rule(Mkdir binary dirs): correct condition in macro `bin_dir_mkdir` to catch `mkdirat` syscall [[#1250](https://github.com/falcosecurity/falco/pull/1250)]
* rule(Modify binary dirs): correct condition in macro `bin_dir_rename` to catch `rename`, `renameat`, and `unlinkat` syscalls [[#1250](https://github.com/falcosecurity/falco/pull/1250)]
* rule(Create files below dev): correct condition to catch `openat` syscall [[#1250](https://github.com/falcosecurity/falco/pull/1250)]
* rule(macro user_known_set_setuid_or_setgid_bit_conditions): create macro [[#1213](https://github.com/falcosecurity/falco/pull/1213)]

### Statistics

| Merged PRs        | Number  |
|-------------------|---------|
| Not user-facing   | 9       |
| Release note      | 29       |
| Total             | 38       |