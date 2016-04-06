#ifndef __BITS__
#define __BITS__

#include <stdint.h>

/*****************************************************************************/

#define _BIB 8 /* Bits in byte. */
typedef uint8_t B_EL_TYPE;
#define _MAXB_ELTYPE 256
#define _B_STRAIT

/*****************************************************************************/

#ifdef _B_STRAIT

#define _B_ORDER << /* Order bit in bytes */
#define _BASE_NUM (B_EL_TYPE) 1 /* Start num for shift */

#endif

#ifdef _B_INV

#define _B_ORDER >> /* Order bit in bytes */
#define _BASE_NUM (B_EL_TYPE) (_MAXB_ELTYPE / 2) /* Start num for shift */

#endif

/*****************************************************************************/

#define _B_OFFSET(x) (B_EL_TYPE) ((x) % _BIB)

#define BARR_SIZE(x) (((x) + _BIB - 1) / _BIB)

/*****************************************************************************/

#define STATIC_BARR(BARR_NAME,SIZE) B_EL_TYPE BARR_NAME[BARR_SIZE(SIZE)]

#define DYN_BARR(BARR_NAME) \
        B_EL_TYPE BARR_NAME = NULL

#define BARR_CALLOC(BARR_NAME, SIZE) \
        BARR_NAME = (B_EL_TYPE*) calloc (BARR_SIZE(SIZE), sizeof(B_EL_TYPE))

#define BARR_MALLOC(BARR_NAME, SIZE) \
        BARR_NAME = (B_EL_TYPE*) malloc (BARR_SIZE(SIZE) * sizeof(B_EL_TYPE))

#define BARR_FREE(BARR_NAME) free(BARR_NAME)

#define WRITE_BARR(FD, BARR_NAME, SIZE) \
        (write((FD), BARR_NAME, BARR_SIZE(SIZE)) * _BIB)

#define READ_BARR(FD, BARR_NAME, SIZE) \
        (read((FD), BARR_NAME, BARR_SIZE(SIZE)) * _BIB)

/*****************************************************************************/

#define GET_B(BARR_NAME, x) \
        !!(BARR_NAME[(x) / _BIB] & ((_BASE_NUM) _B_ORDER _B_OFFSET((x))))

#define SET_B(BARR_NAME, x) \
        BARR_NAME[(x) / _BIB] |= (_BASE_NUM) _B_ORDER _B_OFFSET((x))

#define CLR_B(BARR_NAME, x) \
        BARR_NAME[(x) / _BIB] &= ~(_BASE_NUM _B_ORDER _B_OFFSET((x)))

#define INV_B(BARR_NAME, x) \
        BARR_NAME[(x) / _BIB] ^= _BASE_NUM _B_ORDER _B_OFFSET((x))

/*****************************************************************************/

//#undef _BIB
//#undef B_EL_TYPE
//#undef _MAXB_EL_TYPE
//#undef _B_ORDER
//#undef _BASE_NUM
//#undef _B_OFFSET
//
//#ifdef _B_STRAIT
//#undef _B_STRAIT
//#endif
//
//#ifdef _B_INV
//#undef _B_INV
//#endif

/*****************************************************************************/

#endif
