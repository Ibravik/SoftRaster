// Grafiquitas.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
// 1/18/2025
// texturas potencia de 2 y multiplo de 4
// el hacerlo potencia de 2 implica que puedes solo hacer un recorrimiento de bit 010-> 0100 y asi
// por eso quedan de la mitad del tamaño, es mas rapido solo leer el bit
// en memoria el q no sea un solo bit no tendria mucho sentido y quedarian flotantes
// piensa en dejar como imagen la base y textura el contenedor para la cosa q estas haciendo
// 
// 
// isotropico= la escala es proporcional en los 2 ejes
// anisotropico,: la escala solo es proporcional a 1 eje
// rasterizacion: informacion geometrica a pixeles, proyectarla, todo el proceso
// 
// teniendo el punto inicial y final en una linea, queremos dibujar los pixeles por donde va a pasar la linea, por lo tanto necesitamos 
// 
// el algorithmo de brezenham viene para poder hacer lineas sin puntosflotantes, por ser con enteros es mucho mas rapida que con flotantes en una linea matematicamente pura
// 
// 
// el proceso de hacer las lineas exxternas de la geometris se le llama covertura
//

// 1/25/2025
// el anisotropico solo toma un ovalo en lugar de un cuadrado, ya que en un eje esta mas estirado que een otro
// 
// clip vs clamp: en el clamp, al quedar a 0 la linea cambia al cambiar el punto de origen
// 
// coon el clip, lo que haces es determinar la proyeccion de la linea en la pantalla para poder generar una nueva linea, y con o pintar pixeles fuera de la pantalla
// 
// 
// escribir una nueva funcion que escriba dos brezenham para hacer el triangulo, cada pixel que avances tendras que hacer una linea entre ellas
// basicamente al avanzar una en y por ejemplo, pintas la linea entre ellas en X
// tendrias que checarte lo del cliping y lo del ccw cw 
//
// 2/1/2025
// añadir funcion para poner wireframe
// añadir funciones de rasterizxacion de triangulos
// 
//

#include <iostream>
#include "RePiGeometryStage.h"
#include "RePiRasterizerStage.h"

#include "RePiResourceManager.h"
#include "RePi3DModel.h"
#include "RePiAnimator.h"
#include "RePiCamera.h"
#include "RePiTexture.h"
#include "RePiMaterial.h"

#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

/* We will use this renderer to draw into this window every frame. */
static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_Texture* texture = NULL;

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

//#include "MathObjects.h"
//#include "Camera.h"
//#include "Mesh.h"


// Resources
static std::vector<std::weak_ptr<RePi3DModel>> g_ModelList;
static std::vector<std::weak_ptr<RePiAnimator>> g_AnimatorList;
static std::map<std::string, std::shared_ptr<RePiCamera>> g_CameraList;
std::shared_ptr<GeometryConstantBuffer> g_GeometryConstantBuffer;
std::shared_ptr<RasterizerConstantBuffer> g_RasterizerConstantBuffer;
RasteriserSettings g_RasteriserSettings;

std::shared_ptr<RePiTexture> g_RenderTarget;
std::shared_ptr<RePiTexture> g_Depth;

RePiGeometryStage g_Geometrystage;
RePiRasterizerStage g_RasterizerStage;
VertexShader g_VertexShader;
PixelShader g_PixelShaderColorOnly;
PixelShader g_PixelShaderDiffuse;

// Global Camera
static RePiFloat2 ScreenSize(WINDOW_WIDTH, WINDOW_HEIGHT);
static RePiFloat3 CameraPos(300.f, 300.f, -300.f);
static RePiFloat3 CameraLookAt(0.f, 0.f, 0.f);
static RePiFloat3 WorldUp(0.f, 1.f, 0.f);
static float CameraSpeed = 500.f;
static float CameraRotationSpeed = RePiMath::TWO_PI;
static float FovAngleY = RePiMath::HALF_PI * 0.5f;
static float NearZ = 0.01f;
static float FarZ = 10000.f;
static std::string GlobalCamera("GlobalCamera");

static uint32_t RegisteredButtonState = 0;
static RePiFloat3 LastMousePosition;
static RePiFloat3 CurrentMousePosition;

#pragma region InputManagerXD
enum RegisteredButton
{
    eW = 1,
    eA,
    eS,
    eD,
    eQ,
    eE,
    eMRB
};

