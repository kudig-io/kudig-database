# Flannel Governance

This document outlines the governance model for the Flannel project.

## Overview

Flannel is an open source project committed to vendor-neutral governance. While the current maintainer team is primarily affiliated with SUSE/Rancher, the project welcomes contributors from any organization and is committed to making decisions in the interest of the broader community.

## Code of Conduct

Flannel adheres to the [CNCF Code of Conduct](https://github.com/cncf/foundation/blob/main/code-of-conduct.md). All community members are expected to follow this code.

## Roles

### Users

Users are community members who use Flannel. They contribute by filing issues, requesting features, and providing feedback. There are no formal requirements to become a user.

### Contributors

Contributors are community members who contribute code, documentation, or other improvements. Contributions are accepted via GitHub pull requests and must be signed off with the [Developer Certificate of Origin (DCO)](DCO).

### Maintainers

Maintainers are responsible for the overall health and direction of the project. They review and approve pull requests, triage issues, and make decisions about the project's roadmap.

**Current Maintainers:**

| Name | GitHub Handle | Affiliation |
|------|--------------|-------------|
| Manuel Buil | [@manuelbuil](https://github.com/manuelbuil) | SUSE |
| Michael Fritch | [@mgfritch](https://github.com/mgfritch) | SUSE |
| Roberto Bonafiglia | [@rbrtbnfgl](https://github.com/rbrtbnfgl) | SUSE |
| Thomas Ferrandiz | [@thomasferrandiz](https://github.com/thomasferrandiz) | SUSE |

The authoritative list of maintainers is maintained in the [OWNERS](OWNERS) file.

## Maintainer Lifecycle

### Becoming a Maintainer

To become a maintainer, a contributor should:

1. Have a track record of quality contributions (code, documentation, reviews) over at least 3 months.
2. Demonstrate an understanding of the codebase and project direction.
3. Be nominated by an existing maintainer.
4. Receive approval from a majority of current maintainers (simple majority vote with a minimum 5-business-day voting period).

New maintainers are added to the [OWNERS](OWNERS) file via a pull request approved by existing maintainers.

### Maintainer Responsibilities

Maintainers are expected to:

- Review pull requests in a timely manner (within 2 weeks).
- Participate in community meetings when possible.
- Help with issue triage and release processes.
- Act in the best interest of the project and its community.
- Uphold the project's vendor-neutral governance.

### Stepping Down / Emeritus Status

A maintainer who is no longer actively contributing may step down at any time by opening a pull request to remove themselves from the [OWNERS](OWNERS) file. Maintainers who have been inactive for 6+ months may be moved to emeritus status by a vote of the remaining maintainers.

Emeritus maintainers are recognized for their past contributions and may return to active maintainer status through the standard nomination process.

## Decision Making

### Routine Decisions

Day-to-day decisions (e.g., accepting pull requests, closing issues) are made by any maintainer. Pull requests require at least one maintainer approval before merging, and the author may not approve their own pull request.

### Significant Decisions

Significant changes — including changes to governance, major architectural decisions, and changes to the project's direction — require a lazy consensus vote among maintainers:

1. Open a GitHub issue or pull request describing the proposal.
2. Allow a minimum of 5 business days for discussion and objections.
3. If no maintainer objects, the proposal is approved.
4. If there is an objection, discussion continues until resolved or a majority vote is taken.

### Conflict Resolution

In the event of a conflict that cannot be resolved through discussion, a majority vote of the maintainers is held. If consensus cannot be reached within the project, the CNCF TOC may be asked to mediate.

## Vendor Neutrality

Flannel is committed to vendor-neutral governance. No single company or organization controls the project's direction. Decisions are made in the interest of the project's users and the broader cloud native community, not any individual company. Contributors and maintainers from any organization are welcome.

## Meetings

Flannel holds a community meeting on the **3rd Thursday of each month at 8:30 AM PST**. Meeting notes are recorded in the [Community Meeting Agenda](https://docs.google.com/document/d/1kPMMFDhljWL8_CUZajrfL8Q9sdntd9vvUpe-UGhX5z8).

All community members are welcome to attend.

## Communications

- **Slack:** `#flannel-users` channel on the [Calico Slack](https://calicousers.slack.com) and `#k3s` on the [Rancher Slack](https://slack.rancher.io)
- **GitHub Issues:** [https://github.com/flannel-io/flannel/issues](https://github.com/flannel-io/flannel/issues)
- **Security:** See [SECURITY.md](SECURITY.md) for reporting security vulnerabilities

## Amendments

This governance document may be amended by a significant decision as described above. All changes must be made via pull request and approved by a majority of maintainers.
