/*
 * Copyright (C) 2018 The Android Open Source Project
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


#define LOG_TAG "aml_audio_port"

#include <errno.h>
#include <stdlib.h>
#include <cutils/log.h>
#include <linux/ioctl.h>
#include <sound/asound.h>
#include <tinyalsa/asoundlib.h>
#include <string.h>

#include "audio_port.h"
#include "aml_ringbuffer.h"
#include "audio_hw_utils.h"
#include "audio_hwsync.h"
#include "aml_malloc_debug.h"

#ifdef ENABLE_AEC_APP
#include "audio_aec.h"
#endif

#define BUFF_CNT                    (4)
#define SYS_BUFF_CNT                (4)

static ssize_t input_port_write(struct input_port *port, const void *buffer, int bytes)
{
    unsigned char *data = (unsigned char *)buffer;
    int bytes_to_write = bytes;
    int written = 0;

    written = ring_buffer_write(port->r_buf, data, bytes_to_write, UNCOVER_WRITE);
    if (getprop_bool("vendor.media.audiohal.inport")) {
        if (port->enInPortType == AML_MIXER_INPUT_PORT_PCM_SYSTEM)
            aml_audio_dump_audio_bitstreams("/data/audio/inportSys.raw", buffer, written);
        //else if (port->port_index == AML_MIXER_INPUT_PORT_PCM_DIRECT)
            //aml_audio_dump_audio_bitstreams("/data/audio/inportDirect.raw", buffer, written);
    }

    ALOGV("%s() written %d", __func__, written);
    return written;
}

static ssize_t input_port_read(struct input_port *port, void *buffer, int bytes)
{
    int read = 0;
    read = ring_buffer_read(port->r_buf, buffer, bytes);
    if (read > 0)
        port->consumed_bytes += read;

    return read;
}

int inport_buffer_level(struct input_port *port)
{
    return get_buffer_read_space(port->r_buf);
}

int get_inport_avail_size(struct input_port *port)
{
    int read_avail = get_buffer_read_space(port->r_buf);
    if (0) {
        ALOGI("%s, port index %d, avail %d, chunk len %zu",
            __func__, port->enInPortType, read_avail, port->data_len_bytes);
    }
    return read_avail;
}

bool is_direct_flags(audio_output_flags_t flags) {
    return flags & (AUDIO_OUTPUT_FLAG_DIRECT | AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD);
}

uint32_t inport_get_latency_frames(struct input_port *port) {
    int frame_size = 4;
    uint32_t latency_frames = inport_buffer_level(port) / frame_size;
    // return full frames latency when no data in ring buffer
    if (latency_frames == 0)
        return port->r_buf->size / frame_size;

    return latency_frames;
}

aml_mixer_input_port_type_e get_input_port_type(struct audio_config *config,
        audio_output_flags_t flags)
{
    int channel_cnt = 2;
    aml_mixer_input_port_type_e enPortType = AML_MIXER_INPUT_PORT_PCM_SYSTEM;

    channel_cnt = audio_channel_count_from_out_mask(config->channel_mask);
    switch (config->format) {
        case AUDIO_FORMAT_PCM_16_BIT:
        case AUDIO_FORMAT_PCM_32_BIT:
            //if (config->sample_rate == 48000) {
            if (1) {
                ALOGI("%s(), samplerate %d", __func__, config->sample_rate);
                // FIXME: remove channel check when PCM_SYSTEM_SOUND supports multi-channel
                if (AUDIO_OUTPUT_FLAG_MMAP_NOIRQ & flags) {
                    enPortType = AML_MIXER_INPUT_PORT_PCM_MMAP;
                } else if (is_direct_flags(flags) || channel_cnt > 2) {
                    enPortType = AML_MIXER_INPUT_PORT_PCM_DIRECT;
                } else {
                    enPortType = AML_MIXER_INPUT_PORT_PCM_SYSTEM;
                }
                break;
            }
        case AUDIO_FORMAT_AC3:
        case AUDIO_FORMAT_E_AC3:
            //port_index = MIXER_INPUT_PORT_BITSTREAM_RAW;
            //break;
        default:
            ALOGE("%s() stream not supported for mFormat:%#x",
                    __func__, config->format);
    }

    return enPortType;
}

void inport_reset(struct input_port *port)
{
    ALOGD("%s()", __func__);
    port->port_status = STOPPED;
    //port->is_hwsync = false;
    port->consumed_bytes = 0;
}

int send_inport_message(struct input_port *port, enum PORT_MSG msg)
{
    struct port_message *p_msg = aml_audio_calloc(1, sizeof(struct port_message));
    if (p_msg == NULL) {
        ALOGE("%s(), no memory", __func__);
        return -ENOMEM;
    }

    p_msg->msg_what = msg;
    pthread_mutex_lock(&port->msg_lock);
    list_add_tail(&port->msg_list, &p_msg->list);
    pthread_mutex_unlock(&port->msg_lock);

    return 0;
}

const char *str_port_msg[MSG_CNT] = {
    "MSG_PAUSE",
    "MSG_FLUSH",
    "MSG_RESUME"
};

const char *port_msg_to_str(enum PORT_MSG msg)
{
    return str_port_msg[msg];
}

struct port_message *get_inport_message(struct input_port *port)
{
    struct port_message *p_msg = NULL;
    struct listnode *item = NULL;

    pthread_mutex_lock(&port->msg_lock);
    if (!list_empty(&port->msg_list)) {
        item = list_head(&port->msg_list);
        p_msg = node_to_item(item, struct port_message, list);
        ALOGI("%s(), msg: %s", __func__, port_msg_to_str(p_msg->msg_what));
    }
    pthread_mutex_unlock(&port->msg_lock);
    return p_msg;
}

int remove_inport_message(struct input_port *port, struct port_message *p_msg)
{
    if (port == NULL || p_msg == NULL) {
        ALOGE("%s(), NULL pointer", __func__);
        return -EINVAL;
    }
    pthread_mutex_lock(&port->msg_lock);
    list_remove(&p_msg->list);
    pthread_mutex_unlock(&port->msg_lock);
    aml_audio_free(p_msg);

    return 0;
}

int remove_all_inport_messages(struct input_port *port)
{
    struct port_message *p_msg = NULL;
    struct listnode *node = NULL, *n = NULL;
    pthread_mutex_lock(&port->msg_lock);
    list_for_each_safe(node, n, &port->msg_list) {
        p_msg = node_to_item(node, struct port_message, list);
        ALOGI("%s(), msg what %s", __func__, port_msg_to_str(p_msg->msg_what));
        if (p_msg->msg_what == MSG_PAUSE)
            aml_hwsync_set_tsync_pause(NULL);
        list_remove(&p_msg->list);
        aml_audio_free(p_msg);
    }
    pthread_mutex_unlock(&port->msg_lock);
    return 0;
}

static int setPortConfig(struct audioCfg *cfg, struct audio_config *config)
{
    if (cfg == NULL || config == NULL) {
        ALOGE("%s(), NULL pointer", __func__);
        return -EINVAL;
    }

    cfg->channelCnt = audio_channel_count_from_out_mask(config->channel_mask);
    cfg->format = config->format;
    cfg->sampleRate = config->sample_rate;
    cfg->frame_size = cfg->channelCnt * audio_bytes_per_sample(config->format);
    return 0;
}

/* padding buf with zero to avoid underrun of audioflinger */
static int inport_padding_zero(struct input_port *port, size_t bytes)
{
    char *feed_mem = NULL;
    ALOGI("%s(), padding size %zu 0s to inport %d",
            __func__, bytes, port->enInPortType);
    feed_mem = aml_audio_calloc(1, bytes);
    if (!feed_mem) {
        ALOGE("%s(), no memory", __func__);
        return -ENOMEM;
    }

    input_port_write(port, feed_mem, bytes);
    port->padding_frames = bytes / port->cfg.frame_size;
    aml_audio_free(feed_mem);
    return 0;
}