static void SetMousePosition(const RePiFloat3& NewPosition)
{
    POINT Position{};
    Position.x = static_cast<int32_t>(NewPosition.x);
    Position.y = static_cast<int32_t>(NewPosition.y);

    SetCursorPos(Position.x, Position.y);
}

static RePiFloat3 GetMousePosition()
{
    POINT Position;

    GetCursorPos(&Position);

    return RePiFloat3(static_cast<float>(Position.x), static_cast<float>(Position.y), 0.f);
}

static void ButtonDown(const RegisteredButton Button)
{
    RegisteredButtonState |= (1 << Button);
}

static void ButtonUp(const RegisteredButton Button)
{
    RegisteredButtonState &= ~(1 << Button);
}

static bool IsButtonPressed(RegisteredButton Button)
{
    return  (RegisteredButtonState & (1 << Button)) != 0;
}
#pragma endregion

static void UpdateCameraInputs(const float Tick, const std::string& ActiveCamera)
{
    RePiFloat3 CameraTranslation = RePiFloat3::ZERO;
    RePiFloat3 CameraRotation = RePiFloat3::ZERO;
    if (IsButtonPressed(RegisteredButton::eW))
    {
        CameraTranslation.z -= 1.f;
    }

    if (IsButtonPressed(RegisteredButton::eS))
    {
        CameraTranslation.z += 1.f;
    }

    if (IsButtonPressed(RegisteredButton::eA))
    {
        CameraTranslation.x += 1.f;
    }

    if (IsButtonPressed(RegisteredButton::eD))
    {
        CameraTranslation.x -= 1.f;
    }

    if (IsButtonPressed(RegisteredButton::eQ))
    {
        CameraTranslation.y -= 1.f;
    }

    if (IsButtonPressed(RegisteredButton::eE))
    {
        CameraTranslation.y += 1.f;
    }

    if (IsButtonPressed(RegisteredButton::eMRB))
    {
        CurrentMousePosition = GetMousePosition();

        CameraRotation = LastMousePosition - CurrentMousePosition;
        CameraRotation = RePiFloat3(CameraRotation.y, CameraRotation.x, 0.f);

        SetMousePosition(LastMousePosition);
    }

    std::weak_ptr<RePiCamera> CurrentCamera = g_CameraList[ActiveCamera];
    if (auto pCurrentCamera = CurrentCamera.lock())
    {
        pCurrentCamera->Move(CameraTranslation.getSafeNormal());

        pCurrentCamera->Rotate(CameraRotation.getSafeNormal());

        g_GeometryConstantBuffer->View = pCurrentCamera->GetView();
        g_GeometryConstantBuffer->Projection = pCurrentCamera->GetProjection();
    }
    else
    {
        g_GeometryConstantBuffer->View = RePiMatrix::IDENTITY;
        g_GeometryConstantBuffer->Projection = RePiMatrix::IDENTITY;
    }
}

static void Update(const float Tick)
{
    UpdateCameraInputs(Tick, "GlobalCamera");

    for (auto& Camera : g_CameraList)
    {
        if (nullptr != Camera.second)
        {
            Camera.second->Update(Tick);
        }
    }

    for (auto& Animator : g_AnimatorList)
    {
        if (auto pAnimator = Animator.lock())
        {
            pAnimator->Update(Tick);
        }
    }

    for (auto& Model : g_ModelList)
    {
        if (auto pModel = Model.lock())
        {
            pModel->Update();
        }
    }
}

