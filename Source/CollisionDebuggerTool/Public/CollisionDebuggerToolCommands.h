// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "CollisionDebuggerToolStyle.h"

class FCollisionDebuggerToolCommands : public TCommands<FCollisionDebuggerToolCommands>
{
public:

	FCollisionDebuggerToolCommands()
		: TCommands<FCollisionDebuggerToolCommands>(TEXT("CollisionDebuggerTool"), NSLOCTEXT("Contexts", "CollisionDebuggerTool", "CollisionDebuggerTool Plugin"), NAME_None, FCollisionDebuggerToolStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > PluginAction;
};