int set_inport_padding_size(struct input_port *port, size_t bytes)
{
    port->padding_frames = bytes / port->cfg.frame_size;
    return 0;
}

struct input_port *new_input_port(
        //aml_mixer_input_port_type_e port_index,
        //audio_format_t format//,
        size_t buf_frames,
        struct audio_config *config,
        audio_output_flags_t flags,
        float volume,
        bool direct_on)
{
    struct input_port *port = NULL;
    struct ring_buffer *ringbuf = NULL;
    aml_mixer_input_port_type_e enPortType;
    int channel_cnt = 2;
    char *data = NULL;
    int input_port_rbuf_size = 0;
    int thunk_size = 0;
    int ret = 0;

    port = aml_audio_calloc(1, sizeof(struct input_port));
    if (!port) {
        ALOGE("%s(), no memory", __func__);
        goto err;
    }

    setPortConfig(&port->cfg, config);
    thunk_size = buf_frames * port->cfg.frame_size;
    data = aml_audio_calloc(1, thunk_size);
    if (!data) {
        ALOGE("%s(), no memory", __func__);
        goto err_data;
    }

    ringbuf = aml_audio_calloc(1, sizeof(struct ring_buffer));
    if (!ringbuf) {
        ALOGE("%s(), no memory", __func__);
        goto err_rbuf;
    }

    enPortType = get_input_port_type(config, flags);
    // system buffer larger than direct to cache more for mixing?
    if (enPortType == AML_MIXER_INPUT_PORT_PCM_SYSTEM) {
        input_port_rbuf_size = thunk_size * SYS_BUFF_CNT;
    } else {
        input_port_rbuf_size = thunk_size * BUFF_CNT;
    }

    ALOGD("[%s:%d] inport:%s, rbuf size:%d, direct_on:%d, format:%#x, rate:%d", __func__, __LINE__,
        mixerInputType2Str(enPortType), input_port_rbuf_size, direct_on, port->cfg.format, port->cfg.sampleRate);
    ret = ring_buffer_init(ringbuf, input_port_rbuf_size);
    if (ret) {
        ALOGE("init ring buffer fail, buffer_size = %d", input_port_rbuf_size);
        goto err_rbuf_init;
    }
    port->inport_start_threshold = 0;
    /* increase the input size to prevent underrun */
    if (enPortType == AML_MIXER_INPUT_PORT_PCM_MMAP) {
        port->inport_start_threshold = input_port_rbuf_size / 2;
    }

    port->enInPortType = enPortType;
    //port->format = config->format;
    port->r_buf = ringbuf;
    port->data_valid = 0;
    port->data = data;
    port->data_buf_frame_cnt = buf_frames;
    port->data_len_bytes = thunk_size;
    port->buffer_len_ns = (input_port_rbuf_size / port->cfg.frame_size) * 1000000000LL / port->cfg.sampleRate;
    port->first_write = true;
    port->last_write_time_ns = 0;
    port->read = input_port_read;
    port->write = input_port_write;
    port->rbuf_avail = get_inport_avail_size;
    port->get_latency_frames = inport_get_latency_frames;
    port->port_status = STOPPED;
    port->is_hwsync = false;
    port->consumed_bytes = 0;
    port->volume = volume;
    list_init(&port->msg_list);
    //TODO
    //set_inport_hwsync(port);
    //if (port_index == AML_MIXER_INPUT_PORT_PCM_SYSTEM && !direct_on) {
    //if (port_index == AML_MIXER_INPUT_PORT_PCM_SYSTEM) {
    //    inport_padding_zero(port, rbuf_size);
    //}
    return port;

err_rbuf_init:
    aml_audio_free(ringbuf);
err_rbuf:
    aml_audio_free(data);
err_data:
    aml_audio_free(port);
err:
    return NULL;
}

