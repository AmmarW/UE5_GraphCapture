// File: UE5_GraphCaptureJsonExporter.cpp
#include "UE5_GraphCaptureJsonExporter.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"    // FJsonSerializer
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "UObject/UnrealType.h"    // for FStrProperty

// Prototypes for local helpers
static TArray<TSharedPtr<FJsonValue>> MakeJsonStringArray(const TArray<FString>& In);
static TArray<TSharedPtr<FJsonValue>> GatherRegions(UEdGraph* Graph);

void FUE5_GraphCaptureJsonExporter::ExportGraph(
    const TSharedRef<SGraphEditor>& GraphEditor,
    const FJsonExportOptions& Opts)
{
    UEdGraph* Graph = GraphEditor->GetCurrentGraph();
    if (!Graph)
    {
        // no graph open
        return;
    }

    // Build JSON DOM
    TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> NodesArray, LinksArray;

    // -- meta --
    if (Opts.bIncludeMeta)
    {
        Root->SetStringField("graphType",  Graph->GetClass()->GetName());
        Root->SetStringField("schemaName", Graph->GetSchema()->GetClass()->GetName());
    }

    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (!Node) continue;

        // Node entry
        TSharedRef<FJsonObject> NodeObj = MakeShared<FJsonObject>();
        NodeObj->SetStringField("id", Node->GetName());
        NodeObj->SetStringField("type", Node->GetClass()->GetName());
        if (Opts.bIncludePosition)
        {
            NodeObj->SetNumberField("posX", Node->NodePosX);
            NodeObj->SetNumberField("posY", Node->NodePosY);
        }

        // Gather pin names by direction:
        if (Opts.bIncludePins)
        {
            // inputs
            TArray<FString> Ins, Outs;
            for (UEdGraphPin* Pin : Node->Pins)
            {
                if (Pin->Direction == EGPD_Input)  Ins.Add(Pin->PinName.ToString());
                else if (Pin->Direction == EGPD_Output) Outs.Add(Pin->PinName.ToString());
            }
            // write them
            NodeObj->SetArrayField("inputs",  MakeJsonStringArray(Ins));
            NodeObj->SetArrayField("outputs", MakeJsonStringArray(Outs));
        }


        // If this is a UNiagaraNodeCustomHlsl, pull its 'CustomHlsl' FString property via reflection
        if (Node->GetClass()->IsChildOf(UNiagaraNodeCustomHlsl::StaticClass()))
        {
            // Look up the UPROPERTY() named "CustomHlsl"
            if (FProperty* Prop = Node->GetClass()->FindPropertyByName(TEXT("CustomHlsl")))
            {
                if (FStrProperty* StrProp = CastField<FStrProperty>(Prop))
                {
                    // Read the FString value
                    const FString HlslCode = StrProp->GetPropertyValue_InContainer(Node);
                    NodeObj->SetStringField("customHlsl", HlslCode);
                }
            }
        }


        NodesArray.Add(MakeShared<FJsonValueObject>(NodeObj));

        // Pin links
        for (UEdGraphPin* Pin : Node->Pins)
        {
            for (UEdGraphPin* Linked : Pin->LinkedTo)
            {
                if (!Linked) continue;
                TSharedRef<FJsonObject> LinkObj = MakeShared<FJsonObject>();
                LinkObj->SetStringField("fromNode", Node->GetName());
                LinkObj->SetStringField("fromPin", Pin->PinName.ToString());
                LinkObj->SetStringField("toNode", Linked->GetOwningNode()->GetName());
                LinkObj->SetStringField("toPin", Linked->PinName.ToString());
                LinksArray.Add(MakeShared<FJsonValueObject>(LinkObj));
            }
        }

        // TArray<TSharedPtr<FJsonValue>> RegionsArray;
        // for (UEdGraphNode* RawNode : Graph->Nodes)
        // {
        //     if (UEdGraphNode_Comment* Comment = Cast<UEdGraphNode_Comment>(RawNode))
        //     {
        //         // Comment box rect:
        //         float X = Comment->NodePosX;
        //         float Y = Comment->NodePosY;
        //         float W = Comment->NodeWidth;
        //         float H = Comment->NodeHeight;

        //         TArray<TSharedPtr<FJsonValue>> Ids;
        //         // find all non-comment nodes inside the box
        //         for (UEdGraphNode* Other : Graph->Nodes)
        //         {
        //             if (Cast<UEdGraphNode_Comment>(Other)) continue;
        //             if (Other->NodePosX >= X && Other->NodePosX <= X+W
        //             && Other->NodePosY >= Y && Other->NodePosY <= Y+H)
        //             {
        //                 Ids.Add(MakeShared<FJsonValueString>(Other->GetName()));
        //             }
        //         }

        //         TSharedRef<FJsonObject> RegObj = MakeShared<FJsonObject>();
        //         RegObj->SetStringField("name", Comment->NodeComment);
        //         RegObj->SetArrayField("nodeIds", MoveTemp(Ids));
        //         RegionsArray.Add(MakeShared<FJsonValueObject>(RegObj));
        //         Root->SetArrayField("regions", RegionsArray);
        //     }
        // }
        
        
    }
    // Add metadata
    Root->SetStringField("graphType", Graph->GetClass()->GetName());               // e.g. "NiagaraGraph" or "EdGraph"
    Root->SetStringField("schemaName", Graph->GetSchema()->GetClass()->GetName()); // e.g. "EdGraphSchema_Niagara" or "EdGraphSchema_K2"

    Root->SetArrayField("nodes", NodesArray);
    Root->SetArrayField("links", LinksArray);

    // comment‐box regions
    if (Opts.bIncludeRegions)
    {
        Root->SetArrayField("regions", GatherRegions(Graph));
    }
    

    // Serialize to string
    FString Output;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Root, Writer);
    Writer->Close();

    // Write file to Saved/
    const FString FilePath = FPaths::ProjectSavedDir() / TEXT("GraphExport.json");
    const bool bSuccess = FFileHelper::SaveStringToFile(Output, *FilePath);

    // Notify editor
    if (GIsEditor && !GIsAutomationTesting)
    {
        FNotificationInfo Info(
            bSuccess
            ? NSLOCTEXT("UE5_GraphCapture", "ExportGraphJsonSuccess", "Graph exported to Saved/GraphExport.json")
            : NSLOCTEXT("UE5_GraphCapture", "ExportGraphJsonFail",    "Failed to write GraphExport.json")
        );
        Info.ExpireDuration = 3.0f;
        FSlateNotificationManager::Get().AddNotification(Info);
    }
}

