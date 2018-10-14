#include "SoapyFCDPP.hpp"
#include <SoapySDR/Logger.hpp>

#include <algorithm>

SoapyFCDPP::SoapyFCDPP(const std::string &hid_path, const std::string &alsa_device) :
d_pcm_handle(nullptr),
d_period_size(4096),
d_sample_rate(192000.),
d_frequency(0),
d_lna_gain(0),
d_mixer_gain(0),
d_if_gain(0),
d_hid_path(hid_path),
d_alsa_device(alsa_device)
{
    d_handle = hid_open_path(d_hid_path.c_str());
    if (d_handle == nullptr) {
        throw std::runtime_error("hid_open_path failed");
    }
    // Sample buffer
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
    return "FCDPP";
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
    fullScale = (1 << 24);
    return "CS32";
}

SoapySDR::ArgInfoList SoapyFCDPP::getStreamArgsInfo(const int direction, const size_t channel) const
{
    SoapySDR::ArgInfoList streamArgs;
    return streamArgs;
}

SoapySDR::Stream *SoapyFCDPP::setupStream(const int direction, const std::string &format, const std::vector<size_t> &channels, const SoapySDR::Kwargs &args)
{
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
    d_converter_func = SoapySDR::ConverterRegistry::getFunction("CS16", format);
    assert(d_converter_func != nullptr);
    
    //d_pcm_handle = alsa_pcm_handle("hw:CARD=V20,DEV=0", d_period_size, SND_PCM_STREAM_CAPTURE);
    d_pcm_handle = alsa_pcm_handle(d_alsa_device.c_str(), d_period_size, SND_PCM_STREAM_CAPTURE);
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
    SoapySDR_log(SOAPY_SDR_INFO, "activate stream");
    
    // snd_pcm_prepare(d_pcm_handle);
    snd_pcm_start(d_pcm_handle);
    
    return 0;
}

int SoapyFCDPP::deactivateStream(SoapySDR::Stream *stream, const int flags, const long long timeNs)
{
    SoapySDR_log(SOAPY_SDR_INFO, "deactivate stream");
    
    if (flags != 0) return SOAPY_SDR_NOT_SUPPORTED;
    
    snd_pcm_drop(d_pcm_handle);
    snd_pcm_prepare(d_pcm_handle);
    
    return 0;
}

int SoapyFCDPP::readStream(SoapySDR::Stream *stream,
                           void * const *buffs,
                           const size_t numElems,
                           int &flags,
                           long long &timeNs,
                           const long timeoutUs)
{
    // This function has to be well defined at all times
    if (d_pcm_handle == nullptr) {
        return 0;
    }
    
    // Are we running?
    if (snd_pcm_state(d_pcm_handle) != SND_PCM_STATE_RUNNING) {
        return 0;
    }
    
    // Timeout if not ready
    if(snd_pcm_wait(d_pcm_handle, int(timeoutUs / 1000)) == 0) {
        return SOAPY_SDR_TIMEOUT;
    }
    
    // Read from ALSA
    snd_pcm_sframes_t frames = 0;
    int err = 0;
    // no program is complete without a goto
    // TODO: or maybe I should just put a for loop here?
again:
    // read numElems or d_period_size
    frames = snd_pcm_readi(d_pcm_handle, &d_buff[0], std::min<size_t>(d_period_size, numElems));
    // try to handle xruns
    if(frames < 0) {
        err = (int) frames;
        if(snd_pcm_recover(d_pcm_handle, err, 0) == 0) {
            SoapySDR_logf(SOAPY_SDR_ERROR, "readStream recoverd from %s", snd_strerror(err));
            goto again;
        } else {
            SoapySDR_logf(SOAPY_SDR_ERROR, "readStream error: %s", snd_strerror(err));
            return SOAPY_SDR_STREAM_ERROR;
        }
    }
    
    // Convert. Format is setup in setupStream. 1.0 means to do nothing.
    d_converter_func(&d_buff[0], buffs[0], frames, 1.0);
    
    return (int)frames;
}


