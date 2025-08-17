#ifndef HDQ_SIMULATION_DATA_GENERATOR_H
#define HDQ_SIMULATION_DATA_GENERATOR_H

#include <SimulationChannelDescriptor.h>
#include <string>
class HdqAnalyzerSettings;

class HdqSimulationDataGenerator
{
public:
    HdqSimulationDataGenerator();
    ~HdqSimulationDataGenerator();

    void Initialize(U32 simulation_sample_rate, HdqAnalyzerSettings* settings);
    U32 GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel);

protected:
    HdqAnalyzerSettings* mSettings;
    U32 mSimulationSampleRateHz;
    SimulationChannelDescriptor mHdqSimulationData;
    
    void CreateBreakSignal();
    void CreateByte(U8 value);
    void CreateBit(U32 bit_value);
    
    U32 mBitTime;
    U32 mBreakTime;
};

#endif // HDQ_SIMULATION_DATA_GENERATOR_H