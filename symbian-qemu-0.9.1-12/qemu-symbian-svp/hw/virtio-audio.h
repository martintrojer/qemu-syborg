/*
 * Virtio Audio Device Header
 *
 * Copyright 2009 CodeSourcery
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "virtio.h"
#include "audio/audio.h"

#define VIRTIO_ID_AUDIO 0xffff

#define VIRTIO_AUDIO_CMD_SET_ENDIAN   1
#define VIRTIO_AUDIO_CMD_SET_CHANNELS 2
#define VIRTIO_AUDIO_CMD_SET_FMT      3
#define VIRTIO_AUDIO_CMD_SET_FREQ     4
#define VIRTIO_AUDIO_CMD_INIT         5
#define VIRTIO_AUDIO_CMD_RUN          6

struct virtio_audio_cmd {
    uint32_t cmd;
    uint32_t stream;
    uint32_t arg;
};

struct virtio_audio_cfg {
    uint8_t num_streams;
};

void virtio_audio_init(VirtIOBindFn bind, void *bind_arg, AudioState *audio);
