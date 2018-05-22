#include "SoapyFCDPP.hpp"
#include <SoapySDR/Logger.hpp>

#include <algorithm>
//#include <SoapySDR/ConverterPrimatives.hpp>


// CS32 <> CF32
/*static void genericCS32toCF32(const void *srcBuff, void *dstBuff, const size_t numElems, const double scaler)
{
    const size_t elemDepth = 2;
    
    if (scaler == 1.0)
    {
        auto *src = (int32_t*)srcBuff;
        auto *dst = (float*)dstBuff;
        for (size_t i = 0; i < numElems*elemDepth; i++)
        {
            dst[i] = SoapySDR::S32toF32(src[i]);
        }
    }
    else
    {
        auto *src = (int32_t*)srcBuff;
        auto *dst = (int32_t*)dstBuff;
        for (size_t i = 0; i < numElems*elemDepth; i++)
        {
            dst[i] = SoapySDR::S32toF32(src[i]) * scaler;
        }
    }
}

// CS32 <> CS16 scaler ignored
static void genericCS32toCS16(const void *srcBuff, void *dstBuff, const size_t numElems, const double scaler)
{
    const size_t elemDepth = 2;
    
    auto *src = (int32_t*)srcBuff;
    auto *dst = (int16_t*)dstBuff;
    for (size_t i = 0; i < numElems*elemDepth; i++)
    {
        dst[i] = SoapySDR::S32toS16(src[i]);
    }
}*/


SoapyFCDPP::SoapyFCDPP() :
d_agc_mode(false),
d_period_size(4096),
d_frequency(0),
d_sample_rate(89286)
{
    // Sample buffer
    d_buff.resize(2 * d_period_size);
}

SoapyFCDPP::~SoapyFCDPP()
{

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
    SoapySDR_log(SOAPY_SDR_INFO, "getFullDuplex");
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
    
    SoapySDR::ArgInfo chanArg;
    chanArg.key = "chan";
    chanArg.value = "stereo_iq";
    chanArg.name = "Channel Setup";
    chanArg.description = "Input channel configuration.";
    chanArg.type = SoapySDR::ArgInfo::STRING;
    
    std::vector<std::string> chanOpts;
    std::vector<std::string> chanOptNames;
    
    chanOpts.push_back("stereo_iq");
    chanOptNames.push_back("Complex L/R = I/Q");

    chanArg.options = chanOpts;
    chanArg.optionNames = chanOptNames;
    
    streamArgs.push_back(chanArg);
    
    return streamArgs;
}

SoapySDR::Stream *SoapyFCDPP::setupStream(const int direction, const std::string &format, const std::vector<size_t> &channels, const SoapySDR::Kwargs &args)
{
    // Register format converters once
    /*static SoapySDR::ConverterRegistry registerGenericCS32toCF32(SOAPY_SDR_CS32, SOAPY_SDR_CF32, SoapySDR::ConverterRegistry::GENERIC, &genericCS32toCF32);
    static SoapySDR::ConverterRegistry registerGenericCS32toCS16(SOAPY_SDR_CS32, SOAPY_SDR_CS16, SoapySDR::ConverterRegistry::GENERIC, &genericCS32toCS16);
    */
    
    if (direction != SOAPY_SDR_RX) {
        throw std::runtime_error("setupStream only RX supported");
    }
    
    //check the channel configuration
    if (channels.size() > 1 or (channels.size() > 0 and channels.at(0) != 0))
    {
        throw std::runtime_error("setupStream invalid channel selection");
    }
    
    SoapySDR_logf(SOAPY_SDR_INFO, "Wants format %s", format.c_str());
    
    // Format converter function
    d_converter_func = SoapySDR::ConverterRegistry::getFunction("CS16", format);
    assert(d_converter_func != nullptr);
    
    d_pcm_handle = alsa_pcm_handle("hw:3,0", d_period_size, SND_PCM_STREAM_CAPTURE);
    assert(d_pcm_handle != nullptr);
    
    return (SoapySDR::Stream *) this;
}

