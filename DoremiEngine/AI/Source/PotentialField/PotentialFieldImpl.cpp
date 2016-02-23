#include <Internal/PotentialField/PotentialFieldImpl.hpp>
#include <Internal/SubModule/PotentialFieldSubModuleImpl.hpp>
#include <Internal/HelperFunctions.hpp>

// Config module
#include <DoremiEngine/Configuration/Include/ConfigurationModule.hpp>

#include <iostream>


namespace DoremiEngine
{
    namespace AI
    {
        PotentialFieldImpl::PotentialFieldImpl(AIContext& p_aiContext) : m_phermoneEffect(15), m_context(p_aiContext)
        {
            m_jumpDistance = m_context.config.GetAllConfigurationValues().AIJumpDistance;
        }
        PotentialFieldImpl::~PotentialFieldImpl() {}
        void PotentialFieldImpl::SetGrid(PotentialFieldGridPoint* p_grid)
        {
            // m_grid = p_grid;
            m_grid = p_grid;
        }
        void PotentialFieldImpl::Update()
        {
            // TODOKO optimize!!!! threads would be awesome here...
            // Lets all the actors update all the gridpoints in the potentialfield using the distance and charge
            using namespace DirectX;

            for(size_t x = 0; x < m_numberOfQuadsWidth; x++)
            {
                for(size_t y = 0; y < m_numberOfQuadsHeight; y++)
                {
                    XMFLOAT2 quadPos = GetGridQuadPosition(x, y);
                    float totalCharge = 0;
                    for(auto actor : m_staticActors)
                    {
                        XMINT2 myQuad = XMINT2(x, y);
                        XMINT2 closestQuad = actor->GetClosestOccupied(myQuad);
                        if(myQuad.x == closestQuad.x && myQuad.y == closestQuad.y)
                        {
                            // Same quad
                            m_grid[x + m_numberOfQuadsWidth * y].occupied = true;
                            // std::cout << "x " << x << " y " << y << std::endl;
                        }
                        XMFLOAT2 actorQuadPosition = GetGridQuadPosition(closestQuad.x, closestQuad.y);

                        float actorCharge = actor->GetCharge();
                        float actorRange = actor->GetRange();
                        // Calculate charge
                        XMVECTOR actorQuadPosVec = XMLoadFloat2(&actorQuadPosition);
                        XMVECTOR quadPosVec = XMLoadFloat2(&quadPos);

                        XMVECTOR distance = actorQuadPosVec - quadPosVec;
                        float dist = *XMVector2Length(distance).m128_f32;
                        if(dist < actorRange)
                        {
                            float force = actorCharge * std::fmaxf(1.0f - dist / actorRange, 0.0f);
                            totalCharge += force;
                        }
                    }
                    m_grid[x + m_numberOfQuadsWidth * y].charge = totalCharge;
                }
            }
            for(size_t x = 0; x < m_numberOfQuadsWidth; x++)
            {
                for(size_t y = 0; y < m_numberOfQuadsHeight; y++)
                {
                    if(m_grid[x + m_numberOfQuadsWidth * y].occupied == true)
                    {
                        std::cout << "X";
                    }
                    else
                    {
                        std::cout << " ";
                    }
                }
                std::cout << std::endl;
            }
        }
        void PotentialFieldImpl::AddActor(PotentialFieldActor* p_newActor)
        {
            if(p_newActor->IsStatic() && m_staticActors.find(p_newActor) == m_staticActors.end())
            {
                // The actor is static and not in list
                m_staticActors.insert(p_newActor);
            }
            else if(!p_newActor->IsStatic())
            {
                // TODOKO might need to check if it's in the list
                m_dynamicActors.push_back(p_newActor);
            }
        }
        void PotentialFieldImpl::RemoveActor(PotentialFieldActor* p_newActor)
        {
            if(m_staticActors.count(p_newActor) != 0)
            {
                m_staticActors.erase(p_newActor);
            }
            else
            {
                size_t length = m_dynamicActors.size();
                for (size_t i = 0; i < length; i++)
                {
                    if (m_dynamicActors[i] == p_newActor)
                    {
                        m_dynamicActors.erase(m_dynamicActors.begin() + i);
                        length = m_dynamicActors.size();
                    }
                }
            }
        }
        DirectX::XMINT2 PotentialFieldImpl::WhatGridPosAmIOn(const DirectX::XMFLOAT3& p_unitPosition)
        {
            using namespace DirectX;
            XMFLOAT2 position2D = XMFLOAT2(p_unitPosition.x, p_unitPosition.z); // Needs to be modifiable
            float gridQuadWidth = m_width / static_cast<float>(m_numberOfQuadsWidth); // Gets the width and hight of one quad
            float gridQuadHeight = m_height / static_cast<float>(m_numberOfQuadsHeight);
            // Offset given position with the fields offset to take it back to origo so we are able to calculate which quad we are in
            XMFLOAT2 bottomLeft = XMFLOAT2(m_center.x - m_width / 2.0f, m_center.z - m_height / 2.0f);
            position2D.x -= bottomLeft.x;
            position2D.y -= bottomLeft.y;
            int quadNrX = static_cast<int>(std::floor(position2D.x / gridQuadWidth)); // What quad in x and y
            int quadNrY = static_cast<int>(std::floor(position2D.y / gridQuadHeight));
            if(quadNrX >= 0 && quadNrX < m_numberOfQuadsWidth && quadNrY >= 0 && quadNrY < m_numberOfQuadsHeight)
            {
                return XMINT2(quadNrX, quadNrY);
            }
            else
            {
                return XMINT2(-1, -1);
            }
        }

