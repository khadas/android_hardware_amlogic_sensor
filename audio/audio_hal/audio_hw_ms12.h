/*
 * Copyright (C) 2010 Amlogic Corporation.
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
#ifndef MS12_V24_ENABLE

#ifndef _AUDIO_HW_MS12_H_
#define _AUDIO_HW_MS12_H_

#include <tinyalsa/asoundlib.h>
#include <system/audio.h>
#include <stdbool.h>
#include <aml_audio_ms12.h>

#include "audio_hw.h"

#define DDP_OUTPUT_SAMPLE_RATE (48000)
#define SAMPLE_NUMS_IN_ONE_BLOCK (256)
#define DDP_FRAME_DURATION(sample_nums, sample_rate) ((sample_nums) / (sample_rate))

/*
 *@brief get dolby ms12 prepared
 */
int get_the_dolby_ms12_prepared(
    struct aml_stream_out *aml_out
    , audio_format_t input_format
    , audio_channel_mask_t input_channel_mask
    , int input_sample_rate);

/*
 *@brief get the data of direct thread
 *
 * input parameters
 *     stream: audio_stream_out handle
 *     buffer: data buffer address
 *     bytes: data size
 * output parameters
 *     used_size: buffer used size
 */
int dolby_ms12_main_process(
    struct audio_stream_out *stream
    , const void *buffer
    , size_t bytes
    , size_t *used_size);


/*
 *@brief dolby ms12 system process
 *
 * input parameters
 *     stream: audio_stream_out handle
 *     buffer: data buffer address
 *     bytes: data size
 * output parameters
 *     used_size: buffer used size
 */
int dolby_ms12_system_process(
    struct audio_stream_out *stream
    , const void *buffer
    , size_t bytes
    , size_t *used_size);

/*
 *@brief dolby ms12 app process
 *
 * input parameters
 *     stream: audio_stream_out handle
 *     buffer: data buffer address
 *     bytes: data size
 * output parameters
 *     used_size: buffer used size
 */
int dolby_ms12_app_process(
    struct audio_stream_out *stream
    , const void *buffer
    , size_t bytes
    , size_t *used_size);

/*
 *@brief get dolby ms12 cleanup
 */
int get_dolby_ms12_cleanup(struct dolby_ms12_desc *ms12);

/*
 *@brief set dolby ms12 primary gain
 */
int set_dolby_ms12_primary_input_db_gain(struct dolby_ms12_desc *ms12,
        int db_gain, int duration);

/*
 *@brief set system app mixing status
 * if normal pcm stream status is STANDBY, set mixing off(-xs 0)
 * if normal pcm stream status is active, set mixing on(-xs 1)
 */
int set_system_app_mixing_status(struct aml_stream_out *aml_out, int stream_status);

/*
 *@brief an callback for dolby ms12 pcm output
 */
int dap_pcm_output(void *buffer, void *priv_data, size_t size);

/*
 *@brief an callback for dolby ms12 pcm output
 */
int stereo_pcm_output(void *buffer, void *priv_data, size_t size);

/*
 *@brief an callback for dolby ms12 bitstream output
 */
int bitstream_output(void *buffer, void *priv_data, size_t size);

/*
 *@brief an callback for dolby ms12 bitstream output
 */
int spdif_bitstream_output(void *buffer, void *priv_data, size_t size);

/*
 *@brief dolby ms12 register the callback
 */
int dolby_ms12_register_callback(struct aml_stream_out *aml_out);

/*
 *@brief dolby ms12 downmix output, get the bytes of PCM frame
 */
int nbytes_of_dolby_ms12_downmix_output_pcm_frame();

void dolby_ms12_app_flush();
/*
 *@brief dolby ms12 flush the main related buffer
 */
int dolby_ms12_main_flush(struct audio_stream_out *stream);
/*
 *@brief set dolby ms12 mixing level
 */
void set_ms12_ad_mixing_level(struct dolby_ms12_desc *ms12, int mixing_level);

/*
 *@brief set dolby ms12 pause
 */
void set_dolby_ms12_runtime_pause(struct dolby_ms12_desc *ms12, int is_pause);

/*
 *@brief set dolby ms12 ad mixing enable
 */
void set_ms12_ad_mixing_enable(struct dolby_ms12_desc *ms12, int ad_mixing_enable);

void set_ms12_atmos_lock(struct dolby_ms12_desc *ms12, bool is_atmos_lock_on);

void set_ms12_acmod2ch_lock(struct dolby_ms12_desc *ms12, bool is_lock_on);

void set_dolby_ms12_runtime_system_mixing_enable(struct dolby_ms12_desc *ms12, int sys_mixing_enable);

bool is_ms12_continous_mode(struct aml_audio_device *adev);
/*
 *@brief get the platform's capability of DDP-ATMOS.
 */
bool is_platform_supported_ddp_atmos(bool atmos_supported, enum OUT_PORT current_out_port);
bool is_dolby_ms12_main_stream(struct audio_stream_out *stream);
bool is_support_ms12_reset(struct audio_stream_out *stream);
bool is_bypass_dolbyms12(struct audio_stream_out *stream);
bool is_dolbyms12_dap_enable(struct aml_stream_out *aml_out);


int dolby_ms12_hwsync_init(void);
int dolby_ms12_hwsync_release(void);
int dolby_ms12_hwsync_checkin_pts(int offset, int apts);
int dolby_ms12_bypass_process(struct audio_stream_out *stream, void *buffer, size_t bytes);
/*
 *@brief dolby ms12 insert one frame, it is 32ms
 */
int dolby_ms12_output_insert_oneframe(struct audio_stream_out *stream);

/*
 *@brief get how many bytes consumed by main decoder
 */
unsigned long long dolby_ms12_get_main_bytes_consumed(struct audio_stream_out *stream);
/*
 *@brief get how many pcm frames generated by main decoder
 */
unsigned long long dolby_ms12_get_main_pcm_generated(struct audio_stream_out *stream);

/*
 *@brief get ms12 continuous reset status
 */
bool is_need_reset_ms12_continuous(struct audio_stream_out *stream);

#endif //end of _AUDIO_HW_MS12_H_

#else
#include "audio_hw_ms12_v2.h"
#endif
