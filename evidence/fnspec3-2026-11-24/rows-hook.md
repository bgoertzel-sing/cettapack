# match-rows row hook (0004) vs embedding callback

fn-spec: the compiled function as an embedding callback (cpk-fnspec:f); fn-rows: the same generated code inside match-rows through the 0004 row hook (cpk-rows:f). Both run the same guards (program token / definition check, argument checks), the same row function (condition, template, per-answer normal-form check) and the same first-row cut. Paired: run i of fn-spec over run i of fn-rows; > 1 means the hook is faster.

## evidence/fnspec3-2026-11-24/hook-bench

| site | shape | queries | fn-spec us | **fn-rows us** | **fn-rows vs fn-spec, paired** | answers per batch | peak RSS fn-rows / fn-spec MiB | path (all runs) |
|---|---|---|---|---|---|---|---|---|
| gv_collapse_cond | collapse-cond | 200 | 44.0 (41.1-52.2) | **39.7** (34.1-43.2) | **1.11x** (1.02-1.31x, 7/7) | 200 | 44.9 / 45.0 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| gv_collapse_data | collapse-data | 200 | 52.6 (49.8-61.9) | **45.7** (42.3-68.5) | **1.22x** (0.77-1.36x, 5/7) | 200 | 44.8 / 44.8 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| gv_collapse_filter | collapse-filter | 200 | 56.8 (50.6-60.2) | **49.1** (44.0-55.4) | **1.14x** (0.94-1.37x, 6/7) | 200 | 45.3 / 45.1 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| gv_exist_cond | exist-cond | 200 | 38.7 (33.7-41.2) | **33.8** (29.5-74.8) | **1.03x** (0.55-1.31x, 5/7) | 200 | 45.4 / 45.5 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| gv_exist_data | exist-data | 200 | 40.2 (36.5-65.7) | **37.7** (31.9-68.6) | **1.06x** (0.69-1.89x, 4/7) | 200 | 44.9 / 44.9 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| gv_exist_filter | exist-filter | 200 | 38.9 (37.0-69.8) | **37.0** (35.2-74.5) | **1.09x** (0.50-1.81x, 5/7) | 200 | 45.2 / 45.4 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| gv_once_cond | once-cond | 200 | 41.4 (39.4-47.7) | **36.3** (34.5-77.7) | **1.12x** (0.52-1.27x, 6/7) | 200 | 45.5 / 45.4 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| gv_once_data | once-data | 200 | 42.3 (36.4-47.5) | **35.9** (32.6-56.1) | **1.13x** (0.79-1.26x, 6/7) | 200 | 44.8 / 44.9 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| gv_once_filter | once-filter | 200 | 41.9 (39.1-63.7) | **35.6** (32.1-38.1) | **1.13x** (1.10-1.93x, 7/7) | 200 | 44.3 / 44.1 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| gv_stream_cond | stream-cond | 200 | 88.5 (80.9-117.1) | **79.0** (73.0-129.2) | **1.14x** (0.91-1.21x, 5/7) | 7,268 | 44.9 / 45.0 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| gv_stream_data | stream-data | 200 | 80.5 (70.3-132.3) | **67.7** (58.6-131.4) | **1.14x** (0.60-1.48x, 6/7) | 7,377 | 44.4 / 44.4 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| gv_stream_filter | stream-filter | 200 | 126.5 (68.0-160.0) | **75.4** (66.4-124.5) | **1.27x** (0.89-2.27x, 6/7) | 7,268 | 44.2 / 44.4 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| rs_base_rate_rows | collapse-filter | 200 | 37.7 (36.6-79.2) | **28.4** (26.3-56.1) | **1.36x** (0.73-2.33x, 6/7) | 200 | 90.9 / 91.2 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| rs_compound_connective | exist-data | 200 | 23.2 (22.0-40.6) | **14.5** (12.6-24.1) | **1.75x** (0.92-2.34x, 6/7) | 200 | 22.0 / 21.8 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| rs_given_rule | exist-data | 200 | 24.1 (20.3-41.6) | **14.7** (13.9-28.7) | **1.62x** (0.95-1.71x, 6/7) | 200 | 76.9 / 76.7 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| rs_hm_recurs | once-cond | 200 | 41.5 (39.9-46.4) | **32.0** (25.2-42.5) | **1.30x** (1.09-1.65x, 7/7) | 200 | 63.1 / 63.1 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| rs_live_hypotheses | collapse-data | 200 | 216.0 (192.2-242.2) | **174.4** (139.7-199.4) | **1.24x** (1.09-1.73x, 7/7) | 200 | 259.1 / 259.0 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| rs_mil_run | stream-data | 200 | 25.1 (22.1-26.2) | **21.5** (18.4-33.7) | **1.10x** (0.71-1.33x, 4/7) | 201 | 42.2 / 42.1 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| rs_sk_left_out | stream-cond | 200 | 47.1 (39.6-73.0) | **31.7** (29.5-33.2) | **1.47x** (1.33-2.25x, 7/7) | 700 | 102.8 / 103.0 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| rs_sk_needed | exist-filter | 200 | 61.1 (46.8-76.3) | **45.4** (34.5-85.5) | **1.20x** (0.58-2.18x, 4/7) | 200 | 50.4 / 50.3 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| rs_sk_needed_depth | once-filter | 200 | 52.8 (50.0-62.5) | **42.8** (37.4-85.2) | **1.32x** (0.61-1.46x, 5/7) | 200 | 49.9 / 49.9 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| rs_sk_row_present | exist-cond | 200 | 2,370.6 (2,059.3-3,323.0) | **2,560.7** (2,037.7-3,209.6) | **1.03x** (0.64-1.30x, 5/7) | 200 | 72.1 / 72.1 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| rs_sk_summary_memo | once-cond | 200 | 1,115.5 (984.7-1,489.0) | **1,024.9** (943.6-1,148.2) | **1.09x** (0.86-1.58x, 6/7) | 200 | 39.7 / 39.7 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| rs_stored_proof_tvs | once-data | 200 | 22.1 (21.0-37.2) | **14.6** (12.7-30.2) | **1.57x** (0.80-2.02x, 6/7) | 200 | 50.8 / 50.8 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| te_collapse_cond | collapse-cond | 200 | 27.5 (24.0-29.3) | **23.4** (19.9-29.6) | **1.19x** (0.91-1.21x, 6/7) | 200 | 45.0 / 44.8 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| te_collapse_data | collapse-data | 200 | 30.3 (25.8-32.7) | **24.8** (21.5-26.8) | **1.21x** (1.03-1.35x, 7/7) | 200 | 44.6 / 44.3 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| te_collapse_filter | collapse-filter | 200 | 30.1 (24.4-56.7) | **24.2** (21.9-25.8) | **1.28x** (1.03-2.27x, 7/7) | 200 | 45.2 / 45.1 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| te_exist_cond | exist-cond | 200 | 25.1 (21.0-28.3) | **20.6** (17.9-39.0) | **1.19x** (0.54-1.52x, 5/7) | 200 | 45.3 / 45.1 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| te_exist_data | exist-data | 200 | 24.1 (23.5-45.1) | **19.6** (17.1-33.8) | **1.34x** (0.79-2.30x, 6/7) | 200 | 44.1 / 44.3 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| te_exist_filter | exist-filter | 200 | 22.1 (20.9-24.0) | **17.8** (16.3-35.9) | **1.23x** (0.66-1.46x, 6/7) | 200 | 45.3 / 45.2 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| te_once_cond | once-cond | 200 | 27.1 (25.6-54.3) | **20.9** (19.0-43.5) | **1.27x** (0.79-2.85x, 6/7) | 200 | 44.2 / 44.2 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| te_once_data | once-data | 200 | 27.7 (24.0-53.1) | **19.8** (17.5-37.8) | **1.37x** (0.79-2.72x, 6/7) | 200 | 44.0 / 44.1 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| te_once_filter | once-filter | 200 | 24.1 (22.8-31.2) | **20.4** (18.2-21.2) | **1.26x** (1.12-1.72x, 7/7) | 200 | 44.4 / 44.4 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| te_stream_cond | stream-cond | 200 | 37.9 (31.5-55.8) | **32.5** (27.7-34.0) | **1.18x** (0.98-1.91x, 6/7) | 1,620 | 44.8 / 44.8 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| te_stream_data | stream-data | 200 | 31.2 (28.7-35.5) | **27.6** (25.5-53.3) | **1.18x** (0.58-1.39x, 5/7) | 1,653 | 44.2 / 44.2 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| te_stream_filter | stream-filter | 200 | 30.6 (29.3-33.8) | **27.2** (25.2-31.2) | **1.12x** (1.01-1.25x, 7/7) | 1,620 | 44.9 / 44.8 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| tf_collapse_cond | collapse-cond | 40 | 3,733.2 (2,647.0-4,258.0) | **3,239.3** (3,197.3-3,535.2) | **1.15x** (0.75-1.29x, 4/7) | 40 | 45.1 / 45.3 | fn-spec 1,400/1,400, fn-rows 1,400/1,400 specialised |
| tf_collapse_data | collapse-data | 40 | 5,980.9 (5,148.0-6,339.0) | **5,443.4** (4,724.9-6,003.7) | **1.06x** (0.98-1.15x, 6/7) | 40 | 46.4 / 46.4 | fn-spec 1,400/1,400, fn-rows 1,400/1,400 specialised |
| tf_collapse_filter | collapse-filter | 40 | 6,691.3 (5,361.6-7,302.8) | **5,720.9** (4,738.7-6,400.5) | **1.09x** (0.89-1.46x, 5/7) | 40 | 47.1 / 47.1 | fn-spec 1,400/1,400, fn-rows 1,400/1,400 specialised |
| tf_exist_cond | exist-cond | 40 | 2,666.7 (2,222.3-2,879.6) | **2,712.5** (2,212.1-4,149.4) | **1.04x** (0.66-1.24x, 4/7) | 40 | 45.7 / 45.8 | fn-spec 1,400/1,400, fn-rows 1,400/1,400 specialised |
| tf_exist_data | exist-data | 40 | 2,713.1 (2,198.9-3,257.8) | **2,668.9** (2,194.0-3,265.6) | **1.10x** (0.67-1.25x, 4/7) | 40 | 45.4 / 45.3 | fn-spec 1,400/1,400, fn-rows 1,400/1,400 specialised |
| tf_exist_filter | exist-filter | 40 | 3,053.9 (2,218.8-4,090.1) | **2,592.9** (2,330.9-4,263.8) | **0.86x** (0.72-1.65x, 3/7) | 40 | 45.7 / 45.5 | fn-spec 1,400/1,400, fn-rows 1,400/1,400 specialised |
| tf_once_cond | once-cond | 40 | 2,663.2 (2,300.6-2,829.0) | **2,588.8** (2,115.5-3,494.2) | **1.03x** (0.66-1.34x, 5/7) | 40 | 46.4 / 46.1 | fn-spec 1,400/1,400, fn-rows 1,400/1,400 specialised |
| tf_once_data | once-data | 40 | 2,503.5 (2,125.1-3,608.1) | **2,731.9** (2,644.2-6,291.0) | **0.83x** (0.57-0.93x, 0/7) | 40 | 45.0 / 45.2 | fn-spec 1,400/1,400, fn-rows 1,400/1,400 specialised |
| tf_once_filter | once-filter | 40 | 2,737.3 (2,160.0-2,810.1) | **2,756.8** (2,127.7-3,235.9) | **1.01x** (0.69-1.20x, 5/7) | 40 | 45.1 / 45.0 | fn-spec 1,400/1,400, fn-rows 1,400/1,400 specialised |
| tf_stream_cond | stream-cond | 40 | 9,975.8 (8,625.2-13,726.5) | **8,788.9** (8,364.7-11,708.9) | **1.13x** (1.03-1.19x, 7/7) | 173,693 | 49.5 / 49.7 | fn-spec 1,400/1,400, fn-rows 1,400/1,400 specialised |
| tf_stream_data | stream-data | 40 | 8,973.4 (8,190.6-12,704.7) | **9,516.3** (7,771.2-12,672.7) | **1.00x** (0.83-1.19x, 4/7) | 173,693 | 48.7 / 49.0 | fn-spec 1,400/1,400, fn-rows 1,400/1,400 specialised |
| tf_stream_filter | stream-filter | 40 | 8,979.6 (8,156.7-11,563.1) | **8,683.7** (7,170.4-11,554.0) | **1.03x** (0.95-1.30x, 5/7) | 173,693 | 48.3 / 48.8 | fn-spec 1,400/1,400, fn-rows 1,400/1,400 specialised |
| vi_collapse_cond | collapse-cond | 200 | 61.2 (53.1-68.5) | **58.7** (52.2-71.4) | **1.06x** (0.82-1.19x, 5/7) | 200 | 46.6 / 46.7 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| vi_collapse_data | collapse-data | 200 | 74.7 (66.2-90.0) | **67.2** (56.4-78.2) | **1.13x** (0.87-1.60x, 4/7) | 200 | 46.2 / 46.2 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| vi_collapse_filter | collapse-filter | 200 | 84.0 (81.4-85.4) | **75.0** (58.2-77.8) | **1.12x** (1.09-1.43x, 7/7) | 200 | 46.4 / 46.0 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| vi_exist_cond | exist-cond | 200 | 53.9 (50.9-60.7) | **45.1** (37.7-52.3) | **1.20x** (1.04-1.37x, 7/7) | 200 | 47.2 / 47.1 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| vi_exist_data | exist-data | 200 | 53.8 (43.4-62.9) | **46.8** (39.0-59.6) | **1.14x** (0.96-1.38x, 5/7) | 200 | 46.4 / 46.5 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| vi_exist_filter | exist-filter | 200 | 51.1 (45.1-57.5) | **44.2** (40.5-54.5) | **1.12x** (0.84-1.32x, 5/7) | 200 | 46.9 / 47.0 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| vi_once_cond | once-cond | 200 | 50.3 (48.0-60.8) | **45.7** (42.1-57.7) | **1.07x** (0.90-1.30x, 5/7) | 200 | 46.8 / 46.8 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| vi_once_data | once-data | 200 | 50.8 (46.5-98.4) | **52.7** (43.5-64.0) | **1.02x** (0.92-1.87x, 4/7) | 200 | 46.5 / 46.4 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| vi_once_filter | once-filter | 200 | 51.8 (50.2-58.3) | **46.6** (42.1-58.5) | **1.08x** (0.93-1.29x, 4/7) | 200 | 46.7 / 46.7 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| vi_stream_cond | stream-cond | 200 | 119.6 (101.9-181.9) | **102.0** (91.1-117.9) | **1.22x** (0.86-1.80x, 5/7) | 9,643 | 46.4 / 46.4 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| vi_stream_data | stream-data | 200 | 101.6 (98.8-108.4) | **93.6** (79.2-111.8) | **1.06x** (0.88-1.37x, 6/7) | 9,835 | 46.2 / 46.5 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |
| vi_stream_filter | stream-filter | 200 | 107.4 (98.4-132.1) | **93.2** (84.2-118.5) | **1.23x** (1.00-1.27x, 6/7) | 9,643 | 46.5 / 46.4 | fn-spec 7,000/7,000, fn-rows 7,000/7,000 specialised |

## Over all sites

- 60 sites; paired fn-rows vs fn-spec, median over sites 1.14x, range 0.83x-1.75x; faster on 58 of 60 sites (median ratio > 1)

## Checks

- every site: fn-spec and fn-rows give the same answers per batch (answer items, True answers, non-empty tuples); both specialised every timed call; no bad answers

