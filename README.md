## 简介 
本代码为初赛评测程序样例，旨在方便参赛者在提交代码前进行性能自测。

** 注：此样例不等同于实际线上评测逻辑，相对实际评测代码仅保留性能评测部分并做了简化 **

## 如何使用
1. 在项目目录下clone自己fork并实现的engine仓库
如：

```
git clone git@code.aliyun.com:XXX/engine.git

```

2. 执行 make

3. 运行

```

sudo TEST_TMPDIR=./test_benchmark ./benchmark

```

将TEST\_TMPDIR 设置为实际db\_path即可

更多参数具体用法见benchmark.cc

*本样例代码参考RocksDB db_bench_tool实现*

# 测试机器
+ 4C8G
+ disk
    + read: 200MB/s
    + write: 150MB/s

## leveldb
fillrandom   :     990.074 micros/op 1010 ops/sec;    4.0 MB/s
Percentiles: P50: 721.23 P75: 2313.12 P99: 161971.90 P99.9: 16285714.29 P99.99: 17500351.00

readrandom   :      99.903 micros/op 10009 ops/sec;   39.2 MB/s (10000 of 10000 found)
Percentiles: P50: 948.85 P75: 1551.10 P99: 211216.27 P99.9: 776419.21 P99.99: 1467010.31

## v1
+ 功能
    + 无聚合写
    + 无缓存读
    + 无Compaction
+ 结果
```
fillrandom   :    3102.406 micros/op 322 ops/sec;    1.3 MB/s
Percentiles: P50: 658.38 P75: 6742.56 P99: 15578.89 P99.9: 58893048.13 P99.99: 238849557.52
```
