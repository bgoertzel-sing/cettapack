# fnspec benchmark summary

Raw runs: runs-<site>.jsonl; environment: env.txt; inputs: inputs.txt; commands: commands.txt.

## collapse-data (survey rank 1)

| site | queries | fn-generic us | fn-hosted us | **fn-spec** us | fn-fallback us | **fn-spec vs fn-hosted, paired** | fn-spec vs fn-generic, paired | answers per batch | peak RSS fn-spec / fn-hosted MiB | path (all runs) |
|---|---|---|---|---|---|---|---|---|---|---|
| gv_collapse_data | 200 | 183.3 | 228.7 (215.3-243.2) | 55.9 (51.0-62.1) | 214.8 | **4.11x** (3.69-4.55x, 7/7) | 3.29x (2.90-3.80x, 7/7) | 200 tuples, 7,377 items | 44.5 / 44.9 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| rs_live_hypotheses | 200 | 611.7 | 705.0 (579.2-812.0) | 203.9 (179.7-219.8) | 692.7 | **3.48x** (2.79-4.15x, 7/7) | 3.00x (2.44-3.56x, 7/7) | 200 tuples, 12,505 items | 258.7 / 258.8 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| te_collapse_data | 200 | 66.6 | 88.5 (78.5-93.2) | 30.7 (28.0-42.2) | 89.5 | **2.80x** (2.21-2.95x, 7/7) | 2.38x (1.39-3.67x, 7/7) | 200 tuples, 1,653 items | 44.3 / 44.5 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| tf_collapse_data | 40 | 23,270.8 | 26,504.4 (23,859.9-27,924.5) | 6,196.2 (5,233.2-8,375.1) | 24,717.8 | **4.28x** (3.16-5.34x, 7/7) | 3.99x (2.60-4.45x, 7/7) | 40 tuples, 173,693 items | 46.3 / 52.6 | fn-spec 1,400/1,400 specialised; fn-fallback 1,400/1,400 residual |
| vi_collapse_data | 200 | 292.4 | 357.9 (314.8-534.6) | 77.6 (66.3-119.5) | 319.0 | **4.26x** (3.03-7.16x, 7/7) | 3.75x (2.58-5.12x, 7/7) | 200 tuples, 9,835 items | 45.8 / 46.3 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |

Route (CeTTa match backend and native storage of the data space before -> after the timed batches, all modes):

- gv_collapse_data: native rows+dense 82,177+46,399 -> 82,177+46,399
- rs_live_hypotheses: native rows+dense 125,981+0 -> 125,981+0
- te_collapse_data: native rows+dense 82,177+46,399 -> 82,177+46,399
- tf_collapse_data: native rows+dense 82,177+46,399 -> 82,177+46,399
- vi_collapse_data: native rows+dense 112,164+16,412 -> 112,164+16,412

## stream-data (survey rank 2)

| site | queries | fn-generic us | fn-hosted us | **fn-spec** us | fn-fallback us | **fn-spec vs fn-hosted, paired** | fn-spec vs fn-generic, paired | answers per batch | peak RSS fn-spec / fn-hosted MiB | path (all runs) |
|---|---|---|---|---|---|---|---|---|---|---|
| gv_stream_data | 200 | 193.3 | 229.0 (209.2-304.6) | 78.7 (69.9-85.6) | 227.4 | **3.20x** (2.52-3.60x, 7/7) | 2.43x (2.17-2.99x, 7/7) | 7,377 | 44.1 / 44.4 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| rs_mil_run | 200 | 21.7 | 37.3 (35.4-42.4) | 22.8 (21.6-27.3) | 41.9 | **1.69x** (1.30-1.87x, 7/7) | 1.00x (0.79-1.24x, 3/7) | 201 | 42.1 / 42.0 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| te_stream_data | 200 | 62.9 | 76.8 (73.7-86.5) | 34.0 (29.3-39.3) | 89.0 | **2.25x** (1.88-2.79x, 7/7) | 1.85x (1.61-2.96x, 7/7) | 1,653 | 44.0 / 44.2 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| tf_stream_data | 40 | 25,681.9 | 30,624.3 (23,741.0-44,216.9) | 8,832.5 (8,242.7-11,193.6) | 29,280.4 | **2.88x** (2.17-5.01x, 7/7) | 2.78x (2.24-3.47x, 7/7) | 173,693 | 48.8 / 51.9 | fn-spec 1,400/1,400 specialised; fn-fallback 1,400/1,400 residual |
| vi_stream_data | 200 | 297.7 | 332.4 (297.0-361.8) | 100.3 (86.0-148.9) | 333.2 | **3.42x** (2.11-3.93x, 7/7) | 2.83x (2.01-3.53x, 7/7) | 9,835 | 46.3 / 46.2 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |

Route (CeTTa match backend and native storage of the data space before -> after the timed batches, all modes):

- gv_stream_data: native rows+dense 82,177+46,399 -> 82,177+46,399
- rs_mil_run: native rows+dense 109,210+0 -> 109,210+0
- te_stream_data: native rows+dense 82,177+46,399 -> 82,177+46,399
- tf_stream_data: native rows+dense 82,177+46,399 -> 82,177+46,399
- vi_stream_data: native rows+dense 112,164+16,412 -> 112,164+16,412

