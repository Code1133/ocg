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

	// Mip 0 기준
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

    // 1. PNG 파일을 메모리로 읽기
    TArray<uint8> CompressedData;
    if (!FFileHelper::LoadFileToArray(CompressedData, *FullPath))
    {
        UE_LOG(LogTemp, Error, TEXT("ImportMap: Failed to load file: %s"), *FullPath);
        return false;
    }

    // 2. 이미지 래퍼 생성
    IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
    TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

    if (!ImageWrapper.IsValid() || !ImageWrapper->SetCompressed(CompressedData.GetData(), CompressedData.Num()))
    {
        UE_LOG(LogTemp, Error, TEXT("ImportMap: Failed to decode PNG."));
        return false;
    }

    // 3. PNG가 16비트 Grayscale인지 확인 후 원시 데이터 추출
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

    // 4. RawData를 uint16 배열로 변환
    const int32 NumPixels = Width * Height;
    OutMapData.SetNumUninitialized(NumPixels);

    const uint16* RawPtr = reinterpret_cast<const uint16*>(RawData.GetData());
    FMemory::Memcpy(OutMapData.GetData(), RawPtr, NumPixels * sizeof(uint16));

    UE_LOG(LogTemp, Log, TEXT("ImportMap: Successfully imported %d x %d heightmap."), Width, Height);

    // 필요하다면 해상도도 저장
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

	// PNG 디코딩
	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

	if (!ImageWrapper.IsValid() || !ImageWrapper->SetCompressed(FileData.GetData(), FileData.Num()))
	{
		UE_LOG(LogTemp, Error, TEXT("ImportTextureFromPNG: Failed to decode PNG."));
		return nullptr;
	}

	const int32 Width = ImageWrapper->GetWidth();
	const int32 Height = ImageWrapper->GetHeight();

	// RGBA 8비트 포맷으로 불러옴
	TArray<uint8> RawData;
	if (!ImageWrapper->GetRaw(ERGBFormat::RGBA, 8, RawData))
	{
		UE_LOG(LogTemp, Error, TEXT("ImportTextureFromPNG: Failed to extract raw image data."));
		return nullptr;
	}

	const FString AssetName = FPaths::GetBaseFilename(FileName);  // "MyMap.png" → "MyMap"
	
	// 1. 패키지 생성
	const FString PackageName = FPaths::Combine(ContentDir, SubDir, AssetName);
	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create package for texture"));
		return nullptr;
	}

	// 2. UTexture2D 생성
	UTexture2D* Texture = NewObject<UTexture2D>(Package, *AssetName, RF_Public | RF_Standalone);
	FTexturePlatformData* PlatformData = new FTexturePlatformData();
	PlatformData->SizeX = Width;
	PlatformData->SizeY = Height;
	PlatformData->PixelFormat = PF_B8G8R8A8;

	// Mip 생성 및 데이터 설정
	FTexture2DMipMap* Mip = new FTexture2DMipMap();
	Mip->SizeX = Width;
	Mip->SizeY = Height;
	Mip->BulkData.Lock(LOCK_READ_WRITE);
	void* Data = Mip->BulkData.Realloc(RawData.Num());
	FMemory::Memcpy(Data, RawData.GetData(), RawData.Num());
	Mip->BulkData.Unlock();

	// PlatformData에 추가
	PlatformData->Mips.Add(Mip);
	// 공식 API로 설정
	Texture->SetPlatformData(PlatformData);

	// 3. 에셋 등록 및 저장
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
	// 1. 디렉토리 및 전체 경로 구성 (가독성이 뛰어난 방식)
	const FString DirectoryPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Maps/"));
	const FString FullPath = FPaths::Combine(DirectoryPath, FileName);

	UE_LOG(LogTemp, Log, TEXT("Target Directory: %s"), *DirectoryPath);
	UE_LOG(LogTemp, Log, TEXT("Target Full Path: %s"), *FullPath);

	// 2. 일관성을 위해 IPlatformFile 인터페이스로 디렉토리 확인 및 생성
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	if (!PlatformFile.DirectoryExists(*DirectoryPath)) // FPaths 대신 PlatformFile 사용
	{
		UE_LOG(LogTemp, Log, TEXT("Directory does not exist. Creating directory..."));
		if (!PlatformFile.CreateDirectoryTree(*DirectoryPath))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create directory: %s"), *DirectoryPath);
			return false; // 또는 다른 에러 처리
		}
	}

	// 2. 이미지 래퍼 생성
	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

	if (!ImageWrapper.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create Image Wrapper."));
		return false;
	}

	// 3. 데이터 설정
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
	// 1. 디렉토리 및 전체 경로 구성 (가독성이 뛰어난 방식)
	const FString DirectoryPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Maps"));
	const FString FullPath = FPaths::Combine(DirectoryPath, FileName);

	UE_LOG(LogTemp, Log, TEXT("Target Directory: %s"), *DirectoryPath);
	UE_LOG(LogTemp, Log, TEXT("Target Full Path: %s"), *FullPath);

	// 2. 일관성을 위해 IPlatformFile 인터페이스로 디렉토리 확인 및 생성
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	if (!PlatformFile.DirectoryExists(*DirectoryPath)) // FPaths 대신 PlatformFile 사용
	{
		UE_LOG(LogTemp, Log, TEXT("Directory does not exist. Creating directory..."));
		if (!PlatformFile.CreateDirectoryTree(*DirectoryPath))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create directory: %s"), *DirectoryPath);
			return false; // 또는 다른 에러 처리
		}
	}
	
	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

	if (!ImageWrapper.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create Image Wrapper for color map."));
		return false;
	}

	// 3. 데이터 설정
	const int32 Width = Resolution.X;
	const int32 Height = Resolution.Y;

	const int32 ExpectedSize = Width * Height;
	if (InMap.Num() != ExpectedSize)
	{
		UE_LOG(LogTemp, Error, TEXT("InMap size (%d) does not match resolution (%d x %d = %d)"), InMap.Num(), Width, Height, ExpectedSize);
		return false;
	}

	// BGRA8 형식으로 원본 데이터를 설정합니다. FColor는 내부적으로 BGRA 순서입니다.
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
