/*
 * OPERATING SYSTEMS DESING - 16/17
 *
 * @file 	crc.h
 * @brief 	Headers for the CRC functionality.
 * @date	01/03/2017
 *
 * DO NOT MODIFY
 */

#ifndef _CRC_H_
#define _CRC_H_

#include <stdint.h>

/*
 * @brief	CRC16 implementation.
 *
 * @param	<buffer> to compute the CRC on.
 * @param	<length> of the buffer, in bytes.
 * @return	A 16-bit unsigned integer containing the resulting CRC.
 */
uint16_t CRC16(const unsigned char* buffer, unsigned int length);

/*
 * @brief	CRC32 implementation.
 *
 * @param	<buffer> to compute the CRC on.
 * @param	<length> of the buffer, in bytes.
 * @return	A 32-bit unsigned integer containing the resulting CRC.
 */
uint32_t CRC32(const unsigned char* buffer, unsigned int length);

/*
 * @brief	CRC64 implementation.
 *
 * @param	<buffer> to compute the CRC on.
 * @param	<length> of the buffer, in bytes.
 * @return	A 64-bit unsigned integer containing the resulting CRC.
 */
uint64_t CRC64(const unsigned char * buffer, unsigned int length);

#endif
