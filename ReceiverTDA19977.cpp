#include "ReceiverTDA19977.h"
#include "ReceiverTDA19977Data.h"

#include <chrono>
#include <thread>

using namespace TDA19977Data;

ReceiverTDA19977::ReceiverTDA19977(FX2Device_t &device, unsigned clockDelay, bool clockToggle)
    : m_device(device), m_page(-1), m_transmitterPage(-1), m_clockDelay(clockDelay), m_clockToggle(clockToggle),
            m_initialized(false), m_format(nullptr)
{
}

bool ReceiverTDA19977::selectPage(uint8_t page)
{
    if (m_page == page)
        return true;
    const uint8_t command[] = {0xff, page};
    if (!m_device.I2CWrite(0x98, command, sizeof(command)))
    {
        m_page = -1;
        return false;
    }
    m_page = page;
    return true;
}

bool ReceiverTDA19977::read(uint16_t reg, uint8_t &value)
{
    const uint8_t address = reg & 0xff;
    return selectPage(reg >> 8) &&
        m_device.I2CWriteRead(0x98, &address, 1, &value, 1);
}

bool ReceiverTDA19977::write(uint16_t reg, uint8_t value)
{
    const uint8_t command[] = {static_cast<uint8_t>(reg), value};
    return selectPage(reg >> 8) &&
        m_device.I2CWrite(0x98, command, sizeof(command));
}

bool ReceiverTDA19977::init()
{
    if (m_clockDelay > 7)
        return false;
    uint8_t revision = 0;
    if (!read(0x0000, revision))
        return false;
    INFOLOG << "TDA19977 chip revision: 0x" << std::hex
            << static_cast<unsigned>(revision) << std::dec;
    if (revision != 0x41)
        return false;
    if (!m_device.setPortStateBits(FX2_PORT_C, 0, 0x10))
        return false;
    std::this_thread::sleep_for(std::chrono::milliseconds(120));
    const uint8_t transmitterWake[] = {0xff, 0x40};
    const uint8_t transmitterReg = 0x40;
    uint8_t transmitterStatus = 0;
    const uint8_t transmitterEnable[] = {0xff, 0x06};
    if (!m_device.I2CWrite(0x68, transmitterWake, sizeof(transmitterWake)) ||
        !m_device.I2CWriteRead(0x68, &transmitterReg, 1, &transmitterStatus, 1) ||
        !m_device.I2CWrite(0x68, transmitterEnable, sizeof(transmitterEnable)))
        return false;
    if (!update(0x11f9, 0x01, 0) || !write(0x1264, 0) || !write(0x1202, 1) ||
        !update(0x13e0, 0x08, 0) || !update(0x13f8, 0x02, 0) ||
        !write(0x1347, 0) || !update(0x1363, 0x80, 0) ||
        !update(0x00e5, 0x80, 0) || !write(0x000d, 0x0f) ||
        !update(0x000f, 0, 0x0c))
        return false;
    for (unsigned index = 0; index < sizeof(EDID); ++index)
        if (!write(0x2000 + (index < 128 ? index : 0x100 + index - 128), EDID[index]))
            return false;
    const Register auxiliary[] = {
        {0x2180, 8}, {0x2181, 0x10}, {0x2182, 0}, {0x2183, 0x20},
        {0x2184, 0}, {0x2185, 0x30}, {0x2186, 0}, {0x2187, 0x40},
        {0x2188, 0}, {0x2189, 0x7c}, {0x218a, 0x6c}, {0x218b, 0x5c},
        {0x218c, 0x4c}, {0x11f9, 8},
    };
    if (!sequence(auxiliary, sizeof(auxiliary) / sizeof(auxiliary[0])) ||
        !sequence(TMDS, sizeof(TMDS) / sizeof(TMDS[0])))
        return false;
    for (const auto &format : Formats)
        if (format.id == 9 && !configureOutput(format))
            return false;
    m_initialized = true;
    return true;
}

bool ReceiverTDA19977::update(uint16_t reg, uint8_t clear, uint8_t set)
{
    uint8_t value = 0;
    return read(reg, value) && write(reg, (value & ~clear) | set);
}

bool ReceiverTDA19977::sequence(const Register *registers, unsigned count)
{
    for (unsigned index = 0; index < count; ++index)
        if (!write(registers[index].address, registers[index].value))
            return false;
    return true;
}

