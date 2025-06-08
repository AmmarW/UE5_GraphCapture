// UE5_GraphCapture.h
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/SWindow.h"
#include "GraphEditor.h"
#include "Framework/Commands/UICommandList.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "AssetThumbnail.h"
#include "Brushes/SlateDynamicImageBrush.h"
#include "UE5_GraphCaptureJsonExporter.h"  // for ExportGraph()

class FToolBarBuilder;
class FMenuBuilder;
class SJsonExportOptionsDialog;
/**
 * The raw texture data from taking a screenshot of a Slate widget (typically the root window)
 */
struct FWidgetSnapshotTextureData
{
    /** The dimensions of the texture */
    FIntVector Dimensions;

    /** The raw color data for the texture (BGRA) */
    TArray<FColor> ColorData;
};

class FUE5_GraphCaptureModule : public IModuleInterface
{
public:
    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    /** Screenshot handlers (unchanged) */
    void PluginButtonClicked();
    void ButtonClickedForUE5_GraphCapture();

    /** JSON export handler */
    void HandleExportGraphJson();

    void TakeSnapshot();
    void CreateBrushes();
    void DestroyBrushes();
    void Reserve(const int32 NumWindows);
    void Reset();
    void CreateSnapshot(const TArray<TSharedRef<SWindow>>& VisibleWindows);

    TSharedRef<class SDockTab> CreateDefaultTab(const class FSpawnTabArgs& Args);

    static TArray<TSharedPtr<SWidget>> FindGraphEditorsOfCurrentStandaloneAssetEditorToolkit();
    static void RecursivelyFindChildrenWidgetOfType(TSharedPtr<SWidget> Widget, FString WidgetType, TArray<TSharedPtr<SWidget>>& OutFindedChildrenWidgets);

private:
    /** Existing screenshot menu/toolbar extensions */
    void AddToolbarExtension(FToolBarBuilder& Builder);
    void AddMenuExtension(FMenuBuilder& Builder);

    /** New JSON export menu/toolbar extensions */
    void AddMenuExtensionForUE5_GraphCapture(FMenuBuilder& Builder);
    void AddToolbarExtensionForUE5_GraphCapture(FToolBarBuilder& Builder);

    void PrepareForUE5_GraphCapture();
    void DoingUE5_GraphCapture();
    void TakeScreenshot(TSharedPtr<SWidget> Widget, FWidgetSnapshotTextureData& TextureData);
    void OnPostTick(float DeltaTime);

private:
    TSharedPtr<class FUICommandList> PluginCommands;

    /** Thumbnail support for screenshot previews */
    TSharedPtr<class FAssetThumbnailPool> AssetThumbnailPool;
    TSharedPtr<FAssetThumbnail> AssetThumbnail;

    /** Holds one entry per window when capturing screenshots */
    TArray<FWidgetSnapshotTextureData> WindowTextureData;

    /** Slate brushes for previewing those screenshots */
    TArray<TSharedPtr<FSlateDynamicImageBrush>> WindowTextureBrushes;

    bool TakingUE5_GraphCapture = false;

    TSharedPtr<SJsonExportOptionsDialog> DialogWidget;
};
