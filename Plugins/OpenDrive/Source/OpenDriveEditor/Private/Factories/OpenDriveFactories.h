#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "EditorReimportHandler.h"
#include "OpenDriveFactories.generated.h"

/** Imports .xodr files as UOpenDriveAsset. */
UCLASS()
class UOpenDriveImportFactory : public UFactory, public FReimportHandler
{
	GENERATED_BODY()

public:
	UOpenDriveImportFactory();

	virtual UObject* FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;

	//~ FReimportHandler
	virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
	virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
	virtual EReimportResult::Type Reimport(UObject* Obj) override;
	virtual int32 GetPriority() const override { return ImportPriority; }
};

/** Creates a new road network (a single default straight road) from the content browser or the editor mode. */
UCLASS()
class UOpenDriveNewFactory : public UFactory
{
	GENERATED_BODY()

public:
	UOpenDriveNewFactory();

	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
