#include "selection/selection_filter.h"
#include "selection/selection_query.h"
#include "selection/depth_cycle.h"
#include "editor_state.h"

#include <gtest/gtest.h>

// ============================================================================
// SelectionFilter Tests
// ============================================================================

TEST(SelectionFilter, TypeFromString_Brush)
{
  EXPECT_EQ(SelectionFilter::TypeFromString("brush"), SelectableType::Brush);
  EXPECT_EQ(SelectionFilter::TypeFromString("Brush"), SelectableType::Brush);
  EXPECT_EQ(SelectionFilter::TypeFromString("BRUSH"), SelectableType::Brush);
  EXPECT_EQ(SelectionFilter::TypeFromString("brushlist"), SelectableType::Brush);
  EXPECT_EQ(SelectionFilter::TypeFromString("hull"), SelectableType::Brush);
  EXPECT_EQ(SelectionFilter::TypeFromString("polyhedron"), SelectableType::Brush);
}

TEST(SelectionFilter, TypeFromString_Light)
{
  EXPECT_EQ(SelectionFilter::TypeFromString("light"), SelectableType::Light);
  EXPECT_EQ(SelectionFilter::TypeFromString("pointlight"), SelectableType::Light);
  EXPECT_EQ(SelectionFilter::TypeFromString("dirlight"), SelectableType::Light);
  EXPECT_EQ(SelectionFilter::TypeFromString("spotlight"), SelectableType::Light);
  EXPECT_EQ(SelectionFilter::TypeFromString("directionallight"), SelectableType::Light);
  EXPECT_EQ(SelectionFilter::TypeFromString("ambientlight"), SelectableType::Light);
}

TEST(SelectionFilter, TypeFromString_WorldModel)
{
  EXPECT_EQ(SelectionFilter::TypeFromString("worldmodel"), SelectableType::WorldModel);
  EXPECT_EQ(SelectionFilter::TypeFromString("door"), SelectableType::WorldModel);
}

TEST(SelectionFilter, TypeFromString_Volume)
{
  EXPECT_EQ(SelectionFilter::TypeFromString("volume"), SelectableType::Volume);
  EXPECT_EQ(SelectionFilter::TypeFromString("physicsvolume"), SelectableType::Volume);
  EXPECT_EQ(SelectionFilter::TypeFromString("aivolume"), SelectableType::Volume);
}

TEST(SelectionFilter, TypeFromString_Prefab)
{
  EXPECT_EQ(SelectionFilter::TypeFromString("prefab"), SelectableType::Prefab);
  EXPECT_EQ(SelectionFilter::TypeFromString("prefabref"), SelectableType::Prefab);
}

TEST(SelectionFilter, TypeFromString_Container)
{
  EXPECT_EQ(SelectionFilter::TypeFromString("container"), SelectableType::Container);
  EXPECT_EQ(SelectionFilter::TypeFromString("folder"), SelectableType::Container);
  EXPECT_EQ(SelectionFilter::TypeFromString("group"), SelectableType::Container);
}

TEST(SelectionFilter, TypeFromString_DefaultsToObject)
{
  EXPECT_EQ(SelectionFilter::TypeFromString("trigger"), SelectableType::Object);
  EXPECT_EQ(SelectionFilter::TypeFromString("spawner"), SelectableType::Object);
  EXPECT_EQ(SelectionFilter::TypeFromString("unknown_type"), SelectableType::Object);
  EXPECT_EQ(SelectionFilter::TypeFromString(""), SelectableType::Object);
}

TEST(SelectionFilter, IsLightClass_DetectsLightNames)
{
  EXPECT_TRUE(SelectionFilter::IsLightClass("PointLight"));
  EXPECT_TRUE(SelectionFilter::IsLightClass("pointlight"));
  EXPECT_TRUE(SelectionFilter::IsLightClass("DirectionalLight"));
  EXPECT_TRUE(SelectionFilter::IsLightClass("AmbientLight"));
  EXPECT_TRUE(SelectionFilter::IsLightClass("StaticSunLight"));

  EXPECT_FALSE(SelectionFilter::IsLightClass("Door"));
  EXPECT_FALSE(SelectionFilter::IsLightClass("Trigger"));
  EXPECT_FALSE(SelectionFilter::IsLightClass("Brush"));
}

