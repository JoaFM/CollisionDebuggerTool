// Copyright Epic Games, Inc. All Rights Reserved.

#include "CollisionDebuggerTool.h"
#include "CollisionDebuggerToolStyle.h"
#include "CollisionDebuggerToolCommands.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"

static const FName CollisionDebuggerToolTabName("CollisionDebuggerTool");

#define LOCTEXT_NAMESPACE "FCollisionDebuggerToolModule"

void FCollisionDebuggerToolModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FCollisionDebuggerToolStyle::Initialize();
	FCollisionDebuggerToolStyle::ReloadTextures();

	FCollisionDebuggerToolCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FCollisionDebuggerToolCommands::Get().PluginAction,
		FExecuteAction::CreateRaw(this, &FCollisionDebuggerToolModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FCollisionDebuggerToolModule::RegisterMenus));
}

void FCollisionDebuggerToolModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	FCollisionDebuggerToolStyle::Shutdown();
	FCollisionDebuggerToolCommands::Unregister();
}

void FCollisionDebuggerToolModule::PluginButtonClicked()
{
	extern TAutoConsoleVariable<int32> CVarCollisionDebugEnable;
	const int32 CurrentStateOfTool = CVarCollisionDebugEnable.GetValueOnGameThread();
	const int32 NewState = CurrentStateOfTool < 1 ? 1 : 0;
	CVarCollisionDebugEnable.AsVariable()->Set(NewState, EConsoleVariableFlags::ECVF_SetByConsole);
}

void FCollisionDebuggerToolModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
	{
		FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
		Section.AddMenuEntryWithCommandList(FCollisionDebuggerToolCommands::Get().PluginAction, PluginCommands);
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FCollisionDebuggerToolModule, CollisionDebuggerTool)