std::vector<std::string> SoapyFCDPP::listAntennas(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_INFO, "listAntennas");
    
    std::vector<std::string> antennas;
    antennas.push_back("RX");
    // antennas.push_back("TX");
    return antennas;
}

void SoapyFCDPP::setAntenna(const int direction, const size_t channel, const std::string &name)
{
    SoapySDR_log(SOAPY_SDR_INFO, "setAntenna");
    // TODO
}

std::string SoapyFCDPP::getAntenna(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_INFO, "getAntenna");
    return "RX";
    // return "TX";
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
    // d_agc_mode = automatic;
    //SoapySDR_logf(SOAPY_SDR_DEBUG, "Setting Audio AGC: %s", automatic ? "Automatic" : "Manual");
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
    
    if (name == "LNA" && d_lna_gain != value) {
        if(fcdpp_set_lna_gain(d_handle, floor(value)) > 0)
            d_lna_gain = value;
    } else if (name == "Mixer" && d_mixer_gain != value) {
        // SoapyDevice seems to only accept gain ranges but on the FCDpp
        // these are toggles. As such put a threshold at 0.5.
        if(fcdpp_set_mixer_gain(d_handle, uint8_t(value > 0.5)) > 0)
            d_mixer_gain = value;
    } else if (name == "IF" && d_if_gain != value){
        // Same as above
        if(fcdpp_set_if_gain(d_handle, uint8_t(value > 0.5)) > 0)
            d_if_gain = value;
    }
}

double SoapyFCDPP::getGain(const int direction, const size_t channel, const std::string &name) const
{
    SoapySDR_log(SOAPY_SDR_DEBUG, "getGain");
    if (name == "LNA") {
        return d_lna_gain;
    } else if (name == "Mixer") {
        return d_mixer_gain;
    } else if (name == "IF"){
        return d_if_gain;
    } else {
        SoapySDR_logf(SOAPY_SDR_DEBUG, "getGain: unknown element %s", name.c_str());
        return 0.;
    }
}

SoapySDR::Range SoapyFCDPP::getGainRange(const int direction, const size_t channel, const std::string &name) const
{
    SoapySDR_log(SOAPY_SDR_DEBUG, "getGainRange");
    
    if (name == "LNA") {
        return SoapySDR::Range(0,1,1);
    } else if (name == "Mixer") {
        return SoapySDR::Range(0,1,1);
    } else if (name == "IF"){
        return SoapySDR::Range(0,59,1);
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
    SoapySDR_log(SOAPY_SDR_DEBUG, "setFrequency");
    
    int err = 1;

    if (name == "RF" && d_frequency != frequency)
    {
        err = fcdpp_set_freq_hz(d_handle, uint32_t(frequency));
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
        SoapySDR_logf(SOAPY_SDR_INFO, "Found device: %s", cur_dev->path);
        soapyInfo["hid_path"] = cur_dev->path;
        // TODO:
        // Currently only one dongle will work and I don't know how to associate
        // HID path with an alsa path. Perhaps this could be an option to the driver.
        soapyInfo["alsa_device"] = "hw:CARD=V20,DEV=0";
        cur_dev = cur_dev->next;
        results.push_back(soapyInfo);
    }
    hid_free_enumeration(devs);
    
    return results;
}

SoapySDR::Device *makeFCDPP(const SoapySDR::Kwargs &args)
{
    SoapySDR_setLogLevel(SOAPY_SDR_DEBUG);
    SoapySDR_log(SOAPY_SDR_TRACE, "makeFCDPP");
    
    std::string hid_path = args.at("hid_path");
    std::string alsa_device = args.at("alsa_device");
    return (SoapySDR::Device*) new SoapyFCDPP(hid_path, alsa_device);
}

/* Register this driver */
static SoapySDR::Registry registerFCDPP("fcdpp", &findFCDPP, &makeFCDPP, SOAPY_SDR_ABI_VERSION);
