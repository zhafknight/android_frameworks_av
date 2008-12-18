/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_AUDIO_TRACK_SHARED_H
#define ANDROID_AUDIO_TRACK_SHARED_H

#include <stdint.h>
#include <sys/types.h>

#include <utils/threads.h>

namespace android {

// ----------------------------------------------------------------------------

#define MAX_SAMPLE_RATE     65535
#define THREAD_PRIORITY_AUDIO_CLIENT (ANDROID_PRIORITY_AUDIO)

struct audio_track_cblk_t
{

    // The data members are grouped so that members accessed frequently and in the same context
    // are in the same line of data cache.
                Mutex       lock;
                Condition   cv;
    volatile    uint32_t    user;
    volatile    uint32_t    server;
                uint32_t    userBase;
                uint32_t    serverBase;
    void*       buffers;
    uint32_t    frameCount;
    // Cache line boundary
    uint32_t    loopStart;
    uint32_t    loopEnd;
    int         loopCount;
    volatile    union {
                    uint16_t    volume[2];
                    uint32_t    volumeLR;
                };
                uint16_t    sampleRate;
                uint16_t    channels;
                int16_t     flowControlFlag; // underrun (out) or overrrun (in) indication
                uint8_t     out;        // out equals 1 for AudioTrack and 0 for AudioRecord
                uint8_t     forceReady; 
                // Padding ensuring that data buffer starts on a cache line boundary (32 bytes). 
                // See AudioFlinger::TrackBase constructor
                int32_t     Padding[4];
                
                            audio_track_cblk_t();
                uint32_t    stepUser(uint32_t frameCount);
                bool        stepServer(uint32_t frameCount);
                void*       buffer(uint32_t offset) const;
                uint32_t    framesAvailable();
                uint32_t    framesAvailable_l();
                uint32_t    framesReady();
};


// ----------------------------------------------------------------------------

}; // namespace android

#endif // ANDROID_AUDIO_TRACK_SHARED_H
