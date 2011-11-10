/*
 * Virtio Audio Device
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

#include "virtio-audio.h"

//#define DEBUG_VIRTIO_AUDIO

#ifdef DEBUG_VIRTIO_AUDIO
#define DPRINTF(fmt, args...) \
do { printf("virtio-audio: " fmt , ##args); } while (0)
#define BADF(fmt, args...) \
do { fprintf(stderr, "virtio-audio: error: " fmt , ##args); exit(1);} while (0)
#else
#define DPRINTF(fmt, args...) do {} while(0)
#define BADF(fmt, args...) \
do { fprintf(stderr, "virtio-audio: error: " fmt , ##args);} while (0)
#endif

#define NUM_STREAMS 2

#define VIRT_CONTROL_QUEUE_SIZE 0x40
#define VIRT_DATA_QUEUE_SIZE 0x80

typedef struct {
    struct VirtIOAudio *dev;
    VirtQueue *data_vq;
    struct audsettings fmt;
    SWVoiceOut *out_voice;
    SWVoiceIn *in_voice;
    VirtQueueElement elem;
    int data_left;
    int data_offset;
    int has_buffer;
} VirtIOAudioStream;

typedef struct VirtIOAudio
{
    VirtIODevice vdev;
    QEMUSoundCard card;
    VirtQueue *cmd_vq;
    VirtIOAudioStream stream[NUM_STREAMS];
} VirtIOAudio;

static VirtIOAudio *to_virtio_audio(VirtIODevice *vdev)
{
    return (VirtIOAudio *)vdev;
}

static void virtio_audio_get_config(VirtIODevice *vdev, uint8_t *config)
{
    struct virtio_audio_cfg audio_cfg;
    
    audio_cfg.num_streams = NUM_STREAMS;
    memcpy(config, &audio_cfg, sizeof(audio_cfg));
}

static uint32_t virtio_audio_get_features(VirtIODevice *vdev)
{
    uint32_t features = 0;

    return features;
}

static void virtio_audio_set_features(VirtIODevice *vdev, uint32_t features)
{
}

static int virtio_audio_fill(VirtIOAudioStream *stream,
                             int offset, int total_len)
{
    uint8_t *p;
    int to_write;
    int written;
    int size;
    int n;
    struct iovec *iov;
    int iov_len;

    if (stream->in_voice) {
        iov_len = stream->elem.in_num;
        iov = stream->elem.in_sg;
    } else if (stream->out_voice) {
        iov_len = stream->elem.out_num;
        iov = stream->elem.out_sg;
    } else
    {
        DPRINTF("No voice selected skipping block\n");
        return 0;
    }
    written = 0;
    for (n = 0; total_len > 0 && n < iov_len; n++) {
        p = iov[n].iov_base;
        to_write = iov[n].iov_len;
        if (offset) {
            if (offset >= to_write) {
                offset -= to_write;
                continue;
            }
            p += offset;
            to_write -= offset;
            offset = 0;
        }
        if (to_write > total_len)
            to_write = total_len;
        while (to_write) {
            if (stream->in_voice) {
                size = AUD_read(stream->in_voice, p, to_write);
            } else if (stream->out_voice) {
                size = AUD_write(stream->out_voice, p, to_write);
            } else {
                size = 0;
            }
            DPRINTF("Copied %d/%d\n", size, to_write);
            if (size == 0) {
                total_len = 0;
                break;
            }
            to_write -= size;
            total_len -= size;
            written += size;
        }
    }
    return written;
}

static void virtio_audio_callback(void *opaque, int avail)
{
    VirtIOAudioStream *stream = opaque;
    int n;

    DPRINTF("Callback (%d)\n", avail);
    if ((!stream->in_voice)&&(!stream->out_voice))
    {
        DPRINTF("Skipping callback as no voice is selected!\n");
    }
    while (avail) {
        while (stream->data_left == 0) {
            if (stream->has_buffer) {
                virtqueue_push(stream->data_vq, &stream->elem, stream->data_offset);
                virtio_notify(&stream->dev->vdev, stream->data_vq);
                stream->has_buffer = 0;
            }
            if (!virtqueue_pop(stream->data_vq, &stream->elem)) {
                /* Buffer underrun.  */
                stream->has_buffer = 0;
                DPRINTF("Underrun\n");
                break;
            }
            stream->data_offset = 0;
            stream->data_left = 0;
            stream->has_buffer = 1;
            if (stream->in_voice) {
                for (n = 0; n < stream->elem.in_num; n++)
                    stream->data_left += stream->elem.in_sg[n].iov_len;
            } else if (stream->out_voice) {
                for (n = 0; n < stream->elem.out_num; n++)
                    stream->data_left += stream->elem.out_sg[n].iov_len;
            }
        }
        if (stream->data_left == 0)
            break;
        n = virtio_audio_fill(stream, stream->data_offset, avail);
        stream->data_left -= n;
        stream->data_offset += n;
        avail -= n;
        if (!n)
            break;
    }
    if (stream->data_left == 0 && stream->has_buffer) {
        virtqueue_push(stream->data_vq, &stream->elem, stream->data_offset);
        virtio_notify(&stream->dev->vdev, stream->data_vq);
        stream->has_buffer = 0;
    }
}

