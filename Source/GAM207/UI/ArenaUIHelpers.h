#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GAM207.h"

namespace ArenaUI
{
	/** Loads a soft widget class and creates it, logging a clear message when the Widget Blueprint hasn't been made yet. */
	template <typename WidgetT>
	WidgetT* CreateFromSoftClass(APlayerController* Owner, const TSoftClassPtr<WidgetT>& SoftClass, const TCHAR* Label)
	{
		UClass* Class = SoftClass.LoadSynchronous();
		if (!Class || !Class->IsChildOf(WidgetT::StaticClass()))
		{
			UE_LOG(LogArena, Warning, TEXT("%s widget '%s' not found, or its parent class isn't %s. See Docs/Editor_Setup_Guide.md."),
				Label, *SoftClass.ToString(), *WidgetT::StaticClass()->GetName());
			return nullptr;
		}
		return CreateWidget<WidgetT>(Owner, Class);
	}

	inline FText FormatTime(float Seconds)
	{
		const int32 Total = FMath::Max(0, FMath::CeilToInt(Seconds));
		return FText::FromString(FString::Printf(TEXT("%d:%02d"), Total / 60, Total % 60));
	}
}