TEST(SelectionFilter, PassesFilter_WhenInactive)
{
  SelectionFilter filter;
  filter.filter_active = false;

  // When filter is inactive, everything passes
  EXPECT_TRUE(filter.PassesFilter("brush", ""));
  EXPECT_TRUE(filter.PassesFilter("light", ""));
  EXPECT_TRUE(filter.PassesFilter("unknown", ""));
}

TEST(SelectionFilter, PassesFilter_WhenTypeEnabled)
{
  SelectionFilter filter;
  filter.filter_active = true;
  filter.enabled_types.insert(SelectableType::Brush);

  EXPECT_TRUE(filter.PassesFilter("brush", ""));
  EXPECT_TRUE(filter.PassesFilter("Brush", ""));
  EXPECT_FALSE(filter.PassesFilter("light", ""));
  EXPECT_FALSE(filter.PassesFilter("object", ""));
}

TEST(SelectionFilter, PassesFilter_WhenTypeDisabled)
{
  SelectionFilter filter;
  filter.filter_active = true;
  filter.enabled_types.insert(SelectableType::Brush);
  filter.enabled_types.insert(SelectableType::Object);
  // Light is NOT enabled

  EXPECT_FALSE(filter.PassesFilter("light", ""));
  EXPECT_FALSE(filter.PassesFilter("pointlight", "PointLight"));
}

TEST(SelectionFilter, PassesFilter_ObjectWithLightClass)
{
  SelectionFilter filter;
  filter.filter_active = true;
  filter.enabled_types.insert(SelectableType::Light);
  // Object is NOT enabled

  // Object type with light class name should be treated as Light
  EXPECT_TRUE(filter.PassesFilter("object", "PointLight"));
  EXPECT_TRUE(filter.PassesFilter("object", "DirectionalLight"));

  // Object type without light class name should fail
  EXPECT_FALSE(filter.PassesFilter("object", "Door"));
}

TEST(SelectionFilter, SetTypeEnabled_AddsAndRemoves)
{
  SelectionFilter filter;
  filter.filter_active = true;

  EXPECT_FALSE(filter.IsTypeEnabled(SelectableType::Brush));

  filter.SetTypeEnabled(SelectableType::Brush, true);
  EXPECT_TRUE(filter.IsTypeEnabled(SelectableType::Brush));

  filter.SetTypeEnabled(SelectableType::Brush, false);
  EXPECT_FALSE(filter.IsTypeEnabled(SelectableType::Brush));
}

TEST(SelectionFilter, EnableAll_DisablesFilter)
{
  SelectionFilter filter;
  filter.filter_active = true;
  filter.enabled_types.insert(SelectableType::Brush);

  filter.EnableAll();

  EXPECT_FALSE(filter.filter_active);
  EXPECT_TRUE(filter.enabled_types.empty());
}

TEST(SelectionFilter, DisableAll_BlocksEverything)
{
  SelectionFilter filter;
  filter.filter_active = true;
  filter.enabled_types.insert(SelectableType::Brush);
  filter.enabled_types.insert(SelectableType::Light);

  filter.DisableAll();

  EXPECT_TRUE(filter.filter_active);
  EXPECT_TRUE(filter.enabled_types.empty());
  EXPECT_FALSE(filter.PassesFilter("brush", ""));
  EXPECT_FALSE(filter.PassesFilter("light", ""));
}

TEST(SelectionFilter, TypeName_ReturnsReadableNames)
{
  EXPECT_STREQ(SelectionFilter::TypeName(SelectableType::Brush), "Brushes");
  EXPECT_STREQ(SelectionFilter::TypeName(SelectableType::Light), "Lights");
  EXPECT_STREQ(SelectionFilter::TypeName(SelectableType::Object), "Objects");
  EXPECT_STREQ(SelectionFilter::TypeName(SelectableType::WorldModel), "World Models");
  EXPECT_STREQ(SelectionFilter::TypeName(SelectableType::Volume), "Volumes");
  EXPECT_STREQ(SelectionFilter::TypeName(SelectableType::Prefab), "Prefabs");
  EXPECT_STREQ(SelectionFilter::TypeName(SelectableType::Container), "Containers");
}

// ============================================================================
// SelectionQuery Tests
// ============================================================================

namespace {
// Helper to create a simple TreeNode for testing
TreeNode MakeTestNode(const std::string& name)
{
  TreeNode node;
  node.name = name;
  node.is_folder = false;
  node.deleted = false;
  return node;
}

// Helper to create NodeProperties for testing
NodeProperties MakeTestProps(const std::string& type, const std::string& class_name)
{
  NodeProperties props;
  props.type = type;
  props.class_name = class_name;
  props.visible = true;
  props.frozen = false;
  return props;
}
} // namespace

