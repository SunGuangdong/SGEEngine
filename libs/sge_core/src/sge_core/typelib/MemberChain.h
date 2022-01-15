#pragma once

#include <functional>
#include "typeLib.h"

namespace sge {

/// MemberChain
/// Is a way to refer to a remote member (member of a member of a member ...)
/// in some memory. The structure belives you that the chain start from the correct type.
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

	MemberChain(const MemberDesc* mfd, int arrayIdx = -1) {
		knots.emplace_back(MemberFieldChainKnot(mfd, arrayIdx));
	}

	const TypeDesc* getType() const {
		if (knots.size() == 0)
			return nullptr;
		const TypeDesc* const type = typeLib().find(knots.back().mfd->typeId);
		if (type == nullptr)
			return nullptr;

		if (knots.back().arrayIdx != -1 && type->stdVectorUnderlayingType.isValid())
			return typeLib().find(type->stdVectorUnderlayingType);

		return type;
	}

	TypeId getTypeId() const {
		const TypeDesc* type = getType();
		if (type == nullptr) {
			sgeAssert(false && "Is the chain broken? or is the reflection missing something?");
			return TypeId();
		}

		return type->typeId;
	}

	bool add(const MemberDesc* mfd, int idx = -1) {
		return add(MemberFieldChainKnot(mfd, idx));
	}
	bool add(const MemberFieldChainKnot& knot);

	const MemberDesc* getMemberDescIfNotIndexing() const {
		if (knots.size() == 0 || knots.back().arrayIdx >= 0)
			return nullptr;

		return knots.back().mfd;
	}

	void pop();
	void clear();

	void* follow(void* root) const;

	void forEachMember(void* root, std::function<void(void* root, const MemberChain&)>& lambda);

  public:
	void forEachMemberInternal(void* root, MemberChain chain, std::function<void(void* root, const MemberChain&)>& lambda);

	std::vector<MemberFieldChainKnot> knots;
};


} // namespace sge
