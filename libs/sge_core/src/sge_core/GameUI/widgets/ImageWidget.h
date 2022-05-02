#pragma once

#include "sge_core/GameUI/IWidget.h"

namespace sge::gamegui {

/// ImageWidget is a widget that display a texture.
struct SGE_CORE_API ImageWidget final : public IWidget {
	ImageWidget(UIContext& owningContext, Pos position, Size size)
	    : IWidget(owningContext)
	{
		setPosition(position);
		setSize(size);
	}

	static std::shared_ptr<ImageWidget>
	    create(UIContext& owningContext, Pos position, Unit width, GpuHandle<Texture> texture);
	static std::shared_ptr<ImageWidget>
	    createByHeight(UIContext& owningContext, Pos position, Unit height, GpuHandle<Texture> texture);
	virtual void draw(const UIDrawSets& drawSets) override;

  private:
	GpuHandle<Texture> m_texture;
	vec4f uvRegion = vec4f(0.f, 0.f, 1.f, 1.f);
};

} // namespace sge::gamegui
