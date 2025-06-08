#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "UE5_GraphCaptureJsonExporter.h"

class SJsonExportOptionsDialog : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SJsonExportOptionsDialog) {}
        SLATE_EVENT(FSimpleDelegate, OnOk)
        SLATE_EVENT(FSimpleDelegate, OnCancel)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    // Retrieve chosen options
    const FUE5_GraphCaptureJsonExporter::FJsonExportOptions& GetOptions() const
    {
        return Options;
    }

private:
    FReply OnOkClicked();
    FReply OnCancelClicked();

    FSimpleDelegate OkDelegate, CancelDelegate;
    FUE5_GraphCaptureJsonExporter::FJsonExportOptions Options;
};
