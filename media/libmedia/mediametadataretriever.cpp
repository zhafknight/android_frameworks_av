/*
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

//#define LOG_NDEBUG 0
#define LOG_TAG "MediaMetadataRetriever"

#include <utils/IServiceManager.h>
#include <utils/IPCThreadState.h>
#include <media/mediametadataretriever.h>
#include <media/IMediaPlayerService.h>
#include <utils/Log.h>
#include <dlfcn.h>

namespace android {

// client singleton for binder interface to service
Mutex MediaMetadataRetriever::sServiceLock;
sp<IMediaPlayerService> MediaMetadataRetriever::sService;
sp<MediaMetadataRetriever::DeathNotifier> MediaMetadataRetriever::sDeathNotifier;

const sp<IMediaPlayerService>& MediaMetadataRetriever::getService()
{
    Mutex::Autolock lock(sServiceLock);
    if (sService.get() == 0) {
        sp<IServiceManager> sm = defaultServiceManager();
        sp<IBinder> binder;
        do {
            binder = sm->getService(String16("media.player"));
            if (binder != 0) {
                break;
            }
            LOGW("MediaPlayerService not published, waiting...");
            usleep(500000); // 0.5 s
        } while(true);
        if (sDeathNotifier == NULL) {
            sDeathNotifier = new DeathNotifier();
        }
        binder->linkToDeath(sDeathNotifier);
        sService = interface_cast<IMediaPlayerService>(binder);
    }
    LOGE_IF(sService == 0, "no MediaPlayerService!?");
    return sService;
}

MediaMetadataRetriever::MediaMetadataRetriever()
{
    LOGV("constructor");
    const sp<IMediaPlayerService>& service(getService());
    if (service == 0) {
        LOGE("failed to obtain MediaMetadataRetrieverService");
        return;
    }
    sp<IMediaMetadataRetriever> retriever(service->createMetadataRetriever(getpid()));
    if (retriever == 0) {
        LOGE("failed to create IMediaMetadataRetriever object from server");
    }
    mRetriever = retriever;
}

MediaMetadataRetriever::~MediaMetadataRetriever()
{
    LOGV("destructor");
    disconnect();
    IPCThreadState::self()->flushCommands();
}

void MediaMetadataRetriever::disconnect()
{
    LOGV("disconnect");
    sp<IMediaMetadataRetriever> retriever;
    {
        Mutex::Autolock _l(mLock);
        retriever = mRetriever;
        mRetriever.clear();
    }
    if (retriever != 0) {
        retriever->disconnect();
    }
}

status_t MediaMetadataRetriever::setDataSource(const char* srcUrl)
{
    LOGV("setDataSource");
    if (mRetriever == 0) {
        LOGE("retriever is not initialized");
        return INVALID_OPERATION;
    }
    if (srcUrl == NULL) {
        LOGE("data source is a null pointer");
        return UNKNOWN_ERROR;
    }
    LOGV("data source (%s)", srcUrl);
    return mRetriever->setDataSource(srcUrl);
}

status_t MediaMetadataRetriever::setDataSource(int fd, int64_t offset, int64_t length)
{
    LOGV("setDataSource(%d, %lld, %lld)", fd, offset, length);
    if (mRetriever == 0) {
        LOGE("retriever is not initialized");
        return INVALID_OPERATION;
    }
    if (fd < 0 || offset < 0 || length < 0) {
        LOGE("Invalid negative argument");
        return UNKNOWN_ERROR;
    }
    return mRetriever->setDataSource(fd, offset, length);
}

status_t MediaMetadataRetriever::setMode(int mode)
{
    LOGV("setMode(%d)", mode);
    if (mRetriever == 0) {
        LOGE("retriever is not initialized");
        return INVALID_OPERATION;
    }
    return mRetriever->setMode(mode);
}

status_t MediaMetadataRetriever::getMode(int* mode)
{
    LOGV("getMode");
    if (mRetriever == 0) {
        LOGE("retriever is not initialized");
        return INVALID_OPERATION;
    }
    return mRetriever->getMode(mode);
}

sp<IMemory> MediaMetadataRetriever::captureFrame()
{
    LOGV("captureFrame");
    if (mRetriever == 0) {
        LOGE("retriever is not initialized");
        return NULL;
    }
    return mRetriever->captureFrame();
}

const char* MediaMetadataRetriever::extractMetadata(int keyCode)
{
    LOGV("extractMetadata(%d)", keyCode);
    if (mRetriever == 0) {
        LOGE("retriever is not initialized");
        return NULL;
    }
    return mRetriever->extractMetadata(keyCode);
}

sp<IMemory> MediaMetadataRetriever::extractAlbumArt()
{
    LOGV("extractAlbumArt");
    if (mRetriever == 0) {
        LOGE("retriever is not initialized");
        return NULL;
    }
    return mRetriever->extractAlbumArt();
}

void MediaMetadataRetriever::DeathNotifier::binderDied(const wp<IBinder>& who) {
    Mutex::Autolock lock(MediaMetadataRetriever::sServiceLock);
    MediaMetadataRetriever::sService.clear();
    LOGW("MediaMetadataRetriever server died!");
}

MediaMetadataRetriever::DeathNotifier::~DeathNotifier()
{
    Mutex::Autolock lock(sServiceLock);
    if (sService != 0) {
        sService->asBinder()->unlinkToDeath(this);
    }
}

}; // namespace android
