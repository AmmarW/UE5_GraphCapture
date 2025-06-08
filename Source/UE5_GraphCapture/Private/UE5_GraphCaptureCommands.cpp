// File: UE5_GraphCaptureCommands.cpp
#include "UE5_GraphCaptureCommands.h"
#include "Styling/SlateStyle.h"

#define LOCTEXT_NAMESPACE "FUE5_GraphCaptureModule"

void FUE5_GraphCaptureCommands::RegisterCommands()
{
    // existing screenshot commands
    UI_COMMAND(PluginAction, "UE5_GraphCapture", "Execute UE5_GraphCapture action", EUserInterfaceActionType::Button, FInputChord());
    UI_COMMAND(ActionForUE5_GraphCapture, "GraphShot", "Take a screenshot of selected nodes in the graph", EUserInterfaceActionType::Button, FInputChord());

    // new JSON export command (Ctrl+J)
    UI_COMMAND(ExportGraphJson,
        "Export Graph JSON",
        "Export the currently open Blueprint/Niagara graph structure to a JSON file",
        EUserInterfaceActionType::Button,
        FInputChord(EModifierKey::Control, EKeys::J));
}

#undef LOCTEXT_NAMESPACE
