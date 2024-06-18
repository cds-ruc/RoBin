# Workload Description

| workload | dataset | bulkload | insert | lookup | note |
| --- | --- | --- | --- | --- | --- |
| 99 | shuffled | first substring | random point | random point | baseline of random point |
| 999 | shuffled  | first substring | sorted append | random point | baseline of sorted append |
| 9999 | shuffled | all dataset | NO | random point | best lookup performance in theory |
| 99999 | shuffled | null | random point | random point |  |
| 999999 | sorted | null | sorted append | random point |  |
| 1 | sorted | random substring | random point | random point |  |
| 2 | sorted | first substring + last key | random point | random point |  |
| 3 | sorted | random substring + smallest(leftmost) & biggest(rightmost) key | random point | random point |  |
| 4 | sorted | 80% random substring + 20% sampling the rest via normal distribution | random point | random point |  |
| 5 | sorted | generated pseudo-data via Kernel Density Estimation / 插值法补全 | TBD | TBD | TBD |
| 6 | sorted | sampling via gaussian distribution  | random point | random point |  |
| 7 | sorted | random substring | sorted append | random point |  |
| 8 | sorted | first substring + last key | sorted append | random point |  |