# prometheus v2.32 Release Notes

Source: [v2.32.1](https://github.com/prometheus/prometheus/releases/tag/v2.32.1)

* [BUGFIX] Scrape: Fix reporting metrics when sample limit is reached during the report. #9996
* [BUGFIX] Scrape: Ensure that scrape interval and scrape timeout are always set. #10023
* [BUGFIX] TSDB: Expose and fix bug in iterators' `Seek()` method. #10030
