// Copyright Epic Games, Inc. All Rights Reserved.

#include "CollisionDebuggerToolCommands.h"

#define LOCTEXT_NAMESPACE "FCollisionDebuggerToolModule"

void FCollisionDebuggerToolCommands::RegisterCommands()
{
	UI_COMMAND(PluginAction, "CollisionDebuggerTool", "Execute CollisionDebuggerTool action", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
