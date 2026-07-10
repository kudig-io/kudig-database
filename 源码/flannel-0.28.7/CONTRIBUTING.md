# How to Contribute

Flannel is [Apache 2.0 licensed](LICENSE) and accepts contributions via
GitHub pull requests. This document outlines conventions on
development workflow, commit message formatting, contact points, and other
resources to make it easier to get your contribution accepted.

## Community

- **Community meetings:** 3rd Thursday of each month at 8:30 AM PST — [Meeting Agenda](https://docs.google.com/document/d/1kPMMFDhljWL8_CUZajrfL8Q9sdntd9vvUpe-UGhX5z8)
- **Slack:** `#flannel-users` on [Calico Slack](https://calicousers.slack.com) and `#k3s` on [Rancher Slack](https://slack.rancher.io)
- **Issues:** [https://github.com/flannel-io/flannel/issues](https://github.com/flannel-io/flannel/issues)
- **Code of Conduct:** [CNCF Code of Conduct](https://github.com/cncf/foundation/blob/main/code-of-conduct.md)
- **Governance:** [GOVERNANCE.md](GOVERNANCE.md)

# Certificate of Origin

By contributing to this project you agree to the Developer Certificate of
Origin (DCO). This document was created by the Linux Kernel community and is a
simple statement that you, as a contributor, have the legal right to make the
contribution. See the [DCO](DCO) file for details.

## Getting Started

- Fork the repository on GitHub
- Read the [README](README.md) for build and test instructions
- Play with the project, submit bugs, submit patches!

## Contribution Flow

This is a rough outline of what a contributor's workflow looks like:

- Create a topic branch from where you want to base your work (usually master).
- Make commits of logical units.
- Make sure your commit messages are in the proper format (see below).
- Push your changes to a topic branch in your fork of the repository.
- Make sure the tests pass, and add any new tests as appropriate.
- Submit a pull request to the original repository.

Thanks for your contributions!

### Format of the Commit Message

We follow a rough convention for commit messages that is designed to answer two
questions: what changed and why. The subject line should feature the what and
the body of the commit should describe the why.

```
scripts: add the test-cluster command

this uses tmux to setup a test cluster that you can easily kill and
start for debugging.

Fixes #38
```

The format can be described more formally as follows:

```
<subsystem>: <what changed>
<BLANK LINE>
<why this change was made>
<BLANK LINE>
<footer>
```

The first line is the subject and should be no longer than 70 characters, the
second line is always blank, and other lines should be wrapped at 80 characters.
This allows the message to be easier to read on GitHub as well as in various
git tools.
