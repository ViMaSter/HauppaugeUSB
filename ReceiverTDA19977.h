#ifndef RECEIVER_TDA19977_H
#define RECEIVER_TDA19977_H

#include "FX2Device.h"
#include "receiver.h"

namespace TDA19977Data { struct Format; struct Register; struct TransmitterOperation; }

class ReceiverTDA19977 : public receiver_t
{
  public:
    explicit ReceiverTDA19977(FX2Device_t &device, unsigned clockDelay = 7, bool clockToggle = true);
    bool init() override;
    bool setInput(receiverInput_t input) override;
    bool setOutputBusMode(receiverOutputBusMode_t mode) override;
    bool setEDID(const void *data, unsigned length, unsigned spaOffset) override;
    bool hasInputSignal() override;
    bool getOutputParams(receiverOutputParams_t *params) override;
    bool getAudioParams(receiverAudioParams_t *params) override;
    bool getHDMIParams(receiverHDMIParams_t *params) override;
    int getHDMIFormat() override;
    bool showInfo() override;

  private:
    bool selectPage(uint8_t page);
    bool read(uint16_t reg, uint8_t &value);
    bool write(uint16_t reg, uint8_t value);
    bool update(uint16_t reg, uint8_t clear, uint8_t set);
    bool sequence(const TDA19977Data::Register *registers, unsigned count);
    bool configureOutput(const TDA19977Data::Format &format);
    bool configureTransmitter(const TDA19977Data::Format &format);
    bool transmitterSequence(const TDA19977Data::TransmitterOperation *operations, unsigned count);
    bool configureAudio();

    FX2Device_t &m_device;
    int m_page;
    int m_transmitterPage;
    unsigned m_clockDelay;
    bool m_clockToggle;
    bool m_initialized;
    const TDA19977Data::Format *m_format;
};

#endif