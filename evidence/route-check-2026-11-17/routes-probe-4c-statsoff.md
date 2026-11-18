| build | site | mode | backend | native rows + dense facts (before timed) | PathMap shadow | CETTA_PATHMAP_QUERY_INDEX | forced w/o rows | answers/batch | kernel calls / fallbacks | per query: native-query adm. / loop-call-native-query / match-native-probe / pos-index lookup | PathMap/MORK counters |
|---|---|---|---|---|---|---|---|---|---|---|---|
| probe-4c-statsoff | main | fallback | native | 128576 + 0 | None | unset | n/a (no such check) | 1653 | 0 / 200 | (stats off) | (stats off) |
| probe-4c-statsoff | main | generic | native | 55315 + 73261 | None | unset | n/a (no such check) | 1653 | 0 / 0 | (stats off) | (stats off) |
| probe-4c-statsoff | main | hosted-generic | native | 82177 + 46399 | None | unset | n/a (no such check) | 1653 | 0 / 0 | (stats off) | (stats off) |
| probe-4c-statsoff | main | kernel | native | 128576 + 0 | None | unset | n/a (no such check) | 1653 | 200 / 0 | (stats off) | (stats off) |
