# Flannel Roadmap

This document outlines the current direction and future plans for Flannel.

For tracking specific upcoming work, see the [GitHub Milestones](https://github.com/flannel-io/flannel/milestones) and [open issues](https://github.com/flannel-io/flannel/issues).

## Current Focus Areas

### Stability & Maintenance
- Ongoing dependency updates and CVE patching
- Bug fixes reported by users and downstream distributions
- Kubernetes version compatibility (following upstream Kubernetes release cycle)

### Backend Improvements
- **nftables backend:** Replacing the legacy iptables backend with nftables for improved performance and maintainability (see [ADR](Documentation/adrs/add-nftables-implementation.md))
- Continued VXLAN backend stability improvements

### IPv6 / Dual-stack Support
- Improving dual-stack (IPv4 + IPv6) networking support for Kubernetes clusters

### Security
- Ongoing OpenSSF Scorecard improvements
- Regular security audits and vulnerability response (see [SECURITY.md](SECURITY.md))

## Future Considerations

- **Windows support:** Investigating improvements to Windows node support
- **Performance benchmarking:** Establishing a public benchmark suite for backend comparison
- **Documentation improvements:** Expanding user-facing documentation and tutorials

## Process

Roadmap items are tracked as GitHub issues and milestones. The community discusses priorities at the [monthly community meeting](https://docs.google.com/document/d/1kPMMFDhljWL8_CUZajrfL8Q9sdntd9vvUpe-UGhX5z8) (3rd Thursday of each month, 8:30 AM PST).

Proposed significant changes go through the [ADR process](Documentation/adrs/).
