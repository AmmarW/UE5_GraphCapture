#include "UE5_GraphCapture.h"
#include "UE5_GraphCaptureStyle.h"
#include "UE5_GraphCaptureCommands.h"
#include "Misc/MessageDialog.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "EditorStyleSet.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Framework/Application/SlateApplication.h"
#include "UE5_GraphCaptureJsonExporter.h"
#include "UE5_GraphCaptureOptionsDialog.h"

#include "BlueprintEditorModule.h"
#include "MaterialEditorModule.h"
#include "BehaviorTreeEditorModule.h"

#include "Layout/ArrangedChildren.h"
#include "HighResScreenshot.h"
#include "Framework/Notifications/NotificationManager.h"
#include "GraphEditor.h"
#include "Widgets/SWindow.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Framework/Docking/TabManager.h"
#include "Brushes/SlateDynamicImageBrush.h"
#include "HAL/PlatformApplicationMisc.h"
#include "ImageWriteQueue.h"
#include "ImageWriteTask.h"
#include "ImagePixelData.h"

#include "Styling/AppStyle.h"

#include "LevelEditor.h"

static const FName UE5_GraphCaptureTabName("UE5_GraphCapture");

#define LOCTEXT_NAMESPACE "FUE5_GraphCaptureModule"

#define MAX_VIEWPORT_SIZE 16200

// TODO: Support ultra-wide graph snapshots (>16 384px)
//
// Slate will clamp any single sprite capture to 16 384px width and then fail
// if you ask beyond that. To handle very wide graphs, implement one of:
// 
// 1) TILED CAPTURE + STITCH
//    - Compute how many 16 384px-wide slices fit into your total width.
//    - For each slice:
//        • Adjust your graph view offset (scroll X) so that slice is in view.
//        • Call TakeScreenshot on a max-16k rectangle.
//        • Store that slice’s pixel buffer.
//    - After all slices are captured, memcpy them side-by-side into one big buffer
//      of size (totalWidth × totalHeight) and write it once.
//
// 2) DOWNSCALE VIA DPI SETTING
//    - Calculate a temporary scale factor: scale = min(1.0f, 16384.0f / totalWidth).
//    - Call FSlateApplication::Get().SetApplicationScale(scale).
//    - TakeScreenshot normally at the reduced DPI (so width ≤ 16 384).
//    - Reset application scale back to 1.0f.
//
// You can pick tiling for full-res output or downscale for simplicity.

void FUE5_GraphCaptureModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

#if WITH_EDITOR
	if (GIsEditor && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().OnPostTick().AddRaw(this, &FUE5_GraphCaptureModule::OnPostTick);
	}
#endif
	
	FUE5_GraphCaptureStyle::Initialize();
	FUE5_GraphCaptureStyle::ReloadTextures();

	FUE5_GraphCaptureCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FUE5_GraphCaptureCommands::Get().PluginAction,
		FExecuteAction::CreateRaw(this, &FUE5_GraphCaptureModule::PluginButtonClicked),
		FCanExecuteAction());

	PluginCommands->MapAction(
		FUE5_GraphCaptureCommands::Get().ActionForUE5_GraphCapture,
		FExecuteAction::CreateRaw(this, &FUE5_GraphCaptureModule::ButtonClickedForUE5_GraphCapture),
		FCanExecuteAction());
	
	// new JSON export mapping
    PluginCommands->MapAction(
        FUE5_GraphCaptureCommands::Get().ExportGraphJson,
        FExecuteAction::CreateRaw(this, &FUE5_GraphCaptureModule::HandleExportGraphJson),
		FCanExecuteAction());

	{
		TSharedPtr<FExtender> ExtenderOfMenu = MakeShareable(new FExtender());
		ExtenderOfMenu->AddMenuExtension("WindowLayout", EExtensionHook::After, PluginCommands, FMenuExtensionDelegate::CreateRaw(this, &FUE5_GraphCaptureModule::AddMenuExtensionForUE5_GraphCapture));
		FAssetEditorToolkit::GetSharedMenuExtensibilityManager()->AddExtender(ExtenderOfMenu);

		TSharedPtr<FExtender> ExtenderOfToolBar = MakeShareable(new FExtender());
		ExtenderOfToolBar->AddToolBarExtension("Asset", EExtensionHook::After, PluginCommands, FToolBarExtensionDelegate::CreateRaw(this, &FUE5_GraphCaptureModule::AddToolbarExtensionForUE5_GraphCapture));
		FAssetEditorToolkit::GetSharedToolBarExtensibilityManager()->AddExtender(ExtenderOfToolBar);
	}

	/*
	{
		FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");

		{
			TSharedPtr<FExtender> ExtenderOfMenu = MakeShareable(new FExtender());
			ExtenderOfMenu->AddMenuExtension("WindowLayout", EExtensionHook::After, PluginCommands, FMenuExtensionDelegate::CreateRaw(this, &FUE5_GraphCaptureModule::AddMenuExtensionForUE5_GraphCapture));

			BlueprintEditorModule.GetMenuExtensibilityManager()->AddExtender(ExtenderOfMenu);
		}
	}
	*/
}

