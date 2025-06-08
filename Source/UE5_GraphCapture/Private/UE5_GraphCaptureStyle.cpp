#include "UE5_GraphCaptureStyle.h"
#include "UE5_GraphCapture.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"

TSharedPtr< FSlateStyleSet > FUE5_GraphCaptureStyle::StyleInstance = NULL;

void FUE5_GraphCaptureStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FUE5_GraphCaptureStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FUE5_GraphCaptureStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("UE5_GraphCaptureStyle"));
	return StyleSetName;
}

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )


const FVector2D Icon256x256(256.0f, 256.0f);
const FVector2D Icon512x512(512.0f, 512.0f);

TSharedRef< FSlateStyleSet > FUE5_GraphCaptureStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("UE5_GraphCaptureStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("UE5_GraphCapture")->GetBaseDir() / TEXT("Resources"));

	// Style->Set("UE5_GraphCapture.PluginAction", 
	// 	new IMAGE_BRUSH(TEXT("Icon512"), Icon512x512));
	// Style->Set("UE5_GraphCapture.ActionForUE5_GraphCapture", 
	// 	new IMAGE_BRUSH(TEXT("Icon512"), Icon512x512));
	// Style->Set("UE5_GraphCapture.ActionForUE5_GraphCapture.Small", 
	// 	new IMAGE_BRUSH(TEXT("Icon256"), Icon256x256));

	// add these two:
	Style->Set("UE5GraphCapture.GraphShot",
		new IMAGE_BRUSH(TEXT("Icon512"), FVector2D(16,16)));
	Style->Set("UE5GraphCapture.ExportGraphJson",
		new IMAGE_BRUSH(TEXT("Icon512"), FVector2D(16,16)));

	return Style;
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT

void FUE5_GraphCaptureStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FUE5_GraphCaptureStyle::Get()
{
	return *StyleInstance;
}
