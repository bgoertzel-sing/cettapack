# fnspec benchmark summary

Raw runs: runs-<site>.jsonl; environment: env.txt; inputs: inputs.txt; commands: commands.txt.

## exist-filter (existence test + filter, survey rank 4)

| site | queries | fn-generic us | fn-hosted us | **fn-spec** us | fn-fallback us | **fn-spec vs fn-hosted, paired** | fn-spec vs fn-generic, paired | answers per batch | peak RSS fn-spec / fn-hosted MiB | path (all runs) |
|---|---|---|---|---|---|---|---|---|---|---|
| gv_exist_filter | 200 | 904.3 | 965.9 (872.1-1,074.6) | 38.4 (35.1-42.4) | 883.8 | **24.91x** (23.17-26.36x, 7/7) | 23.40x (19.19-25.79x, 7/7) | 200 answers, 190 True | 45.4 / 45.7 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| rs_sk_needed | 200 | 547.4 | 642.6 (518.8-689.3) | 46.3 (40.0-53.6) | 600.6 | **12.96x** (10.76-16.26x, 7/7) | 11.93x (10.09-13.08x, 7/7) | 200 answers, 100 True | 50.3 / 50.3 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| te_exist_filter | 200 | 242.0 | 245.5 (219.1-294.3) | 26.0 (24.4-27.3) | 245.2 | **9.79x** (8.27-10.83x, 7/7) | 9.83x (8.57-12.18x, 7/7) | 200 answers, 190 True | 45.5 / 45.3 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| tf_exist_filter | 40 | 98,554.3 | 114,242.9 (107,668.8-135,048.8) | 3,214.8 (2,254.3-4,212.2) | 103,443.0 | **39.89x** (25.56-51.63x, 7/7) | 36.49x (22.50-47.34x, 7/7) | 40 answers, 38 True | 45.7 / 64.5 | fn-spec 1,400/1,400 specialised; fn-fallback 1,400/1,400 residual |
| vi_exist_filter | 200 | 1,176.8 | 1,275.7 (1,149.5-1,470.1) | 54.2 (49.1-60.5) | 1,198.6 | **23.56x** (22.37-24.32x, 7/7) | 21.95x (18.81-25.05x, 7/7) | 200 answers, 189 True | 47.1 / 47.6 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |

Route (CeTTa match backend and native storage of the data space before -> after the timed batches, all modes):

- gv_exist_filter: native rows+dense 82,177+46,399 -> 82,177+46,399
- rs_sk_needed: native rows+dense 105,081+0 -> 105,081+0
- te_exist_filter: native rows+dense 82,177+46,399 -> 82,177+46,399
- tf_exist_filter: native rows+dense 82,177+46,399 -> 82,177+46,399
- vi_exist_filter: native rows+dense 112,164+16,412 -> 112,164+16,412

## once-data (collapse(once) + data, survey rank 5)

| site | queries | fn-generic us | fn-hosted us | **fn-spec** us | fn-fallback us | **fn-spec vs fn-hosted, paired** | fn-spec vs fn-generic, paired | answers per batch | peak RSS fn-spec / fn-hosted MiB | path (all runs) |
|---|---|---|---|---|---|---|---|---|---|---|
| gv_once_data | 200 | 181.2 | 296.9 (253.7-434.3) | 43.6 (35.8-56.3) | 207.2 | **6.86x** (5.27-12.14x, 7/7) | 4.14x (3.47-4.71x, 7/7) | 200 tuples, 190 non-empty | 44.4 / 44.6 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| rs_stored_proof_tvs | 200 | 33.6 | 68.8 (61.8-77.0) | 25.5 (22.1-45.3) | 58.7 | **2.76x** (1.52-3.16x, 7/7) | 1.32x (0.73-1.41x, 5/7) | 200 tuples, 102 non-empty | 50.7 / 50.7 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| te_once_data | 200 | 65.6 | 102.4 (90.3-119.1) | 25.4 (24.0-29.0) | 89.3 | **4.02x** (3.38-4.53x, 7/7) | 2.51x (2.20-4.09x, 7/7) | 200 tuples, 190 non-empty | 44.0 / 44.5 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| tf_once_data | 40 | 21,755.1 | 33,941.8 (27,694.5-40,130.0) | 3,181.7 (2,550.5-5,555.2) | 21,863.6 | **11.78x** (5.98-14.19x, 7/7) | 6.84x (3.86-9.89x, 7/7) | 40 tuples, 38 non-empty | 45.0 / 61.4 | fn-spec 1,400/1,400 specialised; fn-fallback 1,400/1,400 residual |
| vi_once_data | 200 | 279.2 | 395.5 (354.1-529.0) | 58.0 (49.8-120.3) | 278.9 | **6.69x** (3.43-10.63x, 7/7) | 4.58x (2.27-7.60x, 7/7) | 200 tuples, 189 non-empty | 46.0 / 46.7 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |

