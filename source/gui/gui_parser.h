#pragma once

#include "gui_params.h"

namespace GUI
{
  class CImageWidget;
  class CEffect;

  class CParser
  {
  public:
    void parseFile(const std::string& file);
    CWidget* parseWidget(const json& data, CWidget* parent);

    CWidget* parseWidget(const json& data);
    CWidget* parseImage(const json& data);
    CWidget* parseText(const json& data);
    CWidget* parseButton(const json& data);
    CWidget* parseBar(const json& data);

    CEffect* parseEffect(const json& data, CWidget* wdgt);
    CEffect* parseAnimateUVEffect(const json& data);

    void parseParams(TParams& params, const json& data);
    void parseImageParams(TImageParams& params, const json& data);
    void parseTextParams(TTextParams& params, const json& data);
    void parseBarParams(TBarParams& params, const json& data);
  };
}