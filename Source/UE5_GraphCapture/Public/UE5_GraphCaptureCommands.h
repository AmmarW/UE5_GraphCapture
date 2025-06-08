// File: UE5_GraphCaptureCommands.h
#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "Styling/SlateStyle.h"
#include "UE5_GraphCaptureStyle.h"      // for FUE5_GraphCaptureStyle


class FUE5_GraphCaptureStyle;

class FUE5_GraphCaptureCommands : public TCommands<FUE5_GraphCaptureCommands>
{
public:
    FUE5_GraphCaptureCommands()
        : TCommands<FUE5_GraphCaptureCommands>(
            TEXT("UE5GraphCapture"),
            NSLOCTEXT("Contexts", "UE5GraphCapture", "Graph Capture Plugin"),
            NAME_None,
            FUE5_GraphCaptureStyle::GetStyleSetName())
    {}

    virtual void RegisterCommands() override;

public:
    TSharedPtr<FUICommandInfo> PluginAction;
    TSharedPtr<FUICommandInfo> ActionForUE5_GraphCapture;
    TSharedPtr<FUICommandInfo> ExportGraphJson;  // new command
};
