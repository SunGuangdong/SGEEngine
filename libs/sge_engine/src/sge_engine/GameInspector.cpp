#include "GameInspector.h"
#include "sge_engine/EngineGlobal.h"
#include "sge_engine/InspectorCmds.h"
#include "sge_engine/traits/TraitCamera.h"
#include "sge_utils/text/format.h"

namespace sge {

//--------------------------------------------------------------------
// GameInspector
//--------------------------------------------------------------------
GameInspector::GameInspector()
{
	clear();
}

bool GameInspector::isSteppingAllowed() const
{
	return !m_disableAutoStepping || m_stepOnce;
}

void GameInspector::redoCommand()
{
	if ((m_lastExecutedCommandIdx + 1 < m_commandHistory.size()) && (m_commandHistory.size() != 0)) {
		std::string cmdText;
		m_commandHistory[m_lastExecutedCommandIdx + 1]->getText(cmdText);
		getEngineGlobal()->showNotification(string_format("Redo '%s'", cmdText.c_str()));

		m_commandHistory[m_lastExecutedCommandIdx + 1]->redo(this);
		m_lastExecutedCommandIdx++;
	}
}

void GameInspector::undoCommand()
{
	if (m_lastExecutedCommandIdx < m_commandHistory.size() && m_lastExecutedCommandIdx >= 0) {
		std::string cmdText;
		m_commandHistory[m_lastExecutedCommandIdx]->getText(cmdText);
		getEngineGlobal()->showNotification(string_format("Undo '%s'", cmdText.c_str()));

		m_commandHistory[m_lastExecutedCommandIdx]->undo(this);
		m_lastExecutedCommandIdx--;
	}
}


void GameInspector::appendCommand(InspectorCmd* const cmd, bool shouldApply)
{
	if (cmd) {
		m_commandHistory.resize(m_lastExecutedCommandIdx + 1);
		m_commandHistory.emplace_back(cmd);
		m_lastExecutedCommandIdx = int(m_commandHistory.size()) - 1;

		if (shouldApply) {
			cmd->apply(this);
		}

		std::string notification;
		cmd->getText(notification);
		getEngineGlobal()->showNotification(std::move(notification));
	}
}

bool GameInspector::isSelected(ObjectId const id, bool* const outIsPrimary) const
{
	if (outIsPrimary) {
		*outIsPrimary = false;
	}

	for (size_t t = 0; t < m_selection.size(); ++t) {
		if (m_selection[t].editMode == editMode_actors && m_selection[t].objectId == id) {
			if (outIsPrimary) {
				*outIsPrimary = (t == 0);
			}

			return true;
		}
	}

	return false;
}

bool GameInspector::isPrimarySelected(ObjectId const id) const
{
	if (m_selection.empty() == false) {
		if (m_selection[0].editMode == editMode_actors && m_selection[0].objectId == id) {
			return true;
		}
	}

	return false;
}

ObjectId GameInspector::getPrimarySelection() const
{
	if (m_selection.empty())
		return ObjectId();

	return m_selection[0].objectId;
}

ObjectId GameInspector::getSecondarySelection() const
{
	if (m_selection.size() < 2) {
		return ObjectId();
	}

	return m_selection[1].objectId;
}

ObjectId GameInspector::getSecondarySelectedActor() const
{
	if (m_selection.size() < 2) {
		return ObjectId();
	}

	if (m_selection[1].editMode != editMode_actors) {
		return ObjectId();
	}

	return m_selection[1].objectId;
}

void GameInspector::getAllSelectedObjects(vector_set<ObjectId>& allActors)
{
	for (size_t t = 0; t < m_selection.size(); ++t) {
		if (m_selection[t].editMode == editMode_actors) {
			allActors.insert(m_selection[t].objectId);
		}
	}
}

void GameInspector::select(ObjectId const id, bool const selectAsPrimary)
{
	if (selectAsPrimary) {
		if (isSelected(id)) {
			deselect(id);
		}

		if (m_selection.empty())
			m_selection.push_back(SelectedItem(id));
		else
			m_selection.emplace(m_selection.begin(), SelectedItem(id));

		m_selectionChangeIdx++;
	}
	else {
		if (!isSelected(id)) {
			m_selection.push_back(SelectedItem(id));
			m_selectionChangeIdx++;
		}
	}
}

void GameInspector::deselect(ObjectId const id)
{
	for (int t = 0; t < m_selection.size(); ++t) {
		if (m_selection[t].editMode == editMode_actors && id == m_selection[t].objectId) {
			m_selection.erase(m_selection.begin() + t);
			m_selectionChangeIdx++;
			return;
		}
	}
}

void GameInspector::toggleSelected(ObjectId const id)
{
	for (int t = 0; t < m_selection.size(); ++t) {
		if (m_selection[t].editMode == editMode_actors && id == m_selection[t].objectId) {
			m_selection.erase(m_selection.begin() + t);
			m_selectionChangeIdx++;
			return;
		}
	}

	m_selection.push_back(SelectedItem(id));
	m_selectionChangeIdx++;
}

void GameInspector::deselectAll()
{
	m_selection.clear();
	m_selectionChangeIdx++;
}

void GameInspector::duplicateSelection(vector_set<ObjectId>* outNewObjects)
{
	if (m_selection.size() == 0) {
		return;
	}

	vector_set<ObjectId> objectsToDuplicate;
	for (int t = 0; t < m_selection.size(); ++t) {
		if (m_selection[t].editMode == editMode_actors) {
			objectsToDuplicate.insert(m_selection[t].objectId);
		}
	}

	GameWorld prefabWorld;
	getWorld()->createPrefab(prefabWorld, false, &objectsToDuplicate);
	vector_set<ObjectId> newObjects;
	getWorld()->instantiatePrefab(prefabWorld, true, true, nullptr, &newObjects);

	// Now select the newly created objects.
	deselectAll();
	for (int t = 0; t < newObjects.size(); ++t) {
		select(newObjects.getNth(t));
	}

	if (outNewObjects) {
		*outNewObjects = newObjects;
	}
}

void GameInspector::deleteSelection(bool const deleteHierarchyUnderSelectedObjects)
{
	CmdCompound* const compoundCmd = new CmdCompound();

	std::map<GameObject*, vector_set<SelectedItem>> perObjectSelection;

	for (const SelectedItem& sel : m_selection) {
		// Filter by the current mode to avoid complications.
		if (sel.editMode != editMode) {
			continue;
		}

		GameObject* const object = m_world->getObjectById(sel.objectId);
		if_checked(object) { perObjectSelection[object].add(sel); }
	}

	deselectAll();

	// If all object that we are going to delete are actors (which is a really common case)
	// Batch them in a single command as a simple optimization.
	if (editMode == editMode_actors) {
		vector_set<ObjectId> actorsToDelete;
		for (auto& pair : perObjectSelection) {
			if (deleteHierarchyUnderSelectedObjects) {
				getWorld()->getAllChildren(actorsToDelete, pair.first->getId());
			}
			actorsToDelete.insert(pair.first->getId());
		}

		CmdObjectDeletion* cmd = new CmdObjectDeletion;
		cmd->setupDeletion(*getWorld(), actorsToDelete);
		appendCommand(cmd, true);
	}
	else {
		for (auto& itr : perObjectSelection) {
			Actor* actor = dynamic_cast<Actor*>(itr.first);
			InspectorCmd* cmd = nullptr;
			if (actor) {
				cmd = actor->generateDeleteItemCmd(this, itr.second.data(), itr.second.size(), deleteHierarchyUnderSelectedObjects);
			}
			else {
				CmdObjectDeletion* cmdDel = new CmdObjectDeletion;
				vector_set<ObjectId> objToDel;
				objToDel.insert(itr.first->getId());
				cmdDel->setupDeletion(*getWorld(), objToDel);
				cmd = cmdDel;
			}
			if (cmd) {
				compoundCmd->addCommand(cmd);
			}
		}

		appendCommand(compoundCmd, true);
	}
}

void GameInspector::focusOnSelection()
{
	if (m_selection.size() == 0) {
		getWorld()->m_editorCamera.m_orbitCamera.orbitPoint = vec3f(0.f);
		getWorld()->m_editorCamera.m_orbitCamera.radius = 27.0f;
		return;
	}

	Box3f combinedBoundingBox;
	vec3f averagePosition = vec3f(0.f);

	for (int t = 0; t < m_selection.size(); ++t) {
		Actor* const actor = m_world->getActorById(m_selection[t].objectId);
		if (actor == nullptr)
			continue;

		if (m_selection[t].editMode == editMode_actors) {
			Box3f const box = actor->getBBoxOS().getTransformed(actor->getTransform().toMatrix());
			if (!box.IsEmpty())
				combinedBoundingBox.expand(box);
			else
				combinedBoundingBox.expand(actor->getTransform().p);
		}
		else {
			transf3d tr;
			actor->getItemTransform(tr, m_selection[t].editMode, m_selection[t].index);
			combinedBoundingBox.expand(tr.p);
		}
	}

	averagePosition *= 1.f / (float)m_selection.size();

	// TODO: Figure out a better way to update the camera this is hacky.
	if (!combinedBoundingBox.IsEmpty()) {
		getWorld()->m_editorCamera.m_orbitCamera.orbitPoint = combinedBoundingBox.center();
		getWorld()->m_editorCamera.m_orbitCamera.radius = combinedBoundingBox.halfDiagonal().length() * 2.7f;

		if (getWorld()->m_editorCamera.m_orbitCamera.radius < 2.7f) {
			getWorld()->m_editorCamera.m_orbitCamera.radius = 2.7f;
		}
	}

	sgeAssert(getWorld()->m_editorCamera.m_orbitCamera.orbitPoint.hasNan() == false);
	sgeAssert(std::isnan(getWorld()->m_editorCamera.m_orbitCamera.radius) == false);
}


} // namespace sge
