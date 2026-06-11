# Corpus report

Total runs: **36**

## Outcomes (overall)

| outcome | count | meaning |
|---|---|---|
| E | 36 | clean success (behavioral) |

## Outcomes by tier

| tier | E | E-weak | D | C | B | A |
|---|---|---|---|---|---|---|
| 0 | 3 | 0 | 0 | 0 | 0 | 0 |
| 1 | 2 | 0 | 0 | 0 | 0 | 0 |
| 2 | 4 | 0 | 0 | 0 | 0 | 0 |
| 3 | 9 | 0 | 0 | 0 | 0 | 0 |
| 4 | 7 | 0 | 0 | 0 | 0 | 0 |
| 5 | 1 | 0 | 0 | 0 | 0 | 0 |
| 6 | 10 | 0 | 0 | 0 | 0 | 0 |

## Per-run

| slug | tier | outcome | constexpr | emit | surface | gate | strategy | pin | notes |
|---|---|---|---|---|---|---|---|---|---|
| _fixture_pod | 0 | E | E | E | pass | 6 | single_stage |  |  surface_diff: 0 mismatch(es), 4 top-level object(s) compared; |
| _fixture_recursive | 0 | E | E | E | pass | 6 | single_stage |  |  surface_diff: 0 mismatch(es), 1 top-level object(s) compared; |
| linalg | 0 | E | E | - | skip | 6 | single_stage | v2.2 |   |
| glm | 1 | E | E | E | pass | 6 | single_stage | 1.0.1 |  surface_diff: 0 mismatch(es), 2 top-level object(s) compared; |
| json | 1 | E | E | E | pass | 6 | two_stage | v3.11.3 |  surface_diff: 0 mismatch(es), 22 top-level object(s) compared; |
| date | 2 | E | E | - | skip | 6 | single_stage | v3.0.4 |   |
| fast_float | 2 | E | E | - | skip | 6 | single_stage | v8.2.8 |   |
| fmt | 2 | E | E | - | skip | 6 | single_stage | 11.2.0 |   |
| tinyobjloader | 2 | E | E | - | skip | 6 | single_stage | v1.0.7 |   |
| cli11 | 3 | E | E | - | skip | 6 | single_stage | v2.6.2 |   |
| concurrentqueue | 3 | E | E | - | skip | 6 | single_stage | v1.0.5 |   |
| expected | 3 | E | E | - | skip | 6 | single_stage | v1.3.1 |   |
| pugixml | 3 | E | E | - | skip | 6 | single_stage | v1.15 |   |
| simdjson | 3 | E | E | - | skip | 6 | single_stage | v4.6.4 |   |
| spdlog | 3 | E | E | - | skip | 6 | single_stage | v1.17.0 |   |
| tomlplusplus | 3 | E | E | - | skip | 6 | single_stage | v3.4.0 |   |
| unordered_dense | 3 | E | E | - | skip | 6 | single_stage | v4.8.1 |   |
| yamlcpp | 3 | E | E | - | skip | 6 | single_stage | yaml-cpp-0.9.0 |   |
| _fixture_virtual | 4 | E | E | E | pass | 6 | two_stage |  |  surface_diff: 0 mismatch(es), 4 top-level object(s) compared; |
| box2d | 4 | E | E | - | skip | 6 | single_stage | v2.4.2 |   |
| httplib | 4 | E | E | - | skip | 6 | single_stage | v0.47.0 |   |
| immer | 4 | E | E | - | skip | 6 | single_stage | v0.9.1 |   |
| leveldb | 4 | E | E | - | skip | 6 | single_stage | 1.23 |   |
| sqlitecpp | 4 | E | E | - | skip | 6 | single_stage | 3.3.3 |   |
| taskflow | 4 | E | E | - | skip | 6 | single_stage | v4.0.0 |   |
| eigen | 5 | E | E | - | skip | 6 | single_stage | 5.0.1 |   |
| abseil_btree | 6 | E | E | - | skip | 6 | single_stage | 20250814.2 |   |
| abseil_civil_tz | 6 | E | E | - | skip | 6 | single_stage | 20250814.2 |   |
| abseil_containers | 6 | E | E | - | skip | 6 | single_stage | 20250814.2 |   |
| abseil_crc | 6 | E | E | - | skip | 6 | single_stage | 20250814.2 |   |
| abseil_hash | 6 | E | E | - | skip | 6 | single_stage | 20250814.2 |   |
| abseil_numeric | 6 | E | E | - | skip | 6 | single_stage | 20250814.2 |   |
| abseil_status | 6 | E | E | - | skip | 6 | single_stage | 20250814.2 |   |
| abseil_statusor | 6 | E | E | - | skip | 6 | single_stage | 20250814.2 |   |
| abseil_strings | 6 | E | E | - | skip | 6 | single_stage | 20250814.2 |   |
| abseil_time | 6 | E | E | - | skip | 6 | single_stage | 20250814.2 |   |

## Emit lane (production-toolchain source codegen)

Runs with an emit lane: **5** / 36; outcomes: E=5

