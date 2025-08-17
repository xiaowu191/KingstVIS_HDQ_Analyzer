#include "HdqAnalyzerSettings.h"
#include <AnalyzerHelpers.h>

HdqAnalyzerSettings::HdqAnalyzerSettings()
    : mInputChannel(UNDEFINED_CHANNEL),
      // mBitRate(9600),  // 删除这行
      mInverted(false)
{
    mInputChannelInterface.reset(new AnalyzerSettingInterfaceChannel());
    mInputChannelInterface->SetTitleAndTooltip("HDQ", "Standard HDQ");
    mInputChannelInterface->SetChannel(mInputChannel);

    // 删除以下4行
    // mBitRateInterface.reset(new AnalyzerSettingInterfaceInteger());
    // mBitRateInterface->SetTitleAndTooltip("Bit Rate (Bits/s)", "Specify the bit rate in bits per second.");
    // mBitRateInterface->SetMax(6000000);
    // mBitRateInterface->SetMin(1);
    // mBitRateInterface->SetInteger(mBitRate);

    mInvertedInterface.reset(new AnalyzerSettingInterfaceBool());
    mInvertedInterface->SetTitleAndTooltip("Inverted", "Specify if the HDQ signal is inverted");
    mInvertedInterface->SetValue(mInverted);

    AddInterface(mInputChannelInterface.get());
    // AddInterface(mBitRateInterface.get());  // 删除这行
    AddInterface(mInvertedInterface.get());

    AddExportOption(0, "Export as text/csv file");
    AddExportExtension(0, "text", "txt");
    AddExportExtension(0, "csv", "csv");

    ClearChannels();
    AddChannel(mInputChannel, "HDQ", false);
}

HdqAnalyzerSettings::~HdqAnalyzerSettings()
{
}

bool HdqAnalyzerSettings::SetSettingsFromInterfaces()
{
    mInputChannel = mInputChannelInterface->GetChannel();
    // mBitRate = mBitRateInterface->GetInteger();  // 删除这行
    mInverted = mInvertedInterface->GetValue();

    ClearChannels();
    AddChannel(mInputChannel, "HDQ", true);

    return true;
}

void HdqAnalyzerSettings::UpdateInterfacesFromSettings()
{
    mInputChannelInterface->SetChannel(mInputChannel);
    // mBitRateInterface->SetInteger(mBitRate);  // 删除这行
    mInvertedInterface->SetValue(mInverted);
}

void HdqAnalyzerSettings::LoadSettings(const char* settings)
{
    SimpleArchive text_archive;
    text_archive.SetString(settings);

    text_archive >> mInputChannel;
    // text_archive >> mBitRate;  // 删除这行
    text_archive >> mInverted;

    ClearChannels();
    AddChannel(mInputChannel, "HDQ", true);

    UpdateInterfacesFromSettings();
}

const char* HdqAnalyzerSettings::SaveSettings()
{
    SimpleArchive text_archive;

    text_archive << mInputChannel;
    // text_archive << mBitRate;  // 删除这行
    text_archive << mInverted;

    return SetReturnString(text_archive.GetString());
}