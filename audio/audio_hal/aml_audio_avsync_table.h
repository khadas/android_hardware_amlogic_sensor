/*
 * Copyright (C) 2020 Amlogic Corporation.
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



#ifndef  _AUDIO_AVSYNC_TABLE_H_
#define _AUDIO_AVSYNC_TABLE_H_

/* we use this table to tune the AV sync case,
 * if the value is big, it can delay video
 * if the value is small, it can advance the video
 */

/*First we need tune CVBS output, then tune HDMI PCM, then other format*/
#define  AVSYNC_MS12_NONTUNNEL_PCM_LATENCY               (10)
#define  AVSYNC_MS12_NONTUNNEL_DDP_LATENCY               (20)
#define  AVSYNC_MS12_NONTUNNEL_ATMOS_LATENCY             (15)
#define  AVSYNC_MS12_TUNNEL_PCM_LATENCY                  (60)
#define  AVSYNC_MS12_TUNNEL_DDP_LATENCY                  (35)
#define  AVSYNC_MS12_TUNNEL_ATMOS_LATENCY                (20)

#define  AVSYNC_MS12_NONTUNNEL_AC4_LATENCY               (70)
#define  AVSYNC_MS12_TUNNEL_AC4_LATENCY                  (50)

#define  AVSYNC_MS12_NONTUNNEL_BYPASS_LATENCY            (-130)
#define  AVSYNC_MS12_TUNNEL_BYPASS_LATENCY               (-220)


#define  AVSYNC_MS12_PCM_OUT_LATENCY                     (0)
#define  AVSYNC_MS12_DD_OUT_LATENCY                      (50)
#define  AVSYNC_MS12_DDP_OUT_LATENCY                     (60)
#define  AVSYNC_MS12_MAT_OUT_LATENCY                     (90)

#define  AVSYNC_MS12_HDMI_ARC_OUT_LATENCY                (0)
#define  AVSYNC_MS12_HDMI_OUT_LATENCY                    (60)
#define  AVSYNC_MS12_HDMI_SPEAKER_LATENCY                (0)

#define  AVSYNC_MS12_NONTUNNEL_PCM_LATENCY_PROPERTY      "vendor.media.audio.hal.ms12.nontunnel.pcm"
#define  AVSYNC_MS12_NONTUNNEL_DDP_LATENCY_PROPERTY      "vendor.media.audio.hal.ms12.nontunnel.ddp"
#define  AVSYNC_MS12_NONTUNNEL_ATMOS_LATENCY_PROPERTY    "vendor.media.audio.hal.ms12.nontunnel.atmos"
#define  AVSYNC_MS12_NONTUNNEL_AC4_LATENCY_PROPERTY      "vendor.media.audio.hal.ms12.nontunnel.ac4"
#define  AVSYNC_MS12_NONTUNNEL_BYPASS_LATENCY_PROPERTY   "vendor.media.audio.hal.ms12.nontunnel.bypass"


#define  AVSYNC_MS12_TUNNEL_PCM_LATENCY_PROPERTY         "vendor.media.audio.hal.ms12.tunnel.pcm"
#define  AVSYNC_MS12_TUNNEL_DDP_LATENCY_PROPERTY         "vendor.media.audio.hal.ms12.tunnel.ddp"
#define  AVSYNC_MS12_TUNNEL_ATMOS_LATENCY_PROPERTY       "vendor.media.audio.hal.ms12.tunnel.atmos"
#define  AVSYNC_MS12_TUNNEL_AC4_LATENCY_PROPERTY         "vendor.media.audio.hal.ms12.tunnel.ac4"
#define  AVSYNC_MS12_TUNNEL_BYPASS_LATENCY_PROPERTY      "vendor.media.audio.hal.ms12.tunnel.bypass"


#define  AVSYNC_MS12_PCM_OUT_LATENCY_PROPERTY            "vendor.media.audio.hal.ms12.pcmout"
#define  AVSYNC_MS12_DDP_OUT_LATENCY_PROPERTY            "vendor.media.audio.hal.ms12.ddpout"
#define  AVSYNC_MS12_DD_OUT_LATENCY_PROPERTY             "vendor.media.audio.hal.ms12.ddout"
#define  AVSYNC_MS12_MAT_OUT_LATENCY_PROPERTY            "vendor.media.audio.hal.ms12.matout"


