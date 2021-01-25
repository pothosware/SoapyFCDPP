//
//  fcd.h
//  SoapyFCDPP
//
//  Created by Albin Stigö on 22/05/2018.
//  Copyright © 2018 Albin Stigo. All rights reserved.
//

#ifndef fcd_h
#define fcd_h

#ifdef __cplusplus
extern "C"
{
#endif
    
#include <stdint.h>
#include <hidapi.h>
#include "fcdcmd.h"
    
    int fcdpp_get_query_str(hid_device *device, char *str);
    int fcdpp_get_lna_gain(hid_device *device);
    int fcdpp_set_lna_gain(hid_device *device, uint8_t gain);
    tuner_rf_filter_t fcdpp_get_rf_filter(hid_device *device);
    int fcdpp_set_rf_filter(hid_device *device, tuner_rf_filter_t filter);
    tuner_if_filter_t fcdpp_get_if_filter(hid_device *device);
    int fcdpp_set_if_filter(hid_device *device);
    int fcdpp_get_if_gain(hid_device *device, uint8_t is_plus);
    int fcdpp_set_if_gain(hid_device *device, uint8_t is_plus, uint8_t gain);
    int fcdpp_get_bias_tee(hid_device *device);
    int fcdpp_set_bias_tee(hid_device *device, uint8_t tee);
    int fcdpp_get_mixer_gain(hid_device *device);
    int fcdpp_set_mixer_gain(hid_device *device, uint8_t gain);
    int fcdpp_set_freq_hz(hid_device *device, uint32_t freq, double ppm);
    int fcdpp_set_freq_khz(hid_device *device, uint32_t freq);
    int fcdpp_get_freq_hz(hid_device *device);
    //void log_status(hid_device *device);
    
#ifdef __cplusplus
}
#endif

#endif /* fcd_h */
