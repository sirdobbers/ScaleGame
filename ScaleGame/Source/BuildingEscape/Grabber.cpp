// Fill out your copyright notice in the Description page of Project Settings.

#include "BuildingEscape.h"
#include "Grabber.h"

#define OUT

// Sets default values for this component's properties
UGrabber::UGrabber()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
}

// Called when the game starts
void UGrabber::BeginPlay()
{
	Super::BeginPlay();
	FindComponents();
}

// Called every frame
void UGrabber::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	GetPlayerTranslations();

	if (IsGrabbing) {
		FVector GrabLocation = GetGrabLocation();
		GrabbedObject->SetActorLocation(GrabLocation + CurrGrabOffset);
		//DrawDebugSphere(GetWorld(),GrabLocation,5,32,FColor(255, 0, 0));
	}
}

// finds and sets up physics handle and inputcomponent
void UGrabber::FindComponents() {
	InputComponent = GetOwner()->FindComponentByClass<UInputComponent>();

	if (InputComponent) {
		UE_LOG(LogTemp, Warning, TEXT("%s: Input component found! %s"), *(GetOwner()->GetName()), *(InputComponent->GetName()));
		InputComponent->BindAction("Grab", IE_Pressed, this, &UGrabber::Grab);
		InputComponent->BindAction("Grab", IE_Released, this, &UGrabber::Release);
		InputComponent->BindAction("ScrollUp", IE_Pressed, this, &UGrabber::ScrollUp);
		InputComponent->BindAction("ScrollDown", IE_Pressed, this, &UGrabber::ScrollDown);
	}
	else { UE_LOG(LogTemp, Error, TEXT("%s missing Input Component."), *(GetOwner()->GetName())); }
}


//grab object when grab is pressed, attaches it to physics handle
void UGrabber::Grab()
{
	FHitResult HitResult;
	if (GetBodyRayCast(HitResult)){
		GrabbedObject = HitResult.GetActor();
		InitialRotation = GrabbedObject->GetActorRotation();
		IsGrabbing = true;
		Event1.Broadcast();

		UStaticMeshComponent* Mesh = GrabbedObject->FindComponentByClass<UStaticMeshComponent>();
		CurrMass = Mesh->GetMass();
		UE_LOG(LogTemp, Warning, TEXT("Initial mass: %s"), *FString::SanitizeFloat(CurrMass));

		float ScaleRatio;
		FVector OriginLocation = HitResult.Location;
		FVector TeleportLocation = PlayerLocation + PlayerRotation.Vector() * TeleGrabDistance;
		TeleportAndScale(HitResult.GetActor(), OriginLocation, TeleportLocation, ScaleRatio);

		Mesh->SetCollisionProfileName(TEXT("OverlapAll"));
		Mesh->SetEnableGravity(false);
		/*
		Mesh->BodyInstance.bLockXRotation = true;
		Mesh->BodyInstance.bLockYRotation = true;
		Mesh->BodyInstance.bLockZRotation = true;
		*/
	}
}

void UGrabber::Release()
{
	ManualCount = 0;
	IsGrabbing = false;
	if (GrabbedObject) {
		Event2.Broadcast();
		SetFinalDestination();
		
		UStaticMeshComponent* Mesh = GrabbedObject->FindComponentByClass<UStaticMeshComponent>();
		Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
		Mesh->SetEnableGravity(true);
		Mesh->SetMassOverrideInKg(NAME_None, CurrMass, true);
		UE_LOG(LogTemp, Warning, TEXT("Initial mass: %s"), *FString::SanitizeFloat(CurrMass));
		/*
		Mesh->BodyInstance.bLockXRotation = false;
		Mesh->BodyInstance.bLockYRotation = false;
		Mesh->BodyInstance.bLockZRotation = false;
		*/
	}
	GrabbedObject = nullptr;
}

FVector UGrabber::GetGrabLocation() {
	return PlayerLocation + PlayerRotation.Vector()*GrabDistance;
}

void UGrabber::GetPlayerTranslations() {
	GetWorld()->GetFirstPlayerController()->GetPlayerViewPoint(OUT PlayerLocation, OUT PlayerRotation);
}

