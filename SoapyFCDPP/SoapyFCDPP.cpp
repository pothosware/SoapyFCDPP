//  SoapyFCDPP.cpp
//  Copyright (c) 2018 Albin Stigo
//  SPDX-License-Identifier: BSL-1.0

#include "SoapyFCDPP.hpp"

#include <SoapySDR/Logger.hpp>
#include <algorithm>
#include <cmath>

SoapyFCDPP::SoapyFCDPP(const std::string &hid_path, const std::string &alsa_device, const bool is_plus) :
    is_pro_plus(is_plus),
    d_pcm_handle(nullptr),
    d_frequency(0),
    d_lna_gain(0),
    d_bias_tee(false),
    d_mixer_gain(0),
    d_if_gain(0),
    d_trim_ppm(0.0),
    d_hid_path(hid_path),
    d_alsa_device(alsa_device) {

    d_sample_rate=is_pro_plus?192000.:96000.; // This is the default samplerate
    d_period_size=d_sample_rate/4; // default to 250ms sample periods to keep context switch rates low
    d_handle = hid_open_path(d_hid_path.c_str());
    if (d_handle == nullptr) {
        throw std::runtime_error("hid_open_path failed to open: " + d_hid_path);
    }
    // Sample buffer x 2 because complex data.
    d_buff.resize(2 * d_period_size);
}

SoapyFCDPP::~SoapyFCDPP()
{
    if (d_handle != nullptr) {
        hid_close(d_handle);
    }
}

// Identification API
std::string SoapyFCDPP::getDriverKey() const
{
    return "FCDPP";
}

std::string SoapyFCDPP::getHardwareKey() const
{
    return is_pro_plus?"FCDPP":"FCD";
}

// Channels API
size_t SoapyFCDPP::getNumChannels(const int dir) const
{
    return (dir == SOAPY_SDR_RX) ? 1 : 0;
}

bool SoapyFCDPP::getFullDuplex(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_DEBUG, "getFullDuplex");
    return false;
}

// Stream API
std::vector<std::string> SoapyFCDPP::getStreamFormats(const int direction, const size_t channel) const
{
    std::vector<std::string> formats;
    formats.push_back("CS16");
    formats.push_back("CF32");
    return formats;
}

std::string SoapyFCDPP::getNativeStreamFormat(const int direction, const size_t channel, double &fullScale) const
{
    // TODO
    fullScale = INT16_MAX;
    return "CS16";
}

SoapySDR::ArgInfoList SoapyFCDPP::getStreamArgsInfo(const int direction, const size_t channel) const
{
    SoapySDR::ArgInfoList streamArgs;
    return streamArgs;
}

SoapySDR::Stream *SoapyFCDPP::setupStream(const int direction, const std::string &format, const std::vector<size_t> &channels, const SoapySDR::Kwargs &args)
{
    SoapySDR_log(SOAPY_SDR_INFO, "setup stream");
    if (direction != SOAPY_SDR_RX) {
        throw std::runtime_error("setupStream only RX supported");
    }
    
    //check the channel configuration
    if (channels.size() > 1 or (channels.size() > 0 and channels.at(0) != 0))
    {
        throw std::runtime_error("setupStream invalid channel selection");
    }
    
    SoapySDR_logf(SOAPY_SDR_DEBUG, "Wants format %s", format.c_str());
    
    // Format converter function
#if SOAPY_SDR_API_VERSION >= 0x00070000
    d_converter_func = SoapySDR::ConverterRegistry::getFunction("CS16", format);
    assert(d_converter_func != nullptr);
#else
    is_cf32 = (format == "CF32");
#endif
    
    d_pcm_handle = alsa_pcm_handle(d_alsa_device.c_str(), (unsigned int)d_sample_rate, d_period_size, SND_PCM_STREAM_CAPTURE);
    assert(d_pcm_handle != nullptr);
    
    return (SoapySDR::Stream *) this;
}

