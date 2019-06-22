#include <string>
#include <regex>
#include <iomanip>

#include "./Config.hpp"
#include "./Util.hpp"
#include "../misc/StringAlgo.hpp"
#include "../misc/MathUtil.hpp"
#include "../device/AudioDeviceManager.hpp"

NS_HWM_BEGIN

namespace {
    std::string const kConfigFileFormatID_v1 = "config_file_format_v1";
}

Config::FailedToParse::FailedToParse(std::string const &error_msg)
:   std::ios_base::failure("Failed to parse: " + error_msg)
{}

void Config::ScanAudioDeviceStatus()
{
    auto adm = AudioDeviceManager::GetInstance();
    auto dev = adm->GetDevice();
    
    if(!dev) { return; }
    
    auto in_info = dev->GetDeviceInfo(DeviceIOType::kInput);
    auto out_info = dev->GetDeviceInfo(DeviceIOType::kOutput);
    
    if(in_info) {
        audio_input_driver_type_ = in_info->driver_;
        audio_input_device_name_ = in_info->name_;
        audio_input_channel_count_ = in_info->num_channels_;
    } else {
        audio_input_driver_type_ = AudioDriverType::kUnknown;
        audio_input_device_name_ = L"";
        audio_input_channel_count_ = 0;
    }
    
    if(out_info) {
        audio_output_driver_type_ = out_info->driver_;
        audio_output_device_name_ = out_info->name_;
        audio_output_channel_count_ = out_info->num_channels_;
    } else {
        audio_output_driver_type_ = AudioDriverType::kUnknown;
        audio_output_device_name_ = L"";
        audio_output_channel_count_ = 0;
    }
    
    sample_rate_ = dev->GetSampleRate();
    block_size_ = dev->GetBlockSize();
}

std::ostream & operator<<(std::ostream &os, Config const &self)
{
#define WRITE_MEMBER(name) \
    << (#name + std::string(" = ")) << std::quoted(to_s(self. name ## _)) << "\n"
    
    os
    << "format = " << kConfigFileFormatID_v1 << "\n"
    << "# This is a config file of Vst3SampleHost." << "\n"
    << "# The line starting '#' is treated as a comment line." << "\n"
    WRITE_MEMBER(audio_input_driver_type)
    WRITE_MEMBER(audio_input_device_name)
    WRITE_MEMBER(audio_input_channel_count)
    WRITE_MEMBER(audio_output_driver_type)
    WRITE_MEMBER(audio_output_device_name)
    WRITE_MEMBER(audio_output_channel_count)
    WRITE_MEMBER(sample_rate)
    WRITE_MEMBER(block_size)
    ;

#undef WRITE_MEMBER
    
    return os;
}

std::istream & operator>>(std::istream &is, Config &self)
{
    is.exceptions(std::ios::badbit);
    
    auto const lines = get_lines(is);
    
    if(auto val = find_value(lines, "format")) {
        if(*val != kConfigFileFormatID_v1) {
            throw Config::FailedToParse("Unknown format.");
        }
    }
    
#define READ_MEMBER(key, func) \
    if(auto val = find_value(lines, #key)) { self. key ## _ = func(*val); }
    
    READ_MEMBER(audio_input_driver_type, [](std::string const &str) {
        return to_audio_driver_type(str).value_or(AudioDriverType::kUnknown);
    });
    
    READ_MEMBER(audio_input_device_name, [](std::string const &str) {
        return to_wstr(str);
    });
    
    READ_MEMBER(audio_input_channel_count, [](std::string const &str) {
        return std::max<Int32>(stoi_or(str, 0), 0);
    });
    
    READ_MEMBER(audio_output_driver_type, [](std::string const &str) {
        return to_audio_driver_type(str).value_or(AudioDriverType::kUnknown);
    });
    
    READ_MEMBER(audio_output_device_name, [](std::string const &str) {
        return to_wstr(str);
    });
    
    READ_MEMBER(audio_output_channel_count, [](std::string const &str) {
        return std::max<Int32>(stoi_or(str, 0), 0);
    });
    
    READ_MEMBER(sample_rate, [](std::string const &str) {
        return Clamp<double>(stof_or(str, kSupportedSampleRateDefault),
                             kSupportedSampleRateMin,
                             kSupportedSampleRateMax);
    });
    
    READ_MEMBER(block_size, [](std::string const &str) {
        return Clamp<double>(stoi_or(str, kSupportedBlockSizeDefault),
                             kSupportedBlockSizeMin,
                             kSupportedBlockSizeMax);
    });

#undef READ_MEMBER
    
    return is;
}

NS_HWM_END