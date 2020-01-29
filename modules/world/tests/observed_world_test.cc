// Copyright (c) 2019 fortiss GmbH, Julian Bernhard, Klemens Esterle, Patrick
// Hart, Tobias Kessler
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "modules/world/observed_world.hpp"
#include "gtest/gtest.h"
#include "modules/commons/params/default_params.hpp"
#include "modules/commons/params/setter_params.hpp"
#include "modules/geometry/polygon.hpp"
#include "modules/geometry/standard_shapes.hpp"
#include "modules/models/behavior/constant_velocity/constant_velocity.hpp"
#include "modules/models/behavior/motion_primitives/motion_primitives.hpp"
#include "modules/models/dynamic/single_track.hpp"
#include "modules/models/execution/interpolation/interpolate.hpp"
#include "modules/models/tests/make_test_world.hpp"
#include "modules/world/evaluation/evaluator_collision_agents.hpp"
#include "modules/world/map/map_interface.hpp"
#include "modules/world/map/roadgraph.hpp"
#include "modules/world/objects/agent.hpp"
#include "modules/world/opendrive/opendrive.hpp"
#include "modules/world/tests/dummy_road_corridor.hpp"
#include "modules/world/tests/make_test_xodr_map.hpp"

using namespace modules::models::dynamic;
using namespace modules::geometry;
using namespace modules::commons;
using namespace modules::models::behavior;
using namespace modules::models::execution;
using namespace modules::world::opendrive;
using namespace modules::world::map;
using namespace modules::world::objects;
using namespace modules::world;
using namespace modules::world::evaluation;
using namespace modules::world::prediction;
using modules::models::tests::make_test_world;
using modules::world::tests::DummyRoadCorridor;
using StateDefinition::VEL_POSITION;

TEST(observed_world, agent_in_front) {
  using modules::geometry::Model3D;
  using modules::geometry::Polygon;
  using modules::geometry::standard_shapes::CarRectangle;
  using modules::world::goal_definition::GoalDefinitionPolygon;
  using modules::world::tests::MakeXodrMapOneRoadTwoLanes;

  DefaultParams params;

  // Setting Up Map
  OpenDriveMapPtr open_drive_map = MakeXodrMapOneRoadTwoLanes();
  MapInterfacePtr map_interface = std::make_shared<MapInterface>();
  map_interface->interface_from_opendrive(open_drive_map);

  // Goal Definition
  Polygon polygon(
      Pose(1, 1, 0),
      std::vector<Point2d>{Point2d(0, 0), Point2d(0, 2), Point2d(2, 2),
                           Point2d(2, 0), Point2d(0, 0)});
  std::shared_ptr<Polygon> goal_polygon(
      std::dynamic_pointer_cast<Polygon>(polygon.Translate(Point2d(50, -2))));
  auto goal_ptr = std::make_shared<GoalDefinitionPolygon>(*goal_polygon);

  // Setting Up Agents (one in front of another)
  ExecutionModelPtr exec_model(new ExecutionModelInterpolate(&params));
  DynamicModelPtr dyn_model(new SingleTrackModel(&params));
  BehaviorModelPtr beh_model(new BehaviorConstantVelocity(&params));
  Polygon car_polygon = CarRectangle();

  State init_state1(static_cast<int>(StateDefinition::MIN_STATE_SIZE));
  init_state1 << 0.0, 3.0, -1.75, 0.0, 5.0;
  AgentPtr agent1(new Agent(init_state1, beh_model, dyn_model, exec_model,
                            car_polygon, &params, goal_ptr, map_interface,
                            Model3D()));  // NOLINT

  State init_state2(static_cast<int>(StateDefinition::MIN_STATE_SIZE));
  init_state2 << 0.0, 10.0, -1.75, 0.0, 5.0;
  AgentPtr agent2(new Agent(init_state2, beh_model, dyn_model, exec_model,
                            car_polygon, &params, goal_ptr, map_interface,
                            Model3D()));  // NOLINT

  // Construct World
  WorldPtr world(new World(&params));
  world->AddAgent(agent1);
  world->AddAgent(agent2);
  world->UpdateAgentRTree();

  WorldPtr current_world_state1(world->Clone());
  ObservedWorld obs_world1(current_world_state1, agent2->GetAgentId());

  // Leading agent should not have an agent in front
  std::pair<AgentPtr, Frenet> leading_vehicle = obs_world1.GetAgentInFront();
  EXPECT_FALSE(static_cast<bool>(leading_vehicle.first));

  WorldPtr current_world_state2(world->Clone());
  ObservedWorld obs_world2(current_world_state2, agent1->GetAgentId());

  // Agent behind should have leading agent in front
  std::pair<AgentPtr, Frenet> leading_vehicle2 = obs_world2.GetAgentInFront();
  EXPECT_TRUE(static_cast<bool>(leading_vehicle2.first));
  EXPECT_EQ(leading_vehicle2.first->GetAgentId(), agent2->GetAgentId());

  State init_state3(static_cast<int>(StateDefinition::MIN_STATE_SIZE));
  init_state3 << 0.0, 20.0, 0.0, 0.0, 5.0;
  AgentPtr agent3(new Agent(init_state3, beh_model, dyn_model, exec_model,
                            polygon, &params, goal_ptr, map_interface,
                            Model3D()));  // NOLINT
  world->AddAgent(agent3);
  world->UpdateAgentRTree();

  WorldPtr current_world_state3(world->Clone());
  ObservedWorld obs_world3(current_world_state3, agent1->GetAgentId());

  // Adding a third agent in front of leading agent, still leading agent
  // should be in front
  std::pair<AgentPtr, Frenet> leading_vehicle3 = obs_world3.GetAgentInFront();
  EXPECT_TRUE(static_cast<bool>(leading_vehicle3.first));
  EXPECT_EQ(leading_vehicle2.first->GetAgentId(), agent2->GetAgentId());
}

