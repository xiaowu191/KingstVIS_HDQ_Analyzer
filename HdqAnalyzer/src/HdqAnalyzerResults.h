#ifndef HDQ_ANALYZER_RESULTS_H
#define HDQ_ANALYZER_RESULTS_H

#include <AnalyzerResults.h>

#define HDQ_ERROR_FLAG (1 << 0)
#define HDQ_READ_FLAG (1 << 1)
#define HDQ_WRITE_FLAG (1 << 2)

class HdqAnalyzer;
class HdqAnalyzerSettings;

class HdqAnalyzerResults : public AnalyzerResults
{
public:
    HdqAnalyzerResults(HdqAnalyzer* analyzer, HdqAnalyzerSettings* settings);
    virtual ~HdqAnalyzerResults();

    virtual void GenerateBubbleText(U64 frame_index, Channel& channel, DisplayBase display_base);
    virtual void GenerateExportFile(const char* file, DisplayBase display_base, U32 export_type_user_id);

    virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base);
    virtual void GeneratePacketTabularText(U64 packet_id, DisplayBase display_base);
    virtual void GenerateTransactionTabularText(U64 transaction_id, DisplayBase display_base);

protected:
    HdqAnalyzerSettings* mSettings;
    HdqAnalyzer* mAnalyzer;
};

#endif // HDQ_ANALYZER_RESULTS_H