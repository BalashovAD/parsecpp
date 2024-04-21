## Benchmark results

### Recursion

#### Braces
##### Windows

| *                            | BM_bracesSuccess   | BM_bracesFailure   | Slowdown     |
|------------------------------|--------------------|--------------------|--------------|
| bracesLazy                   | 5580ns             | 10010ns            | 1.91x        |
| bracesSelfLazy               | 3924ns             | 3690ns             | 0.925x       |
| bracesCached                 | 4238ns             | 4011ns             | 1x           |
| bracesCacheConstexpr         | 4238ns             | 3990ns             | 1x           |
| bracesForget                 | 4238ns             | 3990ns             | 1x           |
| Ctx/bracesCtx                | 4262ns             | 4098ns             | 1.02x        |
| Ctx/bracesCtxConstexpr       | 4238ns             | 4035ns             | 1.01x        |
| ---------------------------- | ------------------ | ------------------ | ------------ |
| bracesCacheDropConstexpr     | 858ns              | 811ns              | 1x           |
| bracesCachedDrop             | 879ns              | 837ns              | 1.03x        |
| bracesCtxDrop                | 900ns              | 858ns              | 1.05x        |
| bracesCtxDropConstexpr       | 879ns              | 837ns              | 1.03x        |
#### Linux
| *                          | BM_bracesSuccess   | BM_bracesFailure   | Slowdown     |
|----------------------------|--------------------|--------------------|--------------|
| bracesLazy                 | 3039ns             | 2960ns             | 1.89x        |
| bracesSelfLazy             | 1277ns             | 1214ns             | 0.786x       |
| bracesCached               | 1665ns             | 1571ns             | 1.02x        |
| bracesCacheConstexpr       | 1635ns             | 1535ns             | 1x           |
| bracesForget               | 1316ns             | 1240ns             | 0.806x       |
| bracesCtx                  | 1662ns             | 1563ns             | 1.02x        |
| bracesCtxConstexpr         | 1714ns             | 1615ns             | 1.05x        |
| -------------------------- | ------------------ | ------------------ | ------------ |
| bracesCachedDrop           | 1042ns             | 977ns              | 1.05x        |
| bracesCacheDropConstexpr   | 986ns              | 929ns              | 1x           |
| bracesCtxDrop              | 1066ns             | 996ns              | 1.08x        |
| bracesCtxDropConstexpr     | 1006ns             | 949ns              | 1.02x        |

#### Json
#### Windows
| *          | Slowdown   | BM_jsonFile_100k | BM_jsonFile_canada | BM_jsonFile_binance | BM_jsonFile_64kb  | BM_jsonFile_64kb_min | BM_jsonFile_256kb  | BM_jsonFile_256kb_min | BM_jsonFile_5mb  | BM_jsonFile_5mb_min |
|------------|------------|------------------|--------------------|---------------------|-------------------|----------------------|--------------------|-----------------------|------------------|---------------------|
| Lazy       | 2.45x      | 1843.16us        | 109.375ms          | 2589.03us           | 444.984us         | 707.929us            | 2886.55us          | 2761.04us             | 62.5ms           | 60.9375ms           |
| LazyCached | 1x         | 500us            | 25.6696ms          | 1037.6us            | 360.947us         | 336.967us            | 1395.09us          | 1311.38us             | 31.25ms          | 29.2969ms           |
| SelfLazy   | 1.08x      | 530.134us        | 27.5ms             | 1098.63us           | 383.65us          | 368.968us            | 1499.72us          | 1411.9us              | 34.5395ms        | 32.7381ms           |

#### Linux
| *          | Slowdown   | BM_jsonFile_100k | BM_jsonFile_canada | BM_jsonFile_binance | BM_jsonFile_64kb  | BM_jsonFile_64kb_min | BM_jsonFile_256kb  | BM_jsonFile_256kb_min | BM_jsonFile_5mb  | BM_jsonFile_5mb_min |
|------------|------------|------------------|--------------------|---------------------|-------------------|----------------------|--------------------|-----------------------|------------------|---------------------|
| Lazy       | 4.58x      | 2792.89us        | 137.948ms          | 6208.52us           | 1072.7us          | 1043.96us            | 4274.46us          | 4168.02us             | 86.9719ms        | 85.6653ms           |
| LazyCached | 1x         | 639.062us        | 22.5438ms          | 1174.18us           | 234.944us         | 215.955us            | 978.728us          | 887.111us             | 25.4715ms        | 23.8087ms           |
| SelfLazy   | 1.33x      | 825.702us        | 32.9463ms          | 1664.6us            | 311.856us         | 288.252us            | 1274.35us          | 1193.36us             | 31.1524ms        | 29.3887ms           |

## Spec
### Linux
```
AMD Ryzen 7 5700U, clang-18.0-pre
Run on (16 X 4369.92 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 512 KiB (x8)
  L3 Unified 4096 KiB (x2)
```
### Windows
```
AMD Ryzen 7 3700X, minGW-12
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 512 KiB (x8)
  L3 Unified 16384 KiB (x2)
```