static void virtio_audio_cmd_result(uint32_t value, VirtQueueElement *elem,
                                    size_t *out_bytes)
{
    size_t offset = *out_bytes;
    int len;
    int n;

    DPRINTF("cmd result %d\n", value);
    for (n = 0; n < elem->in_num; n++) {
        len = elem->in_sg[n].iov_len;
        if (len < offset) {
            offset -= len;
            len = 0;
        } else {
            if (len  - offset < 4) {
                BADF("buffer too short\n");
                return;
            }
            stl_p(elem->in_sg[n].iov_base + offset, value);
            (*out_bytes) += 4;
            return;
        }
    }
    BADF("No space left\n");
}

/* Command queue.  */
static void virtio_audio_handle_cmd(VirtIODevice *vdev, VirtQueue *vq)
{
    VirtIOAudio *s = to_virtio_audio(vdev);
    VirtIOAudioStream *stream;
    VirtQueueElement elem;
    int out_n;
    uint32_t *p;
    int len;
    size_t out_bytes;
    uint32_t value;

    while (virtqueue_pop(s->cmd_vq, &elem)) {
        size_t bytes_transferred = 0;
        for (out_n = 0; out_n < elem.out_num; out_n++) {
            p = (uint32_t *)elem.out_sg[out_n].iov_base;
            len = elem.out_sg[out_n].iov_len;
            while (len > 0) {
                if (len < 12) {
                    BADF("Bad command length\n");
                    break;
                }
                DPRINTF("Command %d %d %d\n",
                        ldl_p(p), ldl_p(p + 1), ldl_p (p + 2));
                value = ldl_p(p + 1);
                if (value >= NUM_STREAMS)
                    break;
                stream = &s->stream[value];
                value = ldl_p(p + 2);
                switch (ldl_p(p)) {
                case VIRTIO_AUDIO_CMD_SET_ENDIAN:
                    stream->fmt.endianness = value;
                    break;
                case VIRTIO_AUDIO_CMD_SET_CHANNELS:
                    stream->fmt.nchannels = value;
                    break;
                case VIRTIO_AUDIO_CMD_SET_FMT:
                    stream->fmt.fmt = value;
                    break;
                case VIRTIO_AUDIO_CMD_SET_FREQ:
                    stream->fmt.freq = value;
                    break;
                case VIRTIO_AUDIO_CMD_INIT:
                    out_bytes = 0;
                    if (value == 1) {
                        if (stream->out_voice) {
                            AUD_close_out(&s->card, stream->out_voice);
                            stream->out_voice = NULL;
                        }
                        stream->in_voice =
                          AUD_open_in(&s->card, stream->in_voice,
                                      "virtio-audio.in",
                                      stream,
                                      virtio_audio_callback,
                                      &stream->fmt);
                        virtio_audio_cmd_result(0, &elem, &out_bytes);
                    } else if (value == 0) {
                        if (stream->in_voice) {
                            AUD_close_in(&s->card, stream->in_voice);
                            stream->in_voice = NULL;
                        }
                        stream->out_voice =
                          AUD_open_out(&s->card, stream->out_voice,
                                       "virtio-audio.out",
                                       stream,
                                       virtio_audio_callback,
                                       &stream->fmt);
                        value = AUD_get_buffer_size_out(stream->out_voice);
                        virtio_audio_cmd_result(value, &elem, &out_bytes);
                    } else { // let us close all down
                        if (stream->out_voice) {
                            AUD_close_out(&s->card, stream->out_voice);
                            stream->out_voice = NULL;
                        }
                        if (stream->in_voice) {
                            AUD_close_in(&s->card, stream->in_voice);
                            stream->in_voice = NULL;
                        }                        
                    }
                    bytes_transferred += out_bytes;
                    break;
                case VIRTIO_AUDIO_CMD_RUN:
                    if (stream->in_voice) {
                        AUD_set_active_in(stream->in_voice, value);
                    } else if (stream->out_voice) {
                        AUD_set_active_out(stream->out_voice, value);
                    } else
                    {
                        DPRINTF("Cannot execute CMD_RUN as no voice is active\n");
                    }
                    break;
                }
                p += 3;
                len -= 12;
                bytes_transferred += 12;
            }
        }
        virtqueue_push(s->cmd_vq, &elem, bytes_transferred);
        virtio_notify(vdev, s->cmd_vq);		
    }
}