void SoapyFCDPP::closeStream(SoapySDR::Stream *stream)
{
    SoapySDR_log(SOAPY_SDR_INFO, "close stream");
    if (d_pcm_handle != nullptr) {
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
again:
    // read numElems or d_period_size
    frames = snd_pcm_readi(d_pcm_handle, &d_buff[0], std::min(d_period_size, numElems));
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
    // results.push_back("AUDIO");
    return results;
}

bool SoapyFCDPP::hasGainMode(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_INFO, "hasGainMode");
    
    return false;
}

void SoapyFCDPP::setGainMode(const int direction, const size_t channel, const bool automatic)
{
    d_agc_mode = automatic;
    SoapySDR_logf(SOAPY_SDR_DEBUG, "Setting Audio AGC: %s", automatic ? "Automatic" : "Manual");
}

bool SoapyFCDPP::getGainMode(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_INFO, "getGainMode");
    
    return d_agc_mode;
}

void SoapyFCDPP::setGain(const int direction, const size_t channel, const double value)
{
    SoapySDR_log(SOAPY_SDR_INFO, "setGain");
    
    SoapySDR::Device::setGain(direction, channel, value);
}

void SoapyFCDPP::setGain(const int direction, const size_t channel, const std::string &name, const double value)
{
    SoapySDR_logf(SOAPY_SDR_DEBUG, "Setting gain: %f", value);
}

double SoapyFCDPP::getGain(const int direction, const size_t channel, const std::string &name) const
{
    SoapySDR_log(SOAPY_SDR_INFO, "getGain");
    return 0;
}

SoapySDR::Range SoapyFCDPP::getGainRange(const int direction, const size_t channel, const std::string &name) const
{
    SoapySDR_log(SOAPY_SDR_INFO, "getGainRange");
    return SoapySDR::Range(0, 100);
}

// Frequency
void SoapyFCDPP::setFrequency(const int direction,
                                const size_t channel,
                                const std::string &name,
                                const double frequency,
                                const SoapySDR::Kwargs &args)
{
    SoapySDR_log(SOAPY_SDR_INFO, "setFrequency");
    
    if (name == "RF")
    {
        d_frequency = frequency;
    }
}

double SoapyFCDPP::getFrequency(const int direction, const size_t channel, const std::string &name) const
{
    SoapySDR_logf(SOAPY_SDR_INFO, "getFrequency");
    if (name == "RF")
    {
        return d_frequency;
    } else {
        throw std::runtime_error("getFrequency for nonexisting tuner");
    }
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
        results.push_back(SoapySDR::Range(0, 45000000));
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
    SoapySDR_logf(SOAPY_SDR_INFO, "setSampleRate %f", rate);
    
    d_sample_rate = rate;
}

double SoapyFCDPP::getSampleRate(const int direction, const size_t channel) const
{
    SoapySDR_logf(SOAPY_SDR_INFO, "getSampleRate %f", d_sample_rate);
    
    return d_sample_rate;
}

std::vector<double> SoapyFCDPP::listSampleRates(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_INFO, "listSampleRates");
    
    std::vector<double> rates;
    rates.push_back(89286.);
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
    
    SoapySDR_log(SOAPY_SDR_INFO, "getSettingInfo");
    
    return settings;
}

void SoapyFCDPP::writeSetting(const std::string &key, const std::string &value)
{
    SoapySDR_log(SOAPY_SDR_INFO, "writeSetting");
}

std::string SoapyFCDPP::readSetting(const std::string &key) const
{
    SoapySDR_log(SOAPY_SDR_INFO, "readSetting");
    
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
    SoapySDR_log(SOAPY_SDR_INFO, "findFCDPP");
    
    SoapySDR::KwargsList results;
    SoapySDR::Kwargs soapyInfo;
    
    soapyInfo["device_id"] = std::to_string(0);
    soapyInfo["label"] = "fcdpp";
    soapyInfo["device"] = "fcdpp";
    // add some more
    
    results.push_back(soapyInfo);
    
    return results;
}

SoapySDR::Device *makeFCDPP(const SoapySDR::Kwargs &args)
{
    SoapySDR_log(SOAPY_SDR_INFO, "makeFCDPP");
    
    //create an instance of the device object given the args
    //here we will translate args into something used in the constructor
    return (SoapySDR::Device*) new SoapyFCDPP();
}
    
static SoapySDR::Registry registerFCDPP("fcdpp", &findFCDPP, &makeFCDPP, SOAPY_SDR_ABI_VERSION);