Route (CeTTa match backend and native storage of the data space before -> after the timed batches, all modes):

- gv_once_data: native rows+dense 82,177+46,399 -> 82,177+46,399
- rs_stored_proof_tvs: native rows+dense 126,089+0 -> 126,089+0
- te_once_data: native rows+dense 82,177+46,399 -> 82,177+46,399
- tf_once_data: native rows+dense 82,177+46,399 -> 82,177+46,399
- vi_once_data: native rows+dense 112,164+16,412 -> 112,164+16,412

## exist-data (existence test + data, survey rank 6)

| site | queries | fn-generic us | fn-hosted us | **fn-spec** us | fn-fallback us | **fn-spec vs fn-hosted, paired** | fn-spec vs fn-generic, paired | answers per batch | peak RSS fn-spec / fn-hosted MiB | path (all runs) |
|---|---|---|---|---|---|---|---|---|---|---|
| gv_exist_data | 200 | 177.6 | 221.6 (210.5-250.4) | 40.2 (36.0-43.5) | 200.2 | **5.98x** (5.07-6.26x, 7/7) | 4.40x (4.09-5.01x, 7/7) | 200 answers, 190 True | 44.8 / 45.1 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| rs_compound_connective | 200 | 46.3 | 79.1 (66.0-87.2) | 21.8 (18.9-22.4) | 69.2 | **3.63x** (3.05-3.97x, 7/7) | 2.13x (2.03-2.48x, 7/7) | 200 answers, 107 True | 21.7 / 22.0 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| rs_given_rule | 200 | 45.2 | 79.9 (72.3-88.8) | 23.1 (19.8-27.4) | 69.6 | **3.81x** (2.92-3.99x, 7/7) | 2.01x (1.65-2.43x, 7/7) | 200 answers, 102 True | 76.6 / 76.1 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| te_exist_data | 200 | 82.1 | 108.7 (96.3-112.3) | 26.3 (20.6-42.4) | 107.2 | **3.66x** (2.57-5.31x, 7/7) | 2.98x (1.94-5.69x, 7/7) | 200 answers, 190 True | 44.7 / 44.6 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| tf_exist_data | 40 | 17,809.3 | 26,828.3 (23,094.5-32,929.0) | 3,398.2 (2,702.9-5,534.8) | 17,609.7 | **8.56x** (5.95-9.47x, 7/7) | 5.08x (3.24-6.88x, 7/7) | 40 answers, 38 True | 45.2 / 57.8 | fn-spec 1,400/1,400 specialised; fn-fallback 1,400/1,400 residual |
| vi_exist_data | 200 | 256.2 | 336.3 (298.5-420.7) | 57.7 (52.3-82.4) | 276.3 | **5.55x** (4.08-8.04x, 7/7) | 4.24x (2.99-8.88x, 7/7) | 200 answers, 189 True | 46.5 / 46.6 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |

Route (CeTTa match backend and native storage of the data space before -> after the timed batches, all modes):