static void virtio_audio_handle_data(VirtIODevice *vdev, VirtQueue *vq)
{
}


static void virtio_audio_save(QEMUFile *f, void *opaque)
{
    VirtIOAudio *s = opaque;
    VirtIOAudioStream *stream;
    int i;
    int mode;

    virtio_save(&s->vdev, f);

    for (i = 0; i < NUM_STREAMS; i++) {
        stream = &s->stream[i];

        if (stream->in_voice) {
            mode = 2;
            if (AUD_is_active_in(stream->in_voice))
                mode |= 1;
        } else if (stream->out_voice) {
            mode = 4;
            if (AUD_is_active_out(stream->out_voice))
                mode |= 1;
        } else {
            mode = 0;
        }
        qemu_put_byte(f, mode);
        qemu_put_byte(f, stream->fmt.endianness);
        qemu_put_be16(f, stream->fmt.nchannels);
        qemu_put_be32(f, stream->fmt.fmt);
        qemu_put_be32(f, stream->fmt.freq);
    }
}

static int virtio_audio_load(QEMUFile *f, void *opaque, int version_id)
{
    VirtIOAudio *s = opaque;
    VirtIOAudioStream *stream;
    int i;
    int mode;

    if (version_id != 1)
        return -EINVAL;

    /* FIXME: Do bad things happen if there is a transfer in progress?  */

    virtio_load(&s->vdev, f);

    for (i = 0; i < NUM_STREAMS; i++) {
        stream = &s->stream[i];

        stream->has_buffer = 0;
        stream->data_left = 0;
        if (stream->in_voice) {
            AUD_close_in(&s->card, stream->in_voice);
            stream->in_voice = NULL;
        }
        if (stream->out_voice) {
            AUD_close_out(&s->card, stream->out_voice);
            stream->out_voice = NULL;
        }
        mode = qemu_get_byte(f);
        stream->fmt.endianness = qemu_get_byte(f);
        stream->fmt.nchannels = qemu_get_be16(f);
        stream->fmt.fmt = qemu_get_be32(f);
        stream->fmt.freq = qemu_get_be32(f);
        if (mode & 2) {
            stream->in_voice = AUD_open_in(&s->card, stream->in_voice,
                                           "virtio-audio.in",
                                           stream,
                                           virtio_audio_callback,
                                           &stream->fmt);
            AUD_set_active_in(stream->in_voice, mode & 1);
        } else if (mode & 4) {
            stream->out_voice = AUD_open_out(&s->card, stream->out_voice,
                                             "virtio-audio.out",
                                             stream,
                                             virtio_audio_callback,
                                             &stream->fmt);
            AUD_set_active_out(stream->out_voice, mode & 1);
        }
    }

    return 0;
}

void virtio_audio_init(VirtIOBindFn bind, void *bind_arg, AudioState *audio)
{
    VirtIOAudio *s;
    int i;

    s = (VirtIOAudio *)bind(bind_arg, "virtio-audio", 0, VIRTIO_ID_AUDIO,
                            sizeof(struct virtio_audio_cfg),
                            sizeof(VirtIOAudio));
    if (!s)
        return;

    s->vdev.get_config = virtio_audio_get_config;
    s->vdev.get_features = virtio_audio_get_features;
    s->vdev.set_features = virtio_audio_set_features;
    s->cmd_vq = virtio_add_queue(&s->vdev, VIRT_CONTROL_QUEUE_SIZE, virtio_audio_handle_cmd);
    for (i = 0; i < NUM_STREAMS; i++) {
        s->stream[i].data_vq = virtio_add_queue(&s->vdev, VIRT_DATA_QUEUE_SIZE,
                                                virtio_audio_handle_data);
        s->stream[i].dev = s;
    }

    AUD_register_card(audio, "virtio-audio", &s->card);

    register_savevm("virtio-audio", -1, 1,
                    virtio_audio_save, virtio_audio_load, s);
}
