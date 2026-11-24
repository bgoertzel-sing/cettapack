# fnspec benchmark summary

Raw runs: runs-<site>.jsonl; environment: env.txt; inputs: inputs.txt; commands: commands.txt.

## once-cond (collapse(once) + if/case template tree, survey rank 9 (fnspec 3))

| site | queries | fn-generic us | fn-hosted us | **fn-spec** us | fn-fallback us | **fn-spec vs fn-hosted, paired** | fn-spec vs fn-generic, paired | answers per batch | peak RSS fn-spec / fn-hosted MiB | path (all runs) |
|---|---|---|---|---|---|---|---|---|---|---|
| gv_once_cond | 200 | 2,702.1 | 2,687.9 (2,461.1-2,939.0) | 39.3 (36.4-85.8) | 2,707.2 | **66.39x** (34.26-75.01x, 7/7) | 64.36x (31.50-78.66x, 7/7) | 200 tuples, 190 non-empty | 45.4 / 55.1 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| rs_hm_recurs | 200 | 107.7 | 153.3 (147.5-169.2) | 40.7 (37.2-43.0) | 145.3 | **3.97x** (3.53-4.20x, 7/7) | 2.68x (2.53-3.21x, 7/7) | 200 tuples, 150 non-empty | 62.9 / 63.3 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| rs_sk_summary_memo | 200 | 916.2 | 1,114.6 (1,018.1-1,592.2) | 936.2 (886.5-1,043.1) | 884.4 | **1.18x** (1.09-1.78x, 7/7) | 0.95x (0.81-1.14x, 2/7) | 200 tuples, 100 non-empty | 39.7 / 40.1 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| te_once_cond | 200 | 606.5 | 865.8 (614.9-1,172.8) | 27.0 (25.8-28.1) | 661.2 | **31.83x** (23.78-41.81x, 7/7) | 22.47x (19.55-28.95x, 7/7) | 200 tuples, 190 non-empty | 44.3 / 45.5 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| tf_once_cond | 40 | 338,493.7 | 348,548.5 (307,943.2-439,913.8) | 3,303.1 (2,636.8-6,984.2) | 344,521.9 | **101.42x** (57.51-162.44x, 7/7) | 102.48x (61.02-143.50x, 7/7) | 40 tuples, 38 non-empty | 46.3 / 286.2 | fn-spec 1,400/1,400 specialised; fn-fallback 1,400/1,400 residual |
| vi_once_cond | 200 | 3,264.0 | 3,449.9 (3,393.9-3,695.6) | 57.5 (50.1-105.6) | 3,552.9 | **63.63x** (32.65-68.83x, 7/7) | 57.59x (32.56-65.12x, 7/7) | 200 tuples, 189 non-empty | 47.0 / 65.2 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |

Route (CeTTa match backend and native storage of the data space before -> after the timed batches, all modes):

- gv_once_cond: native rows+dense 82,177+46,399 -> 82,177+46,399
- rs_hm_recurs: native rows+dense 104,927+0 -> 104,927+0
- rs_sk_summary_memo: native rows+dense 104,907+0 -> 104,907+0
- te_once_cond: native rows+dense 82,177+46,399 -> 82,177+46,399
- tf_once_cond: native rows+dense 82,177+46,399 -> 82,177+46,399
- vi_once_cond: native rows+dense 112,164+16,412 -> 112,164+16,412

## exist-cond (existence test + if/case template tree, survey rank 11 (fnspec 3))

| site | queries | fn-generic us | fn-hosted us | **fn-spec** us | fn-fallback us | **fn-spec vs fn-hosted, paired** | fn-spec vs fn-generic, paired | answers per batch | peak RSS fn-spec / fn-hosted MiB | path (all runs) |
|---|---|---|---|---|---|---|---|---|---|---|
| gv_exist_cond | 200 | 1,532.3 | 1,528.7 (1,390.2-1,604.0) | 43.0 (37.3-70.8) | 1,491.7 | **36.01x** (19.65-41.03x, 7/7) | 36.10x (20.52-62.69x, 7/7) | 200 answers, 190 True | 45.5 / 46.3 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| rs_sk_row_present | 200 | 1,880.4 | 2,222.9 (2,083.9-2,329.9) | 2,084.7 (2,003.3-2,445.7) | 1,947.6 | **1.07x** (0.91-1.12x, 6/7) | 0.89x (0.78-0.99x, 0/7) | 200 answers, 100 True | 72.0 / 72.3 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| te_exist_cond | 200 | 408.1 | 414.6 (359.2-840.5) | 23.9 (21.3-25.0) | 404.1 | **17.49x** (15.73-33.93x, 7/7) | 16.34x (13.91-19.74x, 7/7) | 200 answers, 190 True | 45.4 / 45.0 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| tf_exist_cond | 40 | 202,475.1 | 231,384.8 (187,072.0-275,419.6) | 3,573.1 (2,967.8-6,235.7) | 211,865.0 | **62.40x** (38.98-92.80x, 7/7) | 56.67x (30.79-83.88x, 7/7) | 40 answers, 38 True | 45.6 / 71.1 | fn-spec 1,400/1,400 specialised; fn-fallback 1,400/1,400 residual |
| vi_exist_cond | 200 | 1,857.1 | 1,913.3 (1,804.8-2,202.8) | 50.8 (44.4-65.4) | 1,921.6 | **37.43x** (33.69-42.54x, 7/7) | 37.84x (30.43-44.18x, 7/7) | 200 answers, 189 True | 47.1 / 47.8 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |

Route (CeTTa match backend and native storage of the data space before -> after the timed batches, all modes):

- gv_exist_cond: native rows+dense 82,177+46,399 -> 82,177+46,399
- rs_sk_row_present: native rows+dense 104,939+0 -> 104,939+0
- te_exist_cond: native rows+dense 82,177+46,399 -> 82,177+46,399
- tf_exist_cond: native rows+dense 82,177+46,399 -> 82,177+46,399
- vi_exist_cond: native rows+dense 112,164+16,412 -> 112,164+16,412

## stream-cond (stream + if/case template tree, survey rank 12 (fnspec 3))

| site | queries | fn-generic us | fn-hosted us | **fn-spec** us | fn-fallback us | **fn-spec vs fn-hosted, paired** | fn-spec vs fn-generic, paired | answers per batch | peak RSS fn-spec / fn-hosted MiB | path (all runs) |
|---|---|---|---|---|---|---|---|---|---|---|
| gv_stream_cond | 200 | 630.5 | 691.4 (598.8-835.4) | 89.8 (78.6-114.1) | 673.0 | **7.57x** (7.00-8.75x, 7/7) | 7.08x (5.20-7.64x, 7/7) | 7,268 | 44.9 / 45.3 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| rs_sk_left_out | 200 | 162.3 | 195.9 (175.8-208.4) | 44.3 (36.6-59.2) | 195.5 | **4.51x** (3.52-5.33x, 7/7) | 3.84x (2.85-4.18x, 7/7) | 700 | 103.2 / 103.0 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| te_stream_cond | 200 | 153.1 | 167.9 (160.7-223.7) | 36.4 (30.6-50.7) | 184.4 | **4.89x** (3.20-7.05x, 7/7) | 4.48x (2.90-5.30x, 7/7) | 1,620 | 44.9 / 45.0 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| tf_stream_cond | 40 | 71,129.7 | 76,682.1 (73,867.1-78,446.9) | 10,045.2 (9,078.5-10,759.9) | 74,329.8 | **7.40x** (7.16-8.64x, 7/7) | 7.08x (6.70-8.17x, 7/7) | 173,693 | 49.5 / 56.7 | fn-spec 1,400/1,400 specialised; fn-fallback 1,400/1,400 residual |
| vi_stream_cond | 200 | 824.6 | 860.4 (825.3-963.2) | 109.6 (99.9-143.6) | 853.4 | **7.92x** (6.71-8.53x, 7/7) | 7.46x (6.55-8.43x, 7/7) | 9,643 | 46.5 / 46.9 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |

Route (CeTTa match backend and native storage of the data space before -> after the timed batches, all modes):

- gv_stream_cond: native rows+dense 82,177+46,399 -> 82,177+46,399
- rs_sk_left_out: native rows+dense 104,995+0 -> 104,995+0
- te_stream_cond: native rows+dense 82,177+46,399 -> 82,177+46,399
- tf_stream_cond: native rows+dense 82,177+46,399 -> 82,177+46,399
- vi_stream_cond: native rows+dense 112,164+16,412 -> 112,164+16,412

## collapse-cond (collapse + template tree with a boolean leaf; not a survey family (0 occurrences), the same compiler (fnspec 3))

| site | queries | fn-generic us | fn-hosted us | **fn-spec** us | fn-fallback us | **fn-spec vs fn-hosted, paired** | fn-spec vs fn-generic, paired | answers per batch | peak RSS fn-spec / fn-hosted MiB | path (all runs) |
|---|---|---|---|---|---|---|---|---|---|---|
| gv_collapse_cond | 200 | 1,330.7 | 1,376.1 (1,240.9-1,653.9) | 48.3 (43.4-50.4) | 1,404.3 | **30.90x** (25.41-34.79x, 7/7) | 29.47x (24.53-31.67x, 7/7) | 200 tuples, 7,268 items | 44.9 / 45.4 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| te_collapse_cond | 200 | 304.7 | 335.4 (302.6-507.7) | 27.3 (24.0-44.4) | 349.2 | **12.33x** (7.56-17.78x, 7/7) | 11.27x (7.52-12.70x, 7/7) | 200 tuples, 1,620 items | 44.9 / 45.0 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| tf_collapse_cond | 40 | 160,033.4 | 170,605.8 (154,162.2-221,151.2) | 4,071.9 (3,293.9-4,496.8) | 167,401.4 | **40.32x** (37.50-58.22x, 7/7) | 39.73x (34.05-65.71x, 7/7) | 40 tuples, 173,693 items | 45.1 / 63.4 | fn-spec 1,400/1,400 specialised; fn-fallback 1,400/1,400 residual |
| vi_collapse_cond | 200 | 1,807.8 | 1,917.1 (1,744.5-3,099.7) | 64.6 (56.3-128.4) | 1,811.5 | **29.85x** (18.89-45.99x, 7/7) | 28.32x (18.14-38.70x, 7/7) | 200 tuples, 9,643 items | 46.6 / 47.4 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |

Route (CeTTa match backend and native storage of the data space before -> after the timed batches, all modes):

- gv_collapse_cond: native rows+dense 82,177+46,399 -> 82,177+46,399
- te_collapse_cond: native rows+dense 82,177+46,399 -> 82,177+46,399
- tf_collapse_cond: native rows+dense 82,177+46,399 -> 82,177+46,399
- vi_collapse_cond: native rows+dense 112,164+16,412 -> 112,164+16,412

## Checks

- every site: the same answers per batch (and answer items) in all 5 modes; fn-spec specialised every timed call; fn-fallback took the residual on every timed call