- gv_exist_data: native rows+dense 82,177+46,399 -> 82,177+46,399
- rs_compound_connective: native rows+dense 6,423+18,841 -> 6,423+18,841
- rs_given_rule: native rows+dense 104,917+0 -> 104,917+0
- te_exist_data: native rows+dense 82,177+46,399 -> 82,177+46,399
- tf_exist_data: native rows+dense 82,177+46,399 -> 82,177+46,399
- vi_exist_data: native rows+dense 112,164+16,412 -> 112,164+16,412

## once-filter (collapse(once) + filter, survey rank 7)

| site | queries | fn-generic us | fn-hosted us | **fn-spec** us | fn-fallback us | **fn-spec vs fn-hosted, paired** | fn-spec vs fn-generic, paired | answers per batch | peak RSS fn-spec / fn-hosted MiB | path (all runs) |
|---|---|---|---|---|---|---|---|---|---|---|
| gv_once_filter | 200 | 1,130.4 | 1,229.3 (1,045.4-1,954.5) | 43.0 (39.7-87.3) | 1,190.3 | **26.33x** (12.21-48.70x, 7/7) | 24.73x (13.01-33.10x, 7/7) | 200 tuples, 190 non-empty | 44.0 / 45.9 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| rs_sk_needed_depth | 200 | 593.9 | 635.1 (513.6-1,131.1) | 51.7 (44.7-54.3) | 591.9 | **13.65x** (10.74-21.86x, 7/7) | 11.11x (9.18-19.33x, 7/7) | 200 tuples, 100 non-empty | 49.6 / 49.8 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| te_once_filter | 200 | 267.2 | 303.9 (266.1-334.1) | 27.4 (23.8-46.8) | 293.6 | **11.00x** (6.88-11.87x, 7/7) | 9.36x (5.94-11.73x, 7/7) | 200 tuples, 190 non-empty | 44.4 / 45.5 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| tf_once_filter | 40 | 135,753.7 | 142,987.9 (133,542.2-149,994.2) | 3,420.7 (2,225.1-3,567.0) | 134,214.5 | **41.80x** (37.44-67.41x, 7/7) | 40.72x (35.48-70.74x, 7/7) | 40 tuples, 38 non-empty | 45.0 / 67.4 | fn-spec 1,400/1,400 specialised; fn-fallback 1,400/1,400 residual |
| vi_once_filter | 200 | 1,371.6 | 1,599.0 (1,385.8-1,802.0) | 56.2 (45.0-60.6) | 1,520.3 | **29.71x** (24.53-40.02x, 7/7) | 25.14x (22.51-31.22x, 7/7) | 200 tuples, 189 non-empty | 46.5 / 47.9 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |

Route (CeTTa match backend and native storage of the data space before -> after the timed batches, all modes):

- gv_once_filter: native rows+dense 82,177+46,399 -> 82,177+46,399
- rs_sk_needed_depth: native rows+dense 105,081+0 -> 105,081+0
- te_once_filter: native rows+dense 82,177+46,399 -> 82,177+46,399
- tf_once_filter: native rows+dense 82,177+46,399 -> 82,177+46,399
- vi_once_filter: native rows+dense 112,164+16,412 -> 112,164+16,412

## Checks

- existence test agrees with collapse(once): te_exist_data 190 True = te_once_data 190 non-empty
- existence test agrees with collapse(once): gv_exist_data 190 True = gv_once_data 190 non-empty
- existence test agrees with collapse(once): tf_exist_data 38 True = tf_once_data 38 non-empty
- existence test agrees with collapse(once): vi_exist_data 189 True = vi_once_data 189 non-empty
- existence test agrees with collapse(once): te_exist_filter 190 True = te_once_filter 190 non-empty
- existence test agrees with collapse(once): gv_exist_filter 190 True = gv_once_filter 190 non-empty
- existence test agrees with collapse(once): tf_exist_filter 38 True = tf_once_filter 38 non-empty
- existence test agrees with collapse(once): vi_exist_filter 189 True = vi_once_filter 189 non-empty
- existence test agrees with collapse(once): rs_sk_needed 100 True = rs_sk_needed_depth 100 non-empty
- every site: the same answers per batch (and answer items) in all four modes; fn-spec specialised every timed call; fn-fallback took the residual on every timed call

