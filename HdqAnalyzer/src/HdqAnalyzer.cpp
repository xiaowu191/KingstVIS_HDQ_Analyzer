#include "HdqAnalyzer.h"
#include "HdqAnalyzerSettings.h"
#include <AnalyzerChannelData.h>

// HDQ协议时序参数（根据规格书）
#define HDQ_BREAK_MIN_WIDTH_US 190      // Break信号最小宽度
#define HDQ_BREAK_RECOVERY_US 40        // Break恢复时间
#define HDQ_BIT1_MAX_WIDTH_US 50        // 逻辑1最大脉冲宽度
#define HDQ_BIT0_MIN_WIDTH_US 86        // 逻辑0最小脉冲宽度
#define HDQ_BIT0_MAX_WIDTH_US 145       // 逻辑0最大脉冲宽度
#define HDQ_RESPONSE_MIN_US 190         // 响应时间最小值
#define HDQ_RESPONSE_MAX_US 950         // 响应时间最大值

HdqAnalyzer::HdqAnalyzer()
    : Analyzer2(),
      mSettings(new HdqAnalyzerSettings()),
      mSimulationInitilized(false),
      mNeedsRerun(false)
{
    SetAnalyzerSettings(mSettings.get());
}

HdqAnalyzer::~HdqAnalyzer()
{
    KillThread();
}

void HdqAnalyzer::SetupResults()
{
    mResults.reset(new HdqAnalyzerResults(this, mSettings.get()));
    SetAnalyzerResults(mResults.get());
    mResults->AddChannelBubblesWillAppearOn(mSettings->mInputChannel);
}

void HdqAnalyzer::WorkerThread()
{
    Setup();

    for (;;) {
        CheckIfThreadShouldExit();

        // 1. 检测Break信号
        U64 break_start, break_end;
        if (!DetectBreakSignal(&break_start, &break_end)) {
            continue;
        }
        
        // 添加Break帧
        Frame break_frame;
        break_frame.mType = 0; // Break信号
        break_frame.mData1 = 0;
        break_frame.mData2 = 0;
        break_frame.mFlags = 0;
        break_frame.mStartingSampleInclusive = break_start;
        break_frame.mEndingSampleInclusive = break_end;
        mResults->AddFrame(break_frame);
        
        // 2. Break Recovery时间 - 等待下一个下降沿
        U64 recovery_start = break_end;
        
        // 确保当前处于高电平状态
        if (mHdq->GetBitState() == BIT_LOW) {
            mHdq->AdvanceToNextEdge(); // 等待回到高电平
        }
        
        // 等待下一个下降沿（命令开始）
        bool found_next_edge = false;
        while (mHdq->DoMoreTransitionsExistInCurrentData()) {
            mHdq->AdvanceToNextEdge();
            if (mHdq->GetBitState() == BIT_LOW) {
                found_next_edge = true;
                break;
            }
        }
        
        U64 recovery_end = mHdq->GetSampleNumber();
        
        // 检查Recovery时间是否满足最小要求（40us）
        U64 recovery_duration = recovery_end - recovery_start;
        // if (recovery_duration < mBreakRecoveryWidth) {
        if (recovery_duration < mBreakRecoveryWidth - 4) {// 4us误差
            // Recovery时间不足，这可能不是有效的HDQ序列
            continue;
        }
        
        // 添加Break Recovery帧
        Frame recovery_frame;
        recovery_frame.mType = 1; // Recovery
        recovery_frame.mData1 = 0;
        recovery_frame.mData2 = 0;
        recovery_frame.mFlags = 0;
        recovery_frame.mStartingSampleInclusive = recovery_start;
        recovery_frame.mEndingSampleInclusive = recovery_end;
        mResults->AddFrame(recovery_frame);
        
        // 如果没有找到下降沿，跳过这个序列
        if (!found_next_edge) {
            continue;
        }
        
        // 3. 读取命令字节（7位地址 + 1位读写标志）
        U8 command = ReadByte();
        
        // 提取7位地址和1位读写标志
        U8 address = command & 0x7F;  // 低7位为地址
        bool is_read = (command & 0x80) == 0x00;  // 最高位为读写标志
        
        // 检查是否有足够的位时间记录
        if (mBitTimings.size() >= 8) {
            // 添加Address帧（覆盖前7位的实际时间范围）
            Frame addr_frame;
            addr_frame.mType = 2; // Address
            addr_frame.mData1 = address;
            addr_frame.mData2 = 0;
            addr_frame.mFlags = 0;
            addr_frame.mStartingSampleInclusive = mBitTimings[0].start; // 第1位开始
            addr_frame.mEndingSampleInclusive = mBitTimings[6].end;     // 第7位结束
            mResults->AddFrame(addr_frame);
            
            // 添加Read/Write标识帧（覆盖第8位的实际时间范围）
            Frame rw_frame;
            rw_frame.mType = is_read ? 5 : 6; // 5=Read标识, 6=Write标识
            rw_frame.mData1 = is_read ? 0 : 1;
            rw_frame.mData2 = 0;
            rw_frame.mFlags = 0;
            rw_frame.mStartingSampleInclusive = mBitTimings[7].start; // 第8位开始
            rw_frame.mEndingSampleInclusive = mBitTimings[7].end;     // 第8位结束
            mResults->AddFrame(rw_frame);
        }
        
        if (is_read) {
            // 读命令：等待从设备响应
            // U64 wait_start = mHdq->GetSampleNumber();
            U64 wait_start = mBitTimings[7].start; // 使用第8位的开始时间作为响应时间起点
            bool found_response = false;
            
            // 在响应时间窗口内寻找数据传输开始（下降沿）
            U64 max_wait_samples = wait_start + mResponseMaxWidth;
            
            // 首先确保当前处于高电平状态
            if (mHdq->GetBitState() == BIT_LOW) {
                mHdq->AdvanceToNextEdge(); // 等待回到高电平
            }
            
            // 在响应时间窗口内等待下降沿（数据传输开始）
            while (mHdq->GetSampleNumber() < max_wait_samples) {
                if (mHdq->DoMoreTransitionsExistInCurrentData()) {
                    mHdq->AdvanceToNextEdge();
                    if (mHdq->GetBitState() == BIT_LOW) {
                        U64 response_time = mHdq->GetSampleNumber() - wait_start;
                        // 检查是否在响应时间范围内（最小和最大）
                        if (response_time >= mResponseMinWidth && response_time <= mResponseMaxWidth) {  
                            found_response = true;
                            break;
                        }
                        // 如果响应时间过短，继续等待
                        // 如果响应时间过长，外层while循环会自动退出
                    }
                } else {
                    break; // 没有更多数据
                }
            }
            
            if (found_response) {
                U8 data = ReadByte();

                // 从mBitTimings获取数据的开始和结束时间
                U64 data_start = mBitTimings[0].start;  // 第一个位的开始时间
                U64 data_end = mBitTimings[7].end;      // 最后一个位的结束时间
                
                // 添加Read Data帧
                Frame data_frame;
                data_frame.mType = 3; // Read Data
                data_frame.mData1 = data;
                data_frame.mData2 = command;
                data_frame.mFlags = 0;
                data_frame.mStartingSampleInclusive = data_start;
                data_frame.mEndingSampleInclusive = data_end;
                mResults->AddFrame(data_frame);
            }
        } else {
            // 写命令：主设备发送数据
            U8 data = ReadByte();

            // 从mBitTimings获取数据的开始和结束时间
            U64 data_start = mBitTimings[0].start;  // 第一个位的开始时间
            U64 data_end = mBitTimings[7].end;      // 最后一个位的结束时间

            // 添加Write Data帧
            Frame data_frame;
            data_frame.mType = 4; // Write Data
            data_frame.mData1 = data;
            data_frame.mData2 = command;
            data_frame.mFlags = 0;
            data_frame.mStartingSampleInclusive = data_start;
            data_frame.mEndingSampleInclusive = data_end;
            mResults->AddFrame(data_frame);
        }
        
        mResults->CommitResults();
        ReportProgress(mHdq->GetSampleNumber());
    }
}