## collapse-filter (survey rank 3)

| site | queries | fn-generic us | fn-hosted us | **fn-spec** us | fn-fallback us | **fn-spec vs fn-hosted, paired** | fn-spec vs fn-generic, paired | answers per batch | peak RSS fn-spec / fn-hosted MiB | path (all runs) |
|---|---|---|---|---|---|---|---|---|---|---|
| gv_collapse_filter | 200 | 1,091.4 | 1,097.8 (997.5-1,268.3) | 54.4 (53.1-62.5) | 1,015.6 | **18.85x** (16.74-23.44x, 7/7) | 18.77x (15.43-23.17x, 7/7) | 200 tuples, 7,268 items | 44.8 / 45.4 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| rs_base_rate_rows | 200 | 136.8 | 187.3 (167.7-196.3) | 38.4 (35.1-42.0) | 184.0 | **4.86x** (4.00-5.46x, 7/7) | 3.54x (3.26-4.86x, 7/7) | 200 tuples, 66 items | 90.7 / 91.6 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| te_collapse_filter | 200 | 270.8 | 297.2 (272.6-360.3) | 29.3 (27.9-34.5) | 304.7 | **9.95x** (7.91-12.28x, 7/7) | 9.29x (6.88-10.11x, 7/7) | 200 tuples, 1,620 items | 44.8 / 45.0 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| tf_collapse_filter | 40 | 126,383.4 | 131,071.7 (127,961.7-145,996.1) | 6,871.4 (5,764.8-12,634.7) | 131,959.3 | **19.06x** (11.56-23.58x, 7/7) | 18.38x (9.50-21.36x, 7/7) | 40 tuples, 173,693 items | 46.9 / 61.5 | fn-spec 1,400/1,400 specialised; fn-fallback 1,400/1,400 residual |
| vi_collapse_filter | 200 | 1,466.3 | 1,459.3 (1,306.8-1,648.1) | 81.7 (72.7-89.7) | 1,383.6 | **18.36x** (16.00-19.09x, 7/7) | 17.92x (14.10-21.06x, 7/7) | 200 tuples, 9,643 items | 46.6 / 47.2 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |

Route (CeTTa match backend and native storage of the data space before -> after the timed batches, all modes):

- gv_collapse_filter: native rows+dense 82,177+46,399 -> 82,177+46,399
- rs_base_rate_rows: native rows+dense 105,107+0 -> 105,107+0
- te_collapse_filter: native rows+dense 82,177+46,399 -> 82,177+46,399
- tf_collapse_filter: native rows+dense 82,177+46,399 -> 82,177+46,399
- vi_collapse_filter: native rows+dense 112,164+16,412 -> 112,164+16,412

## stream-filter (the probe's shape (survey rank 8), kept for the probe sites)

| site | queries | fn-generic us | fn-hosted us | **fn-spec** us | fn-fallback us | **fn-spec vs fn-hosted, paired** | fn-spec vs fn-generic, paired | answers per batch | peak RSS fn-spec / fn-hosted MiB | path (all runs) |
|---|---|---|---|---|---|---|---|---|---|---|
| gv_stream_filter | 200 | 976.6 | 1,096.2 (968.2-1,567.6) | 83.8 (65.8-132.2) | 1,007.5 | **12.75x** (11.78-14.71x, 7/7) | 11.57x (7.10-14.84x, 7/7) | 7,268 | 44.0 / 44.6 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| te_stream_filter | 200 | 259.4 | 281.8 (252.3-311.3) | 33.3 (31.1-35.8) | 283.0 | **8.51x** (7.25-10.01x, 7/7) | 7.60x (6.96-10.42x, 7/7) | 1,620 | 44.6 / 44.9 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |
| tf_stream_filter | 40 | 123,641.9 | 132,531.1 (129,707.8-138,890.5) | 9,078.0 (7,686.5-9,632.9) | 128,225.2 | **15.07x** (13.64-17.34x, 7/7) | 13.69x (12.59-16.47x, 7/7) | 173,693 | 48.6 / 61.4 | fn-spec 1,400/1,400 specialised; fn-fallback 1,400/1,400 residual |
| vi_stream_filter | 200 | 1,448.6 | 1,679.1 (1,288.8-1,855.2) | 111.6 (91.5-143.2) | 1,532.9 | **13.67x** (11.10-19.24x, 7/7) | 12.32x (9.87-16.88x, 7/7) | 9,643 | 46.3 / 46.9 | fn-spec 7,000/7,000 specialised; fn-fallback 7,000/7,000 residual |

Route (CeTTa match backend and native storage of the data space before -> after the timed batches, all modes):

- gv_stream_filter: native rows+dense 82,177+46,399 -> 82,177+46,399
- te_stream_filter: native rows+dense 82,177+46,399 -> 82,177+46,399
- tf_stream_filter: native rows+dense 82,177+46,399 -> 82,177+46,399
- vi_stream_filter: native rows+dense 112,164+16,412 -> 112,164+16,412

## Checks

- every site: the same answers per batch (and answer items) in all four modes; fn-spec specialised every timed call; fn-fallback took the residual on every timed call

