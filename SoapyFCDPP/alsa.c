//  alsa.c
//  Copyright (c) 2018 Albin Stigo
//  SPDX-License-Identifier: BSL-1.0

#include "alsa.h"

/* Try to get an ALSA capture handle */
snd_pcm_t* alsa_pcm_handle(const char* pcm_name,
                           const unsigned int rate,
                           snd_pcm_uframes_t frames_per_period,
                           snd_pcm_stream_t stream) {
    snd_pcm_t *pcm_handle = NULL;
    
    snd_pcm_hw_params_t *hwparams;
    //snd_pcm_sw_params_t *swparams;
    
    const unsigned int periods = 4;       // Number of periods in ALSA ringbuffer.
    const unsigned int channels = 2;      // Always 2 channel IQ samples
    
    snd_pcm_hw_params_alloca(&hwparams);
    // snd_pcm_sw_params_alloca(&swparams);
    
    /* Open normal blocking */
    if (snd_pcm_open(&pcm_handle, pcm_name, stream, 0) < 0) {
        fprintf(stderr, "Error opening PCM device %s\n", pcm_name);
        exit(EXIT_FAILURE);
    }
    
    /* Init hwparams with full configuration space */
    if (snd_pcm_hw_params_any(pcm_handle, hwparams) < 0) {
        fprintf(stderr, "Can not configure this PCM device.\n");
        exit(EXIT_FAILURE);
    }
    
    /* Interleaved access. (IQ interleaved). */
#ifdef FCDPP_USE_MMAP
    if (snd_pcm_hw_params_set_access(pcm_handle, hwparams, SND_PCM_ACCESS_MMAP_INTERLEAVED) < 0) {
#else
    if (snd_pcm_hw_params_set_access(pcm_handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
#endif
        fprintf(stderr, "Error setting access.\n");
        exit(EXIT_FAILURE);
    }
    
    
    /* Set number of channels */
    if (snd_pcm_hw_params_set_channels(pcm_handle, hwparams, channels) < 0) {
        fprintf(stderr, "Error setting channels.\n");
        exit(EXIT_FAILURE);
    }
    
    /* Set sample format */
    /* Use native format of device to avoid costly conversions */
    if (snd_pcm_hw_params_set_format(pcm_handle, hwparams, SND_PCM_FORMAT_S16) < 0) {
        fprintf(stderr, "Error setting format.\n");
        exit(EXIT_FAILURE);
    }
    
    /* Set sample rate. If the exact rate is not supported exit */
    if (snd_pcm_hw_params_set_rate(pcm_handle, hwparams, rate, 0) < 0) {
        fprintf(stderr, "Error setting rate.\n");
        exit(EXIT_FAILURE);
    }
    
    /* Period size */
    int dir = 0;
    if (snd_pcm_hw_params_set_period_size(pcm_handle, hwparams, frames_per_period, dir) < 0) {
        fprintf(stderr, "Error setting period size.\n");
        exit(EXIT_FAILURE);
    }
    
    /* Set number of periods. Periods used to be called fragments. */
    if (snd_pcm_hw_params_set_periods(pcm_handle, hwparams, periods, 0) < 0) {
        fprintf(stderr, "Error setting periods.\n");
        exit(EXIT_FAILURE);
    }
    
    /* Apply HW parameter settings to */
    /* PCM device and prepare device  */
    if (snd_pcm_hw_params(pcm_handle, hwparams) < 0) {
        fprintf(stderr, "Error setting HW params.\n");
        exit(EXIT_FAILURE);
    }
    
    snd_pcm_uframes_t bufs = 0;
    snd_pcm_hw_params_get_buffer_size(hwparams, &bufs);
    
    return pcm_handle;
}

