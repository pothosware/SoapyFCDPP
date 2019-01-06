project('SoapyFCDPP', 'cpp', 'c',
	default_options : ['cpp_std=c++11'],
	version : '0.0.1',
	license : 'MIT')

soapysdr_dep = dependency('SoapySDR')
alsa_dep = dependency('alsa')
hidapi_dep = dependency('hidapi-libusb')

# Optimizer arguments for Raspberry Pi 3
c_args = ['-ggdb', '-mcpu=cortex-a53', '-mfpu=neon-fp-armv8', '-mfloat-abi=hard', '-funsafe-math-optimizations',]

sources = ['SoapyFCDPP.cpp', 'alsa.c', 'fcd.c']
soapy_fcdpp_lib = shared_library('soapyfcdpp',
                        sources,
                        c_args: c_args,
                        cpp_args: c_args,
                        dependencies : [soapysdr_dep, alsa_dep, hidapi_dep],
                        install : true,
                        install_dir : '/usr/local/lib/SoapySDR/modules0.7')
