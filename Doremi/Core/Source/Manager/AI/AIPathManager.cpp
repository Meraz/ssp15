/// Project specific
#include <Manager/AI/AIPathManager.hpp>
#include <EntityComponent/EntityHandler.hpp>
#include <PlayerHandler.hpp>
#include <Helper/ProximityChecker.hpp>
#include <EntityComponent/Components/HealthComponent.hpp>
#include <EntityComponent/Components/RigidBodyComponent.hpp>
#include <EntityComponent/Components/TransformComponent.hpp>
#include <EntityComponent/Components/PotentialFieldComponent.hpp>
#include <EntityComponent/Components/MovementComponent.hpp>
#include <EntityComponent/Components/AIGroupComponent.hpp>
#include <Doremi/Core/Include/PotentialFieldGridCreator.hpp>
#include <EntityComponent/Components/PhysicsMaterialComponent.hpp>
// Events
#include <EventHandler/EventHandler.hpp>
#include <EventHandler/Events/SpecialEntityCreatedEvent.hpp>
#include <EventHandler/Events/PlayerCreationEvent.hpp>
// Force Equations
#include <AIHelper/ForceImpactFunctions.hpp>
// Engine
#include <DoremiEngine/Physics/Include/PhysicsModule.hpp>
#include <DoremiEngine/Physics/Include/RigidBodyManager.hpp>
#include <DoremiEngine/Physics/Include/PhysicsMaterialManager.hpp>
#include <DoremiEngine/Physics/Include/RayCastManager.hpp>
#include <DoremiEngine/AI/Include/Interface/PotentialField/PotentialFieldActor.hpp>
#include <DoremiEngine/AI/Include/Interface/PotentialField/PotentialField.hpp>
#include <DoremiEngine/AI/Include/Interface/PotentialField/PotentialGroup.hpp>
#include <DoremiEngine/AI/Include/Interface/SubModule/PotentialFieldSubModule.hpp>
#include <DoremiEngine/AI/Include/AIModule.hpp>

// Standard
#include <iostream>
#include <DirectXMath.h>
#include <set>
#include <functional>

using namespace std;

namespace Doremi
{
    namespace Core
    {
        AIPathManager::AIPathManager(const DoremiEngine::Core::SharedContext& p_sharedContext) : Manager(p_sharedContext, "AIPathManager")
        {
            // TODOKO do this in a better place, might not work to have here in the future
            m_field =
                m_sharedContext.GetAIModule().GetPotentialFieldSubModule().CreateNewField(250, 230, 100, 100,
                                                                                          XMFLOAT3(-60, 3.0f, -40)); // Fits for first platform
            m_topField =
                m_sharedContext.GetAIModule().GetPotentialFieldSubModule().CreateNewField(350, 500, 50, 50,
                    XMFLOAT3(-280, 150.0f, -85)); // Fits for top platform
            EventHandler::GetInstance()->Subscribe(EventType::AiGroupActorCreation, this);
            EventHandler::GetInstance()->Subscribe(EventType::PotentialFieldActorCreation, this);
            EventHandler::GetInstance()->Subscribe(EventType::PlayerCreation, this);
            //////////////////////// Fixa PotF�lt

            // Testar TODOEA
            Core::EntityHandler& t_entityHandler = Core::EntityHandler::GetInstance();
            PotentialFieldGridCreator t_potentialFieldGridCreator = PotentialFieldGridCreator(m_sharedContext);
            t_potentialFieldGridCreator.BuildGridUsingPhysicXAndGrid(m_field);
            std::cout << "Physical field 1 done";
            m_field->Update();
            std::cout << "Potential field 1 done";

            t_potentialFieldGridCreator.BuildGridUsingPhysicXAndGrid(m_topField);
            std::cout << "Physical field 2 done";
            m_topField->Update();
            std::cout << "Potential field 2 done";
            /// debugskit


            //&& render pos rigidbody, potentialfield
        }

        AIPathManager::~AIPathManager() {}


