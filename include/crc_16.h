#pragma once

void crc_ccitt_16(unsigned char *pcrc, const unsigned char *s, int len, bool isHHLL);
void crc_tbl_16	 (unsigned char *pcrc, const unsigned char *s, int len, bool isHHLL);
void crc_cal_16	 (unsigned char *pcrc, const unsigned char *s, int len, bool isHHLL);

void test_crc16(unsigned char * pcrc, const unsigned char *buf, int len);