void HdqAnalyzer::Setup()
{
    mHdq = GetAnalyzerChannelData(mSettings->mInputChannel);
    
    // 获取采样率
    mSampleRateHz = GetSampleRate();
    mSampleRate = mSampleRateHz;  // 添加这行
    
    // 计算时序参数（采样点数）
    mBreakMinWidth = (mSampleRateHz * HDQ_BREAK_MIN_WIDTH_US) / 1000000;
    mBreakRecoveryWidth = (mSampleRateHz * HDQ_BREAK_RECOVERY_US) / 1000000;
    mBit1MaxWidth = (mSampleRateHz * HDQ_BIT1_MAX_WIDTH_US) / 1000000;
    mBit0MinWidth = (mSampleRateHz * HDQ_BIT0_MIN_WIDTH_US) / 1000000;
    mBit0MaxWidth = (mSampleRateHz * HDQ_BIT0_MAX_WIDTH_US) / 1000000;
    mResponseMinWidth = (mSampleRateHz * HDQ_RESPONSE_MIN_US) / 1000000;
    mResponseMaxWidth = (mSampleRateHz * HDQ_RESPONSE_MAX_US) / 1000000;
    
    // 开始时应该在空闲状态（高电平）
    if (mHdq->GetBitState() == BIT_LOW) {
        mHdq->AdvanceToNextEdge();
    }
}

