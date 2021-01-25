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

// gain ranges for each stage in FCD Pro (V1.x)
static int8_t gains1[]={-3,6};
static int8_t gains2[]={0,3,6,9};
static int8_t gains3[]={0,3,6,9};
static int8_t gains4[]={0,1,2,3};
static int8_t gains5[]={3,6,9,12,15};
static int8_t gains6[]={3,6,9,12,15};
static int8_t *if_gains[]={gains1, gains2, gains3, gains4, gains5, gains6};

// gain distribution cutoff points (see below for usage)
static int8_t cutoff1[]={1,45};
static int8_t cutoff2[]={6,39,42,45,48,51,54};
static int8_t cutoff3[]={3,30,33,36};
static int8_t cutoff4[]={0};
static int8_t cutoff5[]={4,18,21,24,27};
static int8_t cutoff6[]={4,6,9,12,15};
static int8_t *cutoffs[]={cutoff1,cutoff2,cutoff3,cutoff4,cutoff5,cutoff6};

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

int fcdpp_get_if_gain(hid_device *device, uint8_t is_plus)
{
    if (is_plus) {
	    int err = 0;
	    uint8_t buf[BUF_LEN] = { 0x00 };
	    buf[1] = FCD_HID_CMD_GET_IF_GAIN;
	    err = hid_wr_timeout(device, buf, BUF_LEN);
	    if (err < 0) return err;
	    return buf[2];
    } else {
        // Read all gain registers, map and add together..
        uint8_t regs[] = {
            FCD_HID_CMD_GET_IF_GAIN1,
            FCD_HID_CMD_GET_IF_GAIN2,
            FCD_HID_CMD_GET_IF_GAIN3,
            FCD_HID_CMD_GET_IF_GAIN4,
            FCD_HID_CMD_GET_IF_GAIN5,
            FCD_HID_CMD_GET_IF_GAIN6
        };
        int8_t sum = 0;
        for (int i=0; i<6; i++) {
            int err = 0;
            uint8_t buf[BUF_LEN] = { 0x00 };
            buf[1] = regs[i];
            err = hid_wr_timeout(device, buf, BUF_LEN);
            if (err < 0) return err;
            sum += if_gains[i][buf[2]];
        }
        return sum;
    }
}

int fcdpp_set_if_gain(hid_device *device, uint8_t is_plus, uint8_t gain)
{
    if (is_plus) {
	    int err = 0;
	    uint8_t buf[BUF_LEN] = { 0x00 };
	    buf[1] = FCD_HID_CMD_SET_IF_GAIN;
	    buf[2] = gain;
	    err = hid_write(device, buf, BUF_LEN);
	    return err;
    } else {
        // distribute requested gain according to the table on pages 38-9
        // of the datasheet: https://www.nooelec.com/files/e4000datasheet.pdf
        // in 'linearity mode'. We use cutoff values derived from the table
        // for most gains, apart from gain4 which is (gain mod 3)..and gain2
        // which gets reduced by 3 if gain1 is set - neat =)
        int err = 0;
        uint8_t regs[]={
            FCD_HID_CMD_SET_IF_GAIN1,
            FCD_HID_CMD_SET_IF_GAIN2,
            FCD_HID_CMD_SET_IF_GAIN3,
            FCD_HID_CMD_SET_IF_GAIN4,
            FCD_HID_CMD_SET_IF_GAIN5,
            FCD_HID_CMD_SET_IF_GAIN6
        };
        uint8_t pv=0;
        for (int i=0; i<6; i++) {
            int8_t *cutoff=cutoffs[i];
            uint8_t val=0;
            for (uint8_t j=1; j<=cutoff[0]; j++) {
                if (gain>=cutoff[j])
                    val = j;
            }
            // special cases
            if (1==i && pv) val-=3;
            if (3==i) val=gain%3;
            pv = val;
            // write to FCD
            uint8_t buf[BUF_LEN] = { 0x00 };
            buf[1] = regs[i];
            buf[2] = val;
            err = hid_write(device, buf, BUF_LEN);
            if (err<0) break;
        }
        return err;
    }
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
