/***
  This file is part of systemd

  Copyright 2014 Ronny Chevalier

  systemd is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  systemd is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with systemd; If not, see <http://www.gnu.org/licenses/>.
***/

#include "compress.h"
#include "util.h"
#include "macro.h"

#ifdef HAVE_XZ
# define XZ_OK 0
#else
# define XZ_OK -EPROTONOSUPPORT
#endif

#ifdef HAVE_LZ4
# define LZ4_OK 0
#else
# define LZ4_OK -EPROTONOSUPPORT
#endif

typedef int (compress_blob_t)(const void *src, uint64_t src_size,
                              void *dst, size_t *dst_size);
typedef int (decompress_blob_t)(const void *src, uint64_t src_size,
                                void **dst, size_t *dst_alloc_size,
                                size_t* dst_size, size_t dst_max);
typedef int (decompress_sw_t)(const void *src, uint64_t src_size,
                              void **buffer, size_t *buffer_size,
                              const void *prefix, size_t prefix_len,
                              uint8_t extra);

typedef int (compress_stream_t)(int fdf, int fdt, off_t max_bytes);
typedef int (decompress_stream_t)(int fdf, int fdt, off_t max_size);

static void test_compress_decompress(int compression,
                                     compress_blob_t compress,
                                     decompress_blob_t decompress) {
        char text[] = "foofoofoofoo AAAA aaaaaaaaa ghost busters barbarbar FFF"
                      "foofoofoofoo AAAA aaaaaaaaa ghost busters barbarbar FFF";
        char compressed[512];
        size_t csize = 512;
        size_t usize = 0;
        _cleanup_free_ char *decompressed = NULL;
        int r;

        log_info("/* testing %s blob compression/decompression */",
                 object_compressed_to_string(compression));

        r = compress(text, sizeof(text), compressed, &csize);
        assert(r == 0);
        r = decompress(compressed, csize,
                       (void **) &decompressed, &usize, &csize, 0);
        assert(r == 0);
        assert_se(decompressed);
        assert_se(streq(decompressed, text));

        r = decompress("garbage", 7,
                       (void **) &decompressed, &usize, &csize, 0);
        assert(r < 0);

        /* make sure to have the minimal lz4 compressed size */
        r = decompress("00000000\1g", 9,
                       (void **) &decompressed, &usize, &csize, 0);
        assert(r < 0);

        r = decompress("\100000000g", 9,
                       (void **) &decompressed, &usize, &csize, 0);
        assert(r < 0);

        memzero(decompressed, usize);
}

static void test_decompress_startswith(int compression,
                                       compress_blob_t compress,
                                       decompress_sw_t decompress_sw) {

        char text[] = "foofoofoofoo AAAA aaaaaaaaa ghost busters barbarbar FFF"
                      "foofoofoofoo AAAA aaaaaaaaa ghost busters barbarbar FFF";
        char compressed[512];
        size_t csize = 512;
        size_t usize = 0;
        _cleanup_free_ char *decompressed = NULL;

        log_info("/* testing decompress_startswith with %s */",
                 object_compressed_to_string(compression));

        assert_se(compress(text, sizeof(text), compressed, &csize) == 0);
        assert_se(decompress_sw(compressed,
                                csize,
                                (void **) &decompressed,
                                &usize,
                                "foofoofoofoo", 12, ' ') > 0);
        assert_se(decompress_sw(compressed,
                                csize,
                                (void **) &decompressed,
                                &usize,
                                "foofoofoofoo", 12, 'w') == 0);
        assert_se(decompress_sw(compressed,
                                csize,
                                (void **) &decompressed,
                                &usize,
                                "barbarbar", 9, ' ') == 0);
        assert_se(decompress_sw(compressed,
                                csize,
                                (void **) &decompressed,
                                &usize,
                                "foofoofoofoo", 12, ' ') > 0);
}

static void test_compress_stream(int compression,
                                 const char* cat,
                                 compress_stream_t compress,
                                 decompress_stream_t decompress,
                                 const char *srcfile) {

        _cleanup_close_ int src = -1, dst = -1, dst2 = -1;
        char pattern[] = "/tmp/systemd-test.xz.XXXXXX",
             pattern2[] = "/tmp/systemd-test.xz.XXXXXX";
        int r;
        _cleanup_free_ char *cmd = NULL, *cmd2;
        struct stat st = {};

        log_debug("/* testing %s compression */",
                  object_compressed_to_string(compression));

        log_debug("/* create source from %s */", srcfile);

        assert_se((src = open(srcfile, O_RDONLY|O_CLOEXEC)) >= 0);

        log_debug("/* test compression */");

        assert_se((dst = mkostemp_safe(pattern, O_RDWR|O_CLOEXEC)) >= 0);

        assert(compress(src, dst, -1) == 0);

        if (cat) {
                assert_se(asprintf(&cmd, "%s %s | diff %s -", cat, pattern, srcfile) > 0);
                assert(system(cmd) == 0);
        }

        log_debug("/* test decompression */");

        assert_se((dst2 = mkostemp_safe(pattern2, O_RDWR|O_CLOEXEC)) >= 0);

        assert_se(stat(srcfile, &st) == 0);

        assert_se(lseek(dst, 0, SEEK_SET) == 0);
        r = decompress(dst, dst2, st.st_size);
        assert(r == 0);

        assert_se(asprintf(&cmd2, "diff %s %s", srcfile, pattern2) > 0);
        assert_se(system(cmd2) == 0);

        log_debug("/* test faulty decompression */");

        assert_se(lseek(dst, 1, SEEK_SET) == 1);
        r = decompress(dst, dst2, st.st_size);
        assert(r == -EBADMSG);

        assert_se(lseek(dst, 0, SEEK_SET) == 0);
        assert_se(lseek(dst2, 0, SEEK_SET) == 0);
        r = decompress(dst, dst2, st.st_size - 1);
        assert(r == -EFBIG);

        assert_se(unlink(pattern) == 0);
        assert_se(unlink(pattern2) == 0);
}

int main(int argc, char *argv[]) {

        log_set_max_level(LOG_DEBUG);

#ifdef HAVE_XZ
        test_compress_decompress(OBJECT_COMPRESSED_XZ, compress_blob_xz, decompress_blob_xz);
        test_decompress_startswith(OBJECT_COMPRESSED_XZ, compress_blob_xz, decompress_startswith_xz);
        test_compress_stream(OBJECT_COMPRESSED_XZ, "xzcat",
                             compress_stream_xz, decompress_stream_xz, argv[0]);
#else
        log_info("/* XZ test skipped */");
#endif
#ifdef HAVE_LZ4
        test_compress_decompress(OBJECT_COMPRESSED_LZ4, compress_blob_lz4, decompress_blob_lz4);
        test_decompress_startswith(OBJECT_COMPRESSED_LZ4, compress_blob_lz4, decompress_startswith_lz4);

        /* Produced stream is not compatible with lz4 binary, skip lz4cat check. */
        test_compress_stream(OBJECT_COMPRESSED_LZ4, NULL,
                             compress_stream_lz4, decompress_stream_lz4, argv[0]);
#else
        log_info("/* LZ4 test skipped */");
#endif

        return 0;
}
