#include "Mode/SOpenDriveModePanel.h"
#include "OpenDriveEditorContext.h"
#include "Authoring/OpenDriveEditorSettings.h"
#include "OpenDrive/OpenDriveAsset.h"
#include "OpenDrive/OpenDriveMap.h"
#include "OpenDriveEditorModule.h"
#include "AssetRegistry/AssetData.h"
#include "DesktopPlatformModule.h"
#include "Framework/Application/SlateApplication.h"
#include "IDesktopPlatform.h"
#include "IDetailsView.h"
#include "Misc/Paths.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyEditorModule.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "OpenDriveModePanel"

void SOpenDriveModePanel::Construct(const FArguments& InArgs, FOpenDriveEditorContext& InContext)
{
	Context = &InContext;

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs Args;
	Args.bAllowSearch = false;
	Args.bHideSelectionTip = true;
	Args.bShowOptions = false;
	Args.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	SettingsView = PropertyModule.CreateDetailView(Args);
	SettingsView->SetObject(Context->GetSettings());

	auto MakeButton = [](const FText& Label, const FText& Tooltip, TFunction<FReply()> OnClick, TFunction<bool()> IsEnabled)
	{
		return SNew(SButton)
			.Text(Label)
			.ToolTipText(Tooltip)
			.OnClicked_Lambda([OnClick]() { return OnClick(); })
			.IsEnabled_Lambda([IsEnabled]() { return IsEnabled(); });
	};
	FOpenDriveEditorContext* Ctx = Context;
	auto HasAsset = [Ctx]() { return Ctx->GetAsset() != nullptr; };
	auto Always = []() { return true; };

	ChildSlot
	[
		SNew(SScrollBox)

		+ SScrollBox::Slot().Padding(4.f)
		[
			SNew(SExpandableArea)
			.AreaTitle(LOCTEXT("RoadNetworkSection", "Road Network"))
			.InitiallyCollapsed(false)
			.BodyContent()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(2.f)
				[
					SNew(SObjectPropertyEntryBox)
					.AllowedClass(UOpenDriveAsset::StaticClass())
					.ObjectPath(this, &SOpenDriveModePanel::GetAssetPath)
					.OnObjectChanged(this, &SOpenDriveModePanel::OnAssetPicked)
					.DisplayThumbnail(false)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(2.f)
				[
					SNew(SWrapBox).UseAllottedSize(true)
					+ SWrapBox::Slot().Padding(0.f, 0.f, 4.f, 4.f)
					[
						MakeButton(LOCTEXT("New", "New"), LOCTEXT("NewTip", "Create a new road network (one default straight road)"),
							[Ctx]() { Ctx->CreateNewRoadNetwork(); return FReply::Handled(); }, Always)
					]
					+ SWrapBox::Slot().Padding(0.f, 0.f, 4.f, 4.f)
					[
						MakeButton(LOCTEXT("ImportXodr", "Import XODR..."), LOCTEXT("ImportXodrTip", "Import an OpenDRIVE (.xodr) file"),
							[this]() { return OnImport(); }, Always)
					]
					+ SWrapBox::Slot().Padding(0.f, 0.f, 4.f, 4.f)
					[
						MakeButton(LOCTEXT("Export", "Export XODR..."), LOCTEXT("ExportTip", "Save the road network as an .xodr file"),
							[Ctx]() { Ctx->ExportFile(); return FReply::Handled(); }, HasAsset)
					]
					+ SWrapBox::Slot().Padding(0.f, 0.f, 4.f, 4.f)
					[
						MakeButton(LOCTEXT("RoadList", "Road List"), LOCTEXT("RoadListTip", "Open the dockable road/junction browser"),
							[]() { FOpenDriveEditorModule::Get().OpenRoadListTab(); return FReply::Handled(); }, Always)
					]
					+ SWrapBox::Slot().Padding(0.f, 0.f, 4.f, 4.f)
					[
						MakeButton(LOCTEXT("Elevation", "Elevation"), LOCTEXT("ElevationTip", "Open the dockable elevation profile editor"),
							[]() { FOpenDriveEditorModule::Get().OpenElevationTab(); return FReply::Handled(); }, Always)
					]
					+ SWrapBox::Slot().Padding(0.f, 0.f, 4.f, 4.f)
					[
						MakeButton(LOCTEXT("Superelevation", "Superelevation"), LOCTEXT("SuperelevationTip", "Open the dockable superelevation (banking) profile editor"),
							[]() { FOpenDriveEditorModule::Get().OpenSuperelevationTab(); return FReply::Handled(); }, Always)
					]
					+ SWrapBox::Slot().Padding(0.f, 0.f, 4.f, 4.f)
					[
						MakeButton(LOCTEXT("Apply", "Apply"), LOCTEXT("ApplyTip", "Write unapplied edits to the road network asset"),
							[Ctx]() { Ctx->Apply(); return FReply::Handled(); }, [Ctx]() { return Ctx->GetAsset() && Ctx->IsDirty(); })
					]
					+ SWrapBox::Slot().Padding(0.f, 0.f, 4.f, 4.f)
					[
						MakeButton(LOCTEXT("Revert", "Revert"), LOCTEXT("RevertTip", "Discard unapplied edits"),
							[Ctx]() { Ctx->Revert(); return FReply::Handled(); }, [Ctx]() { return Ctx->GetAsset() && Ctx->IsDirty(); })
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(2.f)
				[
					SNew(STextBlock).Text(this, &SOpenDriveModePanel::GetInfoText).AutoWrapText(true)
				]
			]
		]

		+ SScrollBox::Slot().Padding(4.f)
		[
			SNew(SExpandableArea)
			.AreaTitle(LOCTEXT("VizSection", "Visualization and Import"))
			.InitiallyCollapsed(false)
			.BodyContent()
			[
				SettingsView.ToSharedRef()
			]
		]
	];
}

SOpenDriveModePanel::~SOpenDriveModePanel()
{
}

FString SOpenDriveModePanel::GetAssetPath() const
{
	const UOpenDriveAsset* Asset = Context->GetAsset();
	return Asset ? Asset->GetPathName() : FString();
}

void SOpenDriveModePanel::OnAssetPicked(const FAssetData& AssetData)
{
	Context->SetAsset(Cast<UOpenDriveAsset>(AssetData.GetAsset()));
}

FReply SOpenDriveModePanel::OnImport()
{
	IDesktopPlatform* Platform = FDesktopPlatformModule::Get();
	if (!Platform)
	{
		return FReply::Handled();
	}
	TArray<FString> Files;
	const void* Parent = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
	const bool bPicked = Platform->OpenFileDialog(Parent, TEXT("Import OpenDRIVE"), FPaths::ProjectDir(), FString(), TEXT("OpenDRIVE (*.xodr)|*.xodr"), EFileDialogFlags::None, Files);
	if (bPicked)
	{
		for (const FString& File : Files)
		{
			Context->ImportFile(File);
		}
	}
	return FReply::Handled();
}

FText SOpenDriveModePanel::GetInfoText() const
{
	const UOpenDriveAsset* Asset = Context->GetAsset();
	if (!Asset)
	{
		return LOCTEXT("NoAsset", "No road network selected.");
	}
	if (!Asset->IsMapValid())
	{
		return FText::Format(LOCTEXT("Invalid", "OpenDRIVE XML could not be parsed:\n{0}"), FText::FromString(Asset->ParseStatus));
	}
	const FOpenDriveMap& Working = Context->GetWorking();
	FString Text = FString::Printf(TEXT("%s\n%d road(s), %d junction(s), %.0f m total length"),
		Working.GetName().IsEmpty() ? TEXT("(unnamed)") : *Working.GetName(), Working.GetRoads().Num(), Working.GetJunctions().Num(), Working.GetTotalLength());
	if (Context->IsDirty())
	{
		Text += TEXT("\nUnapplied edits.");
	}
	return FText::FromString(Text);
}

#undef LOCTEXT_NAMESPACE