bool ReceiverTDA19977::configureAudio()
{
    uint8_t oldConfig = 0;
    if (!write(0x1101, 0x80) || !write(0x1102, 0x80) || !write(0x1103, 0) ||
        !read(0x1104, oldConfig) || !write(0x1104, oldConfig & 0x1f))
        return false;
    if ((oldConfig & 0x18) && !update(0x1365, 0x18, 0))
        return false;
    if (!update(0x1104, 0x17, 0x10) || !update(0x1104, 0x08, 0) ||
        !write(0x1363, 0x19) || !write(0x11f7, 0) || !write(0x11f8, 0x21))
        return false;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return write(0x11f8, 0) && write(0x13e0, 0);
}

bool ReceiverTDA19977::setInput(receiverInput_t input)
{
    if (!m_initialized || input != RXI_HDMI)
        return false;
    if (!m_device.setPortStateBits(FX2_PORT_E, 0, 0x40))
        return false;
    for (const auto &operation : {std::pair<uint16_t, uint8_t>(0x0114, 0xbf),
                                  std::pair<uint16_t, uint8_t>(0x0115, 0xf0)})
    {
        const uint8_t reg[] = {static_cast<uint8_t>(operation.first >> 8),
                               static_cast<uint8_t>(operation.first)};
        uint8_t value = 0;
        if (!m_device.I2CWriteRead(0x88, reg, sizeof(reg), &value, 1))
            return false;
        const uint8_t command[] = {reg[0], reg[1], static_cast<uint8_t>(value & operation.second)};
        if (!m_device.I2CWrite(0x88, command, sizeof(command)))
            return false;
    }
    return m_device.setPortStateBits(FX2_PORT_C, 0x10, 0) && configureAudio();
}

bool ReceiverTDA19977::configureOutput(const Format &format)
{
    const Register *registers = nullptr;
    unsigned count = 0;
#define TDA_MODE(ID, ARRAY) case ID: registers = ARRAY; count = sizeof(ARRAY) / sizeof(ARRAY[0]); break
    switch (format.id)
    {
        TDA_MODE(5, P1080_24);
        TDA_MODE(9, P720_60);
        TDA_MODE(10, I1080);
        TDA_MODE(15, I1080);
        TDA_MODE(11, I480);
        TDA_MODE(12, P1080_60);
        TDA_MODE(14, P720_50);
        TDA_MODE(16, I576);
        TDA_MODE(17, P1080_50);
        TDA_MODE(18, VGA);
        default: return false;
    }
#undef TDA_MODE
    if (!sequence(registers, count) || !configureAudio())
        return false;
    uint8_t colorimetry = 0;
    if (!read(0x1185, colorimetry) || !write(0x0080, (colorimetry >> 6) ? 1 : 0))
        return false;
    if (!configureTransmitter(format))
        return false;
    if (!update(0x0017, 0x80, 0x80) ||
        !write(0x0016, static_cast<uint8_t>(m_clockDelay)) ||
        !update(0x00e9, 0x80, m_clockToggle ? 0x80 : 0))
        return false;
    INFOLOG << "TDA19977 HDMI: " << format.width << "x" << format.height
            << (format.interlaced ? "i" : "p") << " " << format.rate / 100.0;
    INFOLOG << "TDA19977 pixel-clock delay: " << m_clockDelay;
    INFOLOG << "TDA19977 pixel-clock toggle: " << m_clockToggle;
    return true;
}

bool ReceiverTDA19977::transmitterSequence(const TransmitterOperation *operations, unsigned count)
{
    for (unsigned index = 0; index < count; ++index)
    {
        const auto &operation = operations[index];
        if (operation.delayMs)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(operation.delayMs));
            continue;
        }
        if (operation.slave == 0xe0)
        {
            const uint8_t page = operation.address >> 8;
            if (m_transmitterPage != page)
            {
                const uint8_t command[] = {0xff, page};
                if (!m_device.I2CWrite(0xe0, command, sizeof(command)))
                {
                    m_transmitterPage = -1;
                    return false;
                }
                m_transmitterPage = page;
            }
        }
        const uint8_t command[] = {static_cast<uint8_t>(operation.address), operation.value};
        if (!m_device.I2CWrite(operation.slave, command, sizeof(command)))
            return false;
    }
    return true;
}

