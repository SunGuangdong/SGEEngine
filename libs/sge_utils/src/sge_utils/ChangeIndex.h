#pragma once

namespace sge {

/// ChangeIndex is a way to to track if a change to some other part of the software has changed.
/// It is a workaround in the situation where we do not have callbacks to notify that a change has happened.
/// The way this works, when a change occures, the "changer" needs to call markAChange().
/// Later, during a function that needs to see if any changes have occured 
/// you could call @checkForChangeAndUpdate.
/// and if the function returns true than that means 
/// a unprocessed change has happened since the last call of the function.
struct ChangeIndex {
	/// Constructs a change index that has a change in it.
	/// So @checkForChangeAndUpdate would return true if we call it after the constructor.
	ChangeIndex() = default;

	/// Marks that a change accured so the others interested could check if a change has happened with 
	/// @checkForChangeAndUpdate. 
	void markAChange() {
		upToDateValue++;
	}

	/// Returns true if a change was marked with @markAChange witch accured after the last call to @checkForChangeAndUpdate.
	bool checkForChangeAndUpdate() {
		bool hasChanged = upToDateValue != lastCheckedValued;
		lastCheckedValued = upToDateValue;
		return hasChanged;
	}

	/// Check if the specified @externalTracker is up-to-date with the change index.
	/// In order for it to work correctly, initialize you externalTracker to zero before starting to call this function.
	bool checkForChangeAndUpdate(unsigned& externalTracker) const {
		bool hasChanged = upToDateValue != externalTracker;
		externalTracker = upToDateValue;
		return hasChanged;
	}

  private:
	/// Hold the value that represents the latest change.
	/// Other values are compared and updated accoding to this one.
	unsigned upToDateValue = 1;

	/// Holds what was the value when we last called @checkForChangeAndUpdate.
	unsigned lastCheckedValued = 0;
};


} // namespace sge
