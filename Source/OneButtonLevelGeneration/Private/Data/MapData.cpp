// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/MapData.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"

#if WITH_EDITOR
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#endif

bool MapDataUtils::TextureToHeightArray(UTexture2D* Texture, TArray<uint16>& OutHeightArray)
{
	if (!Texture || !Texture->GetPlatformData() || Texture->GetPixelFormat() != PF_G16)
	{
		UE_LOG(LogTemp, Error, TEXT("Texture is invalid or not PF_G16 format"));
		return false;
	}

	const int32 Width = Texture->GetSizeX();
	const int32 Height = Texture->GetSizeY();
	const int32 NumPixels = Width * Height;

	FTexture2DMipMap Mip = Texture->GetPlatformMips()[0];
	const void* DataPtr = Mip.BulkData.Lock(LOCK_READ_ONLY);

	OutHeightArray.SetNumUninitialized(NumPixels);
	FMemory::Memcpy(OutHeightArray.GetData(), DataPtr, NumPixels * sizeof(uint16));

	Mip.BulkData.Unlock();

	return true;
}

bool MapDataUtils::ImportMap(TArray<uint16>& OutMapData, FIntPoint& OutResolution, const FString& FileName)
{
#if WITH_EDITOR
    const FString ContentDir = FPaths::ProjectContentDir();
    const FString SubDir = TEXT("Maps/");
    const FString FullPath = FPaths::Combine(ContentDir, SubDir, FileName);

    if (!FPaths::FileExists(FullPath))
    {
        UE_LOG(LogTemp, Error, TEXT("ImportMap: File does not exist: %s"), *FullPath);
        return false;
    }

    // Load PNG file
    TArray<uint8> CompressedData;
    if (!FFileHelper::LoadFileToArray(CompressedData, *FullPath))
    {
        UE_LOG(LogTemp, Error, TEXT("ImportMap: Failed to load file: %s"), *FullPath);
        return false;
    }

    // Create Image Wrapper
    IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
    TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

    if (!ImageWrapper.IsValid() || !ImageWrapper->SetCompressed(CompressedData.GetData(), CompressedData.Num()))
    {
        UE_LOG(LogTemp, Error, TEXT("ImportMap: Failed to decode PNG."));
        return false;
    }

    // Check if the image is 16-bit grayscale and extract raw data
    if (ImageWrapper->GetBitDepth() != 16 || ImageWrapper->GetFormat() != ERGBFormat::Gray)
    {
        UE_LOG(LogTemp, Error, TEXT("ImportMap: PNG format is not 16-bit grayscale."));
        return false;
    }

    const int32 Width = ImageWrapper->GetWidth();
    const int32 Height = ImageWrapper->GetHeight();

    TArray<uint8> RawData;
    if (!ImageWrapper->GetRaw(ERGBFormat::Gray, 16, RawData))
    {
        UE_LOG(LogTemp, Error, TEXT("ImportMap: Failed to extract raw image data."));
        return false;
    }

    // Convert raw data to uint16 array
    const int32 NumPixels = Width * Height;
    OutMapData.SetNumUninitialized(NumPixels);

    const uint16* RawPtr = reinterpret_cast<const uint16*>(RawData.GetData());
    FMemory::Memcpy(OutMapData.GetData(), RawPtr, NumPixels * sizeof(uint16));

    UE_LOG(LogTemp, Log, TEXT("ImportMap: Successfully imported %d x %d heightmap."), Width, Height);

    // Save resolution if needed
    OutResolution = FIntPoint(Width, Height);

    return true;
#else
    return false;
#endif
}

UTexture2D* MapDataUtils::ImportTextureFromPNG(const FString& FileName)
{
#if WITH_EDITOR
	const FString ContentDir = FPaths::ProjectContentDir();
	const FString SubDir = TEXT("Maps/");
	const FString FullPath = FPaths::Combine(ContentDir, SubDir, FileName);

	if (!FPaths::FileExists(FullPath))
	{
		UE_LOG(LogTemp, Error, TEXT("ImportTextureFromPNG: File does not exist: %s"), *FullPath);
		return nullptr;
	}

	TArray<uint8> FileData;
	if (!FFileHelper::LoadFileToArray(FileData, *FullPath))
	{
		UE_LOG(LogTemp, Error, TEXT("ImportTextureFromPNG: Failed to load file: %s"), *FullPath);
		return nullptr;
	}

	// Decode PNG file
	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

	if (!ImageWrapper.IsValid() || !ImageWrapper->SetCompressed(FileData.GetData(), FileData.Num()))
	{
		UE_LOG(LogTemp, Error, TEXT("ImportTextureFromPNG: Failed to decode PNG."));
		return nullptr;
	}

	const int32 Width = ImageWrapper->GetWidth();
	const int32 Height = ImageWrapper->GetHeight();

	// Import raw data
	TArray<uint8> RawData;
	if (!ImageWrapper->GetRaw(ERGBFormat::RGBA, 8, RawData))
	{
		UE_LOG(LogTemp, Error, TEXT("ImportTextureFromPNG: Failed to extract raw image data."));
		return nullptr;
	}

	const FString AssetName = FPaths::GetBaseFilename(FileName);  // "MyMap.png" → "MyMap"
	
	// Create Package
	const FString PackageName = FPaths::Combine(ContentDir, SubDir, AssetName);
	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create package for texture"));
		return nullptr;
	}

	// Create UTexture2D
	UTexture2D* Texture = NewObject<UTexture2D>(Package, *AssetName, RF_Public | RF_Standalone);
	FTexturePlatformData* PlatformData = new FTexturePlatformData();
	PlatformData->SizeX = Width;
	PlatformData->SizeY = Height;
	PlatformData->PixelFormat = PF_B8G8R8A8;

	// Create Mip and fill it with raw data
	FTexture2DMipMap* Mip = new FTexture2DMipMap();
	Mip->SizeX = Width;
	Mip->SizeY = Height;
	Mip->BulkData.Lock(LOCK_READ_WRITE);
	void* Data = Mip->BulkData.Realloc(RawData.Num());
	FMemory::Memcpy(Data, RawData.GetData(), RawData.Num());
	Mip->BulkData.Unlock();

	// Add Mip to PlatformData and assign it to Texture
	PlatformData->Mips.Add(Mip);
	Texture->SetPlatformData(PlatformData);

	// Register asset and mark it dirty
	FAssetRegistryModule::AssetCreated(Texture);
	Package->MarkPackageDirty();

	const FString FilePath = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = EObjectFlags::RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;

	const bool bSuccess = UPackage::SavePackage(Package, Texture, *FilePath, SaveArgs);

	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("Texture imported and saved successfully: %s"), *FilePath);
		return Texture;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save texture to disk: %s"), *FilePath);
		return nullptr;
	}
