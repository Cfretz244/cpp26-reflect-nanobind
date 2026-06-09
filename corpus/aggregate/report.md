# Corpus report

Total runs: **5**

## Outcomes (overall)

| outcome | count | meaning |
|---|---|---|
| E | 4 | clean success (behavioral) |
| B | 1 | binding failed to compile |

## Outcomes by tier

| tier | E | E-weak | D | C | B | A |
|---|---|---|---|---|---|---|
| 0 | 2 | 0 | 0 | 0 | 0 | 0 |
| 1 | 1 | 0 | 0 | 0 | 1 | 0 |
| 4 | 1 | 0 | 0 | 0 | 0 | 0 |

## Per-run

| slug | tier | outcome | gate | strategy | pin | notes |
|---|---|---|---|---|---|---|
| _fixture_pod | 0 | E | 6 | single_stage |  |   |
| linalg | 0 | E | 6 | single_stage | v2.2 |   |
| glm | 1 | E | 6 | single_stage | 1.0.1 |   |
| json | 1 | B | 1 | two_stage | v3.11.3 | B.gen_compile  |
| _fixture_virtual | 4 | E | 6 | two_stage |  |   |

