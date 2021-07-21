#include <SoapySDR/Device.hpp>
#include <SoapySDR/Modules.hpp>
#include <stdio.h>
#include <signal.h>

static volatile bool done=false;
void trap(int sig)
{
    done = true;
}

int main(int argc, char **argv)
{
    // find our device
    char *config = "driver=fcdpp";
    if (argc>1)
        config = argv[1];
    SoapySDR::loadModules();
    auto device = SoapySDR::Device::make(config);
    if (!device)
        return 1;
    // get a native stream
    double fullScale;
    std::string fmt = device->getNativeStreamFormat(SOAPY_SDR_RX, 0, fullScale);
    std::vector<size_t> chans;
    SoapySDR::Kwargs args;
    auto stream = device->setupStream(SOAPY_SDR_RX, fmt);
    const size_t period = device->getStreamMTU(stream);
    const size_t rate = device->getSampleRate(SOAPY_SDR_RX, 0);
    fprintf(stderr, "device period=%d rate=%d\n", (int)period, (int)rate);
    // check direct buffers are available
    if (device->getNumDirectAccessBuffers(stream)!=1)
        return 2;
    // go!
    if (device->activateStream(stream)!=0)
        return 3;
    fputs("Reading samples, Ctrl-C to stop..\n", stderr);
    signal(SIGINT, trap);
    int rv = 0;
    int tot = 0;
    int cnt = 0;
    int tmo = 0;
    char spin[4]={ '-', '\\', '|', '/' };
    while (!done) {
        // map the buffer (may wait/timeout)
        size_t handle;
        const void *buf;
        int flg;
        long long timeNs;
        long long timeout = (long long)period*1000000L*2L/(long long)rate;
        // timeout calcualted as twice expected MTU period in usecs
        int rv = device->acquireReadBuffer(stream, handle, &buf, flg, timeNs, (long)timeout);
        if (rv>0) {
            // process the buffer (we simply emit to stdout)
            fwrite(buf, 4, rv, stdout);
            // release the buffer
            device->releaseReadBuffer(stream, handle);
            tot += rv;
        } else if (SOAPY_SDR_TIMEOUT==rv) {
            // count, go round again =)
            ++tmo;
            continue;
        } else {
            break;
        }
        fprintf(stderr, "\r%c ", spin[cnt%4]);
        fflush(stderr);
        ++cnt;
    }
    fprintf(stderr, "processed %d frames, with %d timeouts\n", tot, tmo);
    device->deactivateStream(stream);
    device->closeStream(stream);
    SoapySDR::Device::unmake(device);
    return rv<0 ? 4 : 0;
}
