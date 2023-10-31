/*
 * EMS-ESP - https://github.com/emsesp/EMS-ESP
 * Copyright 2020-2023  Paul Derbyshire
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "extension.h"

namespace emsesp {

REGISTER_FACTORY(Extension, EMSdevice::DeviceType::EXTENSION);

Extension::Extension(uint8_t device_type, uint8_t device_id, uint8_t product_id, const char * version, const char * name, uint8_t flags, uint8_t brand)
    : EMSdevice(device_type, device_id, product_id, version, name, flags, brand) {
    register_telegram_type(0x935, "EM100SetMessage", true, MAKE_PF_CB(process_EM100SetMessage));
    register_telegram_type(0x937, "EM100TempMessage", false, MAKE_PF_CB(process_EM100TempMessage));
    register_telegram_type(0x938, "EM100InputMessage", false, MAKE_PF_CB(process_EM100InputMessage));
    register_telegram_type(0x939, "EM100MonitorMessage", false, MAKE_PF_CB(process_EM100MonitorMessage));
    register_telegram_type(0x93A, "EM100ConfigMessage", false, MAKE_PF_CB(process_EM100ConfigMessage));

    register_device_value(DeviceValueTAG::TAG_DEVICE_DATA,
                          &headerTemp_,
                          DeviceValueType::SHORT,
                          DeviceValueNumOp::DV_NUMOP_DIV10,
                          FL_(flowTempVf),
                          DeviceValueUOM::DEGREES);
    register_device_value(DeviceValueTAG::TAG_DEVICE_DATA, &input_, DeviceValueType::USHORT, DeviceValueNumOp::DV_NUMOP_DIV10, FL_(input), DeviceValueUOM::VOLTS);
    register_device_value(DeviceValueTAG::TAG_DEVICE_DATA, &outPower_, DeviceValueType::UINT, FL_(outPower), DeviceValueUOM::PERCENT);
    register_device_value(DeviceValueTAG::TAG_DEVICE_DATA, &setPower_, DeviceValueType::UINT, FL_(setPower), DeviceValueUOM::PERCENT);
    register_device_value(DeviceValueTAG::TAG_DEVICE_DATA, &setPoint_, DeviceValueType::UINT, FL_(setPoint), DeviceValueUOM::DEGREES);
    register_device_value(DeviceValueTAG::TAG_DEVICE_DATA,
                          &minV_,
                          DeviceValueType::UINT,
                          DeviceValueNumOp::DV_NUMOP_DIV10,
                          FL_(minV),
                          DeviceValueUOM::VOLTS,
                          MAKE_CF_CB(set_minV));
    register_device_value(DeviceValueTAG::TAG_DEVICE_DATA,
                          &maxV_,
                          DeviceValueType::UINT,
                          DeviceValueNumOp::DV_NUMOP_DIV10,
                          FL_(maxV),
                          DeviceValueUOM::VOLTS,
                          MAKE_CF_CB(set_maxV));
    register_device_value(DeviceValueTAG::TAG_DEVICE_DATA, &minT_, DeviceValueType::UINT, FL_(minT), DeviceValueUOM::DEGREES, MAKE_CF_CB(set_minT));
    register_device_value(DeviceValueTAG::TAG_DEVICE_DATA, &maxT_, DeviceValueType::UINT, FL_(maxT), DeviceValueUOM::DEGREES, MAKE_CF_CB(set_maxT));
    register_device_value(DeviceValueTAG::TAG_DEVICE_DATA, &dip_, DeviceValueType::UINT, FL_(mode), DeviceValueUOM::NONE);
    // register_device_value(DeviceValueTAG::TAG_DEVICE_DATA, &errorState_, DeviceValueType::BOOL, FL_(error), DeviceValueUOM::NONE);
}


// 0x935 needs fetch
void Extension::process_EM100SetMessage(std::shared_ptr<const Telegram> telegram) {
    has_update(telegram, minV_, 1); // Input for off, is / 10
    has_update(telegram, maxV_, 2); // Input for 100%, is / 10
    has_update(telegram, minT_, 3); // min temp
    has_update(telegram, maxT_, 4); // max temp
}

// alert(0x15) -B-> All(0x00), ?(0x093A), data: 00 00 00 00 00 00 00 00 00 03 01
void Extension::process_EM100ConfigMessage(std::shared_ptr<const Telegram> telegram) {
    has_update(telegram, dip_, 9);
}

// alert(0x15) -B-> All(0x00), ?(0x0938), data: 01 62
void Extension::process_EM100InputMessage(std::shared_ptr<const Telegram> telegram) {
    has_update(telegram, outPower_, 0); // IO1
    has_update(telegram, input_, 1);
}

// alert(0x15) -B-> All(0x00), ?(0x0939), data: 64 4E 00 00
void Extension::process_EM100MonitorMessage(std::shared_ptr<const Telegram> telegram) {
    has_update(telegram, setPower_, 0); // percent
    has_update(telegram, setPoint_, 1); // °C
    // has_update(telegram, errorState_, 2); // OE1
    // has_update(telegram, errorPump_, 3);  // IE0
}

// alert(0x15) -B-> All(0x00), ?(0x0937), data: 80 00
void Extension::process_EM100TempMessage(std::shared_ptr<const Telegram> telegram) {
    has_update(telegram, headerTemp_, 0);
}

bool Extension::set_minV(const char * value, const int8_t id) {
    float v;
    if (!Helpers::value2float(value, v)) {
        return false;
    }
    write_command(0x935, 1, (uint8_t)(v * 10));
    return true;
}

bool Extension::set_maxV(const char * value, const int8_t id) {
    float v;
    if (!Helpers::value2float(value, v)) {
        return false;
    }
    write_command(0x935, 2, (uint8_t)(v * 10));
    return true;
}

bool Extension::set_minT(const char * value, const int8_t id) {
    int v;
    if (!Helpers::value2temperature(value, v)) {
        return false;
    }
    write_command(0x935, 3, v);
    return true;
}

bool Extension::set_maxT(const char * value, const int8_t id) {
    int v;
    if (!Helpers::value2temperature(value, v)) {
        return false;
    }
    write_command(0x935, 4, v);
    return true;
}

} // namespace emsesp