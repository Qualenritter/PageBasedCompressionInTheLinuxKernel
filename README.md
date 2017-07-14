# BeWalgo

BeWalgo is a compression algorithm with the focus on page based compression. Therefore the algorithm rasterises the data to compress in blocks of 4 or 8 BYTEs. On the one hand this may reduce the compression ratio, on the other hand this improves the throughput enourmously. The speedup is gained since the algorithm does never need to read 4 bytes which overlaps two pages.

The algorithm accepts an acceleration factor to seed up the compression at the cost of compression ratio.

Usage:

To use this algorithm the header **bewalgo_compressor.h** exports the necessary interface for interaction with the kernel module.

```
const extern generic_compressor bewalgo_generic_compressor_linear_U32;
const extern generic_compressor bewalgo_generic_compressor_linear_U64;
const extern generic_compressor bewalgo_generic_compressor_page_U32;
const extern generic_compressor bewalgo_generic_compressor_page_U64;
```

The keywords **linear** and **page** are defining the kind of buffer used by the algorithm. The **linear** buffer based algorithms take an __void\*__ to the data to compress and the destination buffer as argument. The algorithms with **page** take __struct page \*\*__ as arguments for the input and output buffer. This way the using kernel module can decite itself which kind of buffer is available and therefore shoiule be used.

BeWalgo is distributed in to Variants. The U32 Version rasterises the data in 32-bit segments while the U64 version uses a raster of 64bit. While the 32bit version can achieve better compression ratios due to more matches which can be found, the 64 bit version is faster and can find matches over longer distances.

# Verify correctness

To verify the correctness multiple different kind of files are used. The script **test/test_correct.sh** expects 4 files to be present at absolute paths on the system. During the tests the file **/tmp/data/data.compressed** needs to be writeable.

To gain a 100% code coverage the combination of files and used bytes from the files is varied.


# Benchmarks

To execute benchmarks the script **test/run/benchmarks/final.sh** can be used. The script iterates over a given set of files with different accelerations. Than benchmarks are evaluated for both page based and linear based buffers, as well as for compression and decompression.
