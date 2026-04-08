# grafana v7.0 Release Notes

Source: [v7.0.6](https://github.com/grafana/grafana/releases/tag/v7.0.6)

[Download Page](https://grafana.com/grafana/download/7.0.6)
[What's New Highlights](https://grafana.com/docs/grafana/latest/guides/whats-new-in-v7-0/)
[Release Notes](https://community.grafana.com/t/release-notes-v7-0-x/29381)

### Bug fixes

* **Templating**: Fixed recursive queries triggered when switching dashboard settings view [#26137](https://github.com/grafana/grafana/pull/26137)
* **Templating**: Fix recursive loop of template variable queries when changing ad-hoc-variable [#26191](https://github.com/grafana/grafana/pull/26191)
* **Auth**: Add support for forcing authentication in anonymous mode and modify SignIn to use it instead of redirect [#25567](https://github.com/grafana/grafana/pull/25567)
* **Auth**: Fix POST request failures with anonymous access [#26049](https://github.com/grafana/grafana/pull/26049)
