#pragma once

#include "typeLib.h"
#include <functional>

namespace sge {

struct MemberFieldChainKnot {
	MemberFieldChainKnot() = default;
	MemberFieldChainKnot(const MemberDesc* mfd, int arrayIdx = -1)
	    : mfd(mfd)
	    , arrayIdx(arrayIdx)
	{
	}

	const MemberDesc* mfd = nullptr;

	// If the type of the member is array or std::vector this thing is isused to point at the element, if not this is just -1.
	int arrayIdx = -1;
};

/// MemberChain is a way to refer to any member (for example member of a member of a member ...) in some memory.
/// For example:
/// suppouse we have these types:
/// 
/// struct Health { float amountOfHealth; }
/// struct Monster { Health health; }
/// 
/// And we want, via the reflection system, to find that member value.
/// A way of doing that is by using MemberChain:
/// 
/// MemberChain chain;
/// chain.add(sgeFindMember(Monster, health); // We are going to start with an object of type Monster and its member health.
/// chain.add(sgeFindMember(Health, amountOfHealth); // Then at the value pointed above, we want to reach out to it's @amountOfHealth member.
/// 
/// After that we can call:
/// Monster m;
/// m.health.amountOfHealth = 42.f;
/// float* ptr = (float*)chain.follow(&m);
/// *ptr points to the correct memory location and we can use it however we want.
struct SGE_CORE_API MemberChain {
	MemberChain() = default;

	// MemberChain(std::initializer_list<MemberFieldChainKnot> l) {
	//	for (const MemberFieldChainKnot& k : l) {
	//		const bool success = add(k);
	//		if (success == false) {
	//			sgeAssert(false);
	//			knots.clear();
	//			break;
	//		}
	//	}
	//}

	MemberChain(const MemberDesc* mfd, int arrayIdx = -1) { knots.emplace_back(MemberFieldChainKnot(mfd, arrayIdx)); }

	const TypeDesc* getType() const
	{
		if (knots.size() == 0)
			return nullptr;
		const TypeDesc* const type = typeLib().find(knots.back().mfd->typeId);
		if (type == nullptr)
			return nullptr;

		if (knots.back().arrayIdx != -1 && type->stdVectorUnderlayingType.isValid())
			return typeLib().find(type->stdVectorUnderlayingType);

		return type;
	}

	/// Find the type of the end value of the chain.
	TypeId getTypeId() const
	{
		const TypeDesc* type = getType();
		if (type == nullptr) {
			sgeAssert(false && "Is the chain broken? or is the reflection missing something?");
			return TypeId();
		}

		return type->typeId;
	}

	/// Instructs that the chain should next go to the specified member @mfd.
	bool add(const MemberDesc* mfd, int idx = -1) { return add(MemberFieldChainKnot(mfd, idx)); }

	/// Instructs that the chain should next go to the specified member @mfd.
	bool add(const MemberFieldChainKnot& knot);

	const MemberDesc* getMemberDescIfNotIndexing() const
	{
		if (knots.size() == 0 || knots.back().arrayIdx >= 0)
			return nullptr;

		return knots.back().mfd;
	}

	/// Removes the last knot in the chain.
	void pop();

	/// Makes the chain empty.
	void clear();

	/// Find the end value adress pointed by the chain.
	/// The function believes you that root is the correct type.
	void* follow(void* root) const;

	void forEachMember(void* root, std::function<void(void* root, const MemberChain&)>& lambda);

  public:
	void forEachMemberInternal(void* root, MemberChain chain, std::function<void(void* root, const MemberChain&)>& lambda);

	std::vector<MemberFieldChainKnot> knots;
};


} // namespace sge
