#pragma once
#include <d3d11.h>
#include <string>

class Renderer;

class ComputeBenchmarker {
public:
    ComputeBenchmarker(Renderer* renderer);
    ~ComputeBenchmarker();

    bool LoadKernel(int testMode);
    bool InitializeBuffers(int elements);
    void Dispatch(int dispatchSize);

private:
    Renderer* m_renderer;
    
    ID3D11ComputeShader* m_pComputeShader;
    ID3D11Buffer* m_pBufferInA;
    ID3D11Buffer* m_pBufferInB;
    ID3D11Buffer* m_pBufferOutC;
    ID3D11ShaderResourceView* m_pSRV_A;
    ID3D11ShaderResourceView* m_pSRV_B;
    ID3D11UnorderedAccessView* m_pUAV_C;
};
