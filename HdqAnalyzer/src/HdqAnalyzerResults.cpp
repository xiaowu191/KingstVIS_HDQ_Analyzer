#include "HdqAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "HdqAnalyzer.h"
#include "HdqAnalyzerSettings.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <stdio.h>

HdqAnalyzerResults::HdqAnalyzerResults(HdqAnalyzer* analyzer, HdqAnalyzerSettings* settings)
    : AnalyzerResults(),
      mSettings(settings),
      mAnalyzer(analyzer)
{
}

HdqAnalyzerResults::~HdqAnalyzerResults()
{
}

void HdqAnalyzerResults::GenerateBubbleText(U64 frame_index, Channel& /*channel*/, DisplayBase display_base)
{
    ClearResultStrings();
    Frame frame = GetFrame(frame_index);

    char result_str[256];
    
    switch (frame.mType) {
        case 0: // Break信号
            AddResultString("Break");
            AddResultString("Break Signal");
            break;
            
        case 1: // Recovery
            AddResultString("Recovery");
            AddResultString("Recovery Time");
            break;
            
        case 2: // Address
        {
            U8 address = frame.mData1; // 直接使用地址值
            
            AddResultString("Address");
            snprintf(result_str, sizeof(result_str), "0x%02X", address);
            AddResultString(result_str);
            snprintf(result_str, sizeof(result_str), "Address: 0x%02X", address);
            AddResultString(result_str);
            break;
        }
        
        case 3: // Read Data
        {
            U8 address = (frame.mData2 >> 1) & 0x7F;
            char data_str[32];
            AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, data_str, 32);
            
            AddResultString("Read");
            snprintf(result_str, sizeof(result_str), "Data: %s", data_str);
            AddResultString(result_str);
            snprintf(result_str, sizeof(result_str), "Read Data: %s", data_str);
            AddResultString(result_str);
            break;
        }
        
        case 4: // Write Data
        {
            U8 address = (frame.mData2 >> 1) & 0x7F;
            char data_str[32];
            AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, data_str, 32);
            
            AddResultString("Write");
            snprintf(result_str, sizeof(result_str), "Data: %s", data_str);
            AddResultString(result_str);
            snprintf(result_str, sizeof(result_str), "Write Data: %s", data_str);
            AddResultString(result_str);
            break;
        }
        
        case 5: // Read标识
            AddResultString("Read");
            AddResultString("Read Operation");
            break;
            
        case 6: // Write标识
            AddResultString("Write");
            AddResultString("Write Operation");
            break;
            
        default:
            AddResultString("Unknown");
            break;
    }
}

void HdqAnalyzerResults::GenerateExportFile(const char* file, DisplayBase display_base, U32 export_type_user_id)
{
    std::ofstream file_stream(file, std::ios::out);
    
    U64 trigger_sample = mAnalyzer->GetTriggerSample();
    U32 sample_rate = mAnalyzer->GetSampleRate();
    
    U64 num_frames = GetNumFrames();
    for (U32 i = 0; i < num_frames; i++) {
        Frame frame = GetFrame(i);
        
        char time_str[128];
        AnalyzerHelpers::GetTimeString(frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128);
        
        char number_str[128];
        AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, 128);
        
        file_stream << time_str << ",";
        
        if (frame.mType == 0) { // Command frame
            if (frame.mData1 & 0x80) {
                file_stream << "Write,0x" << std::hex << ((frame.mData1 >> 1) & 0x3F) << ",";
            } else {
                file_stream << "Read,0x" << std::hex << ((frame.mData1 >> 1) & 0x3F) << ",";
            }
        } else { // Data frame
            file_stream << "Data,0x" << std::hex << frame.mData1 << ",";
        }
        
        file_stream << std::endl;
        
        if (UpdateExportProgressAndCheckForCancel(i, num_frames) == true) {
            file_stream.close();
            return;
        }
    }
    
    file_stream.close();
}

void HdqAnalyzerResults::GenerateFrameTabularText(U64 frame_index, DisplayBase display_base)
{
    Frame frame = GetFrame(frame_index);
    ClearTabularText();

    switch (frame.mType) {
        case 0: // Break
            AddTabularText("Break");
            AddTabularText("-");
            AddTabularText("-");
            break;
            
        case 1: // Recovery
            AddTabularText("Recovery");
            AddTabularText("-");
            AddTabularText("-");
            break;
            
        case 2: // Address
        {
            // 修正：使用MSB判断读写标志
            bool is_read = (frame.mData1 & 0x80) == 0x00; // MSB=0为读，=1为写
            U8 address = frame.mData1 & 0x7F; // 取低7位作为地址
            char address_str[32];
            AnalyzerHelpers::GetNumberString(address, display_base, 7, address_str, 32);
            
            AddTabularText("Address");
            AddTabularText(address_str);
            AddTabularText(is_read ? "Read" : "Write");
            break;
        }
        
        case 3: // Read Data
        case 4: // Write Data
        {
            char data_str[32];
            AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, data_str, 32);
            
            AddTabularText(frame.mType == 3 ? "Read Data" : "Write Data");
            AddTabularText(data_str);
            AddTabularText("-");
            break;
        }
        
        default:
            AddTabularText("Unknown");
            AddTabularText("-");
            AddTabularText("-");
            break;
    }
}

void HdqAnalyzerResults::GeneratePacketTabularText(U64 /*packet_id*/, DisplayBase /*display_base*/)
{
    ClearTabularText();
    AddTabularText("not supported");
}

void HdqAnalyzerResults::GenerateTransactionTabularText(U64 /*transaction_id*/, DisplayBase /*display_base*/)
{
    ClearTabularText();
    AddTabularText("not supported");
}