static void Render()
{
    if (nullptr != g_Depth)
    {
        g_Depth->ClearData(0.f);
    }

    if (nullptr != g_RenderTarget)
    {
        g_RenderTarget->ClearColor(RePiColor::White);
    }

    for (auto& Model : g_ModelList)
    {
        if (auto pModel = Model.lock())
        {
            g_GeometryConstantBuffer->World = pModel->GetTransform();

            memcpy(g_GeometryConstantBuffer->Bones, pModel->m_boneTransform, sizeof(RePiMatrix) * MaxBoneCapacity);
            g_RasterizerStage.BindPixelShader(g_PixelShaderDiffuse);
            for (auto& Mesh : pModel->mMeshList)
            {
                if (auto pMesh = Mesh.lock())
                {
                    g_Geometrystage.BindIndexBuffer(pMesh->getIndexBuffer());
                    g_Geometrystage.BindVertexBuffer(pMesh->getVertexBuffer());
                    g_Geometrystage.BindTopology(pMesh->getTopology());
                    g_RasterizerStage.BindMaterial(pMesh->getMaterial());

                    g_Geometrystage.Execute();
                    g_RasterizerStage.Execute();
                }
            }

            if (auto pAnimator = pModel->GetAnimator().lock())
            {
                memcpy(g_GeometryConstantBuffer->Bones, pAnimator->mJoinTransform, sizeof(RePiMatrix) * MaxBoneCapacity);

                if (auto pMesh = pAnimator->GetSkeletonMesh().lock())
                {
                    g_Geometrystage.BindIndexBuffer(pMesh->getIndexBuffer());
                    g_Geometrystage.BindVertexBuffer(pMesh->getVertexBuffer());
                    g_Geometrystage.BindTopology(pMesh->getTopology());

                    g_RasterizerStage.BindPixelShader(g_PixelShaderColorOnly);

                    g_Geometrystage.Execute();
                    g_RasterizerStage.Execute();
                }
            }
        }
    }
}

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    SDL_SetAppMetadata("Example Renderer Streaming Textures", "1.0", "com.example.renderer-streaming-textures");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("examples/renderer/streaming-textures", WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, WINDOW_WIDTH, WINDOW_HEIGHT);
    if (!texture) {
        SDL_Log("Couldn't create streaming texture: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    bool Result = true;

    // Resource Manager
    Result = RePiResourceManager::StartUp([]() { RePiResourceManager::Instance().InitCustomResources(); });
    if (!Result)
    {
        return SDL_APP_FAILURE;
    }
    auto& ResourceManager = RePiResourceManager::Instance();

    // Scene
    if (auto pNewModel = ResourceManager.Create3DModel("data/3DObject/guardian/guardian.md5mesh").lock())
    {
        g_ModelList.push_back(pNewModel);
        pNewModel->SetTransform(RePiQuaternion(RePiFloat3::UP, RePiMath::PI) * RePiTranslationMatrix({-200.f, 0.f, -200.f}));
        if (auto pNewAnimator = ResourceManager.CreateAnimator("data/3DObject/guardian/guardian.md5mesh").lock())
        {
            g_AnimatorList.push_back(pNewAnimator);

            ResourceManager.BindAnimations("data/3DObject/guardian/death.md5anim", pNewAnimator);

            pNewModel->BindAnimator(pNewAnimator);

            pNewAnimator->PlayAnimation("data/3DObject/guardian/death.md5anim_0");
        }
    }

    if (auto pNewModel = ResourceManager.Create3DModel("data/3DObject/guardian/guardian.md5mesh").lock())
    {
        g_ModelList.push_back(pNewModel);
        pNewModel->SetTransform(RePiTranslationMatrix({ -200.f, 0.f, 0.f }));

        if (auto pNewAnimator = ResourceManager.CreateAnimator("data/3DObject/guardian/guardian.md5mesh").lock())
        {
            g_AnimatorList.push_back(pNewAnimator);

            ResourceManager.BindAnimations("data/3DObject/guardian/attack1.md5anim", pNewAnimator);

            pNewModel->BindAnimator(pNewAnimator);

            pNewAnimator->PlayAnimation("data/3DObject/guardian/attack1.md5anim_0");
        }
    }

    // Camera
    g_CameraList["GlobalCamera"] = std::make_shared<RePiPerspectiveCamera>(CameraPos, CameraLookAt, WorldUp, ScreenSize, FovAngleY, NearZ, FarZ, CameraSpeed, CameraRotationSpeed, true);

    // Vertex Shader
    g_VertexShader = [](const RePiVertex& Input, const GeometryConstantBuffer& Buffer)->RePiVertex
        {
            RePiVertex Output = Input;

            if (Input.BoneWeight[0] > 0)
            {
                RePiMatrix BoneTransform = Buffer.Bones[Input.BoneIndex[0]] * Input.BoneWeight[0];
                BoneTransform += Buffer.Bones[Input.BoneIndex[1]] * Input.BoneWeight[1];
                BoneTransform += Buffer.Bones[Input.BoneIndex[2]] * Input.BoneWeight[2];
                BoneTransform += Buffer.Bones[Input.BoneIndex[3]] * Input.BoneWeight[3];

                Output.Position = BoneTransform.transformVector4(Output.Position);

                Output.Normal = BoneTransform.transformVector(Output.Normal);
                Output.Binormal = BoneTransform.transformVector(Output.Binormal);
                Output.Tangent = BoneTransform.transformVector(Output.Tangent);
            }

            Output.Position = Buffer.World.transformVector4(Output.Position);
            Output.Position = Buffer.View.transformVector4(Output.Position);
            Output.Position = Buffer.Projection.transformVector4(Output.Position);

            Output.Normal = Buffer.World.transformVector(Output.Normal).getSafeNormal();
            Output.Binormal = Buffer.World.transformVector(Output.Binormal).getSafeNormal();
            Output.Tangent = Buffer.World.transformVector(Output.Tangent).getSafeNormal();

            return Output;
        };

    // geometry constant buffer
    g_GeometryConstantBuffer = std::make_shared<GeometryConstantBuffer>();

    // Pixel Shader
    g_PixelShaderColorOnly = [](const RePiVertex& Input, const std::weak_ptr<RePiMaterial>& Material, const std::weak_ptr<RasterizerConstantBuffer>& Buffer)->RePiLinearColor
        {
            return RePiLinearColor(RePiColor(0, 255, 0, 255));
        };

    g_PixelShaderDiffuse = [](const RePiVertex& Input, const std::weak_ptr<RePiMaterial>& Material, const std::weak_ptr<RasterizerConstantBuffer>& Buffer)->RePiLinearColor
        {
            auto pDiffuse = Material.lock()->mImageList[0].lock();

            return pDiffuse->SampleColor(Input.TexCoord);
        };

    // Render Target
    g_RenderTarget = std::make_shared<RePiTexture>();

    g_RenderTarget->Create(RePiInt2(int32_t(ScreenSize.x), int32_t(ScreenSize.y)), RePiTextureFormat::eR8G8B8A8_UNORM);

    // Depth 
    g_Depth = std::make_shared<RePiTexture>();

    g_Depth->Create(RePiInt2(int32_t(ScreenSize.x), int32_t(ScreenSize.y)), RePiTextureFormat::eR32_FLOAT);

    // rasterizer constant buffer
    g_RasterizerConstantBuffer = std::make_shared<RasterizerConstantBuffer>();

    // rasterizer settings
    g_RasteriserSettings.cullMode = RePiCullMode::eBACK;
    g_RasteriserSettings.wireframe = false;


    g_RasterizerStage.BindConstantBuffer(g_RasterizerConstantBuffer);
    g_RasterizerStage.BindDepth(g_Depth);

    g_RasterizerStage.BindRasteriserSettings(g_RasteriserSettings);
    g_RasterizerStage.BindTarget(g_RenderTarget);

    g_Geometrystage.BindConstantBuffer(g_GeometryConstantBuffer);
    g_Geometrystage.BindVertexShader(g_VertexShader);

    g_RasterizerStage.BindLineList(g_Geometrystage.GetLineList());
    g_RasterizerStage.BindPointList(g_Geometrystage.GetPointList());
    g_RasterizerStage.BindTriangleList(g_Geometrystage.GetTriangleList());

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
    }
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

static std::chrono::steady_clock::time_point g_LastTime;
static std::chrono::steady_clock::time_point g_CurrentTime;

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void* appstate)
{
    g_CurrentTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsedTime = g_CurrentTime - g_LastTime;
    g_LastTime = g_CurrentTime;

    float Tick = static_cast<float>(elapsedTime.count());

    Update(Tick);
    Render();

    // Lock the texture to update pixels
    void* pixels;
    int pitch;
    if (!SDL_LockTexture(texture, nullptr, &pixels, &pitch))
    {
        RePiLog(RePiLogLevel::eWARNING, "SDL_LockTexture Error: " + std::string(SDL_GetError()));
    }

    // Copy pixel data to the texture
    memcpy(pixels, g_RenderTarget->GetBufferData(), WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(uint32_t));

    // Unlock the texture
    SDL_UnlockTexture(texture);

    // Clear the renderer and copy the texture to it
    SDL_RenderClear(renderer);
    if (!SDL_RenderTexture(renderer, texture, nullptr, nullptr))
    {
        RePiLog(RePiLogLevel::eWARNING, "SDL_RenderTexture Error: " + std::string(SDL_GetError()));
    }
    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    SDL_DestroyTexture(texture);
    /* SDL will clean up the window/renderer for us. */
}

/*int32_t main()
{
    




    

    g_RenderTarget->Save("data/targetOut.bmp");
    g_Depth->Save("data/depthOut.bmp");
}*/
