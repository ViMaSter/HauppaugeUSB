## Override path set by Hauppauge's make files, to use our 'wrappers'

HOST_OS := $(shell uname -s)
ifeq ($(HOST_OS),Darwin)
PREFIX ?= $(HOME)/.local
BOOST_PREFIX ?= $(shell brew --prefix boost 2>/dev/null)
BOOST_CPPFLAGS := -I$(BOOST_PREFIX)/include
BOOST_LDFLAGS := -L$(BOOST_PREFIX)/lib
override OS := linux
override ARCH := -pthread
else
PREFIX ?= /opt/Hauppauge
endif

override TOP := ./Hauppauge

override VPATH = ${TOP} $(TOP)/Common/Rx $(TOP)/Common/Rx/ADV7842	\
$(TOP)/Common/Rx/ADV7842/RX/LIB $(TOP)/Common/Rx/ADV7842/RX/HAL	\
$(TOP)/Common/Rx/ADV7842/RX/HAL/4G				\
$(TOP)/Common/Rx/ADV7842/RX/HAL/4G/ADV7842/HAL $(TOP)/Common	\
./Wrappers/$(OS) $(TOP)/Common/FX2API $(TOP)/Common/EncoderDev	\
$(TOP)/Common/EncoderDev/HAPIHost				\
$(TOP)/Common/EncoderDev/HAPIHost/MChip

override INC = $(OS_INC) -I.. -I$(TOP)/Common -I./Wrappers/$(OS)	\
-I$(TOP)/Common/FX2API -I$(TOP)/Common/Rx/ADV7842			\
-I$(TOP)/Common/Rx/ADV7842/RX -I$(TOP)/Common/Rx/ADV7842/RX/LIB	\
-I$(TOP)/Common/Rx/ADV7842/RX/HAL					\
-I$(TOP)/Common/Rx/ADV7842/RX/HAL/4G					\
-I$(TOP)/Common/Rx/ADV7842/RX/HAL/4G/ADV7842/HAL			\
-I$(TOP)/Common/Rx/ADV7842/RX/HAL/4G/ADV7842/MACROS			\
-I$(TOP)/Common/Rx -I$(TOP)/Common/EncoderDev				\
-I$(TOP)/Common/EncoderDev/HAPIHost					\
-I$(TOP)/Common/EncoderDev/HAPIHost/MChip				\
`pkg-config --cflags libusb-1.0`

override OBJS_WRAPPERS = log.o baseif.o registryif.o USBif.o I2Cif.o
override OS_INC := `pkg-config --cflags libusb-1.0`

# override CXXFLAGS := -g -c -Wall -std=c++11 ${CFLAGS}

include ./Hauppauge/TestApp/build-ADV7842/Makefile

ifeq ($(HOST_OS),Darwin)
CFLAGS += -D_DARWIN_C_SOURCE
endif
CFLAGS += -DHAUPPAUGE_FIRMWARE_DIR='"$(PREFIX)/firmware"'

REC_CXX = g++
REC_CXXFLAGS := -g -c -Wall -std=c++11 -fdiagnostics-color \
	-DBOOST_LOG_DYN_LINK ${CFLAGS} $(BOOST_CPPFLAGS)

REC_LDFLAGS =

#	`pkg-config --libs libsystemd` \

REC_LDFLAGS += `pkg-config --libs libusb-1.0` \
	-lpthread $(BOOST_LDFLAGS)

REC_SOURCES = Logger.cpp Common.cpp MythTV.cpp TSRandomAccessFixer.cpp \
	FlipInterlacedFields.cpp HauppaugeDev.cpp ReceiverTDA19977.cpp hauppauge2.cpp

REC_HEADERS = Logger.h Common.h MythTV.h TSRandomAccessFixer.h \
	FlipInterlacedFields.h HauppaugeDev.h ReceiverTDA19977.h ReceiverTDA19977Data.h

REC_OBJECTS = $(REC_SOURCES:.cpp=.o)

CONF = etc/sample.conf

FIRMWARE = Hauppauge/Common/EncoderDev/HAPIHost/bin/llama_usb_vx_host_slave_t22_24.bin \
	Hauppauge/Common/EncoderDev/HAPIHost/bin/mips_vx_host_slave.bin

#
# Generated .cpp files created by the Hauppauge build system.
# These are safe to remove during "make clean".
#
TRANSIENT = FX2Firmware.cpp mchip_binary.cpp

REC_EXE = hauppauge2

REC_LIBS = libADV7842.a
REC_LIBS += -lboost_program_options -lboost_log -lboost_log_setup \
	-lboost_thread -lboost_filesystem


all: ${REC_EXE}


${REC_EXE}: ${REC_OBJECTS} libADV7842.a
	${REC_CXX} ${REC_OBJECTS} -o $@ ${REC_LIBS} ${REC_LDFLAGS}


${REC_OBJECTS}: ${REC_SOURCES}

HauppaugeDev.o: HauppaugeDev.h DeviceProfile.h ReceiverTDA19977.h
ReceiverTDA19977.o: ReceiverTDA19977.h ReceiverTDA19977Data.h
USBif.o: Wrappers/linux/USBif.h DeviceProfile.h


.cpp.o:
	${REC_CXX} ${REC_CXXFLAGS} $< -o $@


#.c.o:
#	${REC_CXX} ${REC_CXXFLAGS} $< -o $@


#
# Override the unsafe clean target inherited from:
#
#   Hauppauge/TestApp/build-ADV7842/Makefile
#
# The upstream target removes:
#
#   *.o *.a *.cpp
#
# which also deletes the HauppaugeUSB application source files.
#
# Only remove generated build products here.
#
clean:
	$(RM) *.o *.a ${REC_EXE} ${TRANSIENT} tests/receiver-test


install: ${REC_EXE}
	ln -snf ${TOP}/Common/*.cfg .
	ln -snf $(TOP)/Common/EncoderDev/HAPIHost/bin/*.bin .
	mkdir -p $(PREFIX)/bin $(PREFIX)/firmware $(PREFIX)/etc
	install -m 755 ${REC_EXE} $(PREFIX)/bin/
	install -m 644 ${FIRMWARE} $(PREFIX)/firmware/
	install -m 644 ${CONF} $(PREFIX)/etc/

.PHONY: all clean install

check: tests/receiver-test
	./tests/receiver-test

tests/receiver-test: tests/ReceiverTDA19977Test.cpp ReceiverTDA19977.cpp ReceiverTDA19977.h ReceiverTDA19977Data.h DeviceProfile.h Logger.cpp
	$(REC_CXX) -std=c++11 -Wall -DBOOST_LOG_DYN_LINK $(if $(filter Darwin,$(HOST_OS)),-D_DARWIN_C_SOURCE) -I. $(BOOST_CPPFLAGS) $(INC) tests/ReceiverTDA19977Test.cpp ReceiverTDA19977.cpp Logger.cpp -o $@ $(filter -l%,$(REC_LIBS)) $(REC_LDFLAGS)

.PHONY: check