bool HdqAnalyzer::DetectBreakSignal(U64* break_start_out, U64* break_end_out)
{
    // 等待下降沿（Break开始）
    if (mHdq->GetBitState() == BIT_HIGH) {
        mHdq->AdvanceToNextEdge(); // 到达下降沿
    }
    
    if (mHdq->GetBitState() != BIT_LOW) {
        return false;
    }
    
    // 在这里记录Break开始时间（下降沿时刻）
    U64 break_start = mHdq->GetSampleNumber();
    
    // 等待上升沿（Break结束）
    mHdq->AdvanceToNextEdge();
    
    if (mHdq->GetBitState() != BIT_HIGH) {
        return false;
    }
    
    U64 break_end = mHdq->GetSampleNumber();
    U64 break_width = break_end - break_start;
    
    // 检查是否为有效的Break信号
    if (break_width >= mBreakMinWidth) {
        // 输出时间参数
        if (break_start_out) *break_start_out = break_start;
        if (break_end_out) *break_end_out = break_end;
        return true;
    }
    
    return false;
}

U8 HdqAnalyzer::ReadBit(U64* bit_start_out, U64* bit_end_out)
{
    // 等待下降沿（位开始）
    if (mHdq->GetBitState() == BIT_HIGH) {
        mHdq->AdvanceToNextEdge();
    }
    
    if (mHdq->GetBitState() != BIT_LOW) {
        return 0; // 错误
    }
    
    // 记录位开始时间（下降沿后）
    U64 bit_start = mHdq->GetSampleNumber();
    
    // 计算最小cycle time对应的采样点
    U64 min_cycle_samples = (U64)(mSampleRate * 190.0 / 1000000.0); // 190us最小cycle time
    U64 expected_bit_end = bit_start + min_cycle_samples;
    
    // 等待上升沿（位结束）
    mHdq->AdvanceToNextEdge();
    
    if (mHdq->GetBitState() != BIT_HIGH) {
        return 0; // 错误
    }
    
    U64 actual_bit_end = mHdq->GetSampleNumber();
    U64 pulse_width = actual_bit_end - bit_start;
    
    // 如果实际的bit结束时间早于最小cycle time，则使用最小cycle time
    U64 bit_end = (actual_bit_end < expected_bit_end) ? expected_bit_end : actual_bit_end;
    
    // 如果需要，移动到计算出的bit结束位置
    if (bit_end > actual_bit_end) {
        mHdq->AdvanceToAbsPosition(bit_end);
    }
    
    // 输出时间参数
    if (bit_start_out) *bit_start_out = bit_start;
    if (bit_end_out) *bit_end_out = bit_end;
    
    // 根据脉冲宽度判断位值
    if (pulse_width <= mBit1MaxWidth) {
        return 1; // 短脉冲 = 逻辑1
    } else if (pulse_width >= mBit0MinWidth && pulse_width <= mBit0MaxWidth) {
        return 0; // 长脉冲 = 逻辑0
    } else {
        return 0; // 无效脉冲 //20250817-ReadByte()里面未处理无效脉冲的情况
    }
}

U8 HdqAnalyzer::ReadByte()
{
    U8 result = 0;
    mBitTimings.clear(); // 清空位时间记录
    
    // HDQ协议：LSB first（最低位先传）
    for (U32 i = 0; i < 8; i++) {
        CheckIfThreadShouldExit();
        
        U64 bit_start, bit_end;
        U8 bit = ReadBit(&bit_start, &bit_end);
        if (bit == 1) {
            result |= (1 << i); // LSB first
        }
        
        // 确保每个位至少有190us的cycle time
        U64 min_cycle_samples = (U64)(mSampleRate * 190.0 / 1000000.0);
        if (bit_end - bit_start < min_cycle_samples) {
            bit_end = bit_start + min_cycle_samples;
        }
        
        mBitTimings.push_back({bit_start, bit_end});
    }
    
    return result;
}

void HdqAnalyzer::CheckIfThreadShouldExit()
{
    Analyzer::CheckIfThreadShouldExit();
}

bool HdqAnalyzer::NeedsRerun()
{
    return false;
}

U32 HdqAnalyzer::GenerateSimulationData(U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels)
{
    if (mSimulationInitilized == false) {
        mSimulationDataGenerator.Initialize(GetSimulationSampleRate(), mSettings.get());
        mSimulationInitilized = true;
    }

    return mSimulationDataGenerator.GenerateSimulationData(minimum_sample_index, device_sample_rate, simulation_channels);
}

U32 HdqAnalyzer::GetMinimumSampleRateHz()
{
    // HDQ协议需要足够的采样率来准确检测时序
    // 最小采样率设为1MHz，确保能够准确检测微秒级的时序
    return 1000000; // 1MHz minimum sample rate
}
//{
//    return mSettings->mBitRate * 10; // 10x oversampling
//}

const char* HdqAnalyzer::GetAnalyzerName() const
{
    return "HDQ";
}

const char* GetAnalyzerName()
{
    return "HDQ";
}

Analyzer* CreateAnalyzer()
{
    return new HdqAnalyzer();
}

void DestroyAnalyzer(Analyzer* analyzer)
{
    delete analyzer;
}