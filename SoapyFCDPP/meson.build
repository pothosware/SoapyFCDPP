project('SoapyFCDPP', 'cpp', 'c',
	default_options : ['cpp_std=c++11'],
	version : '0.0.1',
	license : 'MIT')

# /usr/local/lib/SoapySDR/modules0.7

soapysdr_dep = dependency('SoapySDR')
hidapi_dep = dependency('hidapi-libusb')
alsa_dep = dependency('alsa')

sources = ['SoapyFCDPP.cpp', 'alsa.c', 'fcd.c']
soapy_fcdpp_lib = shared_library('soapyfcdpp',
                        sources,
                        dependencies : [soapysdr_dep, alsa_dep, hidapi_dep],
                        install : true)