        DirectX::XMFLOAT2 PotentialFieldImpl::GetGridQuadPosition(const int& p_x, const int& p_z)
        {
            using namespace DirectX;
            XMFLOAT2 returnPosition; // position to be returned
            float quadWidth = m_quadSize.x;
            float quadHeight = m_quadSize.y;
            XMFLOAT2 bottomLeft = XMFLOAT2(m_center.x - m_width * 0.5f, m_center.z - m_height * 0.5f);
            returnPosition = XMFLOAT2((float)p_x * quadWidth + bottomLeft.x + quadWidth * 0.5f, (float)p_z * quadHeight + bottomLeft.y + quadHeight * 0.5f);
            return returnPosition;
        }

        DirectX::XMFLOAT2 PotentialFieldImpl::GetAttractionPosition(const DirectX::XMFLOAT3& p_unitPosition, bool& p_inField,
                                                                    PotentialFieldActor* p_currentActor, const bool& p_staticCheck)
        {
            using namespace DirectX;
            // If there is no positive actor/goal to go to simply dont move!
            bool goalInRange = AnyPositiveGoalInRange(p_unitPosition);
            if(!goalInRange)
            {
                return XMFLOAT2(p_unitPosition.x, p_unitPosition.z);
            }

            // Good thing to note is that the grid is originated from bottom left corner so [0][0] is bottom left corner
            XMFLOAT2 position2D = XMFLOAT2(p_unitPosition.x, p_unitPosition.z); // Needs to be modifiable
            float gridQuadWidth = m_width / static_cast<float>(m_numberOfQuadsWidth); // Gets the width and hight of one quad
            float gridQuadHeight = m_height / static_cast<float>(m_numberOfQuadsHeight);

            // Offset given position with the fields offset to take it back to origo so we are able to calculate which quad we are in
            XMFLOAT2 bottomLeft = XMFLOAT2(m_center.x - m_width / 2.0f, m_center.z - m_height / 2.0f);
            position2D.x -= bottomLeft.x;
            position2D.y -= bottomLeft.y;

            int quadNrX = static_cast<int>(std::floor(position2D.x / gridQuadWidth)); // What quad in x and y
            int quadNrY = static_cast<int>(std::floor(position2D.y / gridQuadHeight));

            // Add quads that needs checking, 3x3 square around the unit
            std::vector<XMINT2> quadsToCheck;
            for(int x = -1; x < 2; x++) // -1, 0, 1
            {
                for(int y = -1; y < 2; y++)
                {
                    quadsToCheck.push_back(XMINT2(quadNrX + x, quadNrY + y));
                }
            }
            // Remove the quad we are standing on since that one is our start value we dont need to check it again
            quadsToCheck.erase(quadsToCheck.begin() + 4);

            // Check for special cases
            size_t length = quadsToCheck.size();
            XMFLOAT2 highestChargedPos = XMFLOAT2(m_center.x, m_center.z); // if we are outside the field we should walk to center
            float highestCharge = 0;
            if(quadNrX >= 0 && quadNrX < m_numberOfQuadsWidth && quadNrY >= 0 && quadNrY < m_numberOfQuadsHeight)
            {
                // take the quad the unit is in as the highest charge. If all the qauds have the same charge the unit shouldnt move
                highestCharge =
                    CalculateCharge(quadNrX, quadNrY, p_currentActor); // +5 since that the max number of phermonetrails in the list TODOCONFIG
                highestChargedPos = XMFLOAT2(p_unitPosition.x, p_unitPosition.z);
                if(m_grid[quadNrX + quadNrY * m_numberOfQuadsWidth].occupied)
                {
                    // If the one we are in is occupied set highest charge to low just to get out
                    highestCharge = std::numeric_limits<float>::min();
                    // THis might happen when the AI is cutting corners
                }
            }


            for(size_t i = 0; i < length; i++)
            {
                int x = quadsToCheck[i].x;
                int y = quadsToCheck[i].y;
                float quadCharge;
                if(!m_grid[x + y * m_numberOfQuadsWidth].occupied)
                {


                    if(x >= 0 && x < m_numberOfQuadsWidth && y >= 0 && y < m_numberOfQuadsHeight)
                    {
                        // The quad exists!

                        // If we are performing a static check we only check vs static actors and only need to take the value saved in the grid
                        // This will probably never be used...
                        if(p_staticCheck)
                        {
                            quadCharge = m_grid[x + y * m_numberOfQuadsWidth].charge;
                        }
                        else
                        {
                            // CalculateCharge takes both static and dynamic actors in to acount, and phermonetrails and stuff
                            quadCharge = CalculateCharge(x, y, p_currentActor);
                        }
                        if(quadCharge > highestCharge)
                        {
                            highestCharge = quadCharge;
                            highestChargedPos = GetGridQuadPosition(x, y);
                        }
                    }
                    else
                    {
                        // Tries to check outside the field
                        // Here we create a imaginary quad and checks if that quad have a greater charge than the last,
                        // if this is true we check what field that imaginary quad would belong to and sees if it's walkable. Should work for field
                        // transition

                        // FInd the position the grid would have had
                        XMFLOAT2 newPosition = GetGridQuadPosition(x, y);
                        // Use that position to set new unit position

                        // The sign gets wheter or not we are moving outside the field in positive or negativ hence we know in what direction to add
                        // the
                        // jump
                        XMFLOAT3 newUnitPosition = XMFLOAT3(newPosition.x + (static_cast<float>(sign<int>(x)) * m_jumpDistance), p_unitPosition.y,
                                                            newPosition.y + (static_cast<float>(sign<int>(y)) * m_jumpDistance));
                        // Find what field the new position is in, if any
                        PotentialFieldImpl* newField = static_cast<PotentialFieldImpl*>(m_context.PFModule->FindBestPotentialField(newUnitPosition));
                        if(newField != nullptr)
                        {
                            // if a field was found check what grid point in that field we are in
                            XMINT2 gridPoint = newField->WhatGridPosAmIOn(newUnitPosition);
                            // if for some wierd reason we are in the field but not in a grid point screw it...
                            if(gridPoint.x != -1)
                            {
                                // calculate a new charge from the new field
                                quadCharge = newField->CalculateCharge(gridPoint.x, gridPoint.y, p_currentActor);
                                // if higher set it as the new high
                                if(quadCharge > highestCharge)
                                {
                                    highestCharge = quadCharge;
                                    highestChargedPos = newField->GetGridQuadPosition(gridPoint.x, gridPoint.y);
                                }
                            }
                        }
                    }
                }
            }
            // if the wanted position is outside the current field we should change field
            if(WhatGridPosAmIOn(XMFLOAT3(highestChargedPos.x, p_unitPosition.y, highestChargedPos.y)).x == -1)
            {
                p_inField = false;
            }
            return XMFLOAT2(highestChargedPos.x, highestChargedPos.y); // TODOEA Kanske skicka bak en xmfloat3
        }

