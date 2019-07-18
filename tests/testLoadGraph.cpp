/**
 * @file    testDoOptimize.cpp
 * @brief   Unit test for pcm and optimize conditions
 * @author  Yun Chang
 */

#include <CppUnitLite/TestHarness.h>
#include <random>
#include <memory>

#include "RobustPGO/RobustPGO.h"
#include "RobustPGO/pcm/pcm.h" 
#include "test_config.h"

/* ************************************************************************* */
TEST(RobustPGO, Load1)
{
  // load graph
  // read g2o file for robot a 
  gtsam::NonlinearFactorGraph::shared_ptr nfg;
  gtsam::Values::shared_ptr values;
  boost::tie(nfg, values) = gtsam::load3D(std::string(DATASET_PATH) + "/robot_a.g2o");

  // set up RobustPGO solver 
  std::shared_ptr<OutlierRemoval> pcm = std::make_shared<PCM<gtsam::Pose3>>(0.0, 10.0); // set odom check to be small
  pcm->setQuiet(); // turn off print messages for pcm

  std::shared_ptr<RobustPGO> pgo = std::make_shared<RobustPGO>(pcm);
  pgo->setQuiet(); // turn off print messages

  // Create prior
  static const gtsam::SharedNoiseModel& noise = 
      gtsam::noiseModel::Isotropic::Variance(6, 0.01);   

  gtsam::Key init_key = gtsam::Symbol('a', 0);
  gtsam::PriorFactor<gtsam::Pose3> init(init_key, values->at<gtsam::Pose3>(init_key), noise);

  // Load graph using prior
  pgo->loadGraph(*nfg, *values, init);

  gtsam::NonlinearFactorGraph nfg_out = pgo->getFactorsUnsafe();
  gtsam::Values values_out = pgo->calculateEstimate();

  // Since odom check threshold is 0, should only have the odom edges + prior (no lc should have passed)
  EXPECT(nfg_out.size()==size_t(50));
  EXPECT(values_out.size()==size_t(50));
}

/* ************************************************************************* */
TEST(RobustPGO, Add1)
{
  // load graph for robot a (same as above)
  gtsam::NonlinearFactorGraph::shared_ptr nfg;
  gtsam::Values::shared_ptr values;
  boost::tie(nfg, values) = gtsam::load3D(std::string(DATASET_PATH) + "/robot_a.g2o");

  std::shared_ptr<OutlierRemoval> pcm = std::make_shared<PCM<gtsam::Pose3>>(0.0, 10.0); // set odom check to be small
  pcm->setQuiet(); // turn off print messages for pcm

  std::shared_ptr<RobustPGO> pgo = std::make_shared<RobustPGO>(pcm);
  pgo->setQuiet(); // turn off print messages

  static const gtsam::SharedNoiseModel& noise = 
      gtsam::noiseModel::Isotropic::Variance(6, 0.01);   

  gtsam::Key init_key = gtsam::Symbol('a', 0);
  gtsam::PriorFactor<gtsam::Pose3> init(init_key, values->at<gtsam::Pose3>(init_key), noise);
  pgo->loadGraph(*nfg, *values, init);// first load 

  // add graph 
  // read g2o file for robot b
  gtsam::NonlinearFactorGraph::shared_ptr nfg_b;
  gtsam::Values::shared_ptr values_b;
  boost::tie(nfg_b, values_b) = gtsam::load3D(std::string(DATASET_PATH) + "/robot_b.g2o");


  // create the between factor for connection
  gtsam::Key init_key_b = gtsam::Symbol('b', 0);
  gtsam::Pose3 transform_ab = values->at<gtsam::Pose3>(init_key).between(values_b->at<gtsam::Pose3>(init_key_b));
  gtsam::BetweenFactor<gtsam::Pose3> bridge(init_key, init_key_b, transform_ab, noise);

  // add graph 
  pgo->addGraph(*nfg_b, *values_b, bridge);

  gtsam::NonlinearFactorGraph nfg_out = pgo->getFactorsUnsafe();
  gtsam::Values values_out = pgo->calculateEstimate();

  // Since odom check threshold is 0, should only have the odom edges + prior + between (no lc should have passed)
  EXPECT(nfg_out.size()==size_t(92));
  EXPECT(values_out.size()==size_t(92));
}

