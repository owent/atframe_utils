OWenT’s Utils - Algorithm
=============

哈希算法
------
####Hash.h -- 提供了几个简单的Hash函数
更复杂的可以考虑以下几个开源库:

1. [Crypto++](http://www.cryptopp.com/) (License: Boost Software License 1.0)
2. [OpenSSL](http://www.openssl.org/) (License: OpenSSL License, Apache-style licence)
3. [Libtom](http://www.libtom.org) 的libtomcrypt部分 (License: DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE, 尼玛以前一直有耳闻，今天终于见到了这个协议的说)
4. [PolarSSL/mbed TLS](https://tls.mbed.org/) 用于嵌入式比较轻量级的加密库


面向特定应用场景的Hash算法
------

1. [MurmurHash](https://github.com/aappleby/smhasher) 对连续输入有良好散列结果并且性能不错的Hash算法（redis用的是MurmurHash2）
2. [CityHash](https://code.google.com/p/cityhash/) Google受MurmurHash启发搞出来的新Hash算法，未对小字符串做优化，性能更高一点点，但是实现更为复杂
3. [FarmHash](https://code.google.com/p/farmhash/) 还是Google搞出来的更新新Hash算法，官方说比CityHash性能还会高一点点。但是实现巨复杂无比
4. [CRC32/CRC64](https://github.com/owt5008137/libatbus/tree/master/src/detail) @see [libatbus](https://github.com/owt5008137/libatbus)
5. [CRC16](https://github.com/antirez/redis/blob/unstable/src/crc16.c) @see [redis](https://github.com/antirez/redis)

压缩算法
------
1. [snappy](https://github.com/google/snappy) By Google 高效的压缩/解压算法。比zlib压缩率略低，性能高。
2. [zopfli](https://github.com/google/zopfli) By Google 压缩率高但是CPU消耗也高的压缩算法。
3. [brotli](https://github.com/google/brotli) By Google 压缩率高，CPU消耗接近zlib的deflate的压缩算法。


内存混淆整数([mixed_int](mixed_int.h))
------
+ 尽量模拟正常的整数操作
+ 支持主流编译器
+ 内存大小和原生整形一致（内存布局当然不一致了，不然还混淆个毛线啊）
+ 然而这么搞还是会影响模板类型推断，因为这个类会导致多一次隐式类型转换，这时候只能报错或报warning的地方手动加下类型转换了