void SoapyFCDPP::closeStream(SoapySDR::Stream *stream)
{
    SoapySDR_log(SOAPY_SDR_INFO, "close stream");
    if (d_pcm_handle != nullptr) {
        snd_pcm_drop(d_pcm_handle);
        snd_pcm_close(d_pcm_handle);
    }
}

size_t SoapyFCDPP::getStreamMTU(SoapySDR::Stream *stream) const
{
    SoapySDR_log(SOAPY_SDR_INFO, "get mtu");
    return d_period_size;
}

int SoapyFCDPP::activateStream(SoapySDR::Stream *stream,
                               const int flags,
                               const long long timeNs,
                               const size_t numElems)
{
    int err = 0;
    
    SoapySDR_log(SOAPY_SDR_INFO, "activate stream");
    
    return 0;
    err = snd_pcm_prepare(d_pcm_handle);
    if (err < 0) {
        SoapySDR_logf(SOAPY_SDR_WARNING, "snd_pcm_start %s", snd_strerror(err));
    }
    
    return 0;
}

int SoapyFCDPP::deactivateStream(SoapySDR::Stream *stream, const int flags, const long long timeNs)
{
    SoapySDR_log(SOAPY_SDR_INFO, "deactivate stream");
    // drop stops recording imiditatly
    snd_pcm_drop(d_pcm_handle);
    
    if (flags != 0)
        return SOAPY_SDR_NOT_SUPPORTED;
    
    return 0;
}

#if SOAPY_SDR_API_VERSION >= 0x00070000
#else
void SoapyFCDPP::convertCS16toCF32(void *dst, void *src, size_t samples)
{
    int16_t *cs16 = (int16_t *)src;
    float *cf32   = (float *)dst;
    // convert complex samples in range -INT16_MAX>INT16_MAX to -1.0>1.0
    for (size_t idx=0; idx<samples; idx++) {
        int16_t si = cs16[2*idx];
        int16_t sq = cs16[2*idx+1];
        cf32[2*idx]   = (float)si/(float)INT16_MAX;
        cf32[2*idx+1] = (float)sq/(float)INT16_MAX;
    }
}
#endif

int SoapyFCDPP::readStream(SoapySDR::Stream *stream,
                           void * const *buffs,
                           const size_t numElems,
                           int &flags,
                           long long &timeNs,
                           const long timeoutUs)
{
    int err = 0;
    snd_pcm_sframes_t n_err = 0;
    
    SoapySDR_log(SOAPY_SDR_TRACE, "read stream");

    // This function has to be well defined at all times
    if (d_pcm_handle == nullptr) {
        return 0;
    }
    
    snd_pcm_state_t state = snd_pcm_state(d_pcm_handle);
    switch (state) {
        case SND_PCM_STATE_OPEN:
            // not setup properly, we should not get here.
            return SOAPY_SDR_STREAM_ERROR;
        case SND_PCM_STATE_SETUP:
            if((err = snd_pcm_prepare(d_pcm_handle)) < 0) {
                // could not prepare
                SoapySDR_logf(SOAPY_SDR_ERROR, "snd_pcm_prepare %s", snd_strerror(err));
                return SOAPY_SDR_STREAM_ERROR;
            } // fallthrough
        case SND_PCM_STATE_PREPARED:
            // not started
            if((err = snd_pcm_start(d_pcm_handle)) < 0) {
                // could not start
                SoapySDR_logf(SOAPY_SDR_ERROR, "snd_pcm_start %s", snd_strerror(err));
                return SOAPY_SDR_STREAM_ERROR;
            } // fallthrough
        case SND_PCM_STATE_RUNNING:
            // wait for timeout
            if(snd_pcm_wait(d_pcm_handle, int(timeoutUs / 1000.f)) == 0)
                return SOAPY_SDR_TIMEOUT;
            
            // read but no more than one ALSA period. Less will probably be requested.
            n_err = snd_pcm_readi(d_pcm_handle,
                                  &d_buff[0],
                                  std::min<size_t>(d_period_size, numElems));
            if(n_err >= 0) {
                // read ok, convert and return.
#if SOAPY_SDR_API_VERSION >= 0x00070000
                d_converter_func(&d_buff[0], buffs[0], n_err, 1.0);
#else
                if (is_cf32)
                    convertCS16toCF32(buffs[0], &d_buff[0], n_err);
                else
                    memcpy(buffs[0], &d_buff[0], n_err * 4);
#endif
                return (int) n_err;
            } // error, fallthrough
        case SND_PCM_STATE_XRUN:
            err = (int) n_err;
            // try to recover from error.
            if(snd_pcm_recover(d_pcm_handle, err, 0) == 0) {
                // Recover ok
                SoapySDR_logf(SOAPY_SDR_ERROR,
                              "readStream recovered from %s",
                              snd_strerror(err));
                return SOAPY_SDR_OVERFLOW;
            } else {
                // Could not recover
                SoapySDR_logf(SOAPY_SDR_ERROR, "readStream error: %s", snd_strerror(err));
                return SOAPY_SDR_STREAM_ERROR;
            } // this clause always returns
        default:
            SoapySDR_logf(SOAPY_SDR_ERROR,
                          "unknown ALSA state: %s",
                          state);
    }
    return SOAPY_SDR_STREAM_ERROR;
}