/* ************************************************************************* */
TEST(RobustPGO, Load2)
{
  // load graph
  // read g2o file for robot a 
  gtsam::NonlinearFactorGraph::shared_ptr nfg;
  gtsam::Values::shared_ptr values;
  boost::tie(nfg, values) = gtsam::load3D(std::string(DATASET_PATH) + "/robot_a.g2o");

  // set up RobustPGO solver 
  std::shared_ptr<OutlierRemoval> pcm = std::make_shared<PCM<gtsam::Pose3>>(100.0, 100.0); // set odom check to be small
  pcm->setQuiet(); // turn off print messages for pcm

  std::shared_ptr<RobustPGO> pgo = std::make_shared<RobustPGO>(pcm);
  pgo->setQuiet(); // turn off print messages

  // Create prior
  static const gtsam::SharedNoiseModel& noise = 
      gtsam::noiseModel::Isotropic::Variance(6, 0.01);   

  gtsam::Key init_key = gtsam::Symbol('a', 0);
  gtsam::PriorFactor<gtsam::Pose3> init(init_key, values->at<gtsam::Pose3>(init_key), noise);

  // Load graph using prior
  pgo->loadGraph(*nfg, *values, init);

  gtsam::NonlinearFactorGraph nfg_out = pgo->getFactorsUnsafe();
  gtsam::Values values_out = pgo->calculateEstimate();

  // Since thresholds are high, should have all the edges
  EXPECT(nfg_out.size()==size_t(53));
  EXPECT(values_out.size()==size_t(50));
}

/* ************************************************************************* */
TEST(RobustPGO, Add2)
{
  // load graph for robot a (same as above)
  gtsam::NonlinearFactorGraph::shared_ptr nfg;
  gtsam::Values::shared_ptr values;
  boost::tie(nfg, values) = gtsam::load3D(std::string(DATASET_PATH) + "/robot_a.g2o");

  std::shared_ptr<OutlierRemoval> pcm = std::make_shared<PCM<gtsam::Pose3>>(100.0, 100.0); // set odom check to be small
  pcm->setQuiet(); // turn off print messages for pcm

  std::shared_ptr<RobustPGO> pgo = std::make_shared<RobustPGO>(pcm);
  pgo->setQuiet(); // turn off print messages

  static const gtsam::SharedNoiseModel& noise = 
      gtsam::noiseModel::Isotropic::Variance(6, 0.01);   

  gtsam::Key init_key = gtsam::Symbol('a', 0);
  gtsam::PriorFactor<gtsam::Pose3> init(init_key, values->at<gtsam::Pose3>(init_key), noise);
  pgo->loadGraph(*nfg, *values, init);// first load 

  // add graph 
  // read g2o file for robot b
  gtsam::NonlinearFactorGraph::shared_ptr nfg_b;
  gtsam::Values::shared_ptr values_b;
  boost::tie(nfg_b, values_b) = gtsam::load3D(std::string(DATASET_PATH) + "/robot_b.g2o");

  // create the between factor for connection
  gtsam::Key init_key_b = gtsam::Symbol('b', 0);
  gtsam::Pose3 transform_ab = values->at<gtsam::Pose3>(init_key).between(values_b->at<gtsam::Pose3>(init_key_b));
  gtsam::BetweenFactor<gtsam::Pose3> bridge(init_key, init_key_b, transform_ab, noise);

  // add graph 
  pgo->addGraph(*nfg_b, *values_b, bridge);

  gtsam::NonlinearFactorGraph nfg_out = pgo->getFactorsUnsafe();
  gtsam::Values values_out = pgo->calculateEstimate();

  // Since thresholds are high, should have all the edges
  EXPECT(nfg_out.size()==size_t(97));
  EXPECT(values_out.size()==size_t(92));
}
/* ************************************************************************* */
  int main() { TestResult tr; return TestRegistry::runAllTests(tr);}
/* ************************************************************************* */
