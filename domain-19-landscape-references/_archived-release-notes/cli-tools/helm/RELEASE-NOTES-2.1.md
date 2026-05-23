---
title: helm v2.1 Release Notes
description: helm v2.1 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- helm v2.1 Release Notes 是什么
- 如何 helm v2.1 Release Notes
trigger_keywords:
- helm
- v2.1
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
created: "2026-05-23"
---

# [[Helm|helm]] v2.1 Release Notes

Source: [v2.1.3](https://github.com/helm/helm/releases/tag/v2.1.3)

We know we said 2.1.2 would be the last release of the year, but we just couldn't help ourselves. We fixed a few more bugs and bumped up the version of Sprig, and decided to cut one more release.

The Helm Core Team would like to wish you all a happy holidays. Our team is made up of many people from several organizations, but for the holiday season we are going to collectively take a holiday to spend time with our friends, loved ones, and other pet GitHub projects. 🎉 

We will resume our regular meetings the first week of January. Slack, as always, is open for ongoing conversation, though core contributors will be offline from December 23 to January 3. We look forward in seeing you all again in 2017!
- Join the discussion in [[entities/kubernetes|Kubernetes]] Slack](https://slack.k8s.io/): `#helm` 
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://engineyard.zoom.us/j/366425549)
- Test, debug, and contribute charts: [GitHub/kubernetes/charts](https://github.com/kubernetes/charts)

## Notable Changes Since 2.1.2
- Fix for Deis Workflow installation (we accidentally changed an assumption about the contents of a default values. It's fixed now.).
- Update Sprig to 2.8.0
- `helm install` and `helm upgrade` both have improvements to their `--debug` output to help you figure out the real state of your release.
- A bug in namespace detection has been fixed

Version 2.1.3 is compatible with other 2.1 releases.

## Installing and Updating

Helm binaries:
- [OSX](https://get.helm.sh/helm-v2.1.3-darwin-amd64.tar.gz)
- Linux](https://get.helm.sh/helm-v2.1.3-linux-amd64.tar.gz)
- [Linux i386](https://get.helm.sh/helm-v2.1.3-linux-386.tar.gz)
- [Windows](https://get.helm.sh/helm-v2.1.3-windows-amd64.zip)

The [Quickstart Guide](https://github.com/kubernetes/helm/blob/master/docs/quickstart.md) will get you going from there. For **upgrade instructions** or detailed installation notes, check the [install guide](https://github.com/kubernetes/helm/blob/master/docs/install.md). You can also use a [script to install](https://raw.githubusercontent.com/kubernetes/helm/master/scripts/get) on any system with `bash`.

## What Next?

This is the last release for the year. No really... unless we decide to do another one.
- 2.1.4 is the next bug fix release
- 2.2.0 is the next feature release, and it is in development.

## All Changes Since 2.1.0

chore(tiller): update Sprig to 2.8.0 324bdc854d43fa18f2b079d678ddfa590eb8ff3a
Play nicely with roles that don't allow creating namespaces 1cbadb450f14362a5fe6edbf7df23701551e5157
docs(helm): change `trunc 24` in base charts aa8d178c18174e485ecd4436f4158b24ef0408db
docs(helm): change `trunc 24` in base charts 4db22274eb4a8d19fe9972a8b8109d207dd4552b
fix(upgrade):Check the raw vals during an upgrade properly 9fea982deb0b060b68d9b06830c81205731511af