int free_input_port(struct input_port *port)
{
    if (!port) {
        ALOGE("NULL pointer");
        return -EINVAL;
    }

    remove_all_inport_messages(port);
    ring_buffer_release(port->r_buf);
    aml_audio_free(port->r_buf);
    aml_audio_free(port->data);
    aml_audio_free(port);

    return 0;
}

int reset_input_port(struct input_port *port)
{
    if (!port) {
        ALOGE("NULL pointer");
        return -EINVAL;
    }

    inport_reset(port);
    return ring_buffer_reset(port->r_buf);
}

int resize_input_port_buffer(struct input_port *port, uint buf_size)
{
    int ret = 0;

    if (!port) {
        ALOGE("%s() NULL pointer", __func__);
        return -EINVAL;
    }

    if (port->data_len_bytes == buf_size) {
        return 0;
    }
    ALOGI("%s(), new size %d", __func__, buf_size);
    ring_buffer_release(port->r_buf);
    ret = ring_buffer_init(port->r_buf, buf_size * 4);
    if (ret) {
        ALOGE("init ring buffer fail, buffer_size = %d", buf_size * 4);
        ret = -ENOMEM;
        goto err_rbuf_init;
    }

    port->data = (char *)aml_audio_realloc(port->data, buf_size);
    if (!port->data) {
        ALOGE("%s() no mem", __func__);
        ret = -ENOMEM;
        goto err_data;
    }
    port->data_len_bytes = buf_size;

    return 0;
err_data:
    ring_buffer_release(port->r_buf);
err_rbuf_init:
    return ret;
}

