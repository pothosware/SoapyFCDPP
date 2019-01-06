//  alsa.h
//  Copyright (c) 2018 Albin Stigo
//  SPDX-License-Identifier: BSL-1.0

#ifndef alsa_h
#define alsa_h

#ifdef __cplusplus
extern "C"
{
#endif
    
#include <stdio.h>
#include <alsa/asoundlib.h>
    
    snd_pcm_t* alsa_pcm_handle(const char* pcm_name,
                               snd_pcm_uframes_t frames_per_period,
                               snd_pcm_stream_t stream);
    
#ifdef __cplusplus
}
#endif

#endif /* alsa_h */
