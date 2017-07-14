# Interfaces

this branch contains generic files to be used by different compressors.

# file_helper.h
The file **file_helper.h** defines functions to load data from a file into a buffer, and from a buffer into a file. This functionality is needed by the correctness-tests and the benchmark-tests.

# generic_compressor.h
The file **generic_compressor.h** contains the interface definition which every compression algorithm needs to implement. If this interface is used, the generic test and benchmark classes can be easyly used.

# generic_compress_test.h
The file **generic_compress_test.h** contains code with different unit tests for compression and decompression code. To use this thest the **generic_compressor** interface must be used. During the unit tests the compressed version of the data is saved using an absolute path: **/tmp/data/data.compressed**

Usage:
```C
/* your_compressor_test.c */
#include "path/to/your_compressor.h"
static const generic_compressor* const compressor_linear = &your_generic_compressor_linear;
static const generic_compressor* const compressor_page = &your_generic_compressor_page;
static const int too_small_bounds_substractor = how_many_bytes_should_be_at_least_substracted_for_this_kind_of_test;
#include "generic_compress_test.h"
```

# generic_compress_test_time.h
The file **generic_compress_test_time.h** contains code for running benchmark tests for compression and decompression code. To use this thest the **generic_compressor** interface must be used. If the benchmark should run decompression tests the file at the absolute path **/tmp/data/data.compressed** is used. It is always usefull to run the test-code first before running benchmarks on it. The benchmark results are appended in the **csv** format to the file **/src/time_statistics.csv**

Usage:
```c
/* your_compressor_benchmark.c */
#include "path/to/your_compressor.h"
static const generic_compressor* const compressor_linear = &your_generic_compressor_linear;
static const generic_compressor* const compressor_page = &your_generic_compressor_page;
#include "generic_compress_test.h"
```

