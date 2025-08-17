#include "HdqSimulationDataGenerator.h"
#include "HdqAnalyzerSettings.h"
#include <AnalyzerHelpers.h>

HdqSimulationDataGenerator::HdqSimulationDataGenerator()
{
}

HdqSimulationDataGenerator::~HdqSimulationDataGenerator()
{
}

void HdqSimulationDataGenerator::Initialize(U32 simulation_sample_rate, HdqAnalyzerSettings* settings)
{
    mSimulationSampleRateHz = simulation_sample_rate;
    mSettings = settings;
    
    // HDQ协议的位时间基于协议时序，而不是固定的比特率
    // 使用典型的位时间：逻辑0为120us，逻辑1为30us
    mBitTime = (mSimulationSampleRateHz * 120) / 1000000; // 120us for bit 0
    mBreakTime = (mSimulationSampleRateHz * 200) / 1000000; // 200us break
    
    // 使用正确的API初始化仿真通道
    mHdqSimulationData.SetChannel(settings->mInputChannel);
    mHdqSimulationData.SetSampleRate(simulation_sample_rate);
    mHdqSimulationData.SetInitialBitState(BIT_HIGH);
}

U32 HdqSimulationDataGenerator::GenerateSimulationData(U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel)
{
    U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample(largest_sample_requested, sample_rate, mSimulationSampleRateHz);

    while (mHdqSimulationData.GetCurrentSampleNumber() < adjusted_largest_sample_requested) {
        // Generate break signal
        CreateBreakSignal();
        
        // Generate command byte (write to address 0x10)
        CreateByte(0xA0); // Write command to address 0x10
        
        // Generate data byte
        CreateByte(0x55); // Test data
        
        // Add some idle time
        mHdqSimulationData.Advance(mBitTime * 10);
    }
    
    *simulation_channel = &mHdqSimulationData;
    return 1;
}

void HdqSimulationDataGenerator::CreateBreakSignal()
{
    // Break signal: pull low for 200us
    mHdqSimulationData.TransitionIfNeeded(BIT_LOW);
    mHdqSimulationData.Advance(mBreakTime);
    
    // Return to high
    mHdqSimulationData.TransitionIfNeeded(BIT_HIGH);
    mHdqSimulationData.Advance(mBitTime);
}

void HdqSimulationDataGenerator::CreateByte(U8 value)
{
    for (int i = 0; i < 8; i++) {
        CreateBit((value >> i) & 0x01); // LSB first
    }
}

void HdqSimulationDataGenerator::CreateBit(U32 bit_value)
{
    if (bit_value == 1) {
        // Bit 1: short low pulse
        mHdqSimulationData.TransitionIfNeeded(BIT_LOW);
        mHdqSimulationData.Advance(mBitTime / 3);
        mHdqSimulationData.TransitionIfNeeded(BIT_HIGH);
        mHdqSimulationData.Advance(mBitTime * 2 / 3);
    } else {
        // Bit 0: long low pulse
        mHdqSimulationData.TransitionIfNeeded(BIT_LOW);
        mHdqSimulationData.Advance(mBitTime * 2 / 3);
        mHdqSimulationData.TransitionIfNeeded(BIT_HIGH);
        mHdqSimulationData.Advance(mBitTime / 3);
    }
}