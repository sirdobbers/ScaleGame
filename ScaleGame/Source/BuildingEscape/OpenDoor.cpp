// Fill out your copyright notice in the Description page of Project Settings.

#include "BuildingEscape.h"
#include "OpenDoor.h"


// Sets default values for this component's properties
UOpenDoor::UOpenDoor()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void UOpenDoor::BeginPlay()
{
	Super::BeginPlay();

	Owner = GetOwner();

	if (!PressurPlate) {
		UE_LOG(LogTemp, Error, TEXT("%s is missing a pressure plate."), *GetOwner()->GetName());
	}
}


// Called every frame
void UOpenDoor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//poll trigger voljume
	if (GetMassOnPlate() > 280.f) {
		OnOpen.Broadcast();
	}
	else {
		OnClose.Broadcast();
	}
}


//returns total mass on plate
float UOpenDoor::GetMassOnPlate() {
	float Mass = 0;

	if (!PressurPlate) { return Mass; }

	TArray<AActor*> OverlappingActors;
	PressurPlate->GetOverlappingActors(OverlappingActors);
	for (auto& Act : OverlappingActors) {
		Mass += Act->FindComponentByClass<UPrimitiveComponent>()->GetMass();
		//UE_LOG(LogTemp, Warning, TEXT("%s is on plate. Total Mass: %s"), *Act->GetName(),*FString::SanitizeFloat(Mass));
	}
	return Mass;
}