/*netflix tunning part*/
#define  AVSYNC_MS12_NETFLIX_NONTUNNEL_PCM_LATENCY       (10)
#define  AVSYNC_MS12_NETFLIX_NONTUNNEL_DDP_LATENCY       (20)
#define  AVSYNC_MS12_NETFLIX_NONTUNNEL_ATMOS_LATENCY     (15)
#define  AVSYNC_MS12_NETFLIX_TUNNEL_PCM_LATENCY          (0)
#define  AVSYNC_MS12_NETFLIX_TUNNEL_DDP_LATENCY          (50)
#define  AVSYNC_MS12_NETFLIX_TUNNEL_ATMOS_LATENCY        (5)

#define  AVSYNC_MS12_NETFLIX_PCM_OUT_LATENCY             (0)
#define  AVSYNC_MS12_NETFLIX_DD_OUT_LATENCY              (0)
#define  AVSYNC_MS12_NETFLIX_DDP_OUT_LATENCY             (0)
#define  AVSYNC_MS12_NETFLIX_MAT_OUT_LATENCY             (0)

#define  AVSYNC_MS12_NETFLIX_PCM_OUT_LATENCY_PROPERTY    "vendor.media.audio.hal.ms12.netflix.pcmout"
#define  AVSYNC_MS12_NETFLIX_DDP_OUT_LATENCY_PROPERTY    "vendor.media.audio.hal.ms12.netflix.ddpout"
#define  AVSYNC_MS12_NETFLIX_DD_OUT_LATENCY_PROPERTY     "vendor.media.audio.hal.ms12.netflix.ddout"
#define  AVSYNC_MS12_NETFLIX_MAT_OUT_LATENCY_PROPERTY    "vendor.media.audio.hal.ms12.netflix.matout"

#define  AVSYNC_MS12_NETFLIX_NONTUNNEL_PCM_LATENCY_PROPERTY      "vendor.media.audio.hal.ms12.netflix.nontunnel.pcm"
#define  AVSYNC_MS12_NETFLIX_NONTUNNEL_DDP_LATENCY_PROPERTY      "vendor.media.audio.hal.ms12.netflix.nontunnel.ddp"
#define  AVSYNC_MS12_NETFLIX_NONTUNNEL_ATMOS_LATENCY_PROPERTY    "vendor.media.audio.hal.ms12.netflix.nontunnel.atmos"


#define  AVSYNC_MS12_NETFLIX_TUNNEL_PCM_LATENCY_PROPERTY         "vendor.media.audio.hal.ms12.netflix.tunnel.pcm"
#define  AVSYNC_MS12_NETFLIX_TUNNEL_DDP_LATENCY_PROPERTY         "vendor.media.audio.hal.ms12.netflix.tunnel.ddp"
#define  AVSYNC_MS12_NETFLIX_TUNNEL_ATMOS_LATENCY_PROPERTY       "vendor.media.audio.hal.ms12.netflix.tunnel.atmos"


/*below DDP tunning is for roku tv*/
#define  AVSYNC_DDP_NONTUNNEL_PCM_LATENCY               (0)
#define  AVSYNC_DDP_NONTUNNEL_RAW_LATENCY               (0)
#define  AVSYNC_DDP_TUNNEL_PCM_LATENCY                  (0)
#define  AVSYNC_DDP_TUNNEL_RAW_LATENCY                  (0)


#define  AVSYNC_DDP_NONTUNNEL_PCM_LATENCY_PROPERTY      "vendor.media.audio.hal.ddp.nontunnel.pcm"
#define  AVSYNC_DDP_NONTUNNEL_RAW_LATENCY_PROPERTY      "vendor.media.audio.hal.ddp.nontunnel.raw"
#define  AVSYNC_DDP_TUNNEL_PCM_LATENCY_PROPERTY         "vendor.media.audio.hal.ddp.tunnel.pcm"
#define  AVSYNC_DDP_TUNNEL_RAW_LATENCY_PROPERTY         "vendor.media.audio.hal.ddp.tunnel.raw"



#endif

