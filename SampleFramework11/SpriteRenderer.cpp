//======================================================================
//
//	MJP's DX11 Sample Framework
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//======================================================================

#include "PCH.h"

#include "SpriteRenderer.h"
#include "ShaderCompilation.h"
#include "SpriteFont.h"

namespace SampleFramework11
{

SpriteRenderer::SpriteRenderer()
    : initialized(false)
{

}

SpriteRenderer::~SpriteRenderer()
{

}

void SpriteRenderer::Initialize(ID3D11Device* device)
{
	this->device = device;

	// Load the shaders
    ID3D10BlobPtr compiledVS;
    compiledVS.Attach(CompileShader(L"SampleFramework11\\Shaders\\Sprite.hlsl", "SpriteVS", "vs_4_0"));
    DXCall(device->CreateVertexShader(compiledVS->GetBufferPointer(), compiledVS->GetBufferSize(), NULL, &vertexShader));

    ID3D10BlobPtr compiledVSInstanced;
    compiledVSInstanced.Attach(CompileShader(L"SampleFramework11\\Shaders\\Sprite.hlsl", "SpriteInstancedVS", "vs_4_0"));
    DXCall(device->CreateVertexShader(compiledVSInstanced->GetBufferPointer(), compiledVSInstanced->GetBufferSize(), NULL, &vertexShaderInstanced));

    pixelShader.Attach(CompilePSFromFile(device, L"SampleFramework11\\Shaders\\Sprite.hlsl", "SpritePS"));

	// Define the input layouts
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

    DXCall(device->CreateInputLayout(layout, 2, compiledVS->GetBufferPointer(), compiledVS->GetBufferSize(), &inputLayout));

    D3D11_INPUT_ELEMENT_DESC layoutInstanced[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TRANSFORM", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "TRANSFORM", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "TRANSFORM", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "TRANSFORM", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 64, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "SOURCERECT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 80, D3D11_INPUT_PER_INSTANCE_DATA, 1 }
    };

    DXCall(device->CreateInputLayout(layoutInstanced, 8, compiledVSInstanced->GetBufferPointer(), compiledVSInstanced->GetBufferSize(), &inputLayoutInstanced));

    // Create the vertex buffer
    SpriteVertex verts[] =
    { { D3DXVECTOR2(0.0f, 0.0f), D3DXVECTOR2(0.0f, 0.0f) },
      { D3DXVECTOR2(1.0f, 0.0f), D3DXVECTOR2(1.0f, 0.0f) },
      { D3DXVECTOR2(1.0f, 1.0f), D3DXVECTOR2(1.0f, 1.0f) },
      { D3DXVECTOR2(0.0f, 1.0f), D3DXVECTOR2(0.0f, 1.0f) } };
    D3D11_BUFFER_DESC desc;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.ByteWidth = sizeof(SpriteVertex) * 4;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA initData;
    initData.pSysMem = verts;
    initData.SysMemPitch = 0;
    initData.SysMemSlicePitch = 0;
    DXCall(device->CreateBuffer(&desc, &initData, &vertexBuffer));

    // Create the instance data buffer
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.ByteWidth = sizeof(SpriteDrawData) * MaxBatchSize;
    DXCall(device->CreateBuffer(&desc, NULL, &instanceDataBuffer));

    // Create the index buffer
    USHORT indices[] = { 0, 1, 2, 3, 0, 2 };
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.ByteWidth = sizeof(USHORT) * 6;
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    desc.CPUAccessFlags = 0;
    initData.pSysMem = indices;
    DXCall(device->CreateBuffer(&desc, &initData, &indexBuffer));

    // Create our constant buffers
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.ByteWidth = CBSize(sizeof(VSPerBatchCB));
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    DXCall(device->CreateBuffer(&desc, NULL, &vsPerBatchCB));

    desc.ByteWidth = CBSize(sizeof(SpriteDrawData));
    DXCall(device->CreateBuffer(&desc, NULL, &vsPerInstanceCB));

    // Create our states
    D3D11_RASTERIZER_DESC rastDesc;
    rastDesc.AntialiasedLineEnable = FALSE;
    rastDesc.CullMode = D3D11_CULL_NONE;
    rastDesc.DepthBias = 0;
    rastDesc.DepthBiasClamp = 1.0f;
    rastDesc.DepthClipEnable = false;
    rastDesc.FillMode = D3D11_FILL_SOLID;
    rastDesc.FrontCounterClockwise = false;
    rastDesc.MultisampleEnable = true;
    rastDesc.ScissorEnable = false;
    rastDesc.SlopeScaledDepthBias = 0;
    DXCall(device->CreateRasterizerState(&rastDesc, &rastState));

    D3D11_BLEND_DESC blendDesc;
    blendDesc.AlphaToCoverageEnable = false;
    blendDesc.IndependentBlendEnable = false;
    for (UINT i = 0; i < 8; ++i)
    {
        blendDesc.RenderTarget[i].BlendEnable = true;
        blendDesc.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[i].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        blendDesc.RenderTarget[i].SrcBlend = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
    }
    DXCall(device->CreateBlendState(&blendDesc, &alphaBlendState));