std::vector<std::string> SoapyFCDPP::listAntennas(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_INFO, "listAntennas");
    
    std::vector<std::string> antennas;
    if (is_pro_plus) {
        // Only FCD Pro+ has a Bias Tee
        antennas.push_back("Bias_T_Off");
        antennas.push_back("Bias_T_On");
    }
    return antennas;
}

void SoapyFCDPP::setAntenna(const int direction, const size_t channel, const std::string &name)
{
    SoapySDR_log(SOAPY_SDR_INFO, "setAntenna");
    if (is_pro_plus) {
        // Only FCD Pro+ has a Bias Tee
        if (name == "Bias_T_Off") {
            if (d_bias_tee) {
                d_bias_tee = false;
                fcdpp_set_bias_tee(d_handle, d_bias_tee);
            }
        } else if (name == "Bias_T_On") {
            if (!d_bias_tee) {
                d_bias_tee = true;
                fcdpp_set_bias_tee(d_handle, d_bias_tee);
            }
        } else {
            SoapySDR_logf(SOAPY_SDR_DEBUG, "setAntenna: unknown element %s", name.c_str());         
        }
    }
}

std::string SoapyFCDPP::getAntenna(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_INFO, "getAntenna");
    return is_pro_plus? (d_bias_tee ? "Bias_T_On" : "Bias_T_Off") : "undef";
}

bool SoapyFCDPP::hasDCOffsetMode(const int direction, const size_t channel) const
{
    return false;
}

std::vector<std::string> SoapyFCDPP::listGains(const int direction, const size_t channel) const
{
    //list available gain elements,
    //the functions below have a "name" parameter
    std::vector<std::string> results;
    results.push_back("LNA");
    //results.push_back("BiasT");
    results.push_back("Mixer");
    results.push_back("IF");
    return results;
}

bool SoapyFCDPP::hasGainMode(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_DEBUG, "hasGainMode");
    return false;
}

void SoapyFCDPP::setGainMode(const int direction, const size_t channel, const bool automatic)
{
    SoapySDR_log(SOAPY_SDR_DEBUG, "setGainMode");
}

bool SoapyFCDPP::getGainMode(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_DEBUG, "getGainMode");
    return false;
}

void SoapyFCDPP::setGain(const int direction, const size_t channel, const double value)
{
    SoapySDR_log(SOAPY_SDR_DEBUG, "setGain");
    // The superclass actually does something here
    SoapySDR::Device::setGain(direction, channel, value);
}