void FUE5_GraphCaptureModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FUE5_GraphCaptureStyle::Shutdown();

	FUE5_GraphCaptureCommands::Unregister();
}

void FUE5_GraphCaptureModule::PluginButtonClicked()
{

}

BEGIN_FUNCTION_BUILD_OPTIMIZATION

void FUE5_GraphCaptureModule::PrepareForUE5_GraphCapture()
{
	TArray<TSharedPtr<SWidget>> GraphEditorsOfCurrentActiveTab = FindGraphEditorsOfCurrentStandaloneAssetEditorToolkit();

	int32 IndexOfWidget = -1;
	for (auto Widget : GraphEditorsOfCurrentActiveTab)
	{
		IndexOfWidget++;
		if (Widget.IsValid())
		{
			SGraphEditor* GraphEditor = static_cast<SGraphEditor*>(Widget.Get());
			if (GraphEditor)
			{
				FWidgetSnapshotTextureData& TextureData = WindowTextureData[WindowTextureData.AddDefaulted()];
				TakeScreenshot(Widget, TextureData);
			}
		}
	}
}

void FUE5_GraphCaptureModule::DoingUE5_GraphCapture()
{
	TArray<TSharedPtr<SWidget>> GraphEditorsOfCurrentActiveTab = FindGraphEditorsOfCurrentStandaloneAssetEditorToolkit();

	Reset();
	Reserve(GraphEditorsOfCurrentActiveTab.Num());

	int32 IndexOfWidget = -1;
	for (auto Widget : GraphEditorsOfCurrentActiveTab)
	{
		IndexOfWidget++;
		if (Widget.IsValid())
		{
			SGraphEditor* GraphEditor = static_cast<SGraphEditor*>(Widget.Get());
			if (GraphEditor)
			{
				FWidgetSnapshotTextureData& TextureData = WindowTextureData[WindowTextureData.AddDefaulted()];
				TakeScreenshot(Widget, TextureData);

				FHighResScreenshotConfig& HighResScreenshotConfig = GetHighResScreenshotConfig();
				int32 Year, Month, DayOfWeek, Day, Hour, Min, Sec, MSec;
				FPlatformTime::SystemTime(Year, Month, DayOfWeek, Day, Hour, Min, Sec, MSec);

				FString TimeAndIndex = FString::Printf(TEXT("_%04dY-%02dM-%02dD-%02dh-%02dm-%02ds-%03d_%02d"), Year, Month, Day, Hour, Min, Sec, IndexOfWidget);
				FString ImageSavePath = GetDefault<UEngine>()->GameScreenshotSaveDirectory.Path / TEXT("UE5_GraphCapture/UE5_GraphCapture") + TimeAndIndex;

				FString OutFilename = ImageSavePath + TEXT(".png");
				FIntPoint ImageSize(TextureData.Dimensions.X, TextureData.Dimensions.Y);
				// Save the bitmap to disc
				//HighResScreenshotConfig.SaveImage(ImageSavePath, TextureData.ColorData, ImageSize, &OutFilename);
				{
					TUniquePtr<FImageWriteTask> ImageTask = MakeUnique<FImageWriteTask>();
					ImageTask->PixelData = MakeUnique<TImagePixelData<FColor>>(ImageSize, TArray64<FColor>(MoveTemp(TextureData.ColorData)));

					// Set full alpha on the bitmap
					bool bWriteAlpha = false;
					if (!bWriteAlpha)
					{
						ImageTask->PixelPreProcessors.Add(TAsyncAlphaWrite<FColor>(255));
					}

					HighResScreenshotConfig.PopulateImageTaskParams(*ImageTask);
					ImageTask->Filename = ImageSavePath;

					// Save the bitmap to disc
					TFuture<bool> CompletionFuture = HighResScreenshotConfig.ImageWriteQueue->Enqueue(MoveTemp(ImageTask));
					if (CompletionFuture.IsValid())
					{
						CompletionFuture.Wait();
					}
				}

				// Notification of a successful screenshot
				if (GIsEditor && !GIsAutomationTesting)
				{
					auto Message = NSLOCTEXT("UE5_GraphCapture", "UE5_GraphCaptureSavedAs", "Graph Screenshot saved as");
					FNotificationInfo Info(Message);
					Info.bFireAndForget = true;
					Info.ExpireDuration = 3.0f;
					Info.bUseSuccessFailIcons = true;
					Info.bUseLargeFont = true;
					// Info.Image = FEditorStyle::GetBrush(TEXT("NotificationList.SuccessImage"));
					Info.Image = FAppStyle::GetBrush(TEXT("NotificationList.SuccessImage"));

					const FString HyperLinkText = FPaths::ConvertRelativePathToFull(OutFilename);
					Info.Hyperlink = FSimpleDelegate::CreateStatic([](FString SourceFilePath)
						{
							FPlatformProcess::ExploreFolder(*(FPaths::GetPath(SourceFilePath)));
						}, HyperLinkText);
					Info.HyperlinkText = FText::FromString(HyperLinkText);

					FSlateNotificationManager::Get().AddNotification(Info);
				}
			}
		}
	}

	if (GraphEditorsOfCurrentActiveTab.Num() <= 0)
	{
		// Notification of a successful screenshot
		if (GIsEditor && !GIsAutomationTesting)
		{
			auto Message = NSLOCTEXT("UE5_GraphCapture", "UE5_GraphCaptureFailed", "No GraphEditor. Graph Screenshot Failed. ");
			FNotificationInfo Info(Message);
			Info.bFireAndForget = true;
			Info.ExpireDuration = 3.0f;
			Info.bUseSuccessFailIcons = true;
			Info.bUseLargeFont = true;
			// Info.Image = FEditorStyle::GetBrush(TEXT("NotificationList.FailImage"));
			Info.Image = FAppStyle::GetBrush(TEXT("NotificationList.FailImage"));
			
			FSlateNotificationManager::Get().AddNotification(Info);
		}
	}

	CreateBrushes();
}

