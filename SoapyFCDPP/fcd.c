//
//  main.c
//  fcdc
//
//  Created by Albin Stigö on 17/09/16.
//  Copyright © 2016 Albin Stigo. All rights reserved.
//

#include "fcd.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "fcdcmd.h"

#define BUF_LEN 64
#define MAX_STR 255
#define TIMEOUT_MS 20

/*
 // Debug helper
 void debug_log_buf(const char* str, const uint8_t* buf) {
 printf("%s: ", str);
 for(int i = 0; i < 16; i++) {
 printf("%02x ", buf[i]);
 }
 printf("\n");
 }
 */

// Helper function.
// Write content of buffer, then read reply into the same buffer.
int hid_wr_timeout(hid_device *device, uint8_t* buf, uint8_t len)
{
    // write
    hid_write(device, buf, len);
    //debug_log_buf("w", buf);
    // clear buffer after read
    memset(buf, 0x00, len);
    int ret = hid_read_timeout(device, buf, len, TIMEOUT_MS);
    //debug_log_buf("r", buf);
    return ret;
}

int fcdpp_get_query_str(hid_device *device, char *str)
{
    int err;
    uint8_t buf[BUF_LEN] = { 0x00 };
    buf[1] = FCD_HID_CMD_QUERY;
    err = hid_wr_timeout(device, buf, BUF_LEN);
    strncpy(str, (const char*)buf, BUF_LEN);
    return err;
}

// Get LNA gain on/off
int fcdpp_get_lna_gain(hid_device *device)
{
    int err;
    uint8_t buf[BUF_LEN] = { 0x00 };
    buf[1] = FCD_HID_CMD_GET_LNA_GAIN;
    err = hid_wr_timeout(device, buf, BUF_LEN);
    if (err < 0) return err;
    return buf[2];
}

// Set LNA gain on/off
int fcdpp_set_lna_gain(hid_device *device, uint8_t gain)
{
    int err = 0;
    uint8_t buf[BUF_LEN] = { 0x00 };
    buf[1] = FCD_HID_CMD_SET_LNA_GAIN;
    buf[2] = gain;
    err = hid_write(device, buf, BUF_LEN);
    return err;
}

// Get BIAST gain on/off
int fcdpp_get_bias_tee(hid_device *device)
{
    int err;
    uint8_t buf[BUF_LEN] = { 0x00 };
    buf[1] = FCD_HID_CMD_GET_BIAS_TEE;
    err = hid_wr_timeout(device, buf, BUF_LEN);
    if (err < 0) return err;
    return buf[2];
}

// Set BIAST gain on/off
int fcdpp_set_bias_tee(hid_device *device, uint8_t gain)
{
    int err = 0;
    uint8_t buf[BUF_LEN] = { 0x00 };
    buf[1] = FCD_HID_CMD_SET_BIAS_TEE;
    buf[2] = gain;
    err = hid_write(device, buf, BUF_LEN);
    return err;
}

tuner_rf_filter_t fcdpp_get_rf_filter(hid_device *device)
{
    int err = 0;
    uint8_t buf[BUF_LEN] = { 0x00 };
    buf[1] = FCD_HID_CMD_GET_RF_FILTER;
    err =  hid_wr_timeout(device, buf, BUF_LEN);
    if (err < 0) return err;
    return buf[2];
}

int fcdpp_set_rf_filter(hid_device *device, tuner_rf_filter_t filter)
{
    int err = 0;
    uint8_t buf[BUF_LEN] = { 0x00 };
    buf[1] = FCD_HID_CMD_SET_RF_FILTER;
    buf[2] = filter;
    err = hid_write(device, buf, BUF_LEN);
    //err = hid_wr_timeout(device, buf, BUF_LEN);
    return err;

    //if (err < 0) return err;
    //return buf[1];
}

tuner_if_filter_t fcdpp_get_if_filter(hid_device *device)
{
    int err = 0;
    uint8_t buf[BUF_LEN] = { 0x00 };
    buf[1] = FCD_HID_CMD_GET_IF_FILTER;
    err = hid_wr_timeout(device, buf, BUF_LEN);
    if (err < 0) return err;
    return buf[2];
}