        void AIPathManager::Update(double p_dt)
        {
            size_t length = EntityHandler::GetInstance().GetLastEntityIndex();

            // TODOKO Test wall
            if(firstUpdate)
            {
                firstUpdate = false;
            }
            for(size_t i = 0; i < length; i++)
            {
                // Update actors position, should perhaps not be here...
                if(EntityHandler::GetInstance().HasComponents(i, (int)ComponentType::PotentialField | (int)ComponentType::Transform))
                { // This is so the player updates his position too...
                    DoremiEngine::AI::PotentialFieldActor* actor = EntityHandler::GetInstance().GetComponentFromStorage<PotentialFieldComponent>(i)->ChargedActor;
                    XMFLOAT3 pos = EntityHandler::GetInstance().GetComponentFromStorage<TransformComponent>(i)->position;                    actor->SetPosition(pos);
                }
                if(EntityHandler::GetInstance().HasComponents(i, (int)ComponentType::AIAgent | (int)ComponentType::Transform |
                                                                     (int)ComponentType::Movement | (int)ComponentType::AIGroup | (int)ComponentType::PotentialField))
                {
                    // Get the needed components
                    XMFLOAT2 desiredPos;
                    XMFLOAT3 unitPos = EntityHandler::GetInstance().GetComponentFromStorage<TransformComponent>(i)->position;
                    DoremiEngine::AI::PotentialGroup* group = EntityHandler::GetInstance().GetComponentFromStorage<AIGroupComponent>(i)->Group;
                    DoremiEngine::AI::PotentialFieldActor* currentActor = EntityHandler::GetInstance().GetComponentFromStorage<PotentialFieldComponent>(i)->ChargedActor;
                    DoremiEngine::AI::PotentialField* field = EntityHandler::GetInstance().GetComponentFromStorage<PotentialFieldComponent>(i)->Field;
                    // TODOEA BORDE SPARA UNDAN O INTE KOLLA X O Y EFTER VARANN

                    if (currentActor->GetPrevGridPos().x == field->WhatGridPosAmIOn(currentActor->GetPosition()).x && currentActor->GetPrevGridPos().y == field->WhatGridPosAmIOn(currentActor->GetPosition()).y)
                    {
                        // Remove the first in the list om vi skulle anv�nda oss av delta_T f�r att uppdatera trailen med hj�lp av den om n�gon st�r still.
                        // if we are still standing on the same quad as the last update we do nothing 
                        // TODOKO if we have been standing stil for 2 long something might be wrong, Force him to move!!!
                    }
                    else 
                    {                        
                        XMINT2 newPrevPos = field->WhatGridPosAmIOn(currentActor->GetPosition());
                        currentActor->SetPrevGridPosition(newPrevPos);
                        currentActor->UpdatePhermoneTrail(currentActor->GetPrevGridPos());
                    }
                    if(field != nullptr)
                    {
                        desiredPos = field->GetAttractionPosition(unitPos, currentActor, false);
                    }
                    XMFLOAT3 desiredPos3D = XMFLOAT3(desiredPos.x, unitPos.y, desiredPos.y); // The fields impact
                   // XMFLOAT3 groupImpact = group->GetForceDirection(unitPos, currentActor); // The groups impact
                   // XMVECTOR groupImpactVec = XMLoadFloat3(&groupImpact);
                    XMVECTOR desiredPosVec = XMLoadFloat3(&desiredPos3D);
                    XMVECTOR unitPosVec = XMLoadFloat3(&unitPos);
                    XMVECTOR dirVec = desiredPosVec - unitPosVec;
                    dirVec = XMVector3Normalize(dirVec);
                   // dirVec += groupImpactVec * 0.2f; // TODOKO remove this variable!! Its there to make the static field more influencial
                   //  dirVec = XMVector3Normalize(dirVec);
                    XMFLOAT3 direction;
                    XMStoreFloat3(&direction, dirVec * 0.2f); // TODOKO remove this hard coded shiat
                    MovementComponent* moveComp = EntityHandler::GetInstance().GetComponentFromStorage<MovementComponent>(i);
                    moveComp->movement = direction;
                }
            }
        }

