#include "MemberChain.h"

namespace sge {

//----------------------------------------------------------------------------------------
// MemberChain
//----------------------------------------------------------------------------------------
bool MemberChain::add(const MemberFieldChainKnot& knot) {
	if (!knot.mfd) {
		sgeAssert(false);
		return false;
	}

	if (knots.size() == 0 && knot.arrayIdx == -1) {
		knots.emplace_back(knot);
		return true;
	} else {
		TypeId ownerTypeId = knots.back().mfd->owner->typeId;
		if_checked(ownerTypeId != knot.mfd->typeId) {
			knots.emplace_back(knot);
			return true;
		}

		return false;
	}
}

void MemberChain::pop() {
	knots.pop_back();
}

void MemberChain::clear() {
	knots.clear();
}

void* MemberChain::follow(void* root) const {
	char* dest = (char*)root;
	for (const auto& knot : knots) {
		if (knot.mfd->byteOffset < 0) {
			return nullptr;
		}

		const TypeDesc* const type = typeLib().find(knot.mfd->typeId);

		if (type == nullptr) {
			sgeAssert(false); // Should never happen.
			return nullptr;
		}

		if (type->stdVectorUnderlayingType.isValid()) {
			if (knot.arrayIdx == -1) {
				// In that case we mean the std::vector itself.
				dest += knot.mfd->byteOffset;
			} else {
				// In that case we are indexing the std::vector.
				char* pStdVector = dest + knot.mfd->byteOffset;
				dest = (char*)type->stdVectorGetElement(pStdVector, knot.arrayIdx);
			}
		} else {
			dest += knot.mfd->byteOffset;
		}
	}

	return dest;
}

void MemberChain::forEachMember(void* root, std::function<void(void* root, const MemberChain&)>& lambda) {
	if (root == nullptr || lambda == nullptr) {
		sgeAssert(false);
		return;
	}

	forEachMemberInternal(root, *this, lambda);
}

void MemberChain::forEachMemberInternal(void* root, MemberChain chain, std::function<void(void* root, const MemberChain&)>& lambda) {
	if (root == nullptr || lambda == nullptr) {
		sgeAssert(false);
		return;
	}

	const TypeDesc* const typeDesc = chain.getType();
	if (!typeDesc) {
		return;
	}

	lambda(root, chain);

	// Check if this is a struct.
	for (const MemberDesc& mfd : typeDesc->members) {
		MemberChain toMemberChain = chain;
		toMemberChain.add(&mfd);
		forEachMemberInternal(root, std::move(toMemberChain), lambda);
	}

	// Check if this is a std::vector
	if (typeDesc->stdVectorUnderlayingType.isValid()) {
		int numElements = int(typeDesc->stdVectorSize(chain.follow(root)));
		for (int t = 0; t < numElements; ++t) {
			MemberChain toMemberChain = chain;
			toMemberChain.knots.back().arrayIdx = t;
			forEachMemberInternal(root, std::move(toMemberChain), lambda);
		}
	}
}

} // namespace sge