int fcdpp_set_if_filter(hid_device *device)
{
    int err = 0;
    // TODO
    return err;
}

int fcdpp_get_if_gain(hid_device *device)
{
    int err = 0;
    uint8_t buf[BUF_LEN] = { 0x00 };
    buf[1] = FCD_HID_CMD_GET_IF_GAIN;
    err = hid_wr_timeout(device, buf, BUF_LEN);
    if (err < 0) return err;
    return buf[2];
}

int fcdpp_set_if_gain(hid_device *device, uint8_t gain)
{
    int err = 0;
    uint8_t buf[BUF_LEN] = { 0x00 };
    buf[1] = FCD_HID_CMD_SET_IF_GAIN;
    buf[2] = gain;
    err = hid_write(device, buf, BUF_LEN);
    return err;
}

int fcdpp_get_mixer_gain(hid_device *device) {
    int err = 0;
    uint8_t buf[BUF_LEN] = { 0x00 };
    buf[1] = FCD_HID_CMD_GET_MIXER_GAIN;
    err = hid_wr_timeout(device, buf, BUF_LEN);
    // TODO: check return code of cmd in buf[2]
    if (err < 0) return err;
    return buf[2];
}

int fcdpp_set_mixer_gain(hid_device *device, uint8_t gain)
{
    int err = 0;
    uint8_t buf[BUF_LEN] = { 0x00 };
    buf[1] = FCD_HID_CMD_SET_MIXER_GAIN;
    buf[2] = gain;
    err = hid_write(device, buf, BUF_LEN);
    return err;
}

#if 0
int fcdpp_get_bias_tee(hid_device *device)
{
    // TODO
    return -1;
}

int fcdpp_set_bias_tee(hid_device *device, uint8_t tee)
{
    // TODO
    return -1;
}
#endif
int fcdpp_set_freq_hz(hid_device *device, uint32_t freq, double trim) {
    int err;
    uint8_t buf[BUF_LEN] = { 0x00 };
    freq = (uint32_t)((double) freq * (1+(1e-6*trim)));
    // buf[0] is ignored by the FCDP+
    buf[1] = FCD_HID_CMD_SET_FREQUENCY_HZ;
    buf[2] = (uint8_t) (freq);
    buf[3] = (uint8_t) (freq >> 8);
    buf[4] = (uint8_t) (freq >> 16);
    buf[5] = (uint8_t) (freq >> 24);
    err = hid_write(device, buf, BUF_LEN);
    return err;
}

int fcdpp_set_freq_khz(hid_device *device, uint32_t freq) {
    int err = 0;
    uint8_t buf[65] = { 0x00 };
    // buf[0] is ignored by the FCDP+
    buf[1] = FCD_HID_CMD_SET_FREQUENCY_KHZ;
    buf[2] = (uint8_t) (freq);
    buf[3] = (uint8_t) (freq >> 8);
    buf[4] = (uint8_t) (freq >> 16);
    err = hid_write(device, buf, BUF_LEN);
    //err = hid_wr_timeout(device, buf, BUF_LEN);
    //if (err < 0) return err;
    //return buf[1];
    return err;
}

int fcdpp_get_freq_hz(hid_device *device) {
    int err = 0;
    uint8_t buf[BUF_LEN] = { 0x00 };
    uint32_t freq = 0;
    // buf[0] is ignored by the FCDP+
    buf[1] = FCD_HID_CMD_GET_FREQUENCY_HZ;
    err = hid_wr_timeout(device, buf, BUF_LEN);
    // TODO: check return code of command
    if (err < 0) return err;
    freq =  (uint32_t) (buf[2]);
    freq |= (uint32_t) (buf[3] << 8);
    freq |= (uint32_t) (buf[4] << 16);
    freq |= (uint32_t) (buf[5] << 24);
    return freq;
}
