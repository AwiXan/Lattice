/**************************************************************************/
/*  test_packed_scene.cpp                                                 */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "tests/test_macros.h"

TEST_FORCE_LINK(test_packed_scene)

#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/object/callable_mp.h"
#include "scene/resources/packed_scene.h"
#include "tests/test_utils.h"

namespace TestPackedScene {

TEST_CASE("[PackedScene] Pack Scene and Retrieve State") {
	// Create a scene to pack.
	Node *scene = memnew(Node);
	scene->set_name("TestScene");

	// Pack the scene.
	PackedScene packed_scene;
	const Error err = packed_scene.pack(scene);
	CHECK(err == OK);

	// Retrieve the packed state.
	Ref<SceneState> state = packed_scene.get_state();
	CHECK(state.is_valid());
	CHECK(state->get_node_count() == 1);
	CHECK(state->get_node_name(0) == "TestScene");

	memdelete(scene);
}

TEST_CASE("[PackedScene] Signals Preserved when Packing Scene") {
	// Create main scene
	// root
	// `- sub_node (local)
	// `- sub_scene (instance of another scene)
	//    `- sub_scene_node (owned by sub_scene)
	Node *main_scene_root = memnew(Node);
	Node *sub_node = memnew(Node);
	Node *sub_scene_root = memnew(Node);
	Node *sub_scene_node = memnew(Node);

	main_scene_root->add_child(sub_node);
	sub_node->set_owner(main_scene_root);

	sub_scene_root->add_child(sub_scene_node);
	sub_scene_node->set_owner(sub_scene_root);

	main_scene_root->add_child(sub_scene_root);
	sub_scene_root->set_owner(main_scene_root);

	SUBCASE("Signals that should be saved") {
		int main_flags = Object::CONNECT_PERSIST;
		// sub node to a node in main scene
		sub_node->connect("ready", callable_mp(main_scene_root, &Node::is_ready), main_flags);
		// subscene root to a node in main scene
		sub_scene_root->connect("ready", callable_mp(main_scene_root, &Node::is_ready), main_flags);
		//subscene root to subscene root (connected within main scene)
		sub_scene_root->connect("ready", callable_mp(sub_scene_root, &Node::is_ready), main_flags);

		// Pack the scene.
		Ref<PackedScene> packed_scene;
		packed_scene.instantiate();
		const Error err = packed_scene->pack(main_scene_root);
		CHECK(err == OK);

		// Make sure the right connections are in packed scene.
		Ref<SceneState> state = packed_scene->get_state();
		CHECK_EQ(state->get_connection_count(), 3);
	}

	/*
	// FIXME: This subcase requires GH-48064 to be fixed.
	SUBCASE("Signals that should not be saved") {
		int subscene_flags = Object::CONNECT_PERSIST | Object::CONNECT_INHERITED;
		// subscene node to itself
		sub_scene_node->connect("ready", callable_mp(sub_scene_node, &Node::is_ready), subscene_flags);
		// subscene node to subscene root
		sub_scene_node->connect("ready", callable_mp(sub_scene_root, &Node::is_ready), subscene_flags);
		//subscene root to subscene root (connected within sub scene)
		sub_scene_root->connect("ready", callable_mp(sub_scene_root, &Node::is_ready), subscene_flags);

		// Pack the scene.
		Ref<PackedScene> packed_scene;
		packed_scene.instantiate();
		const Error err = packed_scene->pack(main_scene_root);
		CHECK(err == OK);

		// Make sure the right connections are in packed scene.
		Ref<SceneState> state = packed_scene->get_state();
		CHECK_EQ(state->get_connection_count(), 0);
	}
	*/

	memdelete(main_scene_root);
}

TEST_CASE("[PackedScene] Clear Packed Scene") {
	// Create a scene to pack.
	Node *scene = memnew(Node);
	scene->set_name("TestScene");

	// Pack the scene.
	PackedScene packed_scene;
	packed_scene.pack(scene);

	// Clear the packed scene.
	packed_scene.clear();

	// Check if it has been cleared.
	Ref<SceneState> state = packed_scene.get_state();
	CHECK_FALSE(state->get_node_count() == 1);

	memdelete(scene);
}

TEST_CASE("[PackedScene] Can Instantiate Packed Scene") {
	// Create a scene to pack.
	Node *scene = memnew(Node);
	scene->set_name("TestScene");

	// Pack the scene.
	PackedScene packed_scene;
	packed_scene.pack(scene);

	// Check if the packed scene can be instantiated.
	const bool can_instantiate = packed_scene.can_instantiate();
	CHECK(can_instantiate == true);

	memdelete(scene);
}

TEST_CASE("[PackedScene] Instantiate Packed Scene") {
	// Create a scene to pack.
	Node *scene = memnew(Node);
	scene->set_name("TestScene");

	// Pack the scene.
	PackedScene packed_scene;
	packed_scene.pack(scene);

	// Instantiate the packed scene.
	Node *instance = packed_scene.instantiate();
	CHECK(instance != nullptr);
	CHECK(instance->get_name() == "TestScene");

	memdelete(scene);
	memdelete(instance);
}

TEST_CASE("[PackedScene] Instantiate Packed Scene With Children") {
	// Create a scene to pack.
	Node *scene = memnew(Node);
	scene->set_name("TestScene");

	// Add persisting child nodes to the scene.
	Node *child1 = memnew(Node);
	child1->set_name("Child1");
	scene->add_child(child1);
	child1->set_owner(scene);

	Node *child2 = memnew(Node);
	child2->set_name("Child2");
	scene->add_child(child2);
	child2->set_owner(scene);

	// Add non persisting child node to the scene.
	Node *child3 = memnew(Node);
	child3->set_name("Child3");
	scene->add_child(child3);

	// Pack the scene.
	PackedScene packed_scene;
	packed_scene.pack(scene);

	// Instantiate the packed scene.
	Node *instance = packed_scene.instantiate();
	CHECK(instance != nullptr);
	CHECK(instance->get_name() == "TestScene");

	// Validate the child nodes of the instantiated scene.
	CHECK(instance->get_child_count() == 2);
	CHECK(instance->get_child(0)->get_name() == "Child1");
	CHECK(instance->get_child(1)->get_name() == "Child2");
	CHECK(instance->get_child(0)->get_owner() == instance);
	CHECK(instance->get_child(1)->get_owner() == instance);

	memdelete(scene);
	memdelete(instance);
}

TEST_CASE("[PackedScene] Set Path") {
	// Create a scene to pack.
	Node *scene = memnew(Node);
	scene->set_name("TestScene");

	// Pack the scene.
	PackedScene packed_scene;
	packed_scene.pack(scene);

	// Set a new path for the packed scene.
	const String new_path = "NewTestPath";
	packed_scene.set_path(new_path);

	// Check if the path has been set correctly.
	Ref<SceneState> state = packed_scene.get_state();
	CHECK(state.is_valid());
	CHECK(state->get_path() == new_path);

	memdelete(scene);
}

TEST_CASE("[PackedScene] Replace State") {
	// Create a scene to pack.
	Node *scene = memnew(Node);
	scene->set_name("TestScene");

	// Pack the scene.
	PackedScene packed_scene;
	packed_scene.pack(scene);

	// Create another scene state to replace with.
	Ref<SceneState> new_state = memnew(SceneState);
	new_state->set_path("NewPath");

	// Replace the state.
	packed_scene.replace_state(new_state);

	// Check if the state has been replaced.
	Ref<SceneState> state = packed_scene.get_state();
	CHECK(state.is_valid());
	CHECK(state == new_state);

	memdelete(scene);
}

TEST_CASE("[PackedScene] Recreate State") {
	// Create a scene to pack.
	Node *scene = memnew(Node);
	scene->set_name("TestScene");

	// Pack the scene.
	Ref<PackedScene> packed_scene;
	packed_scene.instantiate();
	packed_scene->pack(scene);

	// Recreate the state.
	packed_scene->recreate_state();

	// Check if the state has been recreated.
	Ref<SceneState> state = packed_scene->get_state();
	CHECK(state.is_valid());
	CHECK(state->get_node_count() == 0); // Since the state was recreated, it should be empty.

	memdelete(scene);
}

#ifdef TOOLS_ENABLED
TEST_CASE("[PackedScene] Exposed nodes are saved and loaded") {
	// A scene with a node marked for exposure, as the editor marks it.
	Node *scene = memnew(Node);
	scene->set_name("TestScene");
	Node *sprite = memnew(Node);
	sprite->set_name("Sprite");
	scene->add_child(sprite);
	sprite->set_owner(scene);
	Node *hidden = memnew(Node);
	hidden->set_name("Hidden");
	scene->add_child(hidden);
	hidden->set_owner(scene);
	sprite->set_meta(META_MARKED_FOR_EXPOSURE, true);

	Ref<PackedScene> packed_scene;
	packed_scene.instantiate();
	REQUIRE(packed_scene->pack(scene) == OK);

	Vector<NodePath> exposed = packed_scene->get_state()->get_exposed_nodes();
	REQUIRE(exposed.size() == 1);
	CHECK(exposed[0] == NodePath("Sprite"));

	for (const String extension : { "tscn", "scn" }) {
		{
			INFO("Saved as .", extension);
			const String path = TestUtils::get_temp_path("exposed_nodes_test." + extension);
			REQUIRE(ResourceSaver::save(packed_scene, path) == OK);

			if (extension == "tscn") {
				const String text = FileAccess::get_file_as_string(path);
				CHECK(text.contains("[exposed path=\"Sprite\"]"));
				// The exposure is kept in the [exposed] tag, not as metadata of the node.
				CHECK_FALSE(text.contains("_marked_for_exposure"));
			}

			Error err = OK;
			Ref<PackedScene> loaded = ResourceLoader::load(path, "PackedScene", ResourceFormatLoader::CACHE_MODE_IGNORE, &err);
			REQUIRE(err == OK);
			REQUIRE(loaded.is_valid());
			Vector<NodePath> loaded_exposed = loaded->get_state()->get_exposed_nodes();
			REQUIRE(loaded_exposed.size() == 1);
			CHECK(loaded_exposed[0] == NodePath("Sprite"));

			// Instantiated in another scene, the exposed node is flagged as such, and the other is not.
			Node *instance = loaded->instantiate(PackedScene::GEN_EDIT_STATE_INSTANCE);
			REQUIRE(instance != nullptr);
			CHECK(instance->has_meta(META_CONTAINS_EXPOSED_NODES));
			CHECK(instance->get_node(NodePath("Sprite"))->has_meta(META_EXPOSED_IN_OWNER));
			CHECK(Object::cast_to<Node>(instance->get_node(NodePath("Sprite"))->get_meta(META_EXPOSED_IN_INSTANCE)) == instance);
			CHECK_FALSE(instance->get_node(NodePath("Hidden"))->has_meta(META_EXPOSED_IN_OWNER));
			memdelete(instance);

			// Opened for editing, it stays marked, so it is saved exposed again.
			Node *edited = loaded->instantiate(PackedScene::GEN_EDIT_STATE_MAIN);
			REQUIRE(edited != nullptr);
			CHECK(edited->get_node(NodePath("Sprite"))->has_meta(META_MARKED_FOR_EXPOSURE));
			CHECK_FALSE(edited->get_node(NodePath("Hidden"))->has_meta(META_MARKED_FOR_EXPOSURE));
			Ref<PackedScene> repacked;
			repacked.instantiate();
			REQUIRE(repacked->pack(edited) == OK);
			CHECK(repacked->get_state()->get_exposed_nodes().size() == 1);
			memdelete(edited);

			DirAccess::remove_file_or_error(path);
		}
	}

	memdelete(scene);
}
#endif // TOOLS_ENABLED

} // namespace TestPackedScene
