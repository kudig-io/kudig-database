# thanos v0.17 Release Notes

Source: [v0.17.2](https://github.com/thanos-io/thanos/releases/tag/v0.17.2)

### Fixed

- [#3532](https://github.com/thanos-io/thanos/pull/3532) compact: do not cleanup blocks on boot. Reverts the behavior change introduced in [#3115](https://github.com/thanos-io/thanos/pull/3115) as in some very bad cases the boot of Thanos Compact took a very long time since there were a lot of blocks-to-be-cleaned.
- [#3520](https://github.com/thanos-io/thanos/pull/3520) Fix index out of bound bug when comparing ZLabelSets.

