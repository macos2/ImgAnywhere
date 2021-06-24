/*
 * Img2Binary.c
 *
 *  Created on: 2021年6月21日
 *      Author: tom
 */

#include "Img2Binary.h"

uint64_t bit_to_binary(uint8_t *in, uint8_t **out, uint32_t w, uint32_t h,
		ScanDirection first_dir, ScanDirection second_dir, BitDirection bit_dir,
		ByteOrder bit_order, uint8_t scan_width) {
	uint32_t i, j;
	uint64_t res = 0;
	//error check,the first dir should orth the second dir.
	if (first_dir == SCAN_DIR_LEFT_TO_RIGHT
			|| first_dir == SCAN_DIR_RIGHT_TO_LEFT) {
		if (second_dir == SCAN_DIR_LEFT_TO_RIGHT
				|| second_dir == SCAN_DIR_RIGHT_TO_LEFT) {
			return 0;
		}
	}
	if (first_dir == SCAN_DIR_BOTTOM_TO_TOP
			|| first_dir == SCAN_DIR_TOP_TO_BOTTOM) {
		if (second_dir == SCAN_DIR_BOTTOM_TO_TOP
				|| second_dir == SCAN_DIR_TOP_TO_BOTTOM) {
			return 0;
		}
	}
	//process start
	if (first_dir == SCAN_DIR_LEFT_TO_RIGHT
			|| first_dir == SCAN_DIR_RIGHT_TO_LEFT) {
		for (i = 0; i < w; i++) {

		}
	}
	if (first_dir == SCAN_DIR_BOTTOM_TO_TOP
			|| first_dir == SCAN_DIR_TOP_TO_BOTTOM) {

	}
	return res;
}

uint64_t rgb_to_binary(uint8_t *rgb_in, uint8_t **out, uint32_t w, uint32_t h,
		RGBFormat format) {

	return 0;
}