        float PotentialFieldImpl::CalculateCharge(int p_quadX, int p_quadY, const PotentialFieldActor* p_currentActor)
        {
            using namespace DirectX;
            XMFLOAT2 quadPos = GetGridQuadPosition(p_quadX, p_quadY);
            float totalCharge = 0;
            bool usePhermone = true;
            for(auto actor : m_dynamicActors)
            {
                if(actor != p_currentActor) // this mean that the current actor is skipped when calculating charge
                {
                    // This is for the actors influence
                    totalCharge += GetChargeInfluenceFromActor(quadPos, *actor);
                    // This is for the current actors special influence, if any TODOKO Might be a speed up to do this only if we are in range of actor
                    totalCharge += GetSpecialInfluenceBetweenActors(quadPos, *actor, *p_currentActor, usePhermone);
                }
            }
            // std::cout << p_quadX << " " << p_quadY << " " << totalCharge << std::endl;
            // Only do the phermonetrail thingie if we have a actor
            if(p_currentActor != nullptr && usePhermone && p_currentActor->GetUsePhermonetrail())
            {
                // Going through the phermone trail and adding them to the corresponding gridpos.
                std::vector<XMINT2> phermoneVector = p_currentActor->GetPhermoneTrail();
                size_t t_vecSize = phermoneVector.size();
                for(size_t i = 0; i < t_vecSize; ++i)
                {
                    // Create a smal charge on the phermone position
                    XMFLOAT2 phermonePos = GetGridQuadPosition(phermoneVector[i].x, phermoneVector[i].y);
                    XMVECTOR phermonePosVec = XMLoadFloat2(&phermonePos);
                    XMVECTOR quadPosVec = XMLoadFloat2(&quadPos);

                    // Get the distance
                    XMVECTOR distance = phermonePosVec - quadPosVec;
                    XMVECTOR quadSize = XMLoadFloat2(&m_quadSize);
                    float quadLength = *XMVector2Length(quadSize).m128_f32; // Sort of quad length, from center to corner
                    float dist = *XMVector3Length(distance).m128_f32;
                    float force = 0;
                    force = -(m_phermoneEffect + i) *
                            std::fmaxf(1.0f - dist / quadLength * 2, 0.0f); // the phermone will effect all quads around the phemoned quad
                    totalCharge += force;
                }
            }

            return totalCharge + m_grid[p_quadX + p_quadY * m_numberOfQuadsWidth].charge;
        }
        bool PotentialFieldImpl::AnyPositiveGoalInRange(const DirectX::XMFLOAT3& p_position)
        {
            using namespace DirectX;
            // Here i assume that the only positive charge is from dynamic actors,TODOKO this is wrong but for now it'll do
            // Should cehck occupied quads but i dont thing dynamic actors have these yet...
            for(auto actor : m_dynamicActors)
            {
                float charge = actor->GetCharge();
                if(charge > 0)
                {
                    XMFLOAT3 position3d = actor->GetPosition();
                    float range = actor->GetRange();
                    // TODOXX this is copied from ProximityChecker in gamecore, if that function starts failing thisone needs changing too
                    // It should be safe though
                    XMVECTOR vecBetweenEntities = XMLoadFloat3(&position3d) - XMLoadFloat3(&p_position);
                    XMVECTOR distanceVec = XMVector3Length(vecBetweenEntities);
                    float distance = *distanceVec.m128_f32;
                    if(distance <= range)
                    {
                        // we dont really care if multiple actors are in range
                        return true;
                    }
                    else
                    {
                        // We still need to check the other actors
                    }
                }
            }
            return false;
        }

