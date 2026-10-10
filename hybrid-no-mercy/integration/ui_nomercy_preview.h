#pragma once
#include <RmlUi/Core.h>
namespace recompui {
class ElementNoMercyPreview : public Rml::Element {
public:
    explicit ElementNoMercyPreview(const Rml::String& tag);
protected:
    void OnRender() override;
};
void nomercy_preview_select(int index);
void register_nomercy_images();
}
