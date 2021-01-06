#include "rnd_Tensor.h"

void rnd_InputTensor::AssignData(std::vector<char>& data)
{
   cpuBuffer = std::move(data);
}

void rnd_InputTensor::OnInit(LPCWSTR name)
{

}

void rnd_DynamicTensor::OnInit(LPCWSTR name)
{

}