void SoapyFCDPP::setGain(const int direction, const size_t channel, const std::string &name, const double value)
{
    SoapySDR_logf(SOAPY_SDR_INFO, "Setting %s gain: %f", name.c_str(), value);
    
    // @see getGainRanges below for info on Pro+ gain mapping...
    // in all switchable cases, we choose to switch at mid-point in dB
    if (name == "LNA") {
        if (d_lna_gain == value)
            return;
        if (is_pro_plus) {
            d_lna_gain = (value > 5.0) ? 10.0 : 0.0;
            fcdpp_set_lna_gain(d_handle, (value > 0.5));
        } else {
            uint8_t idx = mapLNAGain(value, &d_lna_gain);
            fcdpp_set_lna_gain(d_handle, idx);
        }

    } else if (name == "Mixer") {
        if (d_mixer_gain == value)
            return;
        if (is_pro_plus) {
            d_mixer_gain = (value > 10.0) ? 20.0 : 0.0;
            fcdpp_set_mixer_gain(d_handle, (value > 10.0));
        } else {
            d_mixer_gain = (value > 8.0) ? 12.0 : 4.0;
            fcdpp_set_mixer_gain(d_handle, (value > 8.0));
        }

    } else if (name == "IF"){
        if (d_if_gain == value)
            return;
        // clamp minimum as caller's ask for values below range minimum..
        double actual = (is_pro_plus?0.0:3.0);
        if (value > actual)
            actual = value;
        // We rely on fcd lib to handle variation between FCD types
        d_if_gain = roundf(actual);
        fcdpp_set_if_gain(d_handle, is_pro_plus, floor(actual));

    } else {
        SoapySDR_logf(SOAPY_SDR_ERROR, "setGain: unknown element '%s'", name.c_str());
    }
}

uint8_t SoapyFCDPP::mapLNAGain(double db, double *actual)
{
    static const double lnagainvalues[]={-5.0,-2.5,-999,-999,0,2.5,5,7.5,10,12.5,15,17.5,20,25,30};
    #define N_LNA_GAINS (sizeof(lnagainvalues)/sizeof(double))
    // find the nearest index to requested gain
    *actual = lnagainvalues[0];
    if (db<=*actual)
        return 0;
    *actual = lnagainvalues[N_LNA_GAINS-1];
    if (db>=*actual)
        return N_LNA_GAINS-1;
    uint8_t idx=1;
    while (idx<N_LNA_GAINS-1) {
        *actual = lnagainvalues[idx];
        if (*actual>db)
            break;
        ++idx;
    }
    return idx;
}

double SoapyFCDPP::getGain(const int direction, const size_t channel, const std::string &name) const
{
    double gain = 0.0;
    if (name == "LNA") {
        gain = d_lna_gain;
    } else if (name == "Mixer") {
        gain = d_mixer_gain;
    } else if (name == "IF"){
        gain = d_if_gain;
    } else {
        SoapySDR_logf(SOAPY_SDR_ERROR, "getGain: unknown element %s", name.c_str());
        return gain;
    }
    SoapySDR_logf(SOAPY_SDR_DEBUG, "getGain %s = %f", name.c_str(), gain);
    return gain;
}

SoapySDR::Range SoapyFCDPP::getGainRange(const int direction, const size_t channel, const std::string &name) const
{
    SoapySDR_log(SOAPY_SDR_DEBUG, "getGainRange");
    
    // Pro+ gains taken from the comments here: http://www.funcubedongle.com/?page_id=1225
    // Pro gains from datasheet of E4000
    if (name == "LNA") {
        // Pro+ LNA is 0/+10dB switchable
        return is_pro_plus ? SoapySDR::Range(0,10,10) : SoapySDR::Range(-5.0,30,2.5);
    } else if (name == "Mixer") {
        // Pro+ Mixer is 0/+20dB switchable
        return is_pro_plus ? SoapySDR::Range(0,20,20) : SoapySDR::Range(4,12,8);
    } else if (name == "IF"){
        // Pro+ IF is 0-59dB in 1dB steps
        return is_pro_plus ? SoapySDR::Range(0,59,1) : SoapySDR::Range(3,57,1);
    } else {
        throw std::runtime_error("getGainRange: unknown gain element");
    }
}