void set_inport_volume(struct input_port *port, float vol)
{
    ALOGD("%s(), volume %f", __func__, vol);
    port->volume = vol;
}

float get_inport_volume(struct input_port *port)
{
    return port->volume;
}

int set_port_notify_cbk(struct input_port *port,
        int (*on_notify_cbk)(void *data), void *data)
{
    port->on_notify_cbk = on_notify_cbk;
    port->notify_cbk_data = data;
    return 0;
}

int set_port_input_avail_cbk(struct input_port *port,
        int (*on_input_avail_cbk)(void *data), void *data)
{
    port->on_input_avail_cbk = on_input_avail_cbk;
    port->input_avail_cbk_data = data;
    return 0;
}

int set_port_meta_data_cbk(struct input_port *port,
        meta_data_cbk_t meta_data_cbk,
        void *data)
{
    if (false == port->is_hwsync) {
        ALOGE("%s(), can't set meta data callback", __func__);
        return -EINVAL;
    }
    port->meta_data_cbk = meta_data_cbk;
    port->meta_data_cbk_data = data;
    return 0;
}

int set_inport_state(struct input_port *port, enum port_state status)
{
    port->port_status = status;
    return 0;
}

enum port_state get_inport_state(struct input_port *port)
{
    return port->port_status;
}

size_t get_inport_consumed_size(struct input_port *port)
{
    return port->consumed_bytes;
}

static ssize_t output_port_write(struct output_port *port, void *buffer, int bytes)
{
    int bytes_to_write = bytes;
    (void *)port;
    do {
        int written = 0;
        ALOGV("%s(), line %d", __func__, __LINE__);

        aml_audio_dump_audio_bitstreams("/data/audio/audioOutPort.raw", buffer, bytes);
        //usleep(bytes*1000/4/48);
        written = bytes;
        bytes_to_write -= written;
    } while (bytes_to_write > 0);
    return bytes;
}

static ssize_t output_port_write_alsa(struct output_port *port, void *buffer, int bytes)
{
    int bytes_to_write = bytes;
    int ret = 0;
    {
        struct snd_pcm_status status;

        pcm_ioctl(port->pcm_handle, SNDRV_PCM_IOCTL_STATUS, &status);
        if (status.state == PCM_STATE_XRUN) {
            ALOGD("%s() alsa underrun", __func__);
        }
    }

    aml_audio_switch_output_mode((int16_t *)buffer, bytes, port->sound_track_mode);
    if (port->pcm_restart) {
        pcm_stop(port->pcm_handle);
        ALOGI("restart pcm device for same src");
        port->pcm_restart = false;
    }

    do {
        int written = 0;
        ALOGV("%s(), line %d", __func__, __LINE__);
        ret = pcm_write(port->pcm_handle, (void *)buffer, bytes);
#ifdef ENABLE_AEC_APP
        if (ret >= 0) {
            struct aec_info info;
            get_pcm_timestamp(port->pcm_handle, port->cfg.sampleRate, &info, true /*isOutput*/);
            info.bytes = bytes;
            int aec_ret = write_to_reference_fifo(port->aec, (void *)buffer, &info);
            if (aec_ret) {
                ALOGE("AEC: Write to speaker loopback FIFO failed!");
            }
        }
#endif
        if (ret == 0) {
           written += bytes;
        } else {
           const char *err_str = pcm_get_error(port->pcm_handle);
           ALOGE("pcm_write failed ret = %d, pcm_get_error(port->pcm):%s",
                ret, err_str);
           if (strstr(err_str, "initial") > 0)
               pcm_ioctl(port->pcm_handle, SNDRV_PCM_IOCTL_PREPARE);
           usleep(1000);
        }
        if (written > 0 && getprop_bool("vendor.media.audiohal.inport")) {
            aml_audio_dump_audio_bitstreams("/data/audio/audioOutPort.raw", buffer, written);
        }
        bytes_to_write -= written;
    } while (bytes_to_write > 0);

    return bytes;
}

