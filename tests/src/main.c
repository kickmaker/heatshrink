/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2020 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/ztest.h>
#include <heatshrink_encoder.h>
#include <heatshrink_decoder.h>

ZTEST(heatshrink_tests, test_static_alloc_small)
{

	char *src = "Lorem ipsum dolor sit amet, consectetur ";

	heatshrink_encoder hse;
	heatshrink_decoder hsd;
	int ret;

	heatshrink_encoder_reset(&hse);
	heatshrink_decoder_reset(&hsd);

	const uint32_t src_size = (strlen(src) + 1);
	const uint32_t max_dst_size = src_size;
	uint8_t *compressed_data = malloc(max_dst_size);

	zassert_not_null(compressed_data, "Compressed buffer allocation failed");

	size_t count = 0;
	uint32_t sunk = 0;
	uint32_t compressed_size = 0;

	while (sunk < src_size) {
		if (heatshrink_encoder_sink(&hse, &src[sunk], src_size - sunk, &count) < 0) {
			printk("Failed to sink data into encoder");
		}

		sunk += count;
		if (sunk == src_size) {
			if (heatshrink_encoder_finish(&hse) != HSER_FINISH_MORE) {
				break;
			}
		}

		HSE_poll_res pres;

		do { /* "turn the crank" */
			pres = heatshrink_encoder_poll(&hse, &compressed_data[compressed_size],
						       max_dst_size - compressed_size, &count);
			if (pres < 0) {
				printk("Failed to poll data from encoder");
			}
			compressed_size += count;

		} while (pres == HSER_POLL_MORE);

		if (pres != HSER_POLL_EMPTY) {
			printk("Inconsistent encoder state");
		}
		if (compressed_size >= max_dst_size) {
			printk("Compression should never expand that much");
			break;
		}
		if (sunk == src_size) {
			if (heatshrink_encoder_finish(&hse) != HSER_FINISH_DONE) {
				break;
			}
		}
	}

	zassert_true(compressed_size > 0, "Failed to compress data");

	compressed_data = (char *)realloc(compressed_data, compressed_size);
	zassert_not_null(compressed_data, "Failed to re-alloc memory for compressed data");

	/* Decompression part */
	const uint32_t decompress_buf_size = src_size + 1;
	uint8_t *decompressed_data = malloc(decompress_buf_size);

	zassert_not_null(decompressed_data, "Failed to allocate memory to decompress data");

	sunk = 0;
	uint32_t decompressed_size = 0;

	while (sunk < compressed_size) {
		if (heatshrink_decoder_sink(&hsd, &compressed_data[sunk], compressed_size - sunk,
					    &count) < 0) {
			printk("Failed to sink data into decoder");
			break;
		}
		sunk += count;

		if (sunk == compressed_size) {
			if (heatshrink_decoder_finish(&hsd) != HSDR_FINISH_MORE) {
				break;
			}
		}

		HSD_poll_res pres;

		do {
			pres = heatshrink_decoder_poll(&hsd, &decompressed_data[decompressed_size],
						       decompress_buf_size - decompressed_size,
						       &count);
			if (pres < 0) {
				printk("Failed to poll data from decoder");
			}
			decompressed_size += count;

		} while (pres == HSDR_POLL_MORE);

		if (pres != HSDR_POLL_EMPTY) {
			printk("Inconsistent decoder state");
			break;
		}
		if (sunk == compressed_size) {
			HSD_finish_res fres = heatshrink_decoder_finish(&hsd);

			if (fres != HSDR_FINISH_DONE) {
				break;
			}
		}
	}

	free(compressed_data);

	zassert_true(decompressed_size > 0, "Failed to decompress data");

	zassert_equal(decompressed_size, src_size, "Decompressed data is different from original");

	ret = memcmp(src, decompressed_data, src_size);
	zassert_true(ret == 0, "Validation failed");

	free(decompressed_data);
}

ZTEST_SUITE(heatshrink_tests, NULL, NULL, NULL, NULL, NULL);