    D3D11_DEPTH_STENCIL_DESC dsDesc;
    dsDesc.DepthEnable = FALSE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
    dsDesc.StencilEnable = false;
    dsDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
    dsDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
    dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
    dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    dsDesc.BackFace = dsDesc.FrontFace;
    DXCall(device->CreateDepthStencilState(&dsDesc, &dsState));

    // linear filtering
    D3D11_SAMPLER_DESC sampDesc;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.BorderColor[0] = 0;
    sampDesc.BorderColor[1] = 0;
    sampDesc.BorderColor[2] = 0;
    sampDesc.BorderColor[3] = 0;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.MaxAnisotropy = 1;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    sampDesc.MinLOD = 0;
    sampDesc.MipLODBias = 0;
    DXCall(device->CreateSamplerState(&sampDesc, &linearSamplerState));

    // point filtering
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    DXCall(device->CreateSamplerState(&sampDesc, &pointSamplerState));

    initialized = true;
}

void SpriteRenderer::Begin(ID3D11DeviceContext* deviceContext, FilterMode filterMode)
{
    _ASSERT(initialized);
    _ASSERT(!context);
    context = deviceContext;

    D3DPERF_BeginEvent(0xFFFFFFFF, L"SpriteRenderer Begin/End");

    // Set the index buffer
    context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R16_UINT, 0);

    // Set primitive topology
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Set the states
    float blendFactor[4] = {1, 1, 1, 1};
    context->RSSetState(rastState);
    context->OMSetBlendState(alphaBlendState, blendFactor, 0xFFFFFFFF);
    context->OMSetDepthStencilState(dsState, 0);

    if (filterMode == Linear)
        context->PSSetSamplers(0, 1, &(linearSamplerState.GetInterfacePtr()));
    else if (filterMode == Point)
        context->PSSetSamplers(0, 1, &(pointSamplerState.GetInterfacePtr()));

    // Set the shaders
    context->PSSetShader(pixelShader, NULL, 0);
    context->GSSetShader(NULL, NULL, 0);
    context->DSSetShader(NULL, NULL, 0);
    context->HSSetShader(NULL, NULL, 0);
}