TEST(SelectionQuery, EvaluateCriterion_Equals)
{
  TreeNode node = MakeTestNode("Door01");
  NodeProperties props = MakeTestProps("Object", "Door");

  SelectionCriterion criterion;
  criterion.field = SelectionCriterionField::Name;
  criterion.op = SelectionCriterionOp::Equals;
  criterion.value = "door01";

  // Case-insensitive equality
  EXPECT_TRUE(EvaluateCriterion(criterion, node, props));

  criterion.value = "Door01";
  EXPECT_TRUE(EvaluateCriterion(criterion, node, props));

  criterion.value = "Door02";
  EXPECT_FALSE(EvaluateCriterion(criterion, node, props));
}

TEST(SelectionQuery, EvaluateCriterion_NotEquals)
{
  TreeNode node = MakeTestNode("Door01");
  NodeProperties props = MakeTestProps("Object", "Door");

  SelectionCriterion criterion;
  criterion.field = SelectionCriterionField::Name;
  criterion.op = SelectionCriterionOp::NotEquals;
  criterion.value = "Door02";

  EXPECT_TRUE(EvaluateCriterion(criterion, node, props));

  criterion.value = "door01";
  EXPECT_FALSE(EvaluateCriterion(criterion, node, props));
}

TEST(SelectionQuery, EvaluateCriterion_Contains)
{
  TreeNode node = MakeTestNode("MyDoorEntity");
  NodeProperties props = MakeTestProps("Object", "Door");

  SelectionCriterion criterion;
  criterion.field = SelectionCriterionField::Name;
  criterion.op = SelectionCriterionOp::Contains;
  criterion.value = "door";

  EXPECT_TRUE(EvaluateCriterion(criterion, node, props));

  criterion.value = "DOOR";
  EXPECT_TRUE(EvaluateCriterion(criterion, node, props));

  criterion.value = "window";
  EXPECT_FALSE(EvaluateCriterion(criterion, node, props));
}

TEST(SelectionQuery, EvaluateCriterion_StartsWith)
{
  TreeNode node = MakeTestNode("Door_Main");
  NodeProperties props = MakeTestProps("Object", "Door");

  SelectionCriterion criterion;
  criterion.field = SelectionCriterionField::Name;
  criterion.op = SelectionCriterionOp::StartsWith;
  criterion.value = "door";

  EXPECT_TRUE(EvaluateCriterion(criterion, node, props));

  criterion.value = "main";
  EXPECT_FALSE(EvaluateCriterion(criterion, node, props));
}

TEST(SelectionQuery, EvaluateCriterion_EndsWith)
{
  TreeNode node = MakeTestNode("Door_Main");
  NodeProperties props = MakeTestProps("Object", "Door");

  SelectionCriterion criterion;
  criterion.field = SelectionCriterionField::Name;
  criterion.op = SelectionCriterionOp::EndsWith;
  criterion.value = "main";

  EXPECT_TRUE(EvaluateCriterion(criterion, node, props));

  criterion.value = "door";
  EXPECT_FALSE(EvaluateCriterion(criterion, node, props));
}

TEST(SelectionQuery, EvaluateCriterion_Matches_ExactMatch)
{
  TreeNode node = MakeTestNode("door");
  NodeProperties props = MakeTestProps("Object", "Door");

  SelectionCriterion criterion;
  criterion.field = SelectionCriterionField::Name;
  criterion.op = SelectionCriterionOp::Matches;
  criterion.value = "door";

  EXPECT_TRUE(EvaluateCriterion(criterion, node, props));
}

TEST(SelectionQuery, EvaluateCriterion_Matches_StarMatchesAny)
{
  TreeNode node = MakeTestNode("doorway");
  NodeProperties props = MakeTestProps("Object", "Door");

  SelectionCriterion criterion;
  criterion.field = SelectionCriterionField::Name;
  criterion.op = SelectionCriterionOp::Matches;
  criterion.value = "door*";

  EXPECT_TRUE(EvaluateCriterion(criterion, node, props));

  TreeNode node2 = MakeTestNode("door");
  EXPECT_TRUE(EvaluateCriterion(criterion, node2, props));
}

