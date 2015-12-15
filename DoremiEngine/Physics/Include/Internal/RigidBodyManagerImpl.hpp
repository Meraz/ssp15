#pragma once
#include <RigidBodyManager.hpp>
#include <unordered_map>
#include <DirectXMath.h>
#include <PhysX/PxPhysicsAPI.h>
using namespace physx;
using namespace std;
using namespace DirectX;
namespace DoremiEngine
{
    namespace Physics
    {
        struct InternalPhysicsUtils;
        class RigidBodyManagerImpl : public RigidBodyManager
        {
            public:
            RigidBodyManagerImpl(InternalPhysicsUtils& p_utils);
            virtual ~RigidBodyManagerImpl();

            int AddBoxBodyDynamic(XMFLOAT3 p_position, XMFLOAT4 p_orientation, XMFLOAT3 p_dims, int p_materialID) override;
            int AddBoxBodyStatic(XMFLOAT3 p_position, XMFLOAT4 p_orientation, XMFLOAT3 p_dims, int p_materialID) override;

            void AddForceToBody(int p_bodyID, XMFLOAT3 p_force) override;
            void AddTorqueToBody(int p_bodyID, XMFLOAT3 p_torque) override;
            void SetBodyVelocity(int p_bodyID, XMFLOAT3 p_velocity) override;
            void SetBodyPosition(int p_bodyID, XMFLOAT3 p_position, XMFLOAT4 p_orientation) override;

            XMFLOAT3 GetBodyPosition(int p_bodyID) override;
            XMFLOAT4 GetBodyOrientation(int p_bodyID) override;
            XMFLOAT3 GetBodyVelocity(int p_bodyID) override;

            private:
            InternalPhysicsUtils& m_utils;

            // Separate lists for dynamic and static bodies
            // unordered_map<int, PxRigidDynamic*> m_bodies;
            // unordered_map<int, PxRigidStatic*> m_staticBodies;
            unordered_map<int, PxRigidActor*> m_bodies;
            int m_nextBody;
            int m_nextStaticBody;
        };
    }
}