D3D11_TEXTURE2D_DESC SpriteRenderer::SetPerBatchData(ID3D11ShaderResourceView* texture)
{
    // Set per-batch constants
    VSPerBatchCB perBatch;

    // Get the viewport dimensions
    UINT numViewports = 1;
    D3D11_VIEWPORT vp;
    context->RSGetViewports(&numViewports, &vp);
    perBatch.ViewportSize = XMFLOAT2(static_cast<float>(vp.Width), static_cast<float>(vp.Height));

    // Get the size of the texture
    ID3D11Resource* resource;
    ID3D11Texture2DPtr texResource;
    D3D11_TEXTURE2D_DESC desc;
    texture->GetResource(&resource);
    texResource.Attach(reinterpret_cast<ID3D11Texture2D*>(resource));
    texResource->GetDesc(&desc);
    perBatch.TextureSize = XMFLOAT2(static_cast<float>(desc.Width), static_cast<float>(desc.Height));

    // Copy it into the buffer
    D3D11_MAPPED_SUBRESOURCE mapped;
    DXCall(context->Map(vsPerBatchCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
    CopyMemory(mapped.pData, &perBatch, sizeof(VSPerBatchCB));
    context->Unmap(vsPerBatchCB, 0);

    return desc;
}

void SpriteRenderer::Render(ID3D11ShaderResourceView* texture,
                            const XMMATRIX& transform,
                            const XMFLOAT4& color,
                            const XMFLOAT4* drawRect)
{
    _ASSERT(context);
    _ASSERT(initialized);

    D3DPERF_BeginEvent(0xFFFFFFFF, L"SpriteRenderer Render");

    // Set the vertex shader
    context->VSSetShader(vertexShader, NULL, 0);

    // Set the input layout
    context->IASetInputLayout(inputLayout);

    // Set the vertex buffer
    UINT stride = sizeof(SpriteVertex);
    UINT offset = 0;
    ID3D11Buffer* vb = vertexBuffer.GetInterfacePtr();
    context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);

    // Set per-batch constants
    D3D11_TEXTURE2D_DESC desc = SetPerBatchData(texture);

    // Set per-instance data
    SpriteDrawData perInstance;
    perInstance.Transform = XMMatrixTranspose(transform);
    perInstance.Color = color;

    // Draw rect
    if (drawRect == NULL)
        perInstance.DrawRect = XMFLOAT4(0, 0, static_cast<float>(desc.Width), static_cast<float>(desc.Height));
    else
    {
        _ASSERT(drawRect->x >= 0 && drawRect->x < desc.Width);
        _ASSERT(drawRect->y >= 0 && drawRect->y < desc.Height);
        _ASSERT(drawRect->z > 0 && drawRect->x + drawRect->z < desc.Width);
        _ASSERT(drawRect->w > 0 && drawRect->y + drawRect->w < desc.Height);
        perInstance.DrawRect = *drawRect;
    }

    // Copy in the buffer data
    D3D11_MAPPED_SUBRESOURCE mapped;
    DXCall(context->Map(vsPerInstanceCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
    CopyMemory(mapped.pData, &perInstance, sizeof(SpriteDrawData));
    context->Unmap(vsPerInstanceCB, 0);

    ID3D11Buffer* buffers [2] = { vsPerBatchCB, vsPerInstanceCB };
    context->VSSetConstantBuffers(0, 2, buffers);

    // Set the texture
    context->PSSetShaderResources(0, 1, &texture);

    context->DrawIndexed(6, 0, 0);

    D3DPERF_EndEvent();
}

void SpriteRenderer::RenderBatch(ID3D11ShaderResourceView* texture,
                                 const SpriteDrawData* drawData,
                                 UINT64 numSprites)
{
    _ASSERT(context);
    _ASSERT(initialized);

    D3DPERF_BeginEvent(0xFFFFFFFF, L"SpriteRenderer RenderBatch");

    // Set the vertex shader
    context->VSSetShader(vertexShaderInstanced, NULL, 0);

    // Set the input layout
    context->IASetInputLayout(inputLayoutInstanced);

    // Set per-batch constants
    D3D11_TEXTURE2D_DESC desc = SetPerBatchData(texture);

    // Make sure the draw rects are all valid
    for (UINT64 i = 0; i < numSprites; ++i)
    {
        XMFLOAT4 drawRect = drawData[i].DrawRect;
        _ASSERT(drawRect.x >= 0 && drawRect.x < desc.Width);
        _ASSERT(drawRect.y >= 0 && drawRect.y < desc.Height);
        _ASSERT(drawRect.z > 0 && drawRect.x + drawRect.z <= desc.Width);
        _ASSERT(drawRect.w > 0 && drawRect.y + drawRect.w <= desc.Height);
    }

    UINT64 numSpritesToDraw = min(numSprites, MaxBatchSize);

    // Copy in the instance data
    D3D11_MAPPED_SUBRESOURCE mapped;
    DXCall(context->Map(instanceDataBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
    CopyMemory(mapped.pData, drawData, static_cast<size_t>(sizeof(SpriteDrawData) * numSpritesToDraw));
    context->Unmap(instanceDataBuffer, 0);

    // Set the constant buffer
    ID3D11Buffer* constantBuffers [1] = { vsPerBatchCB };
    context->VSSetConstantBuffers(0, 1, constantBuffers);

    // Set the vertex buffers
    UINT strides [2] = { sizeof(SpriteVertex), sizeof(SpriteDrawData) };
    UINT offsets [2] = { 0, 0 };
    ID3D11Buffer* vertexBuffers [2] = { vertexBuffer, instanceDataBuffer };
    context->IASetVertexBuffers(0, 2, vertexBuffers, strides, offsets);

    // Set the texture
    context->PSSetShaderResources(0, 1, &texture);

    // Draw
    context->DrawIndexedInstanced(6, static_cast<UINT>(numSpritesToDraw), 0, 0, 0);

    D3DPERF_EndEvent();

    // If there's any left to be rendered, do it recursively
    if(numSprites > numSpritesToDraw)
        RenderBatch(texture, drawData + numSpritesToDraw, numSprites - numSpritesToDraw);
}

void SpriteRenderer::RenderText(const SpriteFont& font,
                                const WCHAR* text,
            				    const XMMATRIX& transform,
                                const XMFLOAT4& color)
{
    D3DPERF_BeginEvent(0xFFFFFFFF, L"SpriteRenderer RenderText");

    size_t length = wcslen(text);

    XMMATRIX textTransform = XMMatrixIdentity();

    UINT64 numCharsToDraw = min(length, MaxBatchSize);
    UINT64 currentDraw = 0;
    for (UINT64 i = 0; i < numCharsToDraw; ++i)
    {
        WCHAR character = text[i];
        if(character == ' ')
            textTransform._41 += font.SpaceWidth();
        else if(character == '\n')
        {
            textTransform._42 += font.CharHeight();
            textTransform._41 = 0;
        }
        else
        {
            SpriteFont::CharDesc desc = font.GetCharDescriptor(character);

            textDrawData[currentDraw].Transform = XMMatrixMultiply(textTransform, transform);
            textDrawData[currentDraw].Color = color;
            textDrawData[currentDraw].DrawRect.x = desc.X;
            textDrawData[currentDraw].DrawRect.y = desc.Y;
            textDrawData[currentDraw].DrawRect.z = desc.Width;
            textDrawData[currentDraw].DrawRect.w = desc.Height;
            currentDraw++;

            textTransform._41 += desc.Width + 1;
        }
    }

    // Submit a batch
    RenderBatch(font.SRView(), textDrawData, currentDraw);

    D3DPERF_EndEvent();

    if(length > numCharsToDraw)
        RenderText(font, text + numCharsToDraw, textTransform, color);
}

void SpriteRenderer::End()
{
    _ASSERT(context);
    _ASSERT(initialized);
    context = NULL;

    D3DPERF_EndEvent();
}

}