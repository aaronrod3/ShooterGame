// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterGameOnlineSubsystem.h"
#include "Online/CoreOnline.h"
#include "Online/OnlineResult.h"
#include "Online/OnlineAsyncOpHandle.h"
#include "Online/OnlineError.h"
#include "Online/OnlineServices.h"
#include "Online/Auth.h"
#include "Online/TitleFile.h"


DEFINE_LOG_CATEGORY(LogShooterGameOnlineSubsystem);


/*** Subsystem only created on clients or standalone users, not servers ***/
/*** This function often used to limit creation of subsytems to a server or client ***/

// Bool whether or not to create this subsystem 
bool UShooterGameOnlineSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_SERVER
	return false;
#else
	return Super::ShouldCreateSubsystem(Outer);
#endif
}

/*** Initialize called after the Game instance is initialized ***/
void UShooterGameOnlineSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UE_LOG(LogShooterGameOnlineSubsystem, Log, TEXT("Survival Game Online Subsystem Initialized"));
	Super::Initialize(Collection);
	
	// Initialize online services
	InitializeOnlineServices();
}

/*** De-Initialize called after the Game instance is de-initialized/shutdown ***/
void UShooterGameOnlineSubsystem::Deinitialize()
{
	UE_LOG(LogShooterGameOnlineSubsystem, Log, TEXT("Survival Game Online Subsystem De-Initialized"));
	
	// Unbind event handles and reset struct info
	OnlineServicesInfoInternal->Reset();
	
	// Deinitialize parent class
	Super::Deinitialize();
}

/*** Initialize the Online Services ***/
void UShooterGameOnlineSubsystem::InitializeOnlineServices()
{
	using namespace UE::Online;
	
	OnlineServicesInfoInternal = new FOnlineServicesInfo();
	
	// Initialize services pointer
	OnlineServicesInfoInternal->OnlineServices = GetServices();
	if (!OnlineServicesInfoInternal->OnlineServices.IsValid())
	{
		UE_LOG(LogShooterGameOnlineSubsystem, Error, TEXT("Online Services not available - check plugin config!"));
		return;
	}
	
	// Verify services type
	OnlineServicesInfoInternal->OnlineServicesType = OnlineServicesInfoInternal->OnlineServices->GetServicesProvider();
	if (OnlineServicesInfoInternal->OnlineServices.IsValid())
	{
		// Initialize services interface
		OnlineServicesInfoInternal->AuthInterface = OnlineServicesInfoInternal->OnlineServices->GetAuthInterface();
		check(OnlineServicesInfoInternal->AuthInterface.IsValid());
		
		OnlineServicesInfoInternal->TitleFileInterface = OnlineServicesInfoInternal->OnlineServices->GetTitleFileInterface();
		check(OnlineServicesInfoInternal->TitleFileInterface.IsValid());
	}
	else
	{
		UE_LOG(LogShooterGameOnlineSubsystem, Error, TEXT("Failed to initialize Online Services"));
	}
	
}

/*** Register the user with local registry OnlineUsersInfo ***/
void UShooterGameOnlineSubsystem::RegisterLocalOnlineUser(FPlatformUserId PlatformUserId)
{
	using namespace UE::Online;
	
	FAuthGetLocalOnlineUserByPlatformUserId::Params GetUserParams;
	GetUserParams.PlatformUserId = PlatformUserId;
	
	if (OnlineServicesInfoInternal && OnlineServicesInfoInternal->AuthInterface.IsValid())
	{
		TOnlineResult<FAuthGetLocalOnlineUserByPlatformUserId> AuthGetResult = OnlineServicesInfoInternal->AuthInterface->GetLocalOnlineUserByPlatformUserId(MoveTemp(GetUserParams));
		
		if (AuthGetResult.IsOk())
		{
			FAuthGetLocalOnlineUserByPlatformUserId::Result& LocalOnlineUser = AuthGetResult.GetOkValue();
			TSharedRef<FAccountInfo> UserAccountInfo = LocalOnlineUser.AccountInfo;
			FAccountInfo UserAccountInfoContent= *UserAccountInfo;
			
			if (!OnlineUserInfos.Contains(UserAccountInfoContent.PlatformUserId))
			{
				UOnlineUserInfo* NewUser = CreateAndRegisterUserInfo(UserAccountInfoContent.AccountId.GetHandle(), PlatformUserId, UserAccountInfoContent.AccountId, UserAccountInfoContent.AccountId.GetOnlineServicesType());
				UE_LOG(LogShooterGameOnlineSubsystem, Log, TEXT("Local user registered: %s"), *NewUser->DebugInfoToString());
			}
			else
			{
				UE_LOG(LogShooterGameOnlineSubsystem, Warning, TEXT("Local user already registered: %s"), *OnlineUserInfos[UserAccountInfoContent.PlatformUserId]->DebugInfoToString());
			}
		}
		else
		{
			FOnlineError ErrorResult = AuthGetResult.GetErrorValue();
			UE_LOG(LogShooterGameOnlineSubsystem, Error, TEXT("Get Local User Error: %s"), *ErrorResult.GetLogString());
		}
	}
	else
	{
		UE_LOG(LogShooterGameOnlineSubsystem, Error, TEXT("Auth Interface Pointer invalid"));
	}
}

