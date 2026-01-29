#include "ui_shared.h"
#include "editor_state.h"

#include <gtest/gtest.h>

TEST(SceneUi, BuildSceneRowIcons_SkipsFolders)
{
  TreeNode node{};
  node.is_folder = true;
  NodeProperties props{};

  const std::vector<SceneRowIcon> icons = BuildSceneRowIcons(node, props);

  EXPECT_TRUE(icons.empty());
}

TEST(SceneUi, BuildSceneRowIcons_VisibilityOnlyWhenNotFrozen)
{
  TreeNode node{};
  node.is_folder = false;
  NodeProperties props{};
  props.frozen = false;

  const std::vector<SceneRowIcon> icons = BuildSceneRowIcons(node, props);

  ASSERT_EQ(icons.size(), 1u);
  EXPECT_EQ(icons[0], SceneRowIcon::Visibility);
}

TEST(SceneUi, BuildSceneRowIcons_AppendsFreezeIcon)
{
  TreeNode node{};
  node.is_folder = false;
  NodeProperties props{};
  props.frozen = true;

  const std::vector<SceneRowIcon> icons = BuildSceneRowIcons(node, props);

  ASSERT_EQ(icons.size(), 2u);
  EXPECT_EQ(icons[0], SceneRowIcon::Visibility);
  EXPECT_EQ(icons[1], SceneRowIcon::Freeze);
}