        void AIPathManager::OnEvent(Event* p_event)
        {
            switch(p_event->eventType)
            {
                case Doremi::Core::EventType::PotentialFieldActorCreation:
                {
                    SpecialEntityCreatedEvent* realEvent = static_cast<SpecialEntityCreatedEvent*>(p_event);
                    if(EntityHandler::GetInstance().HasComponents(realEvent->entityID, (int)ComponentType::PotentialField)) // Make sure the entity
                    // contains the needed
                    // stuff
                    {
                        DoremiEngine::AI::PotentialFieldActor* actor =
                            EntityHandler::GetInstance().GetComponentFromStorage<PotentialFieldComponent>(realEvent->entityID)->ChargedActor;
                        m_field->AddActor(actor);
                    }
                    else
                    {
                        // TODOKO Log error
                    }
                    break;
                }
                case Doremi::Core::EventType::AiGroupActorCreation:
                {
                    SpecialEntityCreatedEvent* realEvent = static_cast<SpecialEntityCreatedEvent*>(p_event);
                    if(EntityHandler::GetInstance().HasComponents(realEvent->entityID, (int)ComponentType::PotentialField | (int)ComponentType::AIGroup)) // Make sure the entity contains the needed stuff
                    {
                        std::function<float(float, float, float)> chargeEquation = ForceEquations::Standard;
                        DoremiEngine::AI::PotentialFieldActor* actor =
                            EntityHandler::GetInstance().GetComponentFromStorage<PotentialFieldComponent>(realEvent->entityID)->ChargedActor;
                        DoremiEngine::AI::PotentialChargeInformation t_newSpecial =
                            DoremiEngine::AI::PotentialChargeInformation(-100, 10, true, false, true, true, DoremiEngine::AI::AIActorType::Player, chargeEquation);
                        DoremiEngine::AI::PotentialChargeInformation t_newSpecial2 =
                            DoremiEngine::AI::PotentialChargeInformation(0, 12, true, false, true, false, DoremiEngine::AI::AIActorType::Player, chargeEquation);
                        actor->AddPotentialVsOther(t_newSpecial);
                        actor->AddPotentialVsOther(t_newSpecial2);
                        DoremiEngine::AI::PotentialField* field;
                        DoremiEngine::AI::PotentialGroup* group =
                            EntityHandler::GetInstance().GetComponentFromStorage<AIGroupComponent>(realEvent->entityID)->Group;
                        if(group == nullptr)
                        {
                            // TODOKO Log error and what happens
                            group = m_sharedContext.GetAIModule().GetPotentialFieldSubModule().CreateNewPotentialGroup();
                        }
                        group->AddActor(actor);
                        // TODOKO should not be done here but in a more suitable event
                        field = m_sharedContext.GetAIModule().GetPotentialFieldSubModule().FindBestPotentialField(actor->GetPosition());
                        EntityHandler::GetInstance().GetComponentFromStorage<PotentialFieldComponent>(realEvent->entityID)->Field = field;
                        field->AddActor(actor);
                    }
                    else
                    {
                        // TODOKO Log error
                    }
                    break;
                }
                case Doremi::Core::EventType::PlayerCreation:
                {
                    PlayerCreationEvent* realEvent = static_cast<PlayerCreationEvent*>(p_event);
                    if(EntityHandler::GetInstance().HasComponents(realEvent->playerEntityID, (int)ComponentType::PotentialField)) // Make sure the
                    // entity contains
                    // the needed stuff
                    {
                        DoremiEngine::AI::PotentialFieldActor* actor =
                            EntityHandler::GetInstance().GetComponentFromStorage<PotentialFieldComponent>(realEvent->playerEntityID)->ChargedActor;
                        m_field->AddActor(actor);
                        m_topField->AddActor(actor);
                    }
                    else
                    {
                        // TODOKO log error
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }
}