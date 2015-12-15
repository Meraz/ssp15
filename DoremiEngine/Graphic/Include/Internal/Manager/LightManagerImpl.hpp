#pragma once

#include <Interface/Manager/LightManager.hpp>
#include <Internal/Light/LightInfoImpl.hpp>
#include <d3d11_1.h>
#include <vector>

namespace DoremiEngine
{
    namespace Graphic
    {
        struct GraphicModuleContext;
        // struct Light;
        struct LightBuffer
        {
            // std::vector<LightInfoImpl> lightList;
            Light lightList[3];
        };

        class LightManagerImpl : public LightManager
        {
            public:
            LightManagerImpl(const GraphicModuleContext& p_graphicContext);
            virtual ~LightManagerImpl();
            int AddLight(Light light) override;
            Light GetLight(int index) override;
            // std::vector<Light> GetLightList() override;
            void InitLightManager() override;

            private:
            const GraphicModuleContext& m_graphicContext;
            ID3D11Device* m_device;
            ID3D11DeviceContext* m_deviceContext;
            LightBuffer m_lightBuffer;
        };
    }
}
