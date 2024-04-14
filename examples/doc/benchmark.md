## Benchmark results

### Recursion

#### Braces
##### Windows

| *                          | BM_bracesSuccess | BM_bracesFailure | Slowdown |
|----------------------------|------------------|------------------|----------|
| bracesLazy                 | 13253ns          | 12835ns          | 2.52x    |
| bracesSelfLazy             | 4708ns           | 4501ns           | 0.888x   |
| bracesCached               | 5469ns           | 5156ns           | 1.02x    |
| bracesCacheConstexpr       | 5371ns           | 5000ns           | 1x       |
| bracesForget               | 5720ns           | 5313ns           | 1.06x    |
| bracesCtx                  | 5469ns           | 5156ns           | 1.02x    |
| bracesCtxConstexpr         | 5625ns           | 5441ns           | 1.07x    |
| -------------------------- | ---------------- | ---------------- | -------- |
| bracesCachedDrop           | 1074ns           | 942ns            | 1.06x    |
| bracesCacheDropConstexpr   | 1004ns           | 900ns            | 1x       |
| bracesCtxDrop              | 1025ns           | 952ns            | 1.04x    |
| bracesCtxDropConstexpr     | 1147ns           | 1088ns           | 1.18x    |


#### Json
#### Windows

| *          | Slowdown   | BM_jsonFile_100k | BM_jsonFile_canada | BM_jsonFile_binance | BM_jsonFile_64kb  | BM_jsonFile_64kb_min | BM_jsonFile_256kb  | BM_jsonFile_256kb_min | BM_jsonFile_5mb  | BM_jsonFile_5mb_min |
|------------|------------|------------------|--------------------|---------------------|-------------------|----------------------|--------------------|-----------------------|------------------|---------------------|
| Lazy       | 2.24x      | 2604.17us        | 156.25ms           | 5859.38us           | 1024.93us         | 1000.98us            | 4087.94us          | 3928.07us             | 84.8214ms        | 84.8214ms           |
| LazyCached | 1x         | 1159.67us        | 63.9205ms          | 2455.36us           | 470.948us         | 449.219us            | 1843.16us          | 1727.58us             | 41.9922ms        | 39.5221ms           |
| SelfLazy   | 3.52x      | 4087.94us        | 221.354ms          | 9375us              | 1650.8us          | 1650.8us             | 6597.22us          | 6423.61us             | 131.25ms         | 127.604ms           |


## Spec
### linux
```
AMD Ryzen 7 5700U, linux gcc 12.3.0 compiler, clang 15.0.7 linker
Run on (16 X 4369.92 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 512 KiB (x8)
  L3 Unified 4096 KiB (x2)
```
### windows
```
AMD Ryzen 7 3700X, minGW-12
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 512 KiB (x8)
  L3 Unified 16384 KiB (x2)
```