int outport_get_latency_frames(struct output_port *port)
{
    int ret = 0, frames = 0;

    if (!port)
        return -EINVAL;

    if (!port->pcm_handle || !pcm_is_ready(port->pcm_handle)) {
        return -EINVAL;
    }
    ret = pcm_ioctl(port->pcm_handle, SNDRV_PCM_IOCTL_DELAY, &frames);
    if (ret < 0)
        return ret;

    return frames;
}

struct output_port *new_output_port(
        enum MIXER_OUTPUT_PORT port_index,
        struct pcm *pcm_handle,
        struct audioCfg cfg,
        size_t buf_frames)
{
    struct output_port *port = NULL;
    char *data = NULL;
    int rbuf_size = buf_frames * cfg.frame_size;
    int ret = 0;

    port = aml_audio_calloc(1, sizeof(struct output_port));
    if (!port) {
        ALOGE("%s(), no memory", __func__);
        goto err;
    }

    data = aml_audio_calloc(1, rbuf_size);
    if (!data) {
        ALOGE("%s(), no memory", __func__);
        goto err_data;
    }
    if (ret < 0)
        ALOGE("%s() thread run failed.", __func__);

    port->enOutPortType = port_index;
    port->cfg = cfg;
    port->pcm_handle = pcm_handle;
    port->data_buf_frame_cnt = buf_frames;
    port->data_buf_len = rbuf_size;
    port->data_buf = data;
    port->write = output_port_write_alsa;

    return port;
err_rbuf:
    aml_audio_free(data);
err_data:
    aml_audio_free(port);
err:
    return NULL;
}

int free_output_port(struct output_port *port)
{
    if (!port) {
        ALOGE("NULL pointer");
        return -EINVAL;
    }

    aml_audio_free(port->data_buf);
    aml_audio_free(port);

    return 0;
}

int resize_output_port_buffer(struct output_port *port, size_t buf_frames)
{
    int ret = 0;
    size_t buf_length = 0;

    if (!port) {
        ALOGE("%s() NULL pointer", __func__);
        return -EINVAL;
    }

    if (port->buf_frames == buf_frames) {
        return 0;
    }
    ALOGI("%s(), new buf_frames %zu", __func__, buf_frames);
    buf_length = buf_frames * port->cfg.frame_size;
    port->data_buf = (char *)aml_audio_realloc(port->data_buf, buf_length);
    if (!port->data_buf) {
        ALOGE("%s() no mem", __func__);
        ret = -ENOMEM;
        goto err_data;
    }
    port->data_buf_len = buf_length;

    return 0;
err_data:
    return ret;
}

bool is_inport_valid(aml_mixer_input_port_type_e index)
{
    return (index >= 0 && index < NR_INPORTS);
}

bool is_outport_valid(enum MIXER_OUTPUT_PORT index)
{
    return (index >= 0 && index < MIXER_OUTPUT_PORT_NUM);
}

int set_inport_pts_valid(struct input_port *in_port, bool valid)
{
    in_port->pts_valid = valid;
    return 0;
}

bool is_inport_pts_valid(struct input_port *in_port)
{
    return in_port->pts_valid;
}

void outport_pcm_restart(struct output_port *port)
{
    port->pcm_restart = true;
}
