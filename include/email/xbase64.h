/* base64.h */
/*--------------------------------------------------------------------------*\
  Copyright (C) 1999-2000 Douglas W. Sauder

  This software is provided "as is," without any express or implied
  warranty.  In no event will the author be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  The original distribution can be obtained from www.hunnysoft.com.
  You can email the author at dwsauder@hunnysoft.com.

  $RCSfile: base64.h,v $
  $Revision: 1.2 $
  $Date: 1999-07-29 18:42:39-04 $
\*--------------------------------------------------------------------------*/

#ifndef DW_BASE64_H
#define DW_BASE64_H

#ifdef __cplusplus
extern "C" {
#endif

#define DW_B64_STATE_0              0
#define DW_B64_STATE_1              1
#define DW_B64_STATE_2              2
#define DW_B64_STATE_3              3
#define DW_B64_STATE_OUTPUT_CRLF    4
#define DW_B64_STATE_OUTPUT_LF      5
#define DW_B64_STATE_OUTPUT_EQ_EQ   6
#define DW_B64_STATE_OUTPUT_EQ      7
#define DW_B64_STATE_DONE           8

/*!
 * dw_b64encoder declarations
 */

typedef struct dw_b64encoder {
    /* buffers */
    unsigned int mInPos;           /*! current position in input buffer */
    int mInLen;           /*! number of chars in input buffer */
    char *mInBuffer;      /*! pointer to input buffer */
    unsigned int mOutPos;          /*! current position in output buffer */
    int mOutLen;          /*! capacity of output buffer */
    char *mOutBuffer;     /*! pointer to output buffer */
    /* options */
    int mMaxLineLen;      /*! maximum number of chars on output line
                           * (not including end of line chars */
    int mOutputCrLf;      /*! TRUE  ==> write CRLF at end of line
                           * FALSE ==> write LF at end of line */
    int mNoFinalNewline;  /*! TRUE ==> don't write LF or CRLF at end */
    /* state */
    int mState;
    int mLastByte;        /*! left over byte from previous input buffer */
    int mLineLen;         /*! current output line length */
} dw_b64encoder;

void dw_b64encoder_set_defaults(dw_b64encoder *encoder);
void dw_b64encoder_start(dw_b64encoder *encoder);
void dw_b64encoder_encode(dw_b64encoder *encoder);
void dw_b64encoder_encode_fast(dw_b64encoder *encoder);
void dw_b64encoder_finish(dw_b64encoder *encoder);

/*!
 * dw_b64decoder declarations
 */

typedef struct dw_b64decoder {
    unsigned int mInPos;           /*! current position in input buffer */
    int mInLen;           /*! number of chars in input buffer */
    char *mInBuffer;      /*! pointer to input buffer */
    unsigned int mOutPos;          /*! current position in output buffer */
    int mOutLen;          /*! capacity of output buffer */
    char *mOutBuffer;     /*! pointer to output buffer */
    /* state */
    int mState;
    int mError;           /*! nonzero if bad input seen */
    int mLastByte;        /*! left over byte from previous input buffer */
} dw_b64decoder;

void dw_b64decoder_start(dw_b64decoder *decoder);
void dw_b64decoder_decode(dw_b64decoder *decoder);
void dw_b64decoder_decode_fast(dw_b64decoder *decoder);

int dw_b64encode(char * d, const void * s, int inlen);
int dw_b64decode(char * d, const void * s, int inlen);
int dw_b64encode_nocrlf(char * d, const void * s, int inlen);

int isBase64Char(char c);

#ifdef __cplusplus
}
#endif

#endif
