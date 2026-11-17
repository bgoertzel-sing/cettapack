# Sites

One pack per site under `packs/<site>/` (`site.metta`, generated
`kernel_plan.c` and `manifest.json`); one host binary per site,
`build/cettapack-host-<site>` (`make all`). The site name in `site.metta` is
`cpk-site-<dir>`, which gives the plan symbol `cpk_plan_cpk_site_<dir>` (M1's
site keeps its hyphenated name, `cpk-site-target-expressed`).

| site | region | seed | why it is here |
|---|---|---|---|
| `target_expressed` | `(match $space (, (target $rs $gene) (expressed $gene $tissue)) ($gene $tissue))` | variant | M1's site |
| `gene_variants` | `(match $space (, (target $rs $gene) (expressed $gene $tissue)) ($rs $tissue))` | gene | option B: the selective field is the second field of `target` |
| `tissue_fanout` | `(match $space (, (expressed $gene $tissue) (target $rs $gene)) ($rs $gene))` | tissue | option B: selective field not first, and about 4,300 answers per query |
| `variant_interactor` | `(match $space (, (target $rs $g1) (interacts $g2 $g1) (expressed $g2 $tissue)) ($g2 $tissue))` | variant | option B: a three-way join through the second field of `interacts` |

## Key layout per factor (cpkc)

M1's index stored each record as its own key, `((target rs5 gene7) <gid>)`, and
narrowed by descending the trie on the ground fields up to the first unbound
one. A ground field after an unbound one (the seed in `(target $rs gene7)`)
then gave no prefix and the kernel would walk every `target` key.

`tools/cpkc.py` now chooses a key layout per factor (in join order): the head,
then the fields that are ground when the factor is reached (constants, inputs,
slots bound by earlier factors) in source order, then the rest
(`CpkFactor.key_order`). An identity layout keeps M1's key (the record itself);
any other layout is stored under a factor-specific head, `((cpk0_target gene7
rs5) <gid>)`. Factors with the same layout share keys. The keys only select
candidates: every answer still comes from CeTTa's seeded rematch on the source
record, and multiplicity from the source records, as in M1.

Cost: one more key per record and permuted layout (variant_interactor's
`interacts` layout takes its peak RSS from 565 to 818 MiB).

## Seeds

`bench/gen_kb.py --query-kind rs|gene|tissue` draws the seeds after the facts, so
the KB bytes are the same for every kind (sha256 `8097f2f4...5439f5c` with the
defaults). 5% of the seeds are absent from the KB.

## Tests per site

`cettapack-host-<site> difftest [--kb FILE --queries FILE]` (run for every site by
`make test` / `make difftest`): bag and support demand on four small spaces with
every symbol as a seed, then every seed of the site's query file on the
benchmark KB; each call must take the kernel path (guard VALID, no fallback)
and give generic CeTTa's bag. M1's guard and fallback cases (`test`) stay on M1's
site: they exercise the pack mechanics, which do not depend on the site.

The host's generic form of a site comes from the plan (`CpkPlan.generic_fmt`,
written by cpkc): the match body with each occurrence of the seed as `%1$s`.
