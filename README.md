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
```
fillrandom   :     990.074 micros/op 1010 ops/sec;    4.0 MB/s
Percentiles: P50: 721.23 P75: 2313.12 P99: 161971.90 P99.9: 16285714.29 P99.99: 17500351.00

readrandom   :      99.903 micros/op 10009 ops/sec;   39.2 MB/s (10000 of 10000 found)
Percentiles: P50: 948.85 P75: 1551.10 P99: 211216.27 P99.9: 776419.21 P99.99: 1467010.31

Average:          DEV       tps  rd_sec/s  wr_sec/s  avgrq-sz  avgqu-sz     await     svctm     %util
Average:          sda     83.04     96.72  46401.44    559.95      1.03     12.36      9.95     82.64
```

## v1
+ 功能
    + 无聚合写
    + 无缓存读
    + 无Compaction
+ 结果
```
fillrandom   :      53.465 micros/op 18703 ops/sec;   73.2 MB/s
Percentiles: P50: 1062.33 P75: 1203.11 P99: 44562.70 P99.9: 111052.63 P99.99: 2900000.00
```
+ 瓶颈: `sar -pd 1 10`
```
Average:          DEV       tps  rd_sec/s  wr_sec/s  avgrq-sz  avgqu-sz     await     svctm     %util
Average:          sda    277.30     71.20 364848.80   1315.98    133.03    461.11      3.45     95.60
```

## v2
+ 同步聚合写
+ 8byte key 128byte value
+ before
```
08:30:41 PM       DEV       tps  rd_sec/s  wr_sec/s  avgrq-sz  avgqu-sz     await     svctm     %util
08:34:22 PM       sda    129.00      0.00 317128.00   2458.36     48.14    540.34      6.20     80.00

fillrandom   :       5.666 micros/op 176485 ops/sec;   22.9 MB/s
Percentiles: P50: 3.56 P75: 3.87 P99: 8892.35 P99.9: 26412.63 P99.99: 49807.33

CPU没有打满，40%，但是带宽才用23MB
```
+ after
```
fillrandom   :       2.920 micros/op 342463 ops/sec;   44.4 MB/s
Percentiles: P50: 130.01 P75: 204.54 P99: 724.11 P99.9: 4885.75 P99.99: 12302.44

08:30:41 PM       DEV       tps  rd_sec/s  wr_sec/s  avgrq-sz  avgqu-sz     await     svctm     %util
11:42:22 PM       sda    156.00      0.00 322896.00   2069.85     63.97    483.33      5.36     83.60

CPU已经70%了，但是磁盘依旧没有满，磁盘等待队列还是太长
```
