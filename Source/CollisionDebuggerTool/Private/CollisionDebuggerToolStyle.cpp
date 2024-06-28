// Copyright Epic Games, Inc. All Rights Reserved.

#include "CollisionDebuggerToolStyle.h"
#include "CollisionDebuggerTool.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FCollisionDebuggerToolStyle::StyleInstance = nullptr;

void FCollisionDebuggerToolStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FCollisionDebuggerToolStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FCollisionDebuggerToolStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("CollisionDebuggerToolStyle"));
	return StyleSetName;
}


const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef< FSlateStyleSet > FCollisionDebuggerToolStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("CollisionDebuggerToolStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("CollisionDebuggerTool")->GetBaseDir() / TEXT("Resources"));

	Style->Set("CollisionDebuggerTool.PluginAction", new IMAGE_BRUSH_SVG(TEXT("PlaceholderButtonIcon"), Icon20x20));
	return Style;
}

void FCollisionDebuggerToolStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FCollisionDebuggerToolStyle::Get()
{
	return *StyleInstance;
}