void FUE5_GraphCaptureModule::TakeScreenshot(TSharedPtr<SWidget> Widget, FWidgetSnapshotTextureData& TextureData)
{
	SGraphEditor* GraphEditor = static_cast<SGraphEditor*>(Widget.Get());
	if (!GraphEditor)
	{
		return;
	}

	FSlateRect BoundsForSelectedNodes;
	FGraphPanelSelectionSet  GraphPanelSelectionSet = GraphEditor->GetSelectedNodes();

	GraphEditor->GetBoundsForSelectedNodes(BoundsForSelectedNodes, 0.0f);
	FVector2D ViewLocation;
	FVector2D NewVewloction;
	float ZoomAmount;
	GraphEditor->GetViewLocation(ViewLocation, ZoomAmount);
	FGeometry CachedGeometry = GraphEditor->GetCachedGeometry();
	FVector2D SizeOfWidget = CachedGeometry.GetLocalSize();

	FVector2D TopLeftPadding = BoundsForSelectedNodes.GetTopLeft() - ViewLocation;

	FVector2D DefaultPaddingOfBoundsForSelectedNodes = FVector2D(100.0f, 100.0f);

	FVector2D PaddingOfWindowSize;
	PaddingOfWindowSize.X = TopLeftPadding.X > 0 ? TopLeftPadding.X * 2 : DefaultPaddingOfBoundsForSelectedNodes.X * 2;
	PaddingOfWindowSize.Y = TopLeftPadding.Y > 0 ? TopLeftPadding.Y * 2 : DefaultPaddingOfBoundsForSelectedNodes.Y * 2;

	NewVewloction.X = TopLeftPadding.X > 0 ? ViewLocation.X : BoundsForSelectedNodes.Left -100;
	NewVewloction.Y = TopLeftPadding.Y > 0 ? ViewLocation.Y : BoundsForSelectedNodes.Top - 100;

	GraphEditor->SetViewLocation(NewVewloction, ZoomAmount);

	float DPIScale = FPlatformApplicationMisc::GetDPIScaleFactorAtPoint(0.0f, 0.0f);

	FVector2D WindowSize = SizeOfWidget * DPIScale;
	if (GraphPanelSelectionSet.Num() > 0)
	{
		WindowSize = (BoundsForSelectedNodes.GetSize() + PaddingOfWindowSize) * DPIScale * ZoomAmount;
	}
	else
	{
		WindowSize = SizeOfWidget * DPIScale;
	}

	WindowSize = WindowSize.ClampAxes(0.0f, MAX_VIEWPORT_SIZE);

	GraphEditor->ClearSelectionSet();

	TSharedRef<SWindow> NewWindowRef = SNew(SWindow)
		.CreateTitleBar(false)
		.ClientSize(WindowSize)
		.ScreenPosition(FVector2D(0.0f, 0.0f))
		.AdjustInitialSizeAndPositionForDPIScale(false)
		.SaneWindowPlacement(false)
		.SupportsTransparency(EWindowTransparency::PerWindow)
		.InitialOpacity(0.0f);

	NewWindowRef->SetContent(Widget.ToSharedRef());

	const bool bShowImmediately = false;
	TSharedPtr<SWindow> WidgetWindow = FSlateApplication::Get().FindWidgetWindow(Widget.ToSharedRef());
	FSlateApplication::Get().AddWindow(NewWindowRef, bShowImmediately);

	GraphEditor->Invalidate(EInvalidateWidgetReason::LayoutAndVolatility);
	NewWindowRef->ShowWindow();

	// Screenshot the current window so we can pick against its current state
	FSlateApplication::Get().TakeScreenshot(NewWindowRef, TextureData.ColorData, TextureData.Dimensions);

	GraphEditor->SetViewLocation(ViewLocation, ZoomAmount);

	for (auto SelectedNode : GraphPanelSelectionSet)
	{
		if (UEdGraphNode* SelectedEdGraphNode = Cast<UEdGraphNode>(SelectedNode))
		{
			GraphEditor->SetNodeSelection(SelectedEdGraphNode, true);
		}
	}

	NewWindowRef->HideWindow();
	NewWindowRef->RequestDestroyWindow();
}