// Frequency
void SoapyFCDPP::setFrequency(const int direction,
                              const size_t channel,
                              const std::string &name,
                              const double frequency,
                              const SoapySDR::Kwargs &args)
{
    SoapySDR_logf(SOAPY_SDR_DEBUG, "setFrequency: %f", frequency);
    
    int err = 1;
    
    if (name == "RF" && d_frequency != frequency)
    {
        err = fcdpp_set_freq_hz(d_handle, uint32_t(frequency), d_trim_ppm);
        if (err > 0) {
            d_frequency = frequency;
        } else {
            SoapySDR_log(SOAPY_SDR_ERROR, "setFrequency failed to set device frequency");
        }
    }
}

double SoapyFCDPP::getFrequency(const int direction, const size_t channel, const std::string &name) const
{
    SoapySDR_logf(SOAPY_SDR_INFO, "getFrequency");
    return d_frequency;
}

std::vector<std::string> SoapyFCDPP::listFrequencies(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_INFO, "listFrequencies");
    std::vector<std::string> names;
    names.push_back("RF");
    return names;
}

SoapySDR::RangeList SoapyFCDPP::getFrequencyRange(const int direction, const size_t channel, const std::string &name) const
{
    SoapySDR_log(SOAPY_SDR_INFO, "getFrequencyRange");
    
    SoapySDR::RangeList results;
    if (name == "RF")
    {
        results.push_back(SoapySDR::Range(150000, 240000000));
        results.push_back(SoapySDR::Range(420000000, 1900000000));
    }
    return results;
}

SoapySDR::ArgInfoList SoapyFCDPP::getFrequencyArgsInfo(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_INFO, "getFrequencyArgsInfo");
    
    SoapySDR::ArgInfoList freqArgs;
    // TODO: frequency arguments
    return freqArgs;
    
}

bool SoapyFCDPP::hasFrequencyCorrection(const int direction, const size_t channel) const
{
    return true;
}

void SoapyFCDPP::setFrequencyCorrection(const int direction, const size_t channel, const double value)
{
    SoapySDR_logf(SOAPY_SDR_DEBUG, "setFreqCorrection %f", value);
    d_trim_ppm = value;

    if (d_frequency != 0.0) {  // don't set correction until tuned initially
        int err = fcdpp_set_freq_hz(d_handle, uint32_t(d_frequency), d_trim_ppm);
        if (err <= 0) {
            SoapySDR_log(SOAPY_SDR_ERROR, "setFrequencyCorrection failed to set device frequency");
        }
    }
}

double SoapyFCDPP::getFrequencyCorrection(const int direction, const size_t channel) const
{
    return d_trim_ppm;
}


void SoapyFCDPP::setSampleRate(const int direction, const size_t channel, const double rate)
{
    SoapySDR_logf(SOAPY_SDR_DEBUG, "setSampleRate %f", rate);
}

double SoapyFCDPP::getSampleRate(const int direction, const size_t channel) const
{
    SoapySDR_logf(SOAPY_SDR_DEBUG, "getSampleRate %f", d_sample_rate);
    return d_sample_rate;
}

std::vector<double> SoapyFCDPP::listSampleRates(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_INFO, "listSampleRates");
    
    std::vector<double> rates;
    rates.push_back(d_sample_rate);
    return rates;
}

void SoapyFCDPP::setBandwidth(const int direction, const size_t channel, const double bw)
{
    SoapySDR_log(SOAPY_SDR_INFO, "setBandwidth");
    SoapySDR::Device::setBandwidth(direction, channel, bw);
}

double SoapyFCDPP::getBandwidth(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_INFO, "getBandwidth");
    return SoapySDR::Device::getBandwidth(direction, channel);
}

SoapySDR::ArgInfoList SoapyFCDPP::getSettingInfo(void) const
{
    SoapySDR::ArgInfoList settings;
    
    SoapySDR_log(SOAPY_SDR_DEBUG, "getSettingInfo");
    
    return settings;
}

void SoapyFCDPP::writeSetting(const std::string &key, const std::string &value)
{
    SoapySDR_log(SOAPY_SDR_DEBUG, "writeSetting");
}

