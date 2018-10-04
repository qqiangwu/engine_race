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
+ 将wait\_for\_room移动到了commit路径，原有方法所有写线程都会先竞争一次
```
fillrandom   :       2.600 micros/op 384546 ops/sec;   49.9 MB/s
Percentiles: P50: 125.06 P75: 178.52 P99: 565.30 P99.9: 6724.10 P99.99: 10098.75
```
+ 观察到CPU利用率每一段时间都会下降一下，同时，磁盘使用率也会下降一下，原因应该是dump造成写线程停顿

## v3
+ 异步聚合写：同步聚合写模型下，存在大量的缓存同步开销，因此使用异步聚合写模型
```
fillrandom   :       2.715 micros/op 368279 ops/sec;   47.8 MB/s
Percentiles: P50: 144.16 P75: 161.72 P99: 378.01 P99.9: 835.96 P99.99: 5505.84

Average:          DEV       tps  rd_sec/s  wr_sec/s  avgrq-sz  avgqu-sz     await     svctm     %util
Average:          sda    105.66    100.80 203033.28   1922.53     41.44    392.20      4.99     52.71

CPU利用率60%
```
+ 结果分析
    + v2与v3吞吐与平均延迟上没有太大差别，但是v3异步聚合的CPU占用更低，这是可以理解的，因为避免了大量的缓存一致性开销。
    + 除了对CPU占用的不同，异步聚合的毛刺更小，这是为什么？
        + 缓存一致性开销的消除，一定程度上降低了毛刺
        + CPU使用率的降低，使得请求者抢不到CPU的情况减少，进而降低毛刺
+ CPU利用率与磁盘利用率间接性下降的问题还没有解决，解决后毛刺还能进一步减小
+ 使用了多immutable memfile方案，但是性能并没有提高，毛刺反而变大了，推测是因为不持续的dump造成的影响。另一方面，还是会存在磁盘利用率drop的场景。初步怀疑是group commit时有时会提交过小的写包。
+ 在group commit中添加一蓄流逻辑，得到以下结果。可以看到，毛刺进一步下降了
```
fillrandom   :       2.792 micros/op 358141 ops/sec;   46.5 MB/s
Percentiles: P50: 144.10 P75: 161.63 P99: 378.77 P99.9: 846.57 P99.99: 4406.43
```

## v4
这个版本主要目标是一些小的优化，进一步降低毛刺。

+ imm释放的异步化
+ redo释放的异步化
+ redo的预分配：创建文件与分配空间
    + fallocate似乎没有什么效果

## v5
这个版本完善了异步处理，并且修复了一些并发方面的bug：主要是一个`pure virtual method called`的问题。同时，加了日志，发现了dump偶尔会特别慢，造成大量请求超时。

优化点：

+ batch提交时，如果要切换memfile，但是后台dump长时间没有完成，则会返回timeout重试。
+ 限制batch的大小，从而避免过长，造成后续请求长时间阻塞（这个还没有做，因为现在看来，主要问题在dump上）
+ redolog的gc，做完之后，毛刺降低很明显
    + 为了避免阻塞dumper，gc是后台完成的
```
fillrandom   :       3.263 micros/op 306500 ops/sec;   39.8 MB/s
Percentiles: P50: 212.69 P75: 234.32 P99: 517.87 P99.9: 575.34 P99.99: 2764.56
```

## v6
之前对leveldb做性能测试感觉有问题，这里对rocksdb做下随机写与随机读的测试
```
fillrandom   :       5.213 micros/op 191826 ops/sec;   24.9 MB/s
Percentiles: P50: 313.79 P75: 376.14 P99: 911.70 P99.9: 2547.00 P99.99: 8874.53

readrandom   :       4.632 micros/op 215880 ops/sec;   28.0 MB/s (1000000 of 1000000 found)
Percentiles: P50: 12.99 P75: 25.07 P99: 9606.84 P99.9: 33983.06 P99.99: 64137.13
```

测试中，发现rocksdb也有类似disk抖动的情况，之前做了多immutable memfile效果不大，但现在目测还是有必要再做一下的

## v7
Percentiles: P50: 212.69 P75: 234.32 P99: 517.87 P99.9: 575.34 P99.99: 2764.56