void FUE5_GraphCaptureModule::ButtonClickedForUE5_GraphCapture()
{
	PrepareForUE5_GraphCapture();
	TakingUE5_GraphCapture = true;
	return;
}

void FUE5_GraphCaptureModule::OnPostTick(float DeltaTime)
{
	if (TakingUE5_GraphCapture)
	{
		DoingUE5_GraphCapture();
		TakingUE5_GraphCapture = false;
	}
}

END_FUNCTION_BUILD_OPTIMIZATION

TArray<TSharedPtr<SWidget>> FUE5_GraphCaptureModule::FindGraphEditorsOfCurrentStandaloneAssetEditorToolkit()
{
	TArray<TSharedPtr<SWidget>> FindedChildrenWidgets;

	TSharedPtr<SDockTab> ActiveTab = FGlobalTabmanager::Get()->GetActiveTab();

	if (!ActiveTab.IsValid())
	{
		return FindedChildrenWidgets;
	}
	FWidgetPath WidgetPathOfActiveTab;
	TSharedRef<const SWidget> WidgetOfActiveTab = StaticCastSharedRef<SWidget>(ActiveTab.ToSharedRef());
	FSlateApplication::Get().FindPathToWidget(WidgetOfActiveTab, WidgetPathOfActiveTab, EVisibility::All);
	FArrangedChildren::FArrangedWidgetArray& ArrangedWidgetArray = WidgetPathOfActiveTab.Widgets.GetInternalArray();
	TSharedPtr<SWidget>  StandaloneAssetEditorToolkitHost = nullptr;
	for (auto ArrangedWidget : ArrangedWidgetArray)
	{
		TSharedPtr<SWidget> CurrentWidget = ArrangedWidget.Widget;
		if (CurrentWidget.IsValid())
		{
			FString TypeOfCurrentWidget = CurrentWidget->GetTypeAsString();
			if (TypeOfCurrentWidget.Equals(TEXT("SStandaloneAssetEditorToolkitHost")))
			{
				StandaloneAssetEditorToolkitHost = CurrentWidget;
				break;
			}
		}
	}

	if (!StandaloneAssetEditorToolkitHost.IsValid())
	{
		return FindedChildrenWidgets;
	}

	FString WidgetType = TEXT("SGraphEditorImpl");
	

	RecursivelyFindChildrenWidgetOfType(StandaloneAssetEditorToolkitHost, WidgetType, FindedChildrenWidgets);

	return FindedChildrenWidgets;
}