// Helpers to build small JSON arrays
static TArray<TSharedPtr<FJsonValue>> MakeJsonStringArray(const TArray<FString>& In)
{
    TArray<TSharedPtr<FJsonValue>> Out;
    for (auto& S : In) Out.Add(MakeShared<FJsonValueString>(S));
    return Out;
}

// Gather comment‐region objects
static TArray<TSharedPtr<FJsonValue>> GatherRegions(UEdGraph* Graph)
{
    TArray<TSharedPtr<FJsonValue>> RegionsArray;
    for (UEdGraphNode* RawNode : Graph->Nodes)
    {
        if (UEdGraphNode_Comment* C = Cast<UEdGraphNode_Comment>(RawNode))
        {
            // find children inside box…
            TArray<FString> IDs;
            float X=C->NodePosX, Y=C->NodePosY, W=C->NodeWidth, H=C->NodeHeight;
            for (UEdGraphNode* Other : Graph->Nodes)
            {
                if (Other==C || Cast<UEdGraphNode_Comment>(Other)) continue;
                if (Other->NodePosX>=X && Other->NodePosX<=X+W
                 && Other->NodePosY>=Y && Other->NodePosY<=Y+H)
                {
                    IDs.Add(Other->GetName());
                }
            }
            auto R = MakeShared<FJsonObject>();
            R->SetStringField("name", C->NodeComment);
           R->SetArrayField("nodeIds", MakeJsonStringArray(IDs));
            RegionsArray.Add(MakeShared<FJsonValueObject>(R));
        }
    }
    return RegionsArray;
}