/*** Create Online User Info and Create and Register User Info ***/
TObjectPtr<UOnlineUserInfo> UShooterGameOnlineSubsystem::CreateOnlineUserInfo(int32 LocalUserIndex, FPlatformUserId PlatformUserId, UE::Online::FAccountId AccountId, UE::Online::EOnlineServices Services)
{
	TObjectPtr<UOnlineUserInfo> NewUser = NewObject<UOnlineUserInfo>(this);
	NewUser->LocalUserIndex = LocalUserIndex;
	NewUser->PlatformUserId = PlatformUserId;
	NewUser->AccountId = AccountId;
	NewUser->Services = Services;
	return NewUser;
}

TObjectPtr<UOnlineUserInfo> UShooterGameOnlineSubsystem::CreateAndRegisterUserInfo(int32 LocalUserIndex, FPlatformUserId PlatformUserId, UE::Online::FAccountId AccountId, UE::Online::EOnlineServices Services)
{
	TObjectPtr<UOnlineUserInfo> NewUser = CreateOnlineUserInfo(LocalUserIndex, PlatformUserId, AccountId, Services);
	OnlineUserInfos.Add(PlatformUserId, NewUser);
	return NewUser;
}

/*** Get Online User Info for provided platform user id ***/
TObjectPtr<class UOnlineUserInfo> UShooterGameOnlineSubsystem::GetOnlineUserInfo(FPlatformUserId PlatformUserId)
{
	TObjectPtr<UOnlineUserInfo> OnlineUser = nullptr;
	if (OnlineUserInfos.Contains(PlatformUserId))
	{
		OnlineUser = *OnlineUserInfos.Find(PlatformUserId);
	}
	else
	{
		UE_LOG(LogShooterGameOnlineSubsystem, Error, TEXT("Online User Info not found for Platform User ID: %d"), PlatformUserId.GetInternalId());
	}
	return OnlineUser;
}

/*** EnumerateFiles, GetEnumerateFiles, and ReadFile. ***/
void UShooterGameOnlineSubsystem::RetrieveTitleFile(FString Filename, FPlatformUserId PlatformUserId)
{
	using namespace UE::Online;
	
	FTitleFileEnumerateFiles::Params EnumerateParams;
	FAccountId LocalAccountId;
	TObjectPtr<UOnlineUserInfo> OnlineUser;
	if (OnlineUserInfos.Contains(PlatformUserId))
	{
		OnlineUser = *OnlineUserInfos.Find(PlatformUserId);
		LocalAccountId = OnlineUser->AccountId;
		EnumerateParams.LocalAccountId = LocalAccountId;
		
		if (OnlineServicesInfoInternal && OnlineServicesInfoInternal->TitleFileInterface.IsValid())
		{
			OnlineServicesInfoInternal->TitleFileInterface->EnumerateFiles(MoveTemp(EnumerateParams)).OnComplete(this, &ThisClass::HandleEnumerateFiles, OnlineUser, Filename);
		}
		else
		{
			UE_LOG(LogShooterGameOnlineSubsystem, Error, TEXT("Title File Interface Pointer invalid"));
		}
	}
	else
	{
		UE_LOG(LogShooterGameOnlineSubsystem, Error, TEXT("Online User Info not found for Platform User ID: %d"), PlatformUserId.GetInternalId());
	}
}