bool ReceiverTDA19977::configureTransmitter(const Format &format)
{
    const TransmitterOperation *operations = nullptr;
    unsigned count = 0;
#define TX_MODE(ID, ARRAY) case ID: operations = ARRAY; count = sizeof(ARRAY) / sizeof(ARRAY[0]); break
    switch (format.id)
    {
        TX_MODE(9, TX_P720_60);
        TX_MODE(10, TX_I1080_60);
        TX_MODE(11, TX_I480);
        TX_MODE(12, TX_P1080_60);
        TX_MODE(14, TX_P720_50);
        TX_MODE(15, TX_I1080_50);
        TX_MODE(16, TX_I576);
        TX_MODE(17, TX_P1080_50);
        TX_MODE(18, TX_VGA);
        default: return false;
    }
#undef TX_MODE
    return transmitterSequence(operations, count);
}

bool ReceiverTDA19977::hasInputSignal()
{
    uint8_t lock = 0;
    if (!m_initialized || !read(0x13cb, lock) || (lock & 6) != 6)
    {
        m_format = nullptr;
        return false;
    }
    unsigned verticalPeriod = 0, horizontalPeriod = 0;
    for (unsigned offset = 0; offset < 3; ++offset)
    {
        uint8_t value = 0;
        if (!read(0x0019 + offset, value)) return false;
        verticalPeriod = (verticalPeriod << 8) | (offset == 0 ? value & 0x7f : value);
    }
    for (unsigned offset = 0; offset < 2; ++offset)
    {
        uint8_t value = 0;
        if (!read(0x001c + offset, value)) return false;
        horizontalPeriod = (horizontalPeriod << 8) | (offset == 0 ? value & 0x7f : value);
    }
    for (const auto &format : Formats)
    {
        if (verticalPeriod < format.vMin || verticalPeriod > format.vMax ||
            horizontalPeriod < format.hMin || horizontalPeriod > format.hMax)
            continue;
        if (m_format != &format && !configureOutput(format))
            return false;
        m_format = &format;
        return true;
    }
    m_format = nullptr;
    return false;
}

bool ReceiverTDA19977::setOutputBusMode(receiverOutputBusMode_t mode)
{
    if (!m_format || mode != RXOBM_422_10x2)
        return false;
    return true;
}

bool ReceiverTDA19977::setEDID(const void *, unsigned, unsigned) { return false; }

bool ReceiverTDA19977::getOutputParams(receiverOutputParams_t *params)
{
    if (!m_format || !params) return false;
    *params = {m_format->width, m_format->height, m_format->rate / 100.0f,
               m_format->interlaced, m_format->width == 640 ? 4.0f / 3 : 16.0f / 9};
    return true;
}

bool ReceiverTDA19977::getHDMIParams(receiverHDMIParams_t *params)
{
    if (!m_format || !params) return false;
    *params = {};
    params->width = m_format->width;
    params->height = m_format->height;
    params->vFreq = m_format->rate / 100.0f;
    params->interlaced = m_format->interlaced;
    params->pixelRepetitionFactor = 1;
    params->aspectRatio = m_format->width == 640 ? 4.0f / 3 : 16.0f / 9;
    return true;
}

bool ReceiverTDA19977::getAudioParams(receiverAudioParams_t *params)
{
    if (params) *params = {};
    return false;
}

int ReceiverTDA19977::getHDMIFormat()
{
    if (!m_format) return 0;
    switch (m_format->id)
    {
        case 5: return 32;
        case 9: return 4;
        case 10: return 5;
        case 11: return 6;
        case 12: return 16;
        case 14: return 19;
        case 15: return 20;
        case 16: return 21;
        case 17: return 31;
        case 18: return 1;
        default: return 0;
    }
}

bool ReceiverTDA19977::showInfo()
{
    uint8_t lock = 0;
    if (!m_initialized || !read(0x13cb, lock)) return false;
    INFOLOG << "TDA19977 HDMI lock status: 0x" << std::hex
            << static_cast<unsigned>(lock) << std::dec;
    for (uint16_t reg : {0x0001, 0x0016, 0x0017, 0x0080, 0x00d4, 0x00d5, 0x00d6,
                        0x00e5, 0x00e6, 0x00e9, 0x1185, 0x11f9, 0x13f0,
                        0x138a, 0x138b, 0x138c})
    {
        uint8_t value = 0;
        if (!read(reg, value)) return false;
        INFOLOG << "TDA19977 register 0x" << std::hex << reg
                << " = 0x" << static_cast<unsigned>(value) << std::dec;
    }
    INFOLOG << "Siena1 hotplug port C state=0x" << std::hex
            << m_device.getPortState(FX2_PORT_C) << " direction=0x"
            << m_device.getPortDir(FX2_PORT_C) << std::dec;
    return true;
}