#ifndef HDQ_ANALYZER_H
#define HDQ_ANALYZER_H

#include <Analyzer.h>
#include "HdqAnalyzerResults.h"
#include "HdqSimulationDataGenerator.h"
#include <memory>
#include <vector>

class HdqAnalyzerSettings;

// 位时序结构体
struct BitTiming {
    U64 start;
    U64 end;
};

class ANALYZER_EXPORT HdqAnalyzer : public Analyzer2
{
public:
    HdqAnalyzer();
    virtual ~HdqAnalyzer();

    virtual void SetupResults();
    virtual void WorkerThread();

    virtual U32 GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels);
    virtual U32 GetMinimumSampleRateHz();

    virtual const char* GetAnalyzerName() const;
    virtual bool NeedsRerun();

protected: // functions
    void Setup();
    bool DetectBreakSignal(U64* break_start_out = nullptr, U64* break_end_out = nullptr);
    U8 ReadCommandByte();
    U8 ReadDataByte();
    U8 ReadBit(U64* bit_start_out = nullptr, U64* bit_end_out = nullptr);
    U8 ReadByte();
    void AdvanceToActiveEdge();
    bool WaitForEdge(U32 timeout_samples);
    U64 GetBitWidth();
    void CheckIfThreadShouldExit();

protected: // vars
    std::unique_ptr<HdqAnalyzerSettings> mSettings;
    std::unique_ptr<HdqAnalyzerResults> mResults;
    AnalyzerChannelData* mHdq;

    HdqSimulationDataGenerator mSimulationDataGenerator;
    bool mSimulationInitilized;

    // HDQ protocol specific variables
    U64 mSampleRateHz;
    U64 mSampleRate;  // 添加这个变量
    U64 mBreakWidth;
    U64 mBitWidth;
    bool mNeedsRerun;
    
    // 新增的HDQ时序参数
    U64 mBreakMinWidth;
    U64 mBreakRecoveryWidth;
    U64 mBit1MaxWidth;
    U64 mBit0MinWidth;
    U64 mBit0MaxWidth;
    U64 mResponseMinWidth;
    U64 mResponseMaxWidth;
    
    // 位时序记录
    std::vector<BitTiming> mBitTimings;
};

extern "C" ANALYZER_EXPORT const char* GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* CreateAnalyzer();
extern "C" ANALYZER_EXPORT void DestroyAnalyzer(Analyzer* analyzer);

#endif // HDQ_ANALYZER_H