TEST(SelectionQuery, EvaluateCriterion_Matches_QuestionMatchesOne)
{
  SelectionCriterion criterion;
  criterion.field = SelectionCriterionField::Name;
  criterion.op = SelectionCriterionOp::Matches;
  criterion.value = "door?";

  NodeProperties props = MakeTestProps("Object", "Door");

  TreeNode node1 = MakeTestNode("doors");
  EXPECT_TRUE(EvaluateCriterion(criterion, node1, props));

  TreeNode node2 = MakeTestNode("door1");
  EXPECT_TRUE(EvaluateCriterion(criterion, node2, props));

  TreeNode node3 = MakeTestNode("door");
  EXPECT_FALSE(EvaluateCriterion(criterion, node3, props));

  TreeNode node4 = MakeTestNode("doorway");
  EXPECT_FALSE(EvaluateCriterion(criterion, node4, props));
}

TEST(SelectionQuery, EvaluateCriterion_Matches_ComplexPattern)
{
  SelectionCriterion criterion;
  criterion.field = SelectionCriterionField::Name;
  criterion.op = SelectionCriterionOp::Matches;
  criterion.value = "*door*";

  NodeProperties props = MakeTestProps("Object", "Door");

  TreeNode node1 = MakeTestNode("MyDoorEntity");
  EXPECT_TRUE(EvaluateCriterion(criterion, node1, props));

  TreeNode node2 = MakeTestNode("door");
  EXPECT_TRUE(EvaluateCriterion(criterion, node2, props));

  TreeNode node3 = MakeTestNode("doorway");
  EXPECT_TRUE(EvaluateCriterion(criterion, node3, props));

  TreeNode node4 = MakeTestNode("window");
  EXPECT_FALSE(EvaluateCriterion(criterion, node4, props));
}

TEST(SelectionQuery, EvaluateCriterion_Matches_CaseInsensitive)
{
  SelectionCriterion criterion;
  criterion.field = SelectionCriterionField::Name;
  criterion.op = SelectionCriterionOp::Matches;
  criterion.value = "DOOR*";

  NodeProperties props = MakeTestProps("Object", "Door");

  TreeNode node = MakeTestNode("doorway");
  EXPECT_TRUE(EvaluateCriterion(criterion, node, props));
}

TEST(SelectionQuery, EvaluateCriterion_FieldType)
{
  TreeNode node = MakeTestNode("SomeName");
  NodeProperties props = MakeTestProps("Brush", "Door");

  SelectionCriterion criterion;
  criterion.op = SelectionCriterionOp::Equals;

  // Test Type field
  criterion.field = SelectionCriterionField::Type;
  criterion.value = "Brush";
  EXPECT_TRUE(EvaluateCriterion(criterion, node, props));

  // Test ClassName field
  criterion.field = SelectionCriterionField::ClassName;
  criterion.value = "Door";
  EXPECT_TRUE(EvaluateCriterion(criterion, node, props));
}

TEST(SelectionQuery, EvaluateQuery_AllCombiner)
{
  TreeNode node = MakeTestNode("Door01");
  NodeProperties props = MakeTestProps("Brush", "Door");

  SelectionQuery query;
  query.combiner = SelectionQueryCombiner::All;

  // Criterion 1: Name contains "door"
  SelectionCriterion c1;
  c1.field = SelectionCriterionField::Name;
  c1.op = SelectionCriterionOp::Contains;
  c1.value = "door";
  query.criteria.push_back(c1);

  // Criterion 2: Type equals "Brush"
  SelectionCriterion c2;
  c2.field = SelectionCriterionField::Type;
  c2.op = SelectionCriterionOp::Equals;
  c2.value = "Brush";
  query.criteria.push_back(c2);

  // Both criteria match
  EXPECT_TRUE(EvaluateQuery(query, node, props));

  // Change type to not match
  NodeProperties props2 = MakeTestProps("Object", "Door");
  EXPECT_FALSE(EvaluateQuery(query, node, props2));
}

