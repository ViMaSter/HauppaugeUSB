#include "ReceiverTDA19977.h"
#include "ReceiverTDA19977Data.h"

#include <cassert>
#include <iostream>
#include <map>

namespace {
std::map<uint16_t, uint8_t> registers;
std::map<uint16_t, uint8_t> transmitterRegisters;
std::map<uint8_t, uint8_t> analogRegisters;
std::map<uint8_t, uint8_t> ports;
uint8_t page = 0;
uint8_t transmitterPage = 0;
bool failWrite = false;
unsigned pageWrites = 0;

void reset()
{
    registers.clear();
    transmitterRegisters.clear();
    analogRegisters.clear();
    ports.clear();
    registers[0] = 0x41;
    page = 0;
    transmitterPage = 0;
    failWrite = false;
    pageWrites = 0;
}

void timing(unsigned vertical, unsigned horizontal)
{
    registers[0x13cb] = 6;
    registers[0x0019] = vertical >> 16;
    registers[0x001a] = vertical >> 8;
    registers[0x001b] = vertical;
    registers[0x001c] = horizontal >> 8;
    registers[0x001d] = horizontal;
}
}

USBWrapper_t::USBWrapper_t() {}
USBWrapper_t::~USBWrapper_t() {}

bool FX2Device_t::I2CWrite(uint8_t address, const uint8_t *data, size_t length)
{
    if (failWrite) return false;
    if (address == 0x68) return length == 2;
    if (address == 0x88) return length == 3;
    if (address == 0x9c)
    {
        assert(length == 2);
        analogRegisters[data[0]] = data[1];
        return true;
    }
    if (address == 0xe0)
    {
        assert(length == 2);
        if (data[0] == 0xff) transmitterPage = data[1];
        else transmitterRegisters[(transmitterPage << 8) | data[0]] = data[1];
        return true;
    }
    assert(address == 0x98 && length == 2);
    if (data[0] == 0xff)
    {
        page = data[1];
        ++pageWrites;
    }
    else registers[(page << 8) | data[0]] = data[1];
    return true;
}

bool FX2Device_t::I2CWriteRead(uint8_t address, const uint8_t *data, size_t length,
                             uint8_t *result, size_t resultLength)
{
    assert(resultLength == 1);
    if (address == 0x9c)
    {
        assert(length == 1);
        *result = analogRegisters[data[0]];
        return true;
    }
    if (address == 0x68)
    {
        assert(length == 1);
        *result = 0;
        return true;
    }
    if (address == 0x88)
    {
        assert(length == 2);
        *result = 0xff;
        return true;
    }
    assert(address == 0x98 && length == 1);
    *result = registers[(page << 8) | data[0]];
    return true;
}

int FX2Device_t::getPortState(uint8_t port) { return ports[port]; }
int FX2Device_t::getPortDir(uint8_t) { return 0xff; }
bool FX2Device_t::setPortState(uint8_t port, uint8_t value)
{
    ports[port] = value;
    return true;
}

int main()
{
    assert(!FindDeviceProfile(0xffff, 0xe502));
    assert(!FindDeviceProfile(0x2040, 0xe5ff));
    assert(FindDeviceProfile(0x2040, 0xe502)->receiver == ReceiverFamily::TDA19977);
    for (uint16_t product : {0xe504, 0xe505, 0xe524, 0xe525, 0xe554})
        assert(FindDeviceProfile(0x2040, product)->receiver == ReceiverFamily::ADV7842);
    for (uint16_t product : {0xe502, 0xe524, 0xe525})
        assert(!FindDeviceProfile(0x2040, product)->opticalAudio);
    for (uint16_t product : {0xe504, 0xe505, 0xe554})
        assert(FindDeviceProfile(0x2040, product)->opticalAudio);
    USBWrapper_t usb;
    FX2Device_t device(usb);
    reset();
    ports[FX2_PORT_C] = 0x10;
    ReceiverTDA19977 receiver(device);
    assert(receiver.init());
    assert(!(ports[FX2_PORT_C] & 0x10));
    assert(pageWrites > 0);
    for (unsigned index = 0; index < 256; ++index)
        assert(registers[0x2000 + (index < 128 ? index : 0x100 + index - 128)] ==
               TDA19977Data::EDID[index]);
    assert(!receiver.setInput(RXI_COMP));
    assert(receiver.setInput(RXI_HDMI));
    assert(analogRegisters[0xe5] == 0x80 && analogRegisters[0xf4] == 0x1f);
    assert(ports[FX2_PORT_C] & 0x10);
    assert(!receiver.hasInputSignal());
    timing(450450, 400);
    registers[0x000f] = 0x0c;
    assert(receiver.hasInputSignal());
    assert(registers[0x000d] == 0x0b && registers[0x000f] == 0x0c);
    receiverHDMIParams_t params{};
    assert(receiver.getHDMIParams(&params));
    assert(params.width == 1920 && params.height == 1080 && !params.interlaced);
    assert(receiver.getHDMIFormat() == 16);
    assert(transmitterRegisters[0x1084] == 1);
    assert(transmitterRegisters[0x1100] == 0);
    assert(receiver.setOutputBusMode(RXOBM_422_10x2));
    assert(registers[0x0017] == 0xaf);
    assert(registers[0x00e5] == 0x79);
    assert(registers[0x00e9] == 0xa0);
    assert(!receiver.setOutputBusMode(RXOBM_656_10_DC));
    timing(540000, 480);
    assert(receiver.hasInputSignal() && receiver.getHDMIFormat() == 31);
    timing(450450, 600);
    assert(receiver.hasInputSignal() && receiver.getHDMIFormat() == 4);
    timing(1, 1);
    assert(!receiver.hasInputSignal());
    assert(receiver.getHDMIFormat() == 0);
    registers[0x13cb] = 0;
    assert(!receiver.hasInputSignal());
    reset();
    registers[0] = 0x42;
    ReceiverTDA19977 wrongChip(device);
    assert(!wrongChip.init());
    reset();
    ReceiverTDA19977 invalidDelay(device, 8);
    assert(!invalidDelay.init() && pageWrites == 0);
    ReceiverTDA19977 adjustedDelay(device, 2);
    assert(adjustedDelay.init());
    assert(registers[0x0016] == 2 && (registers[0x0017] & 0x80));
    ReceiverTDA19977 zeroDelay(device, 0);
    assert(zeroDelay.init());
    assert(registers[0x0016] == 0 && (registers[0x0017] & 0x80));
    ReceiverTDA19977 alternateClock(device, 1, false);
    assert(alternateClock.init());
    assert(registers[0x0016] == 1 && registers[0x00e9] == 0x20);
    ReceiverTDA19977 outputDelay(device, 1, true, 4);
    assert(outputDelay.init());
    assert(registers[0x00eb] == 0x14);
    ReceiverTDA19977 invalidOutputDelay(device, 1, true, 16);
    assert(!invalidOutputDelay.init());
    ReceiverTDA19977 automaticDelay(device, 7, true, 8, true);
    assert(automaticDelay.init());
    assert(registers[0x0016] == 7);
    timing(450450, 400);
    assert(automaticDelay.hasInputSignal());
    assert(registers[0x0016] == 1);
    reset();
    failWrite = true;
    ReceiverTDA19977 failedTransport(device);
    assert(!failedTransport.init());
    std::cout << "TDA19977 receiver tests passed\n";
}