TEST(observed_world, clone) {
  DefaultParams params;
  ExecutionModelPtr exec_model(new ExecutionModelInterpolate(&params));
  DynamicModelPtr dyn_model(new SingleTrackModel(&params));
  BehaviorModelPtr beh_model(new BehaviorConstantVelocity(&params));
  EvaluatorPtr col_checker(new EvaluatorCollisionAgents());

  Polygon polygon(
      Pose(1.25, 1, 0),
      std::vector<Point2d>{Point2d(0, 0), Point2d(0, 2), Point2d(4, 2),
                           Point2d(4, 0), Point2d(0, 0)});

  State init_state1(static_cast<int>(StateDefinition::MIN_STATE_SIZE));
  init_state1 << 0.0, 0.0, 0.0, 0.0, 5.0;
  AgentPtr agent1(new Agent(init_state1, beh_model, dyn_model, exec_model,
                            polygon, &params));  // NOLINT

  State init_state2(static_cast<int>(StateDefinition::MIN_STATE_SIZE));
  init_state2 << 0.0, 8.0, 0.0, 0.0, 5.0;
  AgentPtr agent2(new Agent(init_state2, beh_model, dyn_model, exec_model,
                            polygon, &params));  // NOLINT

  WorldPtr world = std::make_shared<World>(&params);
  world->AddAgent(agent1);
  world->AddAgent(agent2);
  world->UpdateAgentRTree();

  WorldPtr current_world_state(world->Clone());
  ObservedWorldPtr observed_world(
      new ObservedWorld(current_world_state, agent1->GetAgentId()));

  WorldPtr cloned(observed_world->Clone());
  ObservedWorldPtr cloned_observed_world =
      std::dynamic_pointer_cast<ObservedWorld>(cloned);
  EXPECT_EQ(observed_world->GetEgoAgent()->GetAgentId(),
            cloned_observed_world->GetEgoAgent()->GetAgentId());
  EXPECT_EQ(typeid(observed_world->GetEgoBehaviorModel()),
            typeid(cloned_observed_world->GetEgoBehaviorModel()));

  observed_world.reset();
  auto behavior_ego = cloned_observed_world->GetEgoBehaviorModel();
  EXPECT_TRUE(behavior_ego != nullptr);
}

TEST(observed_world, predict) {
  SetterParams params;
  params.SetReal("integration_time_delta", 0.01);
  DynamicModelPtr dyn_model(new SingleTrackModel(&params));
  float ego_velocity = 5.0, rel_distance = 7.0, velocity_difference = 0.0;
  auto observed_world = modules::models::tests::make_test_observed_world(
      1, rel_distance, ego_velocity, velocity_difference);

  // predict all agents with constant velocity
  BehaviorModelPtr prediction_model(new BehaviorConstantVelocity(&params));
  PredictionSettings prediction_settings(prediction_model, prediction_model);
  observed_world.SetupPrediction(prediction_settings);
  WorldPtr predicted_world = observed_world.Predict(1.0f);
  ObservedWorldPtr observed_predicted_world =
      std::dynamic_pointer_cast<ObservedWorld>(predicted_world);
  double distance_ego = modules::geometry::Distance(
      observed_predicted_world->CurrentEgoPosition(),
      observed_world.CurrentEgoPosition());
  double distance_other = modules::geometry::Distance(
      observed_predicted_world->GetOtherAgents()
          .begin()
          ->second->GetCurrentPosition(),  // NOLINT
      observed_world.GetOtherAgents().begin()->second->GetCurrentPosition());

  // distance current and predicted state should be
  // velocity x prediction time span
  EXPECT_NEAR(distance_ego, ego_velocity * 1.0f, 0.06);
  EXPECT_NEAR(distance_other,
              observed_world.GetOtherAgents()
                      .begin()
                      ->second->GetCurrentState()[VEL_POSITION] *
                  1.0f,  // NOLINT
              0.06);

  // predict ego agent with motion primitive model
  BehaviorModelPtr ego_prediction_model(
      new BehaviorMotionPrimitives(dyn_model, &params));
  Input u1(2);
  u1 << 2, 0;
  Input u2(2);
  u2 << 0, 1;
  BehaviorMotionPrimitives::MotionIdx idx1 =
      std::dynamic_pointer_cast<BehaviorMotionPrimitives>(ego_prediction_model)
          ->AddMotionPrimitive(u1);  // NOLINT
  BehaviorMotionPrimitives::MotionIdx idx2 =
      std::dynamic_pointer_cast<BehaviorMotionPrimitives>(ego_prediction_model)
          ->AddMotionPrimitive(u2);  // NOLINT

  BehaviorModelPtr others_prediction_model(
      new BehaviorConstantVelocity(&params));
  PredictionSettings prediction_settings2(ego_prediction_model,
                                          others_prediction_model);
  observed_world.SetupPrediction(prediction_settings2);
  WorldPtr predicted_world2 =
      observed_world.Predict(1.0f, DiscreteAction(idx1));
  auto ego_pred_velocity =
      std::dynamic_pointer_cast<ObservedWorld>(predicted_world2)
          ->CurrentEgoState()[StateDefinition::VEL_POSITION];  // NOLINT
  // distance current and predicted state should be velocity
  // + prediction time span
  EXPECT_NEAR(ego_pred_velocity, ego_velocity + 2 * 1.0f, 0.05);
}
