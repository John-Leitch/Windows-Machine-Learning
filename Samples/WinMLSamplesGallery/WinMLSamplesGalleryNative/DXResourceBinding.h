#pragma once
#include "DXResourceBinding.g.h"

namespace winrt::WinMLSamplesGalleryNative::implementation
{
    struct DXResourceBinding : DXResourceBindingT<DXResourceBinding>
    {
        DXResourceBinding() = default;
        static int BindDXResourcesUsingORT();
    };
}
namespace winrt::WinMLSamplesGalleryNative::factory_implementation
{
    struct DXResourceBinding : DXResourceBindingT<DXResourceBinding, implementation::DXResourceBinding>
    {
    };
}