bool UGrabber::GetWorldRayCast(FHitResult& HitResult){
	FVector AimPos = PlayerRotation.Vector() * 1000000;
	FCollisionQueryParams TraceParams(FName(TEXT("")), false, GetOwner()); 
	GetWorld()->LineTraceSingleByObjectType(OUT HitResult, PlayerLocation, AimPos,
		FCollisionObjectQueryParams(ECollisionChannel::ECC_WorldStatic), TraceParams);
	if (HitResult.GetActor()) {
		return true;
	}
	return false;
}

bool UGrabber::GetBodyRayCast(FHitResult& HitResult) {
	FVector AimPos = PlayerRotation.Vector() * 1000000;
	FCollisionQueryParams TraceParams(FName(TEXT("")), false, GetOwner()); 
	GetWorld()->LineTraceSingleByObjectType(OUT HitResult, PlayerLocation, AimPos,
		FCollisionObjectQueryParams(ECollisionChannel::ECC_PhysicsBody), TraceParams);
	if (HitResult.GetActor()) {
		return true;
	}
	return false;
}

void UGrabber::TeleportAndScale(AActor* Object, FVector OriginLocation, FVector TeleportLocation, float& ScaleRatio) {
	if (Object) {
		float TeleportDistance = (TeleportLocation - PlayerLocation).Size();
		float OriginDistance = (OriginLocation - PlayerLocation).Size();

		ScaleRatio = (TeleportDistance / OriginDistance);
		FVector NewScale = Object->GetActorScale3D() * ScaleRatio;
		UStaticMeshComponent* Mesh = Object->FindComponentByClass<UStaticMeshComponent>();
		Mesh->SetWorldScale3D(NewScale);

		FVector Offset = Object->GetActorLocation() - OriginLocation;
		FVector NewOffset = Offset*ScaleRatio;
		FVector NewLocation = TeleportLocation + NewOffset;
		Object->SetActorLocation(NewLocation);

		CurrGrabOffset = NewOffset;
		CurrMass = CurrMass*ScaleRatio;
		UE_LOG(LogTemp, Warning, TEXT("Initial mass: %s"), *FString::SanitizeFloat(CurrMass));
		GrabDistance = (TeleportLocation-PlayerLocation).Size();
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("TeleScale Couldn't find an actor."));
	}
}


void UGrabber::SetFinalDestination() {
	TArray<AActor*> OverlappingActors;
	UStaticMeshComponent* Mesh = GrabbedObject->FindComponentByClass<UStaticMeshComponent>();
	Mesh->GetOverlappingActors(OverlappingActors);

	float ScaleRatio;
	float Count = 0;

	while (!OverlappingActors.Num()) {
		Count += 10;
		if (Count > 2000) {break;}

		FVector OriginLocation = GrabbedObject->GetActorLocation() - CurrGrabOffset;
		FVector TeleportLocation = PlayerLocation + PlayerRotation.Vector()*(TeleGrabDistance + Count);
		TeleportAndScale(GrabbedObject, OriginLocation, TeleportLocation, ScaleRatio);
		Mesh->GetOverlappingActors(OverlappingActors);
	}
	if (Count > TeleGrabDistance+10) {
		Count -= 10;
	}
	FVector OriginLocation = GrabbedObject->GetActorLocation() - CurrGrabOffset;
	FVector TeleportLocation = PlayerLocation + PlayerRotation.Vector()*(TeleGrabDistance + Count);
	TeleportAndScale(GrabbedObject, OriginLocation, TeleportLocation, ScaleRatio);
}

//Manual Debugging
void UGrabber::ScrollUp(){
	float ScaleRatio;
	ManualCount += 50;
	FVector OriginLocation = GrabbedObject->GetActorLocation() - CurrGrabOffset;
	FVector TeleportLocation = PlayerLocation + PlayerRotation.Vector()*(10 + ManualCount);
	TeleportAndScale(GrabbedObject, OriginLocation, TeleportLocation, ScaleRatio);
}

void UGrabber::ScrollDown() {
	float ScaleRatio;
	ManualCount -= 50;
	FVector OriginLocation = GrabbedObject->GetActorLocation() - CurrGrabOffset;
	FVector TeleportLocation = PlayerLocation + PlayerRotation.Vector()*(10 + ManualCount);
	TeleportAndScale(GrabbedObject, OriginLocation, TeleportLocation, ScaleRatio);
}

UStaticMeshComponent* UGrabber::GetGrabbedStaticMesh() {
	return GrabbedObject->FindComponentByClass<UStaticMeshComponent>();
}