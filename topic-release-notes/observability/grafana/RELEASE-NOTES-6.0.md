# grafana v6.0 Release Notes

Source: [v6.0.2](https://github.com/grafana/grafana/releases/tag/v6.0.2)

Grafana v6.0  introduces a new way of exploring your data, support for log data and tons of other features.

* [What’s new in Grafana v6.0 ](http://docs.grafana.org/guides/whats-new-in-v6-0/)
* [Download](https://grafana.com/grafana/download)

## New major features:

- **Explore** A new query focused workflow for ad-hoc data exploration and troubleshooting.
- **Grafana Loki** Integration with the new open source log aggregation system from Grafana Labs.
- **Gauge Panel** A new standalone panel for gauges.
- **New Panel Editor UX** improves panel editing and enables easy switching between different visualizations.
- **Google Stackdriver Datasource** is out of beta and is officially released.
- **Azure Monitor** plugin is ported from being an external plugin to being a core datasource
- **React Plugin** support enables an easier way to build plugins.
- **Named Colors** in our new improved color picker.
- **Removal of user session storage** makes Grafana easier to deploy & improves security.

# 6.0.2 (2019-03-19)

### Bug Fixes
* **Alerting**: Fixed issue with AlertList panel links resulting in panel not found errors. [#15975](https://github.com/grafana/grafana/pull/15975), [@torkelo](https://github.com/torkelo)
* **Dashboard**: Improved error handling when rendering dashboard panels. [#15970](https://github.com/grafana/grafana/pull/15970), [@torkelo](https://github.com/torkelo)
* **LDAP**: Fix allow anonymous server bind for ldap search. [#15872](https://github.com/grafana/grafana/pull/15872), [@marefr](https://github.com/marefr)
* **Discord**: Fix discord notifier so it doesn't crash when there are no image generated. [#15833](https://github.com/grafana/grafana/pull/15833), [@marefr](https://github.com/marefr)
* **Panel Edit**: Prevent search in VizPicker from stealing focus. [#15802](https://github.com/grafana/grafana/pull/15802), [@peterholmberg](https://github.com/peterholmberg)
* **Datasource admin**: Fixed url of back button in datasource edit page, when root_url configured. [#15759](https://github.com/grafana/grafana/pull/15759), [@dprokop](https://github.com/dprokop)
