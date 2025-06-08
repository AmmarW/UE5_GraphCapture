// File: UE5_GraphCaptureJsonExporter.h
#pragma once

#include "CoreMinimal.h"
#include "GraphEditor.h"
#include "Dom/JsonObject.h"
#include "EdGraph/EdGraph.h"           // for UEdGraph
#include "EdGraph/EdGraphNode.h"       // for UEdGraphNode
#include "EdGraph/EdGraphPin.h"        // for UEdGraphPin
#include "NiagaraNodeCustomHlsl.h"  // for UNiagaraNodeCustomHlsl :contentReference[oaicite:0]{index=0}
#include "EdGraphNode_Comment.h"

class FUE5_GraphCaptureJsonExporter
{
public:
    // Options to toggle inclusion of each data block
    struct FJsonExportOptions
    {
        bool bIncludeMeta        = true;  // graphType, schemaName
        bool bIncludePosition    = true;  // posX, posY
        bool bIncludePins        = true;  // inputs / outputs arrays
        bool bIncludeHlsl        = true;  // customHlsl bodies
        bool bIncludeRegions     = true;  // comment‐box regions
    };

    static void ExportGraph(
        const TSharedRef<SGraphEditor>& GraphEditor,
        const FJsonExportOptions& Options
    );
};