void FUE5_GraphCaptureModule::RecursivelyFindChildrenWidgetOfType(TSharedPtr<SWidget> Widget, FString WidgetType, TArray<TSharedPtr<SWidget>>& OutFindedChildrenWidgets)
{
	if (!Widget.IsValid())
	{
		return;
	}

	FChildren* Children = Widget->GetChildren();
	if (Children)
	{
		for (int32 i = 0; i < Children->Num(); i++)
		{
			TSharedPtr<SWidget> Child = Children->GetChildAt(i);
			if (Child.IsValid())
			{
				if (Child->GetTypeAsString().Equals(WidgetType))
				{
					OutFindedChildrenWidgets.AddUnique(Child);
				}
				RecursivelyFindChildrenWidgetOfType(Child, WidgetType, OutFindedChildrenWidgets);
			}
		}
	}
}

void FUE5_GraphCaptureModule::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FUE5_GraphCaptureCommands::Get().PluginAction);
}

void FUE5_GraphCaptureModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(FUE5_GraphCaptureCommands::Get().PluginAction);
}

void FUE5_GraphCaptureModule::AddMenuExtensionForUE5_GraphCapture(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FUE5_GraphCaptureCommands::Get().ActionForUE5_GraphCapture);
}

void FUE5_GraphCaptureModule::AddToolbarExtensionForUE5_GraphCapture(FToolBarBuilder& Builder)
{
    Builder.AddSeparator();

    Builder.AddToolBarButton(
        FUE5_GraphCaptureCommands::Get().ActionForUE5_GraphCapture,
        NAME_None,
        LOCTEXT("GraphShot_Label",    "GraphShot"),
        LOCTEXT("GraphShot_Tooltip",  "Take a screenshot of the current graph"),
        FSlateIcon(FUE5_GraphCaptureStyle::GetStyleSetName(), "UE5GraphCapture.GraphShot")
    );

    Builder.AddToolBarButton(
        FUE5_GraphCaptureCommands::Get().ExportGraphJson,
        NAME_None,
        LOCTEXT("ExportJson_Label",   "Export Graph JSON"),
        LOCTEXT("ExportJson_Tooltip", "Dump the graph structure to a JSON file"),
        FSlateIcon(FUE5_GraphCaptureStyle::GetStyleSetName(), "UE5GraphCapture.ExportGraphJson")
    );
}

void FUE5_GraphCaptureModule::TakeSnapshot()
{
	TArray<TSharedRef<SWindow>> VisibleWindows;
	FSlateApplication::Get().GetAllVisibleWindowsOrdered(VisibleWindows);

	CreateSnapshot(VisibleWindows);
}

void FUE5_GraphCaptureModule::Reserve(const int32 NumWindows)
{
	WindowTextureData.Reserve(NumWindows);
	WindowTextureBrushes.Reserve(NumWindows);
}

void FUE5_GraphCaptureModule::Reset()
{
	DestroyBrushes();

	//Windows.Reset();
	WindowTextureData.Reset();
	WindowTextureBrushes.Reset();
}

void FUE5_GraphCaptureModule::CreateSnapshot(const TArray<TSharedRef<SWindow>>& VisibleWindows)
{
	Reset();
	Reserve(VisibleWindows.Num());

	for (const auto& VisibleWindow : VisibleWindows)
	{
		// Screenshot the current window so we can pick against its current state
		FWidgetSnapshotTextureData& TextureData = WindowTextureData[WindowTextureData.AddDefaulted()];
		FSlateApplication::Get().TakeScreenshot(VisibleWindow, TextureData.ColorData, TextureData.Dimensions);
	}

	CreateBrushes();
}

