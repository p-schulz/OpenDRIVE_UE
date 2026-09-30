#pragma once

#include "CoreMinimal.h"
#include "Toolkits/BaseToolkit.h"
#include "OpenDriveVersion.h"

class SWidget;

/** Toolkit of the OpenDRIVE editor mode; hosts the mode panel as inline content. */
class FOpenDriveModeToolkit : public FModeToolkit
{
public:
#if ODR_UE_AT_LEAST(5, 1)
	virtual void Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode) override;
#else
	virtual void Init(const TSharedPtr<IToolkitHost>& InitToolkitHost) override;
#endif

	virtual FName GetToolkitFName() const override { return FName("OpenDriveModeToolkit"); }
	virtual FText GetBaseToolkitName() const override;
	virtual TSharedPtr<SWidget> GetInlineContent() const override { return Panel; }

private:
	TSharedPtr<SWidget> Panel;
};
