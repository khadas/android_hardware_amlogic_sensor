/*
 * Copyright (C) 2021 Amlogic Corporation.
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

#define LOG_TAG "audio_hw_primary"



#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <cutils/log.h>


#include "audio_hw.h"
#include "audio_hw_utils.h"
#include "aml_dec_api.h"
#include "aml_audio_spdifout.h"

ssize_t aml_audio_spdif_output(struct audio_stream_out *stream,
                               void *buffer, size_t byte, spdif_config_t *spdif_config)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *) stream;
    struct aml_audio_device *aml_dev = aml_out->dev;
    int ret = 0;

    if (aml_out->spdifout_handle == NULL) {
        ret = aml_audio_spdifout_open(&aml_out->spdifout_handle, spdif_config);
    }

    aml_audio_spdifout_processs(aml_out->spdifout_handle, buffer, byte);

    return ret;
}

int aml_audio_decoder_process_wrapper(struct audio_stream_out *stream, const void *buffer, size_t bytes)
{
    int ret = -1;
    int dec_used_size = 0;
    int left_bytes = 0;
    struct aml_stream_out *aml_out = (struct aml_stream_out *) stream;
    struct aml_audio_device *adev = aml_out->dev;
    int return_bytes = bytes;
    struct aml_audio_patch *patch = adev->audio_patch;
    void *output_buffer = NULL;
    size_t output_buffer_bytes = 0;

    if (aml_out->aml_dec == NULL) {
        config_output(stream, true);
    }
    aml_dec_t *aml_dec = aml_out->aml_dec;
    if (aml_dec) {
        dec_data_info_t * dec_pcm_data = &aml_dec->dec_pcm_data;
        dec_data_info_t * dec_raw_data = &aml_dec->dec_raw_data;
        left_bytes = bytes;
        do {
            ret = aml_decoder_process(aml_dec, (unsigned char *)buffer, left_bytes, &dec_used_size);
            if (ret < 0) {
                ALOGV("aml_decoder_process error");
                return return_bytes;
            }
            left_bytes -= dec_used_size;
            ALOGV("aml_decoder_process ret =%d pcm len =%d raw len=%d", ret, dec_pcm_data->data_len, dec_raw_data->data_len);
            // write pcm data
            if (dec_pcm_data->data_len > 0) {
                audio_format_t output_format = AUDIO_FORMAT_PCM_16_BIT;
                if (adev->patch_src  == SRC_DTV && adev->start_mute_flag == 1) {
                    memset(dec_pcm_data->buf, 0, dec_pcm_data->data_len);
                }
                if (dec_pcm_data->data_sr > 0) {
                    aml_out->config.rate = dec_pcm_data->data_sr;
                }
                if (patch) {
                    patch->sample_rate = dec_pcm_data->data_sr;
                }

                aml_hw_mixer_mixing(&adev->hw_mixer, dec_pcm_data->buf, dec_pcm_data->data_len, output_format);
                if (audio_hal_data_processing(stream, dec_pcm_data->buf, dec_pcm_data->data_len, &output_buffer, &output_buffer_bytes, output_format) == 0) {
                    hw_write(stream, output_buffer, output_buffer_bytes, output_format);
                }
            }

            // write raw data
            if (dec_raw_data->data_len > 0) {
                if (dec_raw_data->data_sr > 0) {
                    aml_out->config.rate = dec_raw_data->data_sr;
                }

                spdif_config_t spdif_config = { 0 };
                spdif_config.audio_format = dec_raw_data->data_format;
                if (spdif_config.audio_format == AUDIO_FORMAT_IEC61937) {
                    spdif_config.sub_format = dec_raw_data->sub_format;
                }
                spdif_config.rate = dec_raw_data->data_sr;
                if (adev->tv_mute ||
                    (adev->patch_src == SRC_DTV && adev->start_mute_flag == 1)) {
                    memset(dec_raw_data->buf, 0, dec_raw_data->data_len);
                }
                aml_audio_spdif_output(stream, (void *)dec_raw_data->buf, dec_raw_data->data_len, &spdif_config);
            }
        } while (dec_pcm_data->data_len || dec_raw_data->data_len);
    }
    return return_bytes;
}

