#define NUM_LIGHTS 500

struct PixelInputType
{
    float4 position : SV_POSITION;
    float4 worldPos : WORLDPOS;
    float2 depth : DEPTH;
    float3 screenPos : SCREENPOS;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 cameraPos : CAMERAPOS;
};

struct PixelOutputType
{
    float4 diffuse : SV_Target0;
    float4 glow : SV_Target1;
    float4 depth : SV_Target2;
};

struct LightGridInfo
{
    uint offset;
    uint value;
};

struct Light
{
    int type; // 0 = def, 1 = dir, 2 = spot, 3 = point
    float3 attenuation;
    float intensity;
    float3 color;
    float coneAngle;
    float3 direction;
    float penumAgle;
    float3 position;
    bool enabled;
    float3 pad;
};

struct MaterialData
{
    int mapMasks;
    float3 color;
    float diffuse;
    float3 ambColor;
    float specCosine;
    float3 specColor;
    float specEccentricity;
    float specRollOff;
    float2 pad;
};

struct Plane
{
    float3 N;   // Plane normal.
    float  d;   // Distance to origin.
};
struct Frustum
{
    Plane plane[4];   // left, right, top, bottom frustum planes.
};


StructuredBuffer<uint> o_LightIndexList : register(t1);
StructuredBuffer<uint> t_LightIndexList : register(t2);
StructuredBuffer<LightGridInfo> o_LightGrid : register(t3);
StructuredBuffer<LightGridInfo> t_LightGrid : register(t4);


cbuffer LightInfo : register(b0)
{
    Light light[NUM_LIGHTS];
};

cbuffer MaterialMessage : register(b1)
{
    float nodeName;
    float diffuseTexturePath;
    float glowTexturePath;
    float specTexturePath;
    float bumpTexturePath;
    int type;
    float2 pad;
    MaterialData materialData;
};

Texture2D ObjTexture : register(t0);
Texture2D GlowTexture : register(t5);

SamplerState ObjSamplerState : register(s0);


float3 CalcDirectionalLight(PixelInputType input, Light l)
{
    float3 lightDir;
    float lightIntensity;
    float4 texcolor = float4(0.0, 0.0, 0.0, 1);
    float3 normal = normalize(input.normal);

    lightDir = -l.direction;
    lightIntensity = saturate(dot(normal, lightDir));
    return saturate(l.color * lightIntensity) * 0.1;
}

float3 CalcSpotLight(PixelInputType input, Light l)
{
    return float3(0, 0, 0);
}

float3 CalcPointLight(PixelInputType input, Light l, float3 texcolor)
{


    float3 normal = normalize(input.normal);

    float3 scatteredLight, reflectedLight;
    float attenuation;

    float3 lightDirection = l.position - input.worldPos.xyz;
    float lightDistance = length(lightDirection);

    if (lightDistance < 100.0f)
    {
        attenuation = 1.f /
            (l.attenuation[0] + l.attenuation[1] * lightDistance +
                l.attenuation[2] * lightDistance * lightDistance);

        float3 halfVector = normalize(lightDirection + normalize(-input.worldPos));

        float diffuse = max(0.0, dot(normal, lightDirection));
        float specular = max(0.0, dot(normal, halfVector));

        scatteredLight = l.color * diffuse * attenuation;
        reflectedLight = l.color * specular * attenuation;
        return min(texcolor.rgb * scatteredLight + reflectedLight, float3(1, 1, 1)) * l.intensity;
    }

    return float3(0, 0, 0);
}

PixelOutputType PS_main(PixelInputType input)
{
    PixelOutputType output;
    float2 screenPos = input.screenPos.xy / input.screenPos.z;
    screenPos.y = (-screenPos.y + 1) * 360;
    screenPos.x = (screenPos.x + 1) * 640;

    //calculate which thread group this pixel was in the compute shader stage
    float2 groupID2 = float2((int)screenPos.x / 16, (int)screenPos.y / 16);
    float groupID = groupID2.x + (groupID2.y * 80);
    //extract data from light grid
    int index = o_LightGrid[groupID].offset;
    int value = o_LightGrid[groupID].value;

    float4 texcolor = ObjTexture.Sample(ObjSamplerState, input.texCoord);
    float4 glowcolor = GlowTexture.Sample(ObjSamplerState, input.texCoord);

    texcolor = saturate(texcolor);


    float3 rgb = float3(0, 0, 0);

    for (int i = index; i < index + value; i++)
    //for (int i = 0; i < 200; i++)
    {
        Light l = light[o_LightIndexList[i]];
        //Light l = light[i];
        if (l.type == 0)
            rgb += float3(0, 0, 0);
        if (l.type == 1)
            rgb += CalcDirectionalLight(input, l); 
        if (l.type == 2)
            rgb += CalcSpotLight(input, l);
        if (l.type == 3)
            rgb += CalcPointLight(input, l, texcolor.rgb);
    }

    if (glowcolor.r < 0.5 && 
        glowcolor.g < 0.5 && glowcolor.b < 0.5)
        output.glow = float4(0, 0, 0, 0);
    else
        output.glow = normalize(texcolor) * 2;

    output.diffuse = float4(rgb, 1) * texcolor * 2.5f;

    float depth = (input.position.z/input.position.w) + 1;
    output.depth = float4(depth, depth, depth, 1);

    return output;


}