        float PotentialFieldImpl::GetChargeInfluenceFromActor(const DirectX::XMFLOAT2& p_position, const PotentialFieldActor& p_actor)
        {
            using namespace DirectX;
            // Get values from actor
            XMFLOAT3 actorPos3d = p_actor.GetPosition(); // Dont really care about the third dimension TODOKO review if 3d is needed
            XMFLOAT2 actorPos = XMFLOAT2(actorPos3d.x, actorPos3d.z);
            float actorCharge = p_actor.GetCharge();
            float actorRange = p_actor.GetRange();
            // Calculate charge
            XMVECTOR actorPosVec = XMLoadFloat2(&actorPos);
            XMVECTOR quadPosVec = XMLoadFloat2(&p_position);

            XMVECTOR distance = actorPosVec - quadPosVec;
            float dist = *XMVector3Length(distance).m128_f32;
            float force = 0;
            // Check if we are inside the range, not really needed due to std::fmax
            if(dist < actorRange)
            {
                force = actorCharge * std::fmaxf(1.0f - dist / actorRange, 0.0f); // std::powf(dist, 2.0f) / std::powf(actorRange, 2.0f), 0.0f);
            }
            return force;
        }

        float PotentialFieldImpl::GetSpecialInfluenceBetweenActors(const DirectX::XMFLOAT2& p_position, const PotentialFieldActor& p_actorToCheck,
                                                                   const PotentialFieldActor& p_yourActor, bool& o_phermoneActive)
        {
            // Get all the special fields in your actor
            const std::vector<PotentialChargeInformation>& t_specialCharges = p_yourActor.GetPotentialVsOthers();
            size_t length = t_specialCharges.size();
            float force = 0;
            for(size_t i = 0; i < length; i++)
            {
                // Check if this field should be used on this actor.
                if(((size_t)t_specialCharges[i].actorToBeAddedTo & (size_t)p_actorToCheck.GetActorType()) == (size_t)p_actorToCheck.GetActorType() &&
                   t_specialCharges[i].active)
                {
                    // Yay, everything fitts! Lets make a new temporary actor with the info provided by special field and look for charge
                    // TODOKO Thats not possible now since we dont have intermodule communication yet here...
                    using namespace DirectX;
                    XMFLOAT3 actorPos3d = p_actorToCheck.GetPosition(); // Dont really care about the third dimension TODOKO review if 3d is needed
                    XMFLOAT2 actorPos = XMFLOAT2(actorPos3d.x, actorPos3d.z);
                    float actorCharge = t_specialCharges[i].charge;
                    float actorRange = t_specialCharges[i].range;
                    // Calculate charge
                    XMVECTOR actorPosVec = XMLoadFloat2(&actorPos);
                    XMVECTOR quadPosVec = XMLoadFloat2(&p_position);

                    XMVECTOR distance = actorPosVec - quadPosVec;
                    float dist = *XMVector3Length(distance).m128_f32;


                    // Check if phermone trail should be used
                    if(dist < actorRange && !t_specialCharges[i].usePhermoneTrail)
                    {
                        o_phermoneActive = false;
                    }
                    force += t_specialCharges[i].forceEquation(actorCharge, dist, actorRange);
                }
            }
            return force;
        }
    }
}