#else
	return nullptr;
#endif
}

bool MapDataUtils::ExportMap(const TArray<uint16>& InMap, const FIntPoint& Resolution, const FString& FileName)
{
#if WITH_EDITOR
	// Create directory and full path for the map file
	const FString DirectoryPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Maps/"));
	const FString FullPath = FPaths::Combine(DirectoryPath, FileName);

	UE_LOG(LogTemp, Log, TEXT("Target Directory: %s"), *DirectoryPath);
	UE_LOG(LogTemp, Log, TEXT("Target Full Path: %s"), *FullPath);

	// Get the platform file interface to check and create directories
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	if (!PlatformFile.DirectoryExists(*DirectoryPath)) 
	{
		UE_LOG(LogTemp, Log, TEXT("Directory does not exist. Creating directory..."));
		if (!PlatformFile.CreateDirectoryTree(*DirectoryPath))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create directory: %s"), *DirectoryPath);
			return false;
		}
	}

	// Create Image Wrapper for PNG format
	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

	if (!ImageWrapper.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create Image Wrapper."));
		return false;
	}

	// Set raw data to the Image Wrapper
	const int32 Width = Resolution.X;
	const int32 Height = Resolution.Y;

	const int32 ExpectedSize = Width * Height;
	if (InMap.Num() != ExpectedSize)
	{
		UE_LOG(LogTemp, Error, TEXT("InMap size (%d) does not match resolution (%d x %d = %d)"), InMap.Num(), Width, Height, ExpectedSize);
		return false;
	}

	if (ImageWrapper->SetRaw(InMap.GetData(), InMap.GetAllocatedSize(), Width, Height, ERGBFormat::Gray, 16))
	{
		const TArray64<uint8>& PngData = ImageWrapper->GetCompressed(100);

		if (FFileHelper::SaveArrayToFile(PngData, *FullPath))
		{
			UE_LOG(LogTemp, Log, TEXT("Map exported successfully to: %s"), *FullPath);
			return true;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to save map file: %s"), *FullPath);
			return false;
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to set raw data to Image Wrapper."));
		return false;
	}
#endif
}

bool MapDataUtils::ExportMap(const TArray<FColor>& InMap, const FIntPoint& Resolution, const FString& FileName)
{
#if WITH_EDITOR
	// Create directory and full path for the map file
	const FString DirectoryPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Maps"));
	const FString FullPath = FPaths::Combine(DirectoryPath, FileName);

	UE_LOG(LogTemp, Log, TEXT("Target Directory: %s"), *DirectoryPath);
	UE_LOG(LogTemp, Log, TEXT("Target Full Path: %s"), *FullPath);

	// Get the platform file interface to check and create directories
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	if (!PlatformFile.DirectoryExists(*DirectoryPath)) 
	{
		UE_LOG(LogTemp, Log, TEXT("Directory does not exist. Creating directory..."));
		if (!PlatformFile.CreateDirectoryTree(*DirectoryPath))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create directory: %s"), *DirectoryPath);
			return false; 
		}
	}
	
	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

	if (!ImageWrapper.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create Image Wrapper for color map."));
		return false;
	}

	// Set raw data to the Image Wrapper
	const int32 Width = Resolution.X;
	const int32 Height = Resolution.Y;

	const int32 ExpectedSize = Width * Height;
	if (InMap.Num() != ExpectedSize)
	{
		UE_LOG(LogTemp, Error, TEXT("InMap size (%d) does not match resolution (%d x %d = %d)"), InMap.Num(), Width, Height, ExpectedSize);
		return false;
	}

	// Set the raw color data to the Image Wrapper
	if (ImageWrapper->SetRaw(InMap.GetData(), InMap.Num() * sizeof(FColor), Width, Height, ERGBFormat::BGRA, 8))
	{
		const TArray64<uint8>& PngData = ImageWrapper->GetCompressed(100);

		if (FFileHelper::SaveArrayToFile(PngData, *FullPath))
		{
			UE_LOG(LogTemp, Log, TEXT("Color map exported successfully to: %s"), *FullPath);
			return true;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to save color map file."));
			return false;
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to set raw color data to Image Wrapper."));
		return false;
	}
#endif
}
