//
//  ndnrtc-defines.hpp
//  ndnrtc
//
//  Copyright 2013 Regents of the University of California
//  For licensing details see the LICENSE file.
//
//  Author:  Peter Gusev
//

#ifndef ndnrtc_ndnrtc_defines_h
#define ndnrtc_ndnrtc_defines_h

#define NdnComponentName "ndnrtc"

#define RESULT_GOOD(res) (res >= RESULT_OK)
#define RESULT_NOT_OK(res) (res < RESULT_OK)
#define RESULT_FAIL(res) (res <= RESULT_ERR)
#define RESULT_NOT_FAIL(res) (res > RESULT_ERR)
#define RESULT_WARNING(res) (res <= RESULT_WARN && res > RESULT_ERR)

#define RESULT_OK 0
#define RESULT_ERR -100
#define RESULT_WARN -1

// ndn library error
#define ERR_NDNLIB  -101

#define DEPRECATED __attribute__ ((deprecated))

// name lengths limitations
#define MAX_STREAM_NAME_LENGTH 20
#define MAX_THREAD_NAME_LENGTH 20

#define USE_WINDOW_PIPELINER
#define FRAME_DELAY_DEADLINE    100

// for tests only
//#define DROP_FRAMES
//#define DELAY_FRAMES

#endif