/*** Handle the asynchronous EnumerateFiles function ***/
void UShooterGameOnlineSubsystem::HandleEnumerateFiles(const UE::Online::TOnlineResult<UE::Online::FTitleFileEnumerateFiles>& EnumerateFilesResult, TObjectPtr<UOnlineUserInfo> OnlineUser, FString Filename)
{
	using namespace UE::Online;
	
	if (EnumerateFilesResult.IsOk())
	{
		FTitleFileGetEnumeratedFiles::Params GetParams;
		GetParams.LocalAccountId = OnlineUser->AccountId;
		
		TOnlineResult<FTitleFileGetEnumeratedFiles> GetResult = OnlineServicesInfoInternal->TitleFileInterface->GetEnumeratedFiles(MoveTemp(GetParams));
		
		if (GetResult.IsOk())
		{
			FTitleFileGetEnumeratedFiles::Result& CachedFiles = GetResult.GetOkValue();
			int32 FileIndex = CachedFiles.Filenames.Find(Filename);
			
			if (FileIndex == INDEX_NONE)
			{
				UE_LOG(LogShooterGameOnlineSubsystem, Error, TEXT("Title File \"%s\" not found!"), *Filename);
			}
			else
			{
				FTitleFileReadFile::Params ReadParams;
				ReadParams.LocalAccountId = OnlineUser->AccountId;
				ReadParams.Filename = Filename;
				
				OnlineServicesInfoInternal->TitleFileInterface->ReadFile(MoveTemp(ReadParams)).OnComplete(this, &ThisClass::HandleReadFile, Filename);
			}
		}
		else
		{
			UE_LOG(LogShooterGameOnlineSubsystem, Error, TEXT("Get Title File Error: %s"), *GetResult.GetErrorValue().GetLogString());
		}
	}
	else
	{
		UE_LOG(LogShooterGameOnlineSubsystem, Error, TEXT("Enumerate Title File Error: %s"), *EnumerateFilesResult.GetErrorValue().GetLogString());
	}
}

/*** Hnadle the asynchronous ReadFile function ***/
void UShooterGameOnlineSubsystem::HandleReadFile(const UE::Online::TOnlineResult<UE::Online::FTitleFileReadFile>& ReadFileResult, FString Filename)
{
	using namespace UE::Online;
	
	if (ReadFileResult.IsOk())
	{
		const FTitleFileReadFile::Result& ReadFileResultValue = ReadFileResult.GetOkValue();
		OnlineServicesInfoInternal->TitleFileContents = *ReadFileResultValue.FileContents;
	}
	else
	{
		UE_LOG(LogShooterGameOnlineSubsystem, Error, TEXT("Read Title File Error: %s"), *ReadFileResult.GetErrorValue().GetLogString());
	}
}

/*** Obtain title file and read ***/
FString UShooterGameOnlineSubsystem::ReadTitleFile(FString Filename, FPlatformUserId PlatformUserId)
{
	using namespace UE::Online;
	
	RetrieveTitleFile(Filename, PlatformUserId);
	FTitleFileContents FileContents = OnlineServicesInfoInternal->TitleFileContents;
	FString FileString = FString(FileContents.Num(), UTF8_TO_TCHAR(FileContents.GetData()));
	
	UE_LOG(LogShooterGameOnlineSubsystem, Log, TEXT("Reading title file: %s"), *Filename);
	return FileString;
}

/*** Constructor for UOnlineUserInfo ***/
UOnlineUserInfo::UOnlineUserInfo()
{
	
}


/*** Return debug string for UOnlineUserInfo ***/
const FString UOnlineUserInfo::DebugInfoToString()
{
	int32 UserIndex = this->LocalUserIndex;
	int32 PlatformId = this->PlatformUserId;
	TArray<FStringFormatArg> FormatArgs;
	FormatArgs.Add(FStringFormatArg(UserIndex));
	FormatArgs.Add(FStringFormatArg(PlatformId));
	return FString::Format(TEXT("Local User Index: {0}, Platform User ID: {1}"), FormatArgs);
}