void FUE5_GraphCaptureModule::CreateBrushes()
{
	DestroyBrushes();

	WindowTextureBrushes.Reserve(WindowTextureData.Num());

	static int32 TextureIndex = 0;
	for (const auto& TextureData : WindowTextureData)
	{
		if (TextureData.ColorData.Num() > 0)
		{
			TArray<uint8> TextureDataAsBGRABytes;
			TextureDataAsBGRABytes.Reserve(TextureData.ColorData.Num() * 4);
			for (const FColor& PixelColor : TextureData.ColorData)
			{
				TextureDataAsBGRABytes.Add(PixelColor.B);
				TextureDataAsBGRABytes.Add(PixelColor.G);
				TextureDataAsBGRABytes.Add(PixelColor.R);
				TextureDataAsBGRABytes.Add(PixelColor.A);
			}

			WindowTextureBrushes.Add(FSlateDynamicImageBrush::CreateWithImageData(
				*FString::Printf(TEXT("FWidgetSnapshotData_WindowTextureBrush_%d"), TextureIndex++),
				FVector2D(TextureData.Dimensions.X, TextureData.Dimensions.Y),
				TextureDataAsBGRABytes
			));
		}
		else
		{
			WindowTextureBrushes.Add(nullptr);
		}
	}
}

void FUE5_GraphCaptureModule::DestroyBrushes()
{
	for (const auto& WindowTextureBrush : WindowTextureBrushes)
	{
		if (WindowTextureBrush.IsValid())
		{
			WindowTextureBrush->ReleaseResource();
		}
	}

	WindowTextureBrushes.Reset();
}
void FUE5_GraphCaptureModule::HandleExportGraphJson()
{
    // 1) Find an active SGraphEditor
    TSharedPtr<SGraphEditor> Editor;
    for (auto& W : FindGraphEditorsOfCurrentStandaloneAssetEditorToolkit())
    {
        if (W.IsValid())
        {
            Editor = StaticCastSharedRef<SGraphEditor>(W.ToSharedRef());
            break;
        }
    }

    // If none found, bail out with a notification
    if (!Editor)
    {
        FNotificationInfo Info(
            NSLOCTEXT("UE5_GraphCapture", "ExportGraphJsonFailed", 
            "No GraphEditor found—JSON export failed.")
        );
        Info.ExpireDuration = 3.0f;
        FSlateNotificationManager::Get().AddNotification(Info);
        return;
    }

    // 2) Pop up the options dialog
    FSimpleDelegate OnOk = FSimpleDelegate::CreateLambda([this, Editor]()
    {
        // Grab the chosen options, close the dialog, then export
        const auto& Opts = DialogWidget->GetOptions();

        if (TSharedPtr<SWindow> Win = FSlateApplication::Get().FindWidgetWindow(DialogWidget.ToSharedRef()))
        {
            Win->RequestDestroyWindow();
        }

        FUE5_GraphCaptureJsonExporter::ExportGraph(Editor.ToSharedRef(), Opts);
    });

    FSimpleDelegate OnCancel = FSimpleDelegate::CreateLambda([this]()
    {
        if (TSharedPtr<SWindow> Win = FSlateApplication::Get().FindWidgetWindow(DialogWidget.ToSharedRef()))
        {
            Win->RequestDestroyWindow();
        }
    });

    // Build and show the modal dialog
    DialogWidget = SNew(SJsonExportOptionsDialog)
        .OnOk(OnOk)
        .OnCancel(OnCancel);

    TSharedRef<SWindow> DialogWindow =
        SNew(SWindow)
        .Title(NSLOCTEXT("GraphCap","JsonOptionsTitle","JSON Export Options"))
        .SupportsMinimize(false)
        .SupportsMaximize(false)
        .ClientSize(FVector2D(300, 300))
        [
            DialogWidget.ToSharedRef()
        ];

    FSlateApplication::Get().AddModalWindow(DialogWindow, nullptr);
}


#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUE5_GraphCaptureModule, UE5_GraphCapture)