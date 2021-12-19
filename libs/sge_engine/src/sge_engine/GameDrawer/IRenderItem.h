#pragma once

#include "sge_engine/GameDrawer/GameDrawer.h"

namespace sge {

struct ObjectLighting;

/// IRenderItem is an interface providing a shell around a particular thing around rendering.
/// It's idea is to provide a way each actor to specify all of its different rendering components.
/// Then the renderer based on these components the renderer will render the render item.
/// 
/// The interface was introduced for a way to do alpha-sorting.
/// Imagine a scene where we have an object inside a semi-transparent sphere - like the one in the Super Monkey Ball games.
/// Imagine that "Mokey Ball" is one FBX file. As we've said the ball is semi transparent, in order to to the alpha blending correctly
/// we must take all different parts of an object (in the case of the mokey ball the monkey and the semi-transparent ball) 
/// separate the opaque from semi-transparent, and then sort semi-transparent in back-to-front order (relative to the camera).
/// 
/// However the interface is not limited to usages like the one described above.
/// It could be used to render sub-objects, like if we have one object that specified a big patch of fooliage, we could
/// break the actual fooliage into smaller cunks and have one render item for each chunk - by doing this we will get frustum culling.
struct IRenderItem {
	IRenderItem() = default;
	virtual ~IRenderItem() = default;

	// Values that must be filled by user:

	/// Must be filled by user. The point in world space to be used for zSorting.
	/// Even if render item does not need alpha blending it is good to compute it
	/// as this is used for fron-to-back rendering of opaque object to reduce the
	/// overdraw done by pixel shaders.
	vec3f zSortingPositionWs = vec3f(0.f); 

	/// Must be filled by user.  True if the item needs to be sorted for correct alpha blending.
	bool needsAlphaSorting = false; 

public:
	/// Internal values used at a later stage.
	/// Do not specify, let the GameDrawer do it.
	
	/// A value computed later, it is used for sorting objects based on the camera orientation and @zSortingPositionWs.
	float zSortingValue = 0; 
};

} // namespace sge