std::string SoapyFCDPP::readSetting(const std::string &key) const
{
    SoapySDR_log(SOAPY_SDR_DEBUG, "readSetting");
    return "empty";
}

std::vector<double> SoapyFCDPP::listBandwidths(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_INFO, "listBandwidths");
    std::vector<double> results;
    return results;
}

// Registry
std::string findAlsaDevice(const char *hidpath)
{
    // Stolen from fcdctl, we locate the audio device for this HID interface by USB bus ID:
    // https://github.com/phlash/fcdctl/blob/8f9e855db26f531c1e2af0dcd3648e566e7f05b9/main.c#L76
    int usb1, usb2, n = sscanf(hidpath,"%x:%x", &usb1, &usb2);
    assert(n==2);
    for (n=0; n<16; n++) {
        char aspath[32];
        sprintf(aspath,"/proc/asound/card%d/usbbus", n);
        FILE *fp = fopen(aspath,"r");
        if (fp) {
            fgets(aspath,sizeof(aspath),fp);
            fclose(fp);
            int asu1, asu2;
            sscanf(aspath,"%d/%d", &asu1, &asu2);
            if (asu1==usb1 && asu2==usb2) {
                sprintf(aspath,"hw:%d,0",n);
                return aspath;
            }
        }
    }
    return "not found";
}

SoapySDR::KwargsList findFCDPP(const SoapySDR::Kwargs &args)
{
    SoapySDR_log(SOAPY_SDR_TRACE, "findFCDPP");
    
    SoapySDR::KwargsList results;
    
    // Find all devices
    struct hid_device_info *devs, *cur_dev;
    
    devs = hid_enumerate(FCDPP_VENDOR_ID, FCDPP_PRODUCT_ID);
    cur_dev = devs;
    while (cur_dev) {
        SoapySDR::Kwargs soapyInfo;
        // This is the name that shows up.
        soapyInfo["device"] = "Funcube Dongle Pro+ (192k)";
        soapyInfo["hid_path"] = cur_dev->path;
        soapyInfo["is_plus"] = "true";
        soapyInfo["alsa_device"] = findAlsaDevice(cur_dev->path);
        SoapySDR_logf(SOAPY_SDR_TRACE, "Found device: %s, %s", cur_dev->path, soapyInfo["alsa_device"].c_str());
        cur_dev = cur_dev->next;
        results.push_back(soapyInfo);
    }
    hid_free_enumeration(devs);
    // now check for older Pro devices
    devs = hid_enumerate(FCDPP_VENDOR_ID, FCD_PRODUCT_ID);
    cur_dev = devs;
    while (cur_dev) {
        SoapySDR::Kwargs soapyInfo;
        // This is the name that shows up.
        soapyInfo["device"] = "Funcube Dongle Pro (96k)";
        soapyInfo["hid_path"] = cur_dev->path;
        soapyInfo["is_plus"] = "false";
        soapyInfo["alsa_device"] = findAlsaDevice(cur_dev->path);
        SoapySDR_logf(SOAPY_SDR_TRACE, "Found device: %s, %s", cur_dev->path, soapyInfo["alsa_device"].c_str());
        cur_dev = cur_dev->next;
        results.push_back(soapyInfo);
    }
    hid_free_enumeration(devs);
    
    SoapySDR_logf(SOAPY_SDR_TRACE, "findFCDPP=%d devices", results.size());
    return results;
}

SoapySDR::Device *makeFCDPP(const SoapySDR::Kwargs &args)
{
    SoapySDR_log(SOAPY_SDR_INFO, "makeFCDPP");
    
    std::string hid_path = args.at("hid_path");
    std::string alsa_device = args.at("alsa_device");
    bool is_plus = args.at("is_plus")=="true";
    return (SoapySDR::Device*) new SoapyFCDPP(hid_path, alsa_device, is_plus);
}

/* Register this driver */
static SoapySDR::Registry registerFCDPP("fcdpp", &findFCDPP, &makeFCDPP, SOAPY_SDR_ABI_VERSION);
