# grafana v5.0 Release Notes

Source: [v5.0.4](https://github.com/grafana/grafana/releases/tag/v5.0.4)

[Download Page](https://grafana.com/grafana/download/5.0.4)
[Installation Guide](http://docs.grafana.org/installation/)

[Release Notes](https://community.grafana.com/t/release-notes-v5-0-x/5250)

* **Docker** Can't start Grafana on Kubernetes 1.7.14, 1.8.9, or 1.9.4 [#140 in grafana-docker repo](https://github.com/grafana/grafana-docker/issues/140) thx [@suquant](https://github.com/suquant)
* **Dashboard** Fixed bug where collapsed panels could not be directly linked to/renderer [#11114](https://github.com/grafana/grafana/issues/11114) & [#11086](https://github.com/grafana/grafana/issues/11086) & [#11296](https://github.com/grafana/grafana/issues/11296)
* **Dashboard** Provisioning dashboard with alert rules should create alerts [#11247](https://github.com/grafana/grafana/issues/11247)
* **Snapshots** For snapshots, the Graph panel renders the legend incorrectly on right hand side [#11318](https://github.com/grafana/grafana/issues/11318)
* **Alerting** Link back to Grafana returns wrong URL if root_path contains sub-path components [#11403](https://github.com/grafana/grafana/issues/11403)
* **Alerting** Incorrect default value for upload images setting for alert notifiers [#11413](https://github.com/grafana/grafana/pull/11413)