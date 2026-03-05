// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Online/OnlineServices.h"
#include "Online/OnlineAsyncOpHandle.h"
#include "Online/TitleFile.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ShooterGameOnlineSubsystem.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(LogShooterGameOnlineSubsystem, Log, All);

UCLASS()
class SHOOTERGAME_API UShooterGameOnlineSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	
		/***  Subsystem Lifetime  ***/
	
	// called to determine whether the subsystem should be created
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	
	// Called to initialize Game Instance Subsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	// called to deinitialize Game Instance Subsystem
	virtual  void Deinitialize() override;
	
		/***  User Registration  ***/
	
	// called to register local online user with the Game Instance Subsystem
	void RegisterLocalOnlineUser(FPlatformUserId PlatformUserId);
	TObjectPtr<class UOnlineUserInfo> GetOnlineUserInfo(FPlatformUserId PlatformUserId);
	
	// Title file read
	FString ReadTitleFile(FString Filename, FPlatformUserId PlatformUserId);
	
protected:
	
	struct FOnlineServicesInfo
	{
		// Online services pointer
		UE::Online::IOnlineServicesPtr OnlineServices = nullptr;
		
		// Interface pointers
		UE::Online::IAuthPtr AuthInterface = nullptr;
		UE::Online::ITitleFilePtr TitleFileInterface = nullptr;
		
		// Online Interfaces Implementation
		UE::Online::EOnlineServices OnlineServicesType = UE::Online::EOnlineServices::None;
		
		// Title File content
		UE::Online::FTitleFileContents TitleFileContents;
		
		// Reset struct to initial settings
		void Reset()
		{
			OnlineServices.Reset();
			AuthInterface.Reset();
			TitleFileInterface.Reset();
			OnlineServicesType = UE::Online::EOnlineServices::None;
		}
	};
	
	/*** Online Services Init ***/
	
	// Pointer to an internal struct containing relevant online services pointers
	FOnlineServicesInfo* OnlineServicesInfoInternal = nullptr;
	
	// called to initialize online services and interface pointers
	void InitializeOnlineServices();
	
	
	/*** Title File ***/
	
	// Called to retrieve title file from online services
	void RetrieveTitleFile(FString Filename, FPlatformUserId PlatformUserId);
	
	
	/*** Events ***/
	
	// Called to handle the EnumerateFiles async event
	void HandleEnumerateFiles(const UE::Online::TOnlineResult<UE::Online::FTitleFileEnumerateFiles>& EnumerateFilesResult, TObjectPtr<UOnlineUserInfo> OnlineUser, FString Filename);
	
	// Called to handle ReadFile async event
	void HandleReadFile(const UE::Online::TOnlineResult<UE::Online::FTitleFileReadFile>& ReadFileResult, FString Filename);
	
	
	/*** Online User Information ***/
	
	// Called to create a UOnlineUserInfo object for this user
	TObjectPtr<UOnlineUserInfo> CreateOnlineUserInfo(
		int32 LocalUserIndex,
		FPlatformUserId PlatformUserId,
		UE::Online::FAccountId AccountId,
		UE::Online::EOnlineServices Services
	);
	
	// Called to register the user with the OnlineUserInfos map and user after creation with CreateOnlineUserInfo
	TObjectPtr<UOnlineUserInfo> CreateAndRegisterUserInfo(
		int32 LocalUserIndex,
		FPlatformUserId PlatformUserId,
		UE::Online::FAccountId AccountId,
		UE::Online::EOnlineServices Services
	);
	
	// Information about each local user
	TMap<FPlatformUserId, TObjectPtr<UOnlineUserInfo>> OnlineUserInfos;
	
	// Friend the UOnlineUserInfo class to access it
	friend UOnlineUserInfo;
};


UCLASS()
class SHOOTERGAME_API UOnlineUserInfo : public UObject
{
	GENERATED_BODY()
	
public:
	UOnlineUserInfo();
	
	/*** Online User Fields ***/
	int32 LocalUserIndex = -1;
	FPlatformUserId PlatformUserId;
	UE::Online::FAccountId AccountId;
	UE::Online::EOnlineServices Services = UE::Online::EOnlineServices::None;
	
	/*** Online User Logging/Debugging Functions ***/
	
	// Called to obtain OnlineUserInfo as as string
	const FString DebugInfoToString();
	
	friend UShooterGameOnlineSubsystem;
};