TEST(SelectionQuery, EvaluateQuery_AnyCombiner)
{
  TreeNode node = MakeTestNode("Door01");
  NodeProperties props = MakeTestProps("Object", "Door");

  SelectionQuery query;
  query.combiner = SelectionQueryCombiner::Any;

  // Criterion 1: Name contains "window" (won't match)
  SelectionCriterion c1;
  c1.field = SelectionCriterionField::Name;
  c1.op = SelectionCriterionOp::Contains;
  c1.value = "window";
  query.criteria.push_back(c1);

  // Criterion 2: Name contains "door" (will match)
  SelectionCriterion c2;
  c2.field = SelectionCriterionField::Name;
  c2.op = SelectionCriterionOp::Contains;
  c2.value = "door";
  query.criteria.push_back(c2);

  // At least one criterion matches
  EXPECT_TRUE(EvaluateQuery(query, node, props));

  // Neither matches
  TreeNode node2 = MakeTestNode("Light01");
  EXPECT_FALSE(EvaluateQuery(query, node2, props));
}

TEST(SelectionQuery, EmptyQuery_ReturnsFalse)
{
  TreeNode node = MakeTestNode("Door01");
  NodeProperties props = MakeTestProps("Object", "Door");

  SelectionQuery query;
  query.criteria.clear();

  EXPECT_FALSE(EvaluateQuery(query, node, props));
}

TEST(SelectionQuery, GetFieldLabel_ReturnsLabels)
{
  EXPECT_STREQ(GetFieldLabel(SelectionCriterionField::Name), "Name");
  EXPECT_STREQ(GetFieldLabel(SelectionCriterionField::Type), "Type");
  EXPECT_STREQ(GetFieldLabel(SelectionCriterionField::ClassName), "Class");
  EXPECT_STREQ(GetFieldLabel(SelectionCriterionField::Property), "Property");
}

TEST(SelectionQuery, GetOperatorLabel_ReturnsLabels)
{
  EXPECT_STREQ(GetOperatorLabel(SelectionCriterionOp::Equals), "Equals");
  EXPECT_STREQ(GetOperatorLabel(SelectionCriterionOp::NotEquals), "Not Equals");
  EXPECT_STREQ(GetOperatorLabel(SelectionCriterionOp::Contains), "Contains");
  EXPECT_STREQ(GetOperatorLabel(SelectionCriterionOp::StartsWith), "Starts With");
  EXPECT_STREQ(GetOperatorLabel(SelectionCriterionOp::EndsWith), "Ends With");
  EXPECT_STREQ(GetOperatorLabel(SelectionCriterionOp::Matches), "Matches");
}

TEST(SelectionQuery, GetCombinerLabel_ReturnsLabels)
{
  EXPECT_STREQ(GetCombinerLabel(SelectionQueryCombiner::All), "All (AND)");
  EXPECT_STREQ(GetCombinerLabel(SelectionQueryCombiner::Any), "Any (OR)");
}

// ============================================================================
// DepthCycleState Tests
// ============================================================================

TEST(DepthCycleState, Reset_ClearsState)
{
  DepthCycleState state;
  state.active = true;
  state.candidates.push_back({1, 10.0f});
  state.candidates.push_back({2, 20.0f});
  state.current_index = 1;
  state.last_click_pos = ImVec2(100.0f, 200.0f);
  state.last_click_time = 5.0f;

  state.Reset();

  EXPECT_FALSE(state.active);
  EXPECT_TRUE(state.candidates.empty());
  EXPECT_EQ(state.current_index, 0);
}

TEST(DepthCycleState, GetDepthCycleStatus_WhenInactive)
{
  DepthCycleState state;
  state.active = false;

  std::string status = GetDepthCycleStatus(state);
  EXPECT_TRUE(status.empty());
}

TEST(DepthCycleState, GetDepthCycleStatus_WhenActive)
{
  DepthCycleState state;
  state.active = true;
  state.candidates.push_back({1, 10.0f});
  state.candidates.push_back({2, 20.0f});
  state.candidates.push_back({3, 30.0f});
  state.current_index = 1;

  std::string status = GetDepthCycleStatus(state);
  EXPECT_EQ(status, "2 of 3");

  state.current_index = 0;
  status = GetDepthCycleStatus(state);
  EXPECT_EQ(status, "1 of 3");

  state.current_index = 2;
  status = GetDepthCycleStatus(state);
  EXPECT_EQ(status, "3 of 3");
}

TEST(DepthCycleState, Constants)
{
  // Verify the constants are reasonable values
  EXPECT_GT(kDepthCycleClickRadius, 0.0f);
  EXPECT_LT(kDepthCycleClickRadius, 50.0f); // Reasonable max

  EXPECT_GT(kDepthCycleTimeout, 0.0f);
  EXPECT_LT(kDepthCycleTimeout, 10.0f); // Reasonable max
}
