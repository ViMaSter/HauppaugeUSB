#ifndef DEVICE_PROFILE_H
#define DEVICE_PROFILE_H

#include <cstdint>

enum class ReceiverFamily { TDA19977, ADV7842, Unimplemented };

struct DeviceProfile
{
    uint16_t product;
    const char *name;
    ReceiverFamily receiver;
    bool opticalAudio;
};

inline const DeviceProfile *FindDeviceProfile(uint16_t vendor, uint16_t product)
{
    if (vendor != 0x2040) return nullptr;
    static const DeviceProfile profiles[] = {
        {0xe500, "HD PVR 2 prototype (Siena1-01/03)", ReceiverFamily::Unimplemented, true},
        {0xe501, "HD PVR 2 prototype (Siena1-05)", ReceiverFamily::Unimplemented, false},
        {0xe502, "HD PVR 2 Gaming Edition model 1480 (Siena1 / 145210)", ReceiverFamily::TDA19977, false},
        {0xe504, "HD PVR 2 Gaming Edition Plus w/SPDIF (Siena2-02)", ReceiverFamily::ADV7842, true},
        {0xe505, "HD PVR 2 Gaming Edition Plus w/SPDIF w/MIC (Siena2-05a)", ReceiverFamily::ADV7842, true},
        {0xe50a, "StreamEez (Siena1 / 145410)", ReceiverFamily::Unimplemented, false},
        {0xe514, "HD PVR 2 model 1512 w/SPDIF (Siena2-02)", ReceiverFamily::ADV7842, true},
        {0xe515, "HD PVR 2 model 1512 w/SPDIF w/MIC (Siena2-05a)", ReceiverFamily::ADV7842, true},
        {0xe524, "HD PVR 2 Gaming Edition (Siena2-02)", ReceiverFamily::ADV7842, false},
        {0xe525, "HD PVR 2 Gaming Edition w/MIC (Siena2-05a)", ReceiverFamily::ADV7842, false},
        {0xe52a, "HD PVR 2 Metal (Siena1 / 145420)", ReceiverFamily::Unimplemented, false},
        {0xe52c, "HD PVR 2 StreamEez (Siena2)", ReceiverFamily::ADV7842, false},
        {0xe554, "HD PVR 2 Gaming Edition Plus w/SPDIF, Blue LEDs (Siena2-02)", ReceiverFamily::ADV7842, true},
        {0xe585, "Colossus 2", ReceiverFamily::ADV7842, true},
    };
    for (const auto &profile : profiles)
        if (profile.product == product) return &profile;
    return nullptr;
}

#endif