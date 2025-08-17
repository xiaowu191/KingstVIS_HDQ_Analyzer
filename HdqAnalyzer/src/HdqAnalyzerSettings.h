#ifndef HDQ_ANALYZER_SETTINGS_H
#define HDQ_ANALYZER_SETTINGS_H

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>
#include <memory>

class HdqAnalyzerSettings : public AnalyzerSettings
{
public:
    HdqAnalyzerSettings();
    virtual ~HdqAnalyzerSettings();

    virtual bool SetSettingsFromInterfaces();
    virtual void UpdateInterfacesFromSettings();
    virtual void LoadSettings(const char* settings);
    virtual const char* SaveSettings();

    Channel mInputChannel;
    // U32 mBitRate;  // 删除这行
    bool mInverted;

protected:
    std::unique_ptr<AnalyzerSettingInterfaceChannel> mInputChannelInterface;
    // std::unique_ptr<AnalyzerSettingInterfaceInteger> mBitRateInterface;  // 删除这行
    std::unique_ptr<AnalyzerSettingInterfaceBool> mInvertedInterface;
};

#endif // HDQ_ANALYZER_SETTINGS_H