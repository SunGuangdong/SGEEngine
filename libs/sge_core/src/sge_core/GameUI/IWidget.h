#include <memory>

#include "sge_core/sgecore_api.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/math/Box2f.h"

#include "Sizing.h"

namespace sge::gamegui {

struct UIContext;
struct UIDrawSets;

/// @brief IWidget is the base interface for all widget.
struct SGE_CORE_API IWidget : public std::enable_shared_from_this<IWidget> {
	friend UIContext;

	IWidget(UIContext& owningContext)
	    : m_owningContext(owningContext)
	{
	}
	virtual ~IWidget() {}

	virtual bool isGamepadTargetable() { return false; }

	virtual bool onHoverEnter() { return false; }
	virtual bool onHoverLeave() { return false; }
	virtual bool onPress() { return false; }
	virtual bool onMouseWheel(int UNUSED(cnt)) { return false; }

	/// Called only if the press happend.
	virtual void onRelease(bool UNUSED(wasReleaseInside)) {}

	virtual void draw(const UIDrawSets& drawSets) = 0;
	virtual void update() {}

	Pos getPosition() const { return m_position; }
	Pos& getPosition() { return m_position; }

	void setPosition(Pos newPos) { m_position = newPos; }

	Size getSize() const { return m_size; }
	Size& getSize() { return m_size; }

	void setSize(Size newSize) { m_size = newSize; }

	Box2f getBBoxPixelsSS() const
	{
		Box2f parentBBoxSS = getParentBBoxSS();
		const Box2f bboxSS =
		    m_position.getBBoxPixelsSS(parentBBoxSS, getParentContentOrigin().toPixels(parentBBoxSS.size()), m_size);
		return bboxSS;
	}

	Box2f getBBoxPixelsParentSpace() const
	{
		Box2f parentBBoxSS = getParentBBoxSS();
		const Box2f bboxSS =
		    m_position.getBBoxPixelsSS(parentBBoxSS, getParentContentOrigin().toPixels(parentBBoxSS.size()), m_size);

		Box2f bboxParentSpacePixels = bboxSS;
		bboxParentSpacePixels.move(-parentBBoxSS.min);

		return bboxParentSpacePixels;
	}

	Box2f getScissorBoxSS() const;
	Rect2s getScissorRect() const;

	void addChild(std::shared_ptr<IWidget> widget);

	template <typename TWidget>
	std::shared_ptr<TWidget> addChildT(std::shared_ptr<TWidget> widget)
	{
		addChild(widget);
		return widget;
	}

	const std::vector<std::shared_ptr<IWidget>>& getChildren() const { return m_children; }

	UIContext& getContext() { return m_owningContext; }
	const UIContext& getContext() const { return m_owningContext; }

	void suspend() { m_isSuspended = true; }
	void unsuspend() { m_isSuspended = false; }
	bool isSuspended() const
	{
		if (m_isSuspended) {
			return true;
		}

		auto parent = getParent();
		return (parent && parent->isSuspended());
	}

	std::shared_ptr<IWidget> getParent() { return m_parentWidget.lock(); }

	std::shared_ptr<IWidget> getParent() const { return m_parentWidget.lock(); }

	const Pos& getContentsOrigin() const { return m_contentsOrigin; }
	void setContentsOrigin(const Pos& o) { m_contentsOrigin = o; }

	float getOpacity() const { return opacity; }
	void setOpacity(float opacity) { this->opacity = opacity; }

	float calcTotalOpacity()
	{
		std::shared_ptr<IWidget> parent = getParent();
		if (parent) {
			return parent->calcTotalOpacity() * opacity;
		}
		return opacity;
	}

  protected:
	Box2f getParentBBoxSS() const;
	Pos getParentContentOrigin() const;

  public:
	float opacity = 1.f;
	Pos m_position;
	Size m_size;
	bool m_isSuspended = false;
	Pos m_contentsOrigin;

  protected:
	bool m_wasHoveredPrevFrame = false;

	UIContext& m_owningContext;
	std::weak_ptr<IWidget> m_parentWidget;
	std::vector<std::shared_ptr<IWidget>> m_children;
};


